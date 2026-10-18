#ifndef __APP_DATA_H__
#define __APP_DATA_H__

#include "main.h"

typedef struct
{
    uint32_t adc_raw; //PA0 的 ADC 原始值
    float adc_voltage; //根据参考电压换算后的电压

    float temperature;//AHT20 温度
    float humidity;//AHT20 湿度

    uint32_t timestamp_ms;//采样时间标记
    uint8_t aht20_valid;//本次温湿度数据是否有效
} SensorData_t;


#endif // __APP_DATA_H__

