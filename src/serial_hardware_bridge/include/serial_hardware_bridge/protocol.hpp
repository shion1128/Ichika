#ifndef SERIAL_HARDWARE_BRIDGE__PROTOCOL_HPP_
#define SERIAL_HARDWARE_BRIDGE__PROTOCOL_HPP_

#include <cstdint>

namespace serial_hardware_bridge
{

// 强制一字节对齐，防止 C++ 编译器自动填充导致与 STM32 内存结构不一致
#pragma pack(push, 1)

    /**
     * @brief 发送给 STM32 的控制指令包 (例如：底盘速度)
     */
    struct ChassisControlPacket
    {
        uint8_t header[2] = {0x55, 0xAA}; // 帧头

        int16_t vx; // mm/s
        int16_t vy; // mm/s
        int16_t vw; // mrad/s

        uint8_t crc8;        // 校验位W
        uint8_t tail = 0x77; // 帧尾
    };

#pragma pack(pop)

} // namespace serial_hardware_bridge

#endif // SERIAL_HARDWARE_BRIDGE__PROTOCOL_HPP_
