#!/usr/bin/env python3
"""
对比多个静止里程计 bag（可以是 2 个、3 个或更多）。

用法示例：
  python3 compare_odom_bags.py A=pointlio_static_A B=pointlio_static_B C=pointlio_static_C
  python3 compare_odom_bags.py pointlio_static_A pointlio_static_B          # 不写标签则用目录名
  python3 compare_odom_bags.py --duration 120 --skip 5 A=bagA B=bagB C=bagC

默认会把所有 bag 截成相同长度（取最短的那个），保证对比公平。
--skip      丢掉每个 bag 开头的 N 秒（避开初始化阶段）
--duration  指定统一截取的长度（秒），不写则自动取最短长度
--topic     里程计话题，默认 /lidar_odometry
"""
import argparse
import math
import os
import sys

import numpy as np
import rosbag2_py

from rclpy.serialization import deserialize_message
from rosidl_runtime_py.utilities import get_message


def quaternion_to_rpy(q):
    """四元数转 Roll、Pitch、Yaw，单位为弧度。"""
    x, y, z, w = q.x, q.y, q.z, q.w

    roll = math.atan2(
        2.0 * (w * x + y * z),
        1.0 - 2.0 * (x * x + y * y)
    )

    sin_pitch = 2.0 * (w * y - z * x)
    pitch = math.asin(max(-1.0, min(1.0, sin_pitch)))

    yaw = math.atan2(
        2.0 * (w * z + x * y),
        1.0 - 2.0 * (y * y + z * z)
    )

    return [roll, pitch, yaw]


def load_bag(bag_path, topic):
    reader = rosbag2_py.SequentialReader()

    reader.open(
        rosbag2_py.StorageOptions(uri=bag_path, storage_id="sqlite3"),
        rosbag2_py.ConverterOptions(
            input_serialization_format="cdr",
            output_serialization_format="cdr"
        )
    )

    topic_types = {
        item.name: item.type
        for item in reader.get_all_topics_and_types()
    }

    if topic not in topic_types:
        raise RuntimeError(
            f"{bag_path} 中不存在话题 {topic}；"
            f"实际话题有：{list(topic_types.keys())}"
        )

    msg_type = get_message(topic_types[topic])

    times, positions, attitudes = [], [], []

    while reader.has_next():
        name, data, _ = reader.read_next()
        if name != topic:
            continue

        msg = deserialize_message(data, msg_type)
        p = msg.pose.pose.position
        q = msg.pose.pose.orientation

        # 使用里程计消息自身的时间戳
        t = msg.header.stamp.sec + msg.header.stamp.nanosec * 1e-9

        times.append(t)
        positions.append([p.x, p.y, p.z])
        attitudes.append(quaternion_to_rpy(q))

    if len(times) < 10:
        raise RuntimeError(f"{bag_path} 中读取到的有效里程计消息不足")

    order = np.argsort(times)
    times = np.asarray(times)[order]
    positions = np.asarray(positions)[order]
    attitudes = np.unwrap(np.asarray(attitudes)[order], axis=0)

    return times, positions, attitudes


def trim(t, pos, rpy, skip, duration):
    """丢掉开头 skip 秒，再截取 duration 秒（duration 为 None 则取到结尾）。"""
    t0 = t[0] + skip
    mask = t >= t0
    if duration is not None:
        mask &= t <= t0 + duration
    return t[mask], pos[mask], rpy[mask]


def linear_fit(t, y):
    """对每一列做一次线性拟合，返回斜率（单位/秒）和去趋势后的标准差。"""
    tt = t - t[0]
    slopes, resid_std = [], []
    for i in range(y.shape[1]):
        k, b = np.polyfit(tt, y[:, i], 1)
        res = y[:, i] - (k * tt + b)
        slopes.append(k)
        resid_std.append(np.std(res, ddof=1))
    return np.array(slopes), np.array(resid_std)


def frame_jitter(y):
    """
    逐帧抖动：相邻两帧差值的标准差 / sqrt(2)。
    对白噪声来说等于噪声标准差，并且基本不受慢漂移影响。
    """
    return np.std(np.diff(y, axis=0), axis=0, ddof=1) / math.sqrt(2.0)


def analyze(t, pos, rpy):
    duration = t[-1] - t[0]
    window = min(30.0, duration / 4.0)

    start_mean = np.mean(pos[t <= t[0] + window], axis=0)
    end_mean = np.mean(pos[t >= t[-1] - window], axis=0)
    head_tail = end_mean - start_mean

    rpy_deg = np.rad2deg(rpy)

    pos_slope, pos_detr = linear_fit(t, pos)

    return {
        "n": len(t),
        "duration": duration,
        "hz": (len(t) - 1) / duration,
        "window": window,
        "pos_std_cm": np.std(pos, axis=0, ddof=1) * 100,
        "pos_detr_cm": pos_detr * 100,
        "pos_jitter_cm": frame_jitter(pos) * 100,
        "drift_mm_min": pos_slope * 60 * 1000,
        "head_tail_mm": head_tail * 1000,
        "head_tail_cm": np.linalg.norm(head_tail) * 100,
        "rpy_std_deg": np.std(rpy_deg, axis=0, ddof=1),
        "rpy_jitter_deg": frame_jitter(rpy_deg),
    }


def fmt(v, digits=3):
    return " / ".join(f"{x:.{digits}f}" for x in np.atleast_1d(v))


def parse_bag_arg(arg):
    """支持 '标签=路径' 或直接给路径。"""
    if "=" in arg:
        label, path = arg.split("=", 1)
    else:
        path = arg
        label = os.path.basename(os.path.normpath(arg))
    return label, path


def main():
    ap = argparse.ArgumentParser(description="对比多个静止里程计 bag")
    ap.add_argument("bags", nargs="+", help="bag 路径，可写成 标签=路径")
    ap.add_argument("--topic", default="/lidar_odometry")
    ap.add_argument("--skip", type=float, default=0.0,
                    help="丢掉每个 bag 开头的秒数")
    ap.add_argument("--duration", type=float, default=None,
                    help="统一截取长度（秒），默认取最短 bag 的长度")
    args = ap.parse_args()

    entries = [parse_bag_arg(a) for a in args.bags]

    # 先全部读入并去掉开头 skip 秒
    loaded = []
    for label, path in entries:
        t, pos, rpy = load_bag(path, args.topic)
        t, pos, rpy = trim(t, pos, rpy, args.skip, None)
        if len(t) < 10:
            raise RuntimeError(f"{label} 在丢掉前 {args.skip}s 后数据不足")
        loaded.append((label, path, t, pos, rpy))

    shortest = min(item[2][-1] - item[2][0] for item in loaded)
    common = shortest if args.duration is None else args.duration
    if common > shortest:
        print(f"[警告] 指定的 {common:.1f}s 超过最短 bag（{shortest:.1f}s），"
              f"已改为 {shortest:.1f}s")
        common = shortest

    print(f"统一对比长度: {common:.1f} s（每个 bag 去掉开头 {args.skip:.1f}s 后再截取）")

    results = []
    for label, path, t, pos, rpy in loaded:
        t, pos, rpy = trim(t, pos, rpy, 0.0, common)
        r = analyze(t, pos, rpy)
        results.append((label, path, r))

    # 逐个 bag 的详细输出
    for label, path, r in results:
        print(f"\n========== {label} ==========")
        print(f"路径: {path}")
        print(f"消息数量: {r['n']}，时间跨度: {r['duration']:.2f} s，"
              f"平均频率: {r['hz']:.2f} Hz")
        print(f"位置标准差 XYZ (cm):          {fmt(r['pos_std_cm'])}")
        print(f"去趋势后位置标准差 XYZ (cm):  {fmt(r['pos_detr_cm'])}")
        print(f"逐帧抖动 XYZ (cm):            {fmt(r['pos_jitter_cm'])}")
        print(f"漂移速度 XYZ (mm/min):        {fmt(r['drift_mm_min'])}")
        print(f"首尾均值差 XYZ (mm):          {fmt(r['head_tail_mm'])}"
              f"  （窗口 {r['window']:.1f}s，距离 {r['head_tail_cm']:.3f} cm）")
        print(f"姿态标准差 RPY (deg):         {fmt(r['rpy_std_deg'], 4)}")
        print(f"姿态逐帧抖动 RPY (deg):       {fmt(r['rpy_jitter_deg'], 4)}")

    # 汇总：按指标分组，方便横向对比
    print("\n\n================ 汇总对比 ================")
    metrics = [
        ("位置标准差 XYZ (cm)", "pos_std_cm", 3),
        ("去趋势后位置标准差 XYZ (cm)", "pos_detr_cm", 3),
        ("逐帧抖动 XYZ (cm)", "pos_jitter_cm", 3),
        ("漂移速度 XYZ (mm/min)", "drift_mm_min", 3),
        ("首尾均值差距离 (cm)", "head_tail_cm", 3),
        ("姿态标准差 RPY (deg)", "rpy_std_deg", 4),
        ("姿态逐帧抖动 RPY (deg)", "rpy_jitter_deg", 4),
    ]
    for title, key, digits in metrics:
        print(f"\n{title}")
        for label, _, r in results:
            print(f"  {label:<20s} {fmt(r[key], digits)}")

    print("\n读法提示：")
    print("  逐帧抖动 —— 高频抖动，最接近 rviz 里肉眼看到的“晃不晃”")
    print("  漂移速度 / 首尾均值差 —— 慢漂移，静止越久越明显")
    print("  去趋势后标准差 —— 把线性漂移去掉后剩下的波动")


if __name__ == "__main__":
    try:
        main()
    except RuntimeError as e:
        print(f"[错误] {e}")
        sys.exit(1)
