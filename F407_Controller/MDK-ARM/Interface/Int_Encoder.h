#ifndef __INT_ENCODER_H
#define __INT_ENCODER_H

#include "main.h"
#include <stdint.h>

/* 启动 TIM8 编码器接口 */
void Int_Encoder_Init(void);

/* 读取两次调用之间的计数变化，正负表示计数方向 */
int16_t Int_Encoder_GetDelta(void);

/* 读取当前 16 位定时器计数值 */
uint16_t Int_Encoder_GetCount(void);

/* 转速计算函数 */
float Int_Encoder_GetOutputRPM(float sample_period_s);
#endif
