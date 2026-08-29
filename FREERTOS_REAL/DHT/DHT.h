#ifndef __DHT_H
#define __DHT_H

#include "stm32f1xx_hal.h"

#define DHT11_STARTTIME 18000
#define DHT22_STARTTIME 12000
#define DHT11 0x01
#define DHT22 0x02

/* Ma trang thai / loi */
#define DHT_OK                     0
#define DHT_ERR_BAD_TYPE           1
#define DHT_ERR_NULL_PTR           2
#define DHT_ERR_TIMER_START        3
#define DHT_ERR_START_LOW_ACK      4
#define DHT_ERR_START_HIGH_ACK     5
#define DHT_ERR_START_END_ACK      6
#define DHT_ERR_BIT_TIMEOUT_HIGH   7
#define DHT_ERR_BIT_TIMEOUT_LOW    8
#define DHT_ERR_CHECKSUM           9

typedef struct
{
    uint16_t Type;
    TIM_HandleTypeDef* Timer;
    uint16_t Pin;
    GPIO_TypeDef* PORT;
    float Temp;
    float Humi;
    uint8_t LastError;
} DHT_Name;

void DHT_Init(DHT_Name* DHT, uint8_t DHT_Type, TIM_HandleTypeDef* Timer, GPIO_TypeDef* DH_PORT, uint16_t DH_Pin);
uint8_t DHT_ReadTempHum(DHT_Name* DHT);

#endif
