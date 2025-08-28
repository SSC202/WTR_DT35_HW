#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include "main.h"
#include <stdbool.h>
#include <string.h>

// 写入数据
uint8_t Msg_Write(uint8_t *data, uint8_t length);

// 读取数据
uint8_t Msg_Read(uint8_t offset);                 // 逻辑偏移读取
void Msg_AddReadIndex(uint8_t length);            // 增加读索引
uint8_t Msg_GetLength();                          // 获取未处理数据长度
uint8_t Msg_GetRemain();                          // 获取剩余可写空间

// 帧相关功能
bool Msg_SyncToHeader(void);                      // 帧头同步（查找 0xAA 0xBB）

// 获取完整帧函数（可选保留）
uint8_t Msg_GetCommand(uint8_t *msg);

#endif // CIRCULAR_BUFFER_H
