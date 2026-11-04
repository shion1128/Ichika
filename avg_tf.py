#!/usr/bin/env python3
"""Statistical average of a robot pose while it stands still (ROS 2).

Sources
  --source tf    (default) average the TF  parent -> child
  --source odom  average a nav_msgs/Odometry topic

Examples
  # GICP 的 map->odom，用来填 init_pose（车必须还停在 Point-LIO 启动时的位置）
  python3 avg_tf.py --parent map --child odom -n 200

  # 车在地图里的位姿，用来和墙/标记点对照
  python3 avg_tf.py --parent map --child base_link -n 200

  # Point-LIO 里程计 (camera_init -> body)，只反映里程计，不是定位结果
  python3 avg_tf.py --source odom --topic /aft_mapped_to_init

Method
  - position : mean / std / peak-to-peak
  - rotation : quaternion average (Markley eigenvector method), so roll, pitch
               and yaw are averaged consistently; yaw also reported with a
               circular mean and circular std (no +-pi wrap problem)
  - duplicates are dropped (same stamp is never counted twice)
  - optional robust outlier rejection (median + MAD)
  - warns if the robot moved during sampling
"""
import argparse
import math
import sys

import numpy as np
import rclpy
from nav_msgs.msg import Odometry
from rclpy.node import Node
from rclpy.time import Time
from tf2_ros import Buffer, TransformException, TransformListener


# ----------------------------------------------------------------- math ----
def wrap(a):
    return np.arctan2(np.sin(a), np.cos(a))


def quat_to_rpy(x, y, z, w):
    roll = math.atan2(2 * (w * x + y * z), 1 - 2 * (x * x + y * y))
    sinp = 2 * (w * y - z * x)
    pitch = math.copysign(math.pi / 2, sinp) if abs(sinp) >= 1 else math.asin(sinp)
    yaw = math.atan2(2 * (w * z + x * y), 1 - 2 * (y * y + z * z))
    return roll, pitch, yaw


def mean_quat(quats):
    """Average of unit quaternions (xyzw); sign-ambiguity safe."""
    A = np.zeros((4, 4))
    for q in quats:
        A += np.outer(q, q)
    _, vecs = np.linalg.eigh(A / len(quats))
    q = vecs[:, -1]
    return q if q[3] >= 0 else -q


def circ_mean_std(angles):
    s, c = np.sin(angles).mean(), np.cos(angles).mean()
    R = max(math.hypot(s, c), 1e-12)
    return math.atan2(s, c), math.sqrt(-2.0 * math.log(min(R, 1.0)))


def inlier_mask(pos, yaw, k, pos_floor=1e-4, yaw_floor=1e-4):
    """Median + MAD gate on position deviation and yaw deviation."""
    d = np.linalg.norm(pos - np.median(pos, axis=0), axis=1)
    dy = np.abs(wrap(yaw - circ_mean_std(yaw)[0]))

    def limit(v, floor):
        med = np.median(v)
        mad = 1.4826 * np.median(np.abs(v - med))
        return med + k * max(mad, floor)

    return (d <= limit(d, pos_floor)) & (dy <= limit(dy, yaw_floor))


def analyze(samples, reject_k):
    a = np.asarray(samples, dtype=float)
    pos, quat = a[:, :3], a[:, 3:]
    quat = quat / np.linalg.norm(quat, axis=1, keepdims=True)
    rpy = np.array([quat_to_rpy(*q) for q in quat])

    keep = np.ones(len(a), dtype=bool)
    if reject_k > 0 and len(a) >= 10:
        keep = inlier_mask(pos, rpy[:, 2], reject_k)
    pos, quat, rpy = pos[keep], quat[keep], rpy[keep]

    ddof = 1 if len(pos) > 1 else 0
    q_mean = mean_quat(quat)
    yaw_c, yaw_std = circ_mean_std(rpy[:, 2])
    yaw_dev = wrap(rpy[:, 2] - yaw_c)

    return {
        'total': len(a),
        'kept': int(keep.sum()),
        'pos_mean': pos.mean(axis=0),
        'pos_std': pos.std(axis=0, ddof=ddof),
        'pos_ptp': np.ptp(pos, axis=0),
        'rpy_mean': np.array(quat_to_rpy(*q_mean)),
        'roll_std': circ_mean_std(rpy[:, 0])[1],
        'pitch_std': circ_mean_std(rpy[:, 1])[1],
        'yaw_circ_mean': yaw_c,
        'yaw_std': yaw_std,
        'yaw_ptp': float(np.ptp(yaw_dev)),
    }


# ----------------------------------------------------------------- node ----
class Collector(Node):
    def __init__(self, args):
        super().__init__('avg_tf')
        self.args = args
        self.samples = []
        self.last_stamp = None
        self.done = False
        self.label = ''
        self.start = self.get_clock().now()
        self.warned_wait = False

        if args.source == 'tf':
            self.label = f'{args.parent} -> {args.child}'
            self.buffer = Buffer()
            self.listener = TransformListener(self.buffer, self)
            self.create_timer(1.0 / args.rate, self.on_tf_timer)
        else:
            self.create_subscription(Odometry, args.topic, self.on_odom, 100)
        self.create_timer(0.5, self.on_watchdog)

    def add(self, stamp, p, q):
        if stamp != (0, 0) and stamp == self.last_stamp:
            return  # same message / same TF update, do not double count
        self.last_stamp = stamp
        self.samples.append((p[0], p[1], p[2], q[0], q[1], q[2], q[3]))
        if len(self.samples) >= self.args.n:
            self.done = True

    def on_tf_timer(self):
        if self.done:
            return
        try:
            t = self.buffer.lookup_transform(self.args.parent, self.args.child, Time())
        except TransformException:
            return
        tr, ro = t.transform.translation, t.transform.rotation
        self.add((t.header.stamp.sec, t.header.stamp.nanosec),
                 (tr.x, tr.y, tr.z), (ro.x, ro.y, ro.z, ro.w))

    def on_odom(self, m):
        if self.done:
            return
        self.label = f'{m.header.frame_id} -> {m.child_frame_id} (topic {self.args.topic})'
        p, q = m.pose.pose.position, m.pose.pose.orientation
        self.add((m.header.stamp.sec, m.header.stamp.nanosec),
                 (p.x, p.y, p.z), (q.x, q.y, q.z, q.w))

    def on_watchdog(self):
        if self.done:
            return
        elapsed = (self.get_clock().now() - self.start).nanoseconds / 1e9
        if not self.samples and elapsed > 3.0 and not self.warned_wait:
            self.warned_wait = True
            print('等待数据中……（TF 名称或话题名是否正确？节点是否在运行？）', file=sys.stderr)
        if elapsed > self.args.timeout:
            print(f'超时 {self.args.timeout:.0f}s，使用已收集的 {len(self.samples)} 个样本',
                  file=sys.stderr)
            self.done = True


# ----------------------------------------------------------------- main ----
def main():
    ap = argparse.ArgumentParser(description='Average a static pose (TF or Odometry).')
    ap.add_argument('--source', choices=['tf', 'odom'], default='tf')
    ap.add_argument('--parent', default='map')
    ap.add_argument('--child', default='odom')
    ap.add_argument('--topic', default='/aft_mapped_to_init')
    ap.add_argument('-n', type=int, default=200, help='number of distinct samples')
    ap.add_argument('--rate', type=float, default=20.0, help='TF polling rate (Hz)')
    ap.add_argument('--timeout', type=float, default=60.0, help='give up after (s)')
    ap.add_argument('--reject', type=float, default=4.0,
                    help='outlier gate in MAD multiples, 0 disables')
    ap.add_argument('--pos-tol', type=float, default=0.02,
                    help='warn if position peak-to-peak exceeds (m)')
    ap.add_argument('--yaw-tol', type=float, default=0.5,
                    help='warn if yaw peak-to-peak exceeds (deg)')
    args = ap.parse_args()

    rclpy.init()
    node = Collector(args)
    try:
        while rclpy.ok() and not node.done:
            rclpy.spin_once(node, timeout_sec=0.1)
    except KeyboardInterrupt:
        pass
    samples, label = list(node.samples), node.label
    node.destroy_node()
    if rclpy.ok():
        rclpy.shutdown()

    if len(samples) < 2:
        print('样本不足，无法统计。', file=sys.stderr)
        sys.exit(1)

    r = analyze(samples, args.reject)
    d = math.degrees
    px, py, pz = r['pos_mean']
    roll, pitch, yaw = r['rpy_mean']

    print(f'frame      : {label}')
    print(f'samples    : {r["total"]}  (kept {r["kept"]}, rejected {r["total"] - r["kept"]})')
    print('-' * 62)
    print(f'x      = {px:+.4f} m    std {r["pos_std"][0]:.4f}   p2p {r["pos_ptp"][0]:.4f}')
    print(f'y      = {py:+.4f} m    std {r["pos_std"][1]:.4f}   p2p {r["pos_ptp"][1]:.4f}')
    print(f'z      = {pz:+.4f} m    std {r["pos_std"][2]:.4f}   p2p {r["pos_ptp"][2]:.4f}')
    print(f'roll   = {roll:+.5f} rad ({d(roll):+.3f} deg)   std {d(r["roll_std"]):.3f} deg')
    print(f'pitch  = {pitch:+.5f} rad ({d(pitch):+.3f} deg)   std {d(r["pitch_std"]):.3f} deg')
    print(f'yaw    = {yaw:+.5f} rad ({d(yaw):+.3f} deg)   std {d(r["yaw_std"]):.3f} deg   '
          f'p2p {d(r["yaw_ptp"]):.3f} deg')
    print('-' * 62)
    print(f'[x, y, z, roll, pitch, yaw] = '
          f'[{px:.4f}, {py:.4f}, {pz:.4f}, {roll:.5f}, {pitch:.5f}, {yaw:.5f}]')
    print('（单位 m / rad；填进 yaml 前请对照节点参数的实际顺序和格式）')

    if r['pos_ptp'].max() > args.pos_tol or d(r['yaw_ptp']) > args.yaw_tol:
        print('\n[警告] 采样期间位姿波动偏大（车被碰动？配准还没收敛？），'
              '这组平均值不可靠，请保持静止并等收敛后重测。', file=sys.stderr)


if __name__ == '__main__':
    main()
