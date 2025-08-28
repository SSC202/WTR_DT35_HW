#include "ads1256.h"

int32_t AD_diff_data[4];

/**
 * @brief 微秒延时函数
 */
inline void delay_us(uint32_t us)
{
    uint32_t cnt = 0;
    __HAL_TIM_SET_COUNTER(&ADS1256_TIM_Port, 0);
    __HAL_TIM_SET_AUTORELOAD(&ADS1256_TIM_Port, us);
    HAL_TIM_Base_Start(&ADS1256_TIM_Port);
    while (cnt != us) {
        cnt = __HAL_TIM_GET_COUNTER(&ADS1256_TIM_Port);
    }
    HAL_TIM_Base_Stop(&ADS1256_TIM_Port);
}

/**
 * @brief 向 ADS1256 同时读写数据
 * @param txData 要写入的数据
 * @return uint8_t 读到的数据
 */
inline uint8_t ADS1256_SPI_RW(uint8_t txdata)
{
    uint8_t rxdata;
    HAL_SPI_TransmitReceive(&ADS1256_SPI_Port, &txdata, &rxdata, 1, 0xFF);
    return rxdata;
}
/**
 * @brief 根据 DRDY 引脚电平判断芯片是否准备就绪
 * @return uint8_t 1:就绪;0:未就绪
 */
inline uint8_t ADS1256_IsReady()
{
    return !HAL_GPIO_ReadPin(ADS1256_DRDY_GPIO_PORT, ADS1256_DRDY_GPIO_PIN);
}

/**
 * @brief 重置ADS1256
 */
inline void ADS1256_Reset()
{
    HAL_GPIO_WritePin(ADS1256_RESET_GPIO_PORT, ADS1256_RESET_GPIO_PIN, GPIO_PIN_RESET);
#if (USE_FreeRTOS == 1)
    osDelay(10);
#else
    HAL_Delay(10);
#endif
    HAL_GPIO_WritePin(ADS1256_RESET_GPIO_PORT, ADS1256_RESET_GPIO_PIN, GPIO_PIN_SET);
#if (USE_FreeRTOS == 1)
    osDelay(10);
#else
    HAL_Delay(10);
#endif
}

/**
 * @brief 片选，要对ADS1256通信，必须开启ADS1256的片选，没有片选引脚可忽略此函数
 * @param value 1:片选开启;0:片选关闭
 */
inline void ADS1256_NSS(uint8_t value)
{
#ifdef ADS1256_NSS_Enable
    if (value == 0) {
        delay_us(2);
        HAL_GPIO_WritePin(ADS1256_NSS_GPIOx, ADS1256_NSS_GPIO_Pin, GPIO_PIN_SET); // 高电平为关
    } else {
        HAL_GPIO_WritePin(ADS1256_NSS_GPIOx, ADS1256_NSS_GPIO_Pin, GPIO_PIN_RESET); // 低电平为开
    }
#endif
}

/**
 * @brief ADS1256写入单个寄存器
 * @param regaddr 寄存器地址
 * @param databyte 要写入的数据(1字节,8位)
 */
inline void ADS1256_WREG(uint8_t regaddr, uint8_t databyte)
{
    ADS1256_NSS(1);

    while (!ADS1256_IsReady());                          // 等待DRDY拉低
    ADS1256_SPI_RW(ADS1256_CMD_WREG | (regaddr & 0x0F)); // 第一指令,发送寄存器地址
    delay_us(5);
    ADS1256_SPI_RW(0x00); // 第二指令,写入一个寄存器
    delay_us(5);
    ADS1256_SPI_RW(databyte);
    delay_us(5);
    ADS1256_NSS(0);
}

/**
 * @brief   ADS1256读取前五个寄存器
 * @return  ADS1256_ALL_REG 前五个寄存器数据
 */
inline ADS1256_ALL_REG ADS1256_RREG_All(void)
{
    ADS1256_NSS(1);

    ADS1256_ALL_REG result;
    while (!ADS1256_IsReady());              // 等待DRDY拉低
    ADS1256_SPI_RW(ADS1256_CMD_RREG | 0x00); // 第一指令,发送寄存器地址
    delay_us(5);
    ADS1256_SPI_RW(0x04); // 第二指令,读取5个寄存器
    delay_us(10);
    result.STATUS = ADS1256_SPI_RW(0xff);
    delay_us(5);
    result.MUX = ADS1256_SPI_RW(0xff);
    delay_us(5);
    result.ADCON = ADS1256_SPI_RW(0xff);
    delay_us(5);
    result.DRATE = ADS1256_SPI_RW(0xff);
    delay_us(5);
    result.IO = ADS1256_SPI_RW(0xff);
    delay_us(5);

    ADS1256_NSS(0);
    return result;
}

/**
 * @brief 发送读取数据命令并对读取的ADC数据解码
 * @return int32_t 解码后的数据
 */
int32_t ADS1256_RDATA()
{
    ADS1256_NSS(1);

    ADS1256_SPI_RW(ADS1256_CMD_RDATA);
    int32_t result     = 0;
    uint8_t RxGroup[3] = {0};
    uint8_t TxGroup[3] = {0};

    delay_us(5); // 发出 RDATA 需要等待一段时间才能正常接收

    HAL_SPI_TransmitReceive(&ADS1256_SPI_Port, TxGroup, RxGroup, 3, HAL_MAX_DELAY);

    // 将读取的三段数据拼接
    result |= ((int32_t)RxGroup[0] << 16);
    result |= ((int32_t)RxGroup[1] << 8);
    result |= (int32_t)RxGroup[2];

    // 处理负数
    if (result & 0x800000) {
        result = ~(unsigned long)result;
        result &= 0x7fffff;
        result += 1;
        result = -result;
    }

    ADS1256_NSS(0);
    return result;
}

/**********************************User API****************************************** */
/**
 * @brief   ADS1256 初始化函数
 */
void ADS1256_Init(void)
{
    ADS1256_NSS(1);

    // 重置 ADS1256
    ADS1256_Reset();

#if (USE_FreeRTOS == 1)
    osDelay(100);
#else
    HAL_Delay(100);
#endif

    // 开启自校准并使能输入缓冲
    ADS1256_WREG(ADS1256_STATUS, 0x06);

#if (USE_FreeRTOS == 1)
    osDelay(100);
#else
    HAL_Delay(100);
#endif

    // 差分正输入引脚 AIN0 (default)
    // 差分负输入引脚 AINCOM
    ADS1256_WREG(ADS1256_MUX, 0x08);

#if (USE_FreeRTOS == 1)
    osDelay(100);
#else
    HAL_Delay(100);
#endif

    // 禁止时钟输出
    ADS1256_WREG(ADS1256_ADCON, 0x00);

#if (USE_FreeRTOS == 1)
    osDelay(100);
#else
    HAL_Delay(100);
#endif

    // 1000SPS AD转换速率
    ADS1256_WREG(ADS1256_DRATE, ADS1256_DRATE_1000);

#if (USE_FreeRTOS == 1)
    osDelay(100);
#else
    HAL_Delay(100);
#endif

    // IO 引脚电平全部设置为0
    ADS1256_WREG(ADS1256_IO, 0x00);

#if (USE_FreeRTOS == 1)
    osDelay(100);
#else
    HAL_Delay(100);
#endif

    // 同步并进入工作状态
    ADS1256_SPI_RW(ADS1256_CMD_SYNC);
    ADS1256_SPI_RW(ADS1256_CMD_WAKEUP);

#if (USE_FreeRTOS == 1)
    osDelay(100);
#else
    HAL_Delay(100);
#endif

    ADS1256_NSS(0);
}

/**
 * @brief   读取一个通道的数据
 * @param   pIChannel 差分正引脚号
 * @param   nIChannel 差分负引脚号
 * @return  
 */
int32_t ADS1256ReadData(uint8_t pIChannel, uint8_t nIChannel)
{
    ADS1256_NSS(1);

    int32_t result;
    static uint8_t channel = 0;

    while (!ADS1256_IsReady());

    if (channel != ((pIChannel << 4) | nIChannel)) {
        channel = (pIChannel << 4) | nIChannel;
        ADS1256_WREG(ADS1256_MUX, channel); // 选择通道
    }

    ADS1256_SPI_RW(ADS1256_CMD_SYNC);
    ADS1256_SPI_RW(ADS1256_CMD_WAKEUP);

    while (!ADS1256_IsReady());
    result = ADS1256_RDATA();

    ADS1256_NSS(0);
    return result;
}

/**
 * @brief   读取四通道的数据
 */
void ADS1256_UpdateDiffData(void)
{
    ADS1256_NSS(1);

    int8_t channel;
    int8_t nIChannel;
    int8_t pIChannel;

    for (int8_t i = 0; i < 4; i++) {
        pIChannel = 2 * i;
        nIChannel = 2 * i + 1;
        channel   = (pIChannel << 4) | nIChannel;

        while (!ADS1256_IsReady());

        ADS1256_WREG(ADS1256_MUX, channel); // 选择通道
        ADS1256_SPI_RW(ADS1256_CMD_SYNC);
        ADS1256_SPI_RW(ADS1256_CMD_WAKEUP);
        AD_diff_data[(i + 3) % 4] = ADS1256_RDATA(); // 此次读取的是上次的数据
    }

    ADS1256_NSS(0);
}