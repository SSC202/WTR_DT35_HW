/**
 * @file Laser.c
 * @author Lary (you@domain.com)
 * @brief  注意转接板2pin输出端AB线接反了
 *         使用了两个DT35
 * @version 0.1
 * @date 2025-04-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Laser.h"
#include "string.h"
#include "stdio.h"

//近点远点真实距离（/m）和原始数据 
//DT35设置的实际近点是0.3m，远点是7.5m
const float MIN_DISTANCE0=0.4;
const float MIN_DISTANCE1=0.4;
const float MAX_DISTANCE0=7.4;
const float MAX_DISTANCE1=7.4;
const float DISTANCE_DIFF0=(MAX_DISTANCE0-MIN_DISTANCE0);
const float DISTANCE_DIFF1=(MAX_DISTANCE0-MIN_DISTANCE1);

const float MIN_RAWDATA0=200130;
const float MIN_RAWDATA1=198900;
const float MAX_RAWDATA0=6452000;
const float MAX_RAWDATA1=6447500;
const float RAWDATA_DIFF1=(MAX_RAWDATA0-MIN_RAWDATA1);
const float RAWDATA_DIFF0=(MAX_RAWDATA0-MIN_RAWDATA0);

static uint32_t Laser_rawdata[4] = {0};
float Laser_x = 0;
float Laser_y = 0;
//uint8_t Laser_rev_buffer[24];
uint8_t Laser_rev_byte = 0;
uint8_t Laser_correct = 0;

void Laser_rev_Init()
{
    HAL_UART_Receive_IT(&LASER_DATA_UART_HANDLE, &Laser_rev_byte, 1);
}


void Laser_Data_Decode(void)
{
    Laser_correct = 0;  // 默认数据错误

    do {
        if (Laser_rev_buffer[0] != FRAME_HEADER1 || Laser_rev_buffer[1] != FRAME_HEADER2 ||
            Laser_rev_buffer[22] != FRAME_TAIL1   || Laser_rev_buffer[23] != FRAME_TAIL2) {
            break;
        }

        if (Laser_rev_buffer[2] != 0x01) break;
        memcpy(&Laser_rawdata[0], &Laser_rev_buffer[3], 4);

        if (Laser_rev_buffer[7] != 0x02) break;
        memcpy(&Laser_rawdata[1], &Laser_rev_buffer[8], 4);

        if (Laser_rev_buffer[12] != 0x03) break;
        memcpy(&Laser_rawdata[2], &Laser_rev_buffer[13], 4);

        if (Laser_rev_buffer[17] != 0x04) break;
        memcpy(&Laser_rawdata[3], &Laser_rev_buffer[18], 4);

        Laser_correct = 1;  // 全部通过校验
    } while (0);

    // 重新挂接收中断
    HAL_UART_Receive_IT(&LASER_DATA_UART_HANDLE, (uint8_t*)&Laser_rev_buffer, sizeof(Laser_rev_buffer));
}


/**
 * @brief 24字节Laser_rev_buffer解码
 * 
 */
void Laser_Buffer_Decode()
{
    Laser_correct = 0;  // 默认数据错误
    // 尝试对齐到帧头（0xAA 0xBB）
    if (!Msg_SyncToHeader()) return;

    // 确认缓冲区中至少有一个完整帧
    while (Msg_GetLength() >= 24) {
        // 确保当前开头仍是帧头
        if (Msg_Read(0) != FRAME_HEADER1 || Msg_Read(1) != FRAME_HEADER2) {
            Msg_AddReadIndex(1);
            if (!Msg_SyncToHeader()) return;
            continue;
        }

        // 检查帧尾
        if (Msg_Read(22) != FRAME_TAIL1 || Msg_Read(23) != FRAME_TAIL2) {
            Msg_AddReadIndex(1);
            if (!Msg_SyncToHeader()) return;
            continue;
        }

        // 校验每段标志位
        if (Msg_Read(2) != 0x01 || Msg_Read(7) != 0x02 || 
            Msg_Read(12) != 0x03 || Msg_Read(17) != 0x04) {
            Msg_AddReadIndex(1);
            if (!Msg_SyncToHeader()) return;
            continue;
        }

        // 解码数据
        for (int i = 0; i < 4; i++) {
            uint8_t offset = 3 + i * 5;
            Laser_rawdata[i] = (Msg_Read(offset) << 24) |
                               (Msg_Read(offset + 1) << 16) |
                               (Msg_Read(offset + 2) << 8) |
                               (Msg_Read(offset + 3));
        }

        // 数据转换
        Laser_x = ((float)(Laser_rawdata[0] - MIN_RAWDATA0)) / RAWDATA_DIFF0 * DISTANCE_DIFF0*0.92593 + MIN_DISTANCE0 ;
        Laser_y = ((float)(Laser_rawdata[1] - MIN_RAWDATA1)) / RAWDATA_DIFF1 * DISTANCE_DIFF1*0.92783 + MIN_DISTANCE1 ;
        Laser_correct = 1;

        Msg_AddReadIndex(24);  // 消费这一帧

        // 尝试查找下一个帧头
        if (!Msg_SyncToHeader()) return;
    }
}

