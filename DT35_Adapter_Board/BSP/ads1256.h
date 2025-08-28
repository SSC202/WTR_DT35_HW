/**
 * @file wtr_ads1256.c
 * @author TITH (1023515576@qq.com)
 * @test  Liam (quan.2003@outlook.com)  添加了相应注释
 * @test  WQ.Y.
 * @test  SSC (e22750706642022@163.com) 封装为ADS1256+STM32F103C8T6转接板的固件
 * @brief ADS1256的驱动程序
 * @version 1.2
 * @date 2023-10-10
 *
 * @copyright Copyright (c) 2023
 *
 * 1. 关于CubeMX SPI的配置
 * 选择Band Rate <= 1,919,385.7965Bits/s
 * CPOL      Low
 * CPHA      2 Edge   即第二个跳变沿数据被采集
 *
 * 2. 引脚配置
 * DRDY      GPIO_Input         DRDY_n为低，表示数据有效(ADS1256向STM32传值)
 * Reset     GPIO_Output
 *
 * 3. TIM配置
 * 分频为1us时基单元
 * 
 * 4. DT35配置
 * 配置为电压模式，校准时采用近远点校准
 */
#ifndef __ADS1256_H
#define __ADS1256_H

#include "spi.h"
#include "tim.h"
#include "stm32f1xx.h"

/****************************Userconfig******************************** */
#define ADS1256_SPI_Port        hspi1      // SPI 接口
#define ADS1256_TIM_Port        htim3      // TIM 接口
#define ADS1256_DRDY_GPIO_PIN   GPIO_PIN_3 // DRDY 接口
#define ADS1256_DRDY_GPIO_PORT  GPIOA
#define ADS1256_RESET_GPIO_PIN  GPIO_PIN_4 // RESET 接口
#define ADS1256_RESET_GPIO_PORT GPIOA
#define USE_FreeRTOS            0

// 寄存器定义
/**
 * 状态寄存器
 * BIT7 | BIT6 | BIT5 | BIT4 | BIT3  | BIT2 | BIT1  | BIT0
 * ID   | ID   | ID   | ID   | ORDER | ACAL | BUFEN | DRDY
 *
 * ID:      出厂编程识别位(只读)
 * ORDER:   输出数据先行(0:MSB;1:LSB)
 * ACAL:    自动校准使能(0:禁止;1:使能)
 *          启用自动校准后，在 WREG 命令更改 PGA(寄存器 ADCON 的第 0-2 位),DR(寄存器 DRATE 的第 7-0 位), BUFEN(寄存器 STATUS 的第 1 位)值时，自校准即开始。
 * BUFEN:   模拟输入缓冲器启用(0:禁止;1:使能)
 * DRDY:    采样数据可以读取(只读)
 */
#define ADS1256_STATUS 0x00 // 状态寄存器
/**
 * 输入控制寄存器
 * BIT7  | BIT6  | BIT5  | BIT4  | BIT3  | BIT2  | BIT1  | BIT0
 * PSEL3 | PSEL2 | PSEL1 | PSEL0 | NSEL3 | NSEL2 | NSEL1 | NSEL0
 *
 * PSELx: 差分正输入引脚
 * NSELx: 差分负输入引脚
 *
 * 0000 = AIN0 (default)
 * 0001 = AIN1
 * 0010 = AIN2 (ADS1256 only)
 * 0011 = AIN3 (ADS1256 only)
 * 0100 = AIN4 (ADS1256 only)
 * 0101 = AIN5 (ADS1256 only)
 * 0110 = AIN6 (ADS1256 only)
 * 0111 = AIN7 (ADS1256 only)
 * 1xxx = AINCOM (when PSEL3 = 1, PSEL2, PSEL1, PSEL0 are “don’t care”)
 */
#define ADS1256_MUX 0x01 // 输入控制寄存器
/**
 * A/D控制寄存器
 * BIT7  | BIT6  | BIT5  | BIT4  | BIT3  | BIT2  | BIT1  | BIT0
 * 0     | CLK1  | CLK0  | SDCS1 | SDCS0 | PGA2  | PGA1  | PGA0
 *
 * CLKx:    时钟输出设置(不使用 CLKOUT 时，建议将其关闭。这些位只能通过 RESET 引脚复位)
 * SDCSx:   传感器检测电流源(激活传感器检测电流源可验证向 ADS1255/6 提供信号的外部传感器的完整性。短路的传感器会产生很小的信号，而开路的传感器会产生很大的信号)
 * PGAx:    可编程增益放大器设置
 */
#define ADS1256_ADCON 0x02 // A/D控制寄存器
/**
 * A/D速率设置
 * BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0
 * DR7  | DR6  | DR5  | DR4  | DR3  | DR2  | DR1  | DR0
 *
 * 11110000 = 30,000SPS (default)
 * 11100000 = 15,000SPS
 * 11010000 = 7,500SPS
 * 11000000 = 3,750SPS
 * 10110000 = 2,000SPS
 * 10100001 = 1,000SPS
 * 10010010 = 500SPS
 * 10000010 = 100SPS
 * 01110010 = 60SPS
 * 01100011 = 50SPS
 * 01010011 = 30SPS
 * 01000011 = 25SPS
 * 00110011 = 15SPS
 * 00100011 = 10SPS
 * 00010011 = 5SPS
 * 00000011 = 2.5SPS
 */
#define ADS1256_DRATE 0x03 // A/D 速率设置寄存器
/**
 * 数字IO引脚设置
 * BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0
 * DIR3 | DIR2 | DIR1 | DIR3 | D1O3 | D1O2 | DIO1 | DIO0
 *
 * ADS1256 有 4 个 I/O 引脚： D3,D2,D1 和 D0/CLKOUT
 */
#define ADS1256_IO 0x04 // 数字 I/O 引脚设置寄存器
// 偏移校准寄存器
#define ADS1256_OFC0 0x05
#define ADS1256_OFC1 0x06
#define ADS1256_OFC2 0x07
// 满量程校准寄存器
#define ADS1256_FSC0 0x08
#define ADS1256_FSC1 0x09
#define ADS1256_FSC2 0x0A

// A/D 速率设置
#define ADS1256_DRATE_2_5   0x03
#define ADS1256_DRATE_5     0x13
#define ADS1256_DRATE_10    0x23
#define ADS1256_DRATE_15    0x33
#define ADS1256_DRATE_25    0x43
#define ADS1256_DRATE_30    0x53
#define ADS1256_DRATE_50    0x63
#define ADS1256_DRATE_60    0x72
#define ADS1256_DRATE_100   0x82
#define ADS1256_DRATE_500   0x92
#define ADS1256_DRATE_1000  0xA1
#define ADS1256_DRATE_2000  0xB0
#define ADS1256_DRATE_3750  0xC0
#define ADS1256_DRATE_7500  0xD0
#define ADS1256_DRATE_15000 0xE0
#define ADS1256_DRATE_30000 0xF0

// 命令
#define ADS1256_CMD_RDATA    0x01 // 单次读取AD转换数据(DRDY低电平后发出)
#define ADS1256_CMD_RDATAC   0x03 // 开始连续读取AD转换数据
#define ADS1256_CMD_SDATAC   0x0F // 停止连续读取AD转换数据
#define ADS1256_CMD_WREG     0x50 // (第一指令)写入寄存器xxx, 0101 xxxx(0x5x)——从xxx开始，第二指令为写入几个寄存器
#define ADS1256_CMD_RREG     0x10 // (第一指令)读取寄存器xxx, 0001 xxxx(0x1x)——从xxx开始，第二指令为读取几个寄存器
#define ADS1256_CMD_SELFCAL  0xF0 // 偏移和增益自校准
#define ADS1256_CMD_SELFOCAL 0xF1 // 偏移校准
#define ADS1256_CMD_SELFGCAL 0xF2 // 增益校准
#define ADS1256_CMD_SYSOCAL  0xF3 // 系统偏移校准
#define ADS1256_CMD_SYSGCAL  0xF4 // 系统增益校准
#define ADS1256_CMD_SYNC     0xFC // AD转换同步指令
#define ADS1256_CMD_STANDBY  0xFD // 开始待机模式
#define ADS1256_CMD_RESET    0xFE // 恢复出厂设置
#define ADS1256_CMD_WAKEUP   0xFF // 完成同步并退出待机模式

typedef struct ADS1256_all_register {
    uint8_t STATUS;
    uint8_t MUX;
    uint8_t ADCON;
    uint8_t DRATE;
    uint8_t IO;
} ADS1256_ALL_REG;

extern int32_t AD_diff_data[4]; // 外部数据接口,4路差分电平

void ADS1256_Init(void);
int32_t ADS1256ReadData(uint8_t pIChannel, uint8_t nIChannel);
void ADS1256_UpdateDiffData(void);

#endif