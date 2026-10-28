#!/usr/bin/env python3
"""Average Point-LIO odom pose over N messages.

Usage: python3 avg_pose.py [odom_topic] [N]
Keep the robot still while it runs. Yaw is a circular mean.
"""
import sys
import math
import rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry


class AvgPose(Node):
    def __init__(self, topic, n):
        super().__init__('avg_pose')
        self.n = n
        self.buf = []
        self.create_subscription(Odometry, topic, self.cb, 10)

    def cb(self, m):
        p, q = m.pose.pose.position, m.pose.pose.orientation
        yaw = math.atan2(2 * (q.w * q.z + q.x * q.y),
                         1 - 2 * (q.y * q.y + q.z * q.z))
        self.buf.append((p.x, p.y, p.z, yaw))
        if len(self.buf) < self.n:
            return
        k = len(self.buf)
        mean = [sum(v[i] for v in self.buf) / k for i in range(3)]
        std = [(sum((v[i] - mean[i]) ** 2 for v in self.buf) / k) ** 0.5
               for i in range(3)]
        yaw = math.atan2(sum(math.sin(v[3]) for v in self.buf),
                         sum(math.cos(v[3]) for v in self.buf))
        print(f'frame_id={m.header.frame_id}  child_frame_id={m.child_frame_id}')
        print(f'x={mean[0]:.4f}  y={mean[1]:.4f}  z={mean[2]:.4f}  '
              f'yaw={yaw:.4f} rad ({math.degrees(yaw):.2f} deg)')
        print(f'std  x={std[0]:.4f}  y={std[1]:.4f}  z={std[2]:.4f}')
        rclpy.shutdown()


def main():
    topic = sys.argv[1] if len(sys.argv) > 1 else '/aft_mapped_to_init'
    n = int(sys.argv[2]) if len(sys.argv) > 2 else 500
    rclpy.init()
    rclpy.spin(AvgPose(topic, n))


if __name__ == '__main__':
    main()
