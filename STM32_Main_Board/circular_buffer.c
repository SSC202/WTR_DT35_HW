#include "circular_buffer.h"

// 缓冲区常量
#define BUFFER_SIZE 128
#define FRAME_HEADER1 0xAA
#define FRAME_HEADER2 0xBB

// 缓冲区及索引
static uint8_t buffer[BUFFER_SIZE];
static uint8_t readIndex = 0;
static uint8_t writeIndex = 0;

void Msg_AddReadIndex(uint8_t length) {
    readIndex = (readIndex + length) % BUFFER_SIZE;
}

uint8_t Msg_Read(uint8_t offset) {
    uint8_t index = (readIndex + offset) % BUFFER_SIZE;
    return buffer[index];
}

uint8_t Msg_GetLength() {
    return (writeIndex + BUFFER_SIZE - readIndex) % BUFFER_SIZE;
}

uint8_t Msg_GetRemain() {
    return BUFFER_SIZE - Msg_GetLength();
}

uint8_t Msg_Write(uint8_t *data, uint8_t length) {
    if (Msg_GetRemain() < length) return 0;

    if (writeIndex + length < BUFFER_SIZE) {
        memcpy(buffer + writeIndex, data, length);
        writeIndex += length;
    } else {
        uint8_t firstPart = BUFFER_SIZE - writeIndex;
        memcpy(buffer + writeIndex, data, firstPart);
        memcpy(buffer, data + firstPart, length - firstPart);
        writeIndex = length - firstPart;
    }

    writeIndex %= BUFFER_SIZE;
    return length;
}

bool Msg_SyncToHeader(void) {
    while (Msg_GetLength() >= 2) {
        if (Msg_Read(0) == FRAME_HEADER1 && Msg_Read(1) == FRAME_HEADER2) {
            return true;
        }
        Msg_AddReadIndex(1);
    }
    return false;
}

// 保留旧的 Msg_GetCommand（如不再使用，可删除）
uint8_t Msg_GetCommand(uint8_t *msg) {
    while (1) {
        if (Msg_GetLength() < 4) return 0;
        if (Msg_Read(0) != 0xAA) {
            Msg_AddReadIndex(1);
            continue;
        }

        uint8_t length = Msg_Read(1);
        if (Msg_GetLength() < length) return 0;

        uint8_t sum = 0;
        for (uint8_t i = 0; i < length - 1; i++) {
            sum += Msg_Read(i);
        }
        if (sum != Msg_Read(length - 1)) {
            Msg_AddReadIndex(1);
            continue;
        }

        for (uint8_t i = 0; i < length; i++) {
            msg[i] = Msg_Read(i);
        }
        Msg_AddReadIndex(length);
        return length;
    }
}
