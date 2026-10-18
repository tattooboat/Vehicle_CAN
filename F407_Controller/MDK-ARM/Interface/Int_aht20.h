
#ifndef __AHT20_H
#define __AHT20_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* AHT20 传感器数据 */
typedef struct
{
    float temperature;  /* 摄氏温度，单位 °C */
    float humidity;     /* 相对湿度，单位 %RH */
} AHT20_Data_t;

/* 函数返回状态 */
typedef enum
{
    AHT20_OK = 0,
    AHT20_ERROR = 1,
    AHT20_TIMEOUT = 2,
    AHT20_INVALID_PARAM = 3
} AHT20_Status_t;

/* 初始化 AHT20 */
AHT20_Status_t AHT20_Init(void);

/* 读取温度和湿度 */
AHT20_Status_t AHT20_ReadData(AHT20_Data_t *data);

/* 检查传感器是否应答 */
AHT20_Status_t AHT20_Check(void);

#ifdef __cplusplus
}
#endif

#endif /* __AHT20_H */
