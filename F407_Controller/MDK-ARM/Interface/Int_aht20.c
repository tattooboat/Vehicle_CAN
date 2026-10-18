#include "Int_aht20.h"
#include "i2c.h"
/*
 * AHT20 使用 I2C1：
 * PB6 = I2C1_SCL
 * PB7 = I2C1_SDA
 *
 * 如果 CubeMX 生成的句柄不是 hi2c1，
 * 请将下面代码中的 hi2c1 替换为实际句柄。
 */

#define AHT20_I2C_ADDR          (0x38U << 1)

#define AHT20_CMD_INIT          0xBEU
#define AHT20_CMD_MEASURE       0xACU
#define AHT20_CMD_SOFT_RESET    0xBAU

#define AHT20_TIMEOUT_MS        100U
#define AHT20_MEASURE_TIMEOUT   150U

/* 检查传感器是否在线 */
AHT20_Status_t AHT20_Check(void)
{
    if (HAL_I2C_IsDeviceReady(&hi2c1,
                              AHT20_I2C_ADDR,
                              3,
                              AHT20_TIMEOUT_MS) == HAL_OK)
    {
        return AHT20_OK;
    }

    return AHT20_ERROR;
}

/* 发送命令 */
static AHT20_Status_t AHT20_WriteCommand(uint8_t *cmd,
                                         uint16_t length)
{
    HAL_StatusTypeDef result;

    result = HAL_I2C_Master_Transmit(&hi2c1,
                                     AHT20_I2C_ADDR,
                                     cmd,
                                     length,
                                     AHT20_TIMEOUT_MS);

    if (result == HAL_TIMEOUT)
    {
        return AHT20_TIMEOUT;
    }

    if (result != HAL_OK)
    {
        return AHT20_ERROR;
    }

    return AHT20_OK;
}

/* 读取传感器状态 */
static AHT20_Status_t AHT20_ReadStatus(uint8_t *status)
{
    if (status == NULL)
    {
        return AHT20_INVALID_PARAM;
    }

    if (HAL_I2C_Master_Receive(&hi2c1,
                               AHT20_I2C_ADDR,
                               status,
                               1,
                               AHT20_TIMEOUT_MS) != HAL_OK)
    {
        return AHT20_ERROR;
    }

    return AHT20_OK;
}

/* 初始化 AHT20 */
AHT20_Status_t AHT20_Init(void)
{
    uint8_t status = 0;
    uint8_t cmd[3];
    AHT20_Status_t result;

    /* 上电后等待传感器稳定 */
    HAL_Delay(40);

    /* 检查 I2C 通信 */
    result = AHT20_Check();

    if (result != AHT20_OK)
    {
        return result;
    }

    /* 读取状态字节 */
    result = AHT20_ReadStatus(&status);

    if (result != AHT20_OK)
    {
        return result;
    }

    /*
     * 状态字节 bit3 为校准使能位。
     * bit3 = 0 时，发送初始化命令。
     */
    if ((status & 0x08U) == 0U)
    {
        cmd[0] = AHT20_CMD_INIT;
        cmd[1] = 0x08U;
        cmd[2] = 0x00U;

        result = AHT20_WriteCommand(cmd, 3);

        if (result != AHT20_OK)
        {
            return result;
        }

        HAL_Delay(10);

        /* 再次读取状态，确认初始化结果 */
        result = AHT20_ReadStatus(&status);

        if (result != AHT20_OK)
        {
            return result;
        }

        if ((status & 0x08U) == 0U)
        {
            return AHT20_ERROR;
        }
    }

    return AHT20_OK;
}

/* 读取温湿度 */
AHT20_Status_t AHT20_ReadData(AHT20_Data_t *data)
{
    uint8_t cmd[3];
    uint8_t buffer[7];
    uint8_t status = 0;
    uint32_t elapsed = 0;
    uint32_t raw_humidity;
    uint32_t raw_temperature;
    AHT20_Status_t result;

    if (data == NULL)
    {
        return AHT20_INVALID_PARAM;
    }

    /* 触发一次测量 */
    cmd[0] = AHT20_CMD_MEASURE;
    cmd[1] = 0x33U;
    cmd[2] = 0x00U;

    result = AHT20_WriteCommand(cmd, 3);

    if (result != AHT20_OK)
    {
        return result;
    }

    /*
     * 等待测量完成。
     * 状态 bit7 = 1 表示忙。
     */
    do
    {
        HAL_Delay(5);
        elapsed += 5;

        result = AHT20_ReadStatus(&status);

        if (result != AHT20_OK)
        {
            return result;
        }

        if ((status & 0x80U) == 0U)
        {
            break;
        }

    } while (elapsed < AHT20_MEASURE_TIMEOUT);

    if ((status & 0x80U) != 0U)
    {
        return AHT20_TIMEOUT;
    }

    /* 读取状态字节和六字节测量数据 */
    if (HAL_I2C_Master_Receive(&hi2c1,
                               AHT20_I2C_ADDR,
                               buffer,
                               7,
                               AHT20_TIMEOUT_MS) != HAL_OK)
    {
        return AHT20_ERROR;
    }

    /* 传感器状态 bit7 为忙时，不应解析数据 */
    if ((buffer[0] & 0x80U) != 0U)
    {
        return AHT20_ERROR;
    }

    /* 湿度原始值：20 bit */
    raw_humidity =
        ((uint32_t)buffer[1] << 12) |
        ((uint32_t)buffer[2] << 4)  |
        ((uint32_t)buffer[3] >> 4);

    /* 温度原始值：20 bit */
    raw_temperature =
        ((uint32_t)(buffer[3] & 0x0FU) << 16) |
        ((uint32_t)buffer[4] << 8)           |
        ((uint32_t)buffer[5]);

    /* 换算成相对湿度 %RH */
    data->humidity =
        ((float)raw_humidity * 100.0f) / 1048576.0f;

    /* 换算成摄氏温度 °C */
    data->temperature =
        ((float)raw_temperature * 200.0f) / 1048576.0f
        - 50.0f;

    return AHT20_OK;
}
