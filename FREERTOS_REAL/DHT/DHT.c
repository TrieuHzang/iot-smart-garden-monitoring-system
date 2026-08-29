#include "DHT.h"
#include "delay_timer.h"

#define DHT_TIMEOUT_US 300

static void DHT_DelayInit(DHT_Name* DHT)
{
    if (DHT == NULL)
        return;

    if (DHT->Timer == NULL)
    {
        DHT->LastError = DHT_ERR_NULL_PTR;
        return;
    }

    if (HAL_TIM_Base_Start(DHT->Timer) != HAL_OK)
    {
        DHT->LastError = DHT_ERR_TIMER_START;
        return;
    }

    DHT->LastError = DHT_OK;
}

static void DHT_DelayUs(DHT_Name* DHT, uint16_t Time)
{
    DELAY_TIM_Us(DHT->Timer, Time);
}

static void DHT_SetPinOut(DHT_Name* DHT)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT->Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT->PORT, &GPIO_InitStruct);
}

static void DHT_SetPinIn(DHT_Name* DHT)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT->Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT->PORT, &GPIO_InitStruct);
}

static void DHT_WritePin(DHT_Name* DHT, uint8_t Value)
{
    HAL_GPIO_WritePin(DHT->PORT, DHT->Pin, Value);
}

static uint8_t DHT_ReadPin(DHT_Name* DHT)
{
    return (uint8_t)HAL_GPIO_ReadPin(DHT->PORT, DHT->Pin);
}

static uint8_t DHT_WaitForState(DHT_Name* DHT, GPIO_PinState state, uint16_t timeout_us)
{
    uint16_t start = __HAL_TIM_GET_COUNTER(DHT->Timer);

    while (HAL_GPIO_ReadPin(DHT->PORT, DHT->Pin) != state)
    {
        if ((uint16_t)(__HAL_TIM_GET_COUNTER(DHT->Timer) - start) >= timeout_us)
        {
            return 0;
        }
    }
    return 1;
}

static uint8_t DHT_WaitWhileState(DHT_Name* DHT, GPIO_PinState state, uint16_t timeout_us)
{
    uint16_t start = __HAL_TIM_GET_COUNTER(DHT->Timer);

    while (HAL_GPIO_ReadPin(DHT->PORT, DHT->Pin) == state)
    {
        if ((uint16_t)(__HAL_TIM_GET_COUNTER(DHT->Timer) - start) >= timeout_us)
        {
            return 0;
        }
    }
    return 1;
}

static uint8_t DHT_Start(DHT_Name* DHT)
{
    DHT_SetPinOut(DHT);
    DHT_WritePin(DHT, 0);
    DHT_DelayUs(DHT, DHT->Type);

    DHT_SetPinIn(DHT);
    DHT_DelayUs(DHT, 35);

    if (!DHT_WaitForState(DHT, GPIO_PIN_RESET, DHT_TIMEOUT_US))
    {
        DHT->LastError = DHT_ERR_START_LOW_ACK;
        return 0;
    }

    if (!DHT_WaitForState(DHT, GPIO_PIN_SET, DHT_TIMEOUT_US))
    {
        DHT->LastError = DHT_ERR_START_HIGH_ACK;
        return 0;
    }

    if (!DHT_WaitWhileState(DHT, GPIO_PIN_SET, DHT_TIMEOUT_US))
    {
        DHT->LastError = DHT_ERR_START_END_ACK;
        return 0;
    }

    return 1;
}

static uint8_t DHT_Read(DHT_Name* DHT, uint8_t *ok)
{
    uint8_t Value = 0;
    DHT_SetPinIn(DHT);

    for (int i = 0; i < 8; i++)
    {
        if (!DHT_WaitForState(DHT, GPIO_PIN_SET, DHT_TIMEOUT_US))
        {
            DHT->LastError = DHT_ERR_BIT_TIMEOUT_HIGH;
            *ok = 0;
            return 0;
        }

        /* doi 35us de lay mau giua 0 va 1 */
        DHT_DelayUs(DHT, 35);

        if (DHT_ReadPin(DHT))
        {
            Value |= (1 << (7 - i));
        }

        if (!DHT_WaitWhileState(DHT, GPIO_PIN_SET, DHT_TIMEOUT_US))
        {
            DHT->LastError = DHT_ERR_BIT_TIMEOUT_LOW;
            *ok = 0;
            return 0;
        }
    }

    *ok = 1;
    return Value;
}
void DHT_Init(DHT_Name* DHT, uint8_t DHT_Type, TIM_HandleTypeDef* Timer, GPIO_TypeDef* DH_PORT, uint16_t DH_Pin)
{
    if (DHT == NULL)
        return;

    DHT->Temp = 0.0f;
    DHT->Humi = 0.0f;
    DHT->LastError = DHT_OK;

    if (Timer == NULL || DH_PORT == NULL)
    {
        DHT->LastError = DHT_ERR_NULL_PTR;
        return;
    }

    if (DHT_Type == DHT11)
    {
        DHT->Type = DHT11_STARTTIME;
    }
    else if (DHT_Type == DHT22)
    {
        DHT->Type = DHT22_STARTTIME;
    }
    else
    {
        DHT->LastError = DHT_ERR_BAD_TYPE;
        return;
    }

    DHT->PORT = DH_PORT;
    DHT->Pin = DH_Pin;
    DHT->Timer = Timer;

    DHT_DelayInit(DHT);
}

uint8_t DHT_ReadTempHum(DHT_Name* DHT)
{
    uint8_t RH1 = 0, RH2 = 0, Temp1 = 0, Temp2 = 0, CheckSum = 0;
    uint16_t TempRaw, HumiRaw;
    uint8_t read_ok = 0;
    uint8_t status = 1;

    if (DHT == NULL)
    {
        return 0;
    }

    DHT->LastError = DHT_OK;

    if (!DHT_Start(DHT))
    {
        /* start fail: van giu gia tri cu trong DHT->Temp/Humi */
        return 0;
    }

    RH1 = DHT_Read(DHT, &read_ok);
    if (!read_ok) { status = 0; goto decode_anyway; }

    RH2 = DHT_Read(DHT, &read_ok);
    if (!read_ok) { status = 0; goto decode_anyway; }

    Temp1 = DHT_Read(DHT, &read_ok);
    if (!read_ok) { status = 0; goto decode_anyway; }

    Temp2 = DHT_Read(DHT, &read_ok);
    if (!read_ok) { status = 0; goto decode_anyway; }

    CheckSum = DHT_Read(DHT, &read_ok);
    if (!read_ok) { status = 0; goto decode_anyway; }

    if (((RH1 + RH2 + Temp1 + Temp2) & 0xFF) != CheckSum)
    {
        DHT->LastError = DHT_ERR_CHECKSUM;
        status = 0;   /* bao la frame loi, nhung van decode */
    }
    else
    {
        DHT->LastError = DHT_OK;
    }

decode_anyway:
    HumiRaw = ((uint16_t)RH1 << 8) | RH2;
    TempRaw = ((uint16_t)Temp1 << 8) | Temp2;

    if (DHT->Type == DHT22_STARTTIME)
    {
        DHT->Humi = (float)HumiRaw / 10.0f;

        if (TempRaw & 0x8000)
        {
            TempRaw &= 0x7FFF;
            DHT->Temp = -((float)TempRaw / 10.0f);
        }
        else
        {
            DHT->Temp = (float)TempRaw / 10.0f;
        }
    }
    else
    {
        DHT->Humi = (float)RH1;
        DHT->Temp = (float)Temp1;
    }

    /* return 1 neu doc du 5 byte va checksum dung
       return 0 neu checksum sai hoac timeout giua chung
       NHUNG Temp/Humi van da duoc cap nhat bang du lieu moi nhat doc duoc */
    return status;
}
