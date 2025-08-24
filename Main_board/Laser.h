#ifndef _LASER_H
#define _LASER_H

#include "usermain.h"

#define LASER_DATA_UART_HANDLE huart3 // 串口选项
#define LASER_DATA_UART        USART3  // 串口选项

#define FRAME_HEADER1 0xAA
#define FRAME_HEADER2 0xBB
#define FRAME_TAIL1   0xCC
#define FRAME_TAIL2   0xDD

extern uint8_t Laser_rev_byte;
extern uint8_t Laser_rev_buffer[24];
extern float Laser_x ;
extern float Laser_y ;

void Laser_rev_Init();
void Laser_Data_Decode();
void Laser_Buffer_Decode();

#endif 