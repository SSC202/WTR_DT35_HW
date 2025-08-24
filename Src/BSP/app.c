#include "app.h"

int flag = 0;

/**
 * 发送数据包(24Byte)
 * 0xAA 0XBB 0x01 [4Byte] 0x02 [4Byte] 0x03 [4Byte] 0x04 [4Byte] 0xCC 0xDD
 */
uint8_t uart_send_buffer[24];

/**
 * @brief   32位整型转换为4个8位整型数据
 */
void int32_to_4uint8(int32_t input_num, uint8_t output_num[])
{
    output_num[0] = (input_num >> 24) & 0xFF;
    output_num[1] = (input_num >> 16) & 0xFF;
    output_num[2] = (input_num >> 8) & 0xFF;
    output_num[2] = (input_num) & 0xFF;
}

/**
 * @brief   串口数据发送
 */
void uart_send(uint8_t channel1[], uint8_t channel2[], uint8_t channel3[], uint8_t channel4[])
{
    uart_send_buffer[0] = 0xAA;
    uart_send_buffer[1] = 0xBB;
    uart_send_buffer[2] = 0x01;
    for (int i = 0; i < 4; i++) {
        uart_send_buffer[i + 3] = channel1[i];
    }
    uart_send_buffer[7] = 0x02;
    for (int i = 0; i < 4; i++) {
        uart_send_buffer[i + 8] = channel2[i];
    }
    uart_send_buffer[12] = 0x03;
    for (int i = 0; i < 4; i++) {
        uart_send_buffer[i + 13] = channel3[i];
    }
    uart_send_buffer[17] = 0x04;
    for (int i = 0; i < 4; i++) {
        uart_send_buffer[i + 18] = channel4[i];
    }
    uart_send_buffer[22] = 0xCC;
    uart_send_buffer[23] = 0xDD;
    HAL_UART_Transmit_DMA(&huart1, uart_send_buffer, 24);
}