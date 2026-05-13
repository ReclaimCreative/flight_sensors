#include "platform.h"
#include "stm32l4xx_hal.h"
#include "main.h"
#include <stdlib.h>
#include <string.h>

uint8_t VL53L7CX_RdByte(VL53L7CX_Platform *p_platform,
                         uint16_t RegisterAdress, uint8_t *p_value)
{
    return VL53L7CX_RdMulti(p_platform, RegisterAdress, p_value, 1);
}

uint8_t VL53L7CX_WrByte(VL53L7CX_Platform *p_platform,
                         uint16_t RegisterAdress, uint8_t value)
{
    return VL53L7CX_WrMulti(p_platform, RegisterAdress, &value, 1);
}

uint8_t VL53L7CX_WrMulti(VL53L7CX_Platform *p_platform,
                          uint16_t RegisterAdress,
                          uint8_t *p_values, uint32_t size)
{
    uint32_t timeout_ms = 100 + size / 20;
    uint8_t *buf = malloc(size + 2);
    if (!buf) return 255;
    buf[0] = (RegisterAdress >> 8) & 0xFF;
    buf[1] = RegisterAdress & 0xFF;
    memcpy(&buf[2], p_values, size);
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(
        p_platform->hi2c,
        p_platform->address,
        buf,
        size + 2,
        timeout_ms);
    free(buf);
    return (ret == HAL_OK) ? 0 : 255;
}

uint8_t VL53L7CX_RdMulti(VL53L7CX_Platform *p_platform,
                          uint16_t RegisterAdress,
                          uint8_t *p_values, uint32_t size)
{
    uint32_t timeout_ms = 50 + size / 100;
    uint8_t reg[2] = { (RegisterAdress >> 8) & 0xFF, RegisterAdress & 0xFF };
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(
        p_platform->hi2c,
        p_platform->address,
        reg,
        2,
        timeout_ms);
    if (ret != HAL_OK) return 255;
    ret = HAL_I2C_Master_Receive(
        p_platform->hi2c,
        p_platform->address,
        p_values,
        size,
        timeout_ms);
    return (ret == HAL_OK) ? 0 : 255;
}

uint8_t VL53L7CX_Reset_Sensor(VL53L7CX_Platform *p_platform)
{
    HAL_GPIO_WritePin(p_platform->lpn_port, p_platform->lpn_pin, GPIO_PIN_RESET);
    VL53L7CX_WaitMs(p_platform, 10);
    HAL_GPIO_WritePin(p_platform->lpn_port, p_platform->lpn_pin, GPIO_PIN_SET);
    VL53L7CX_WaitMs(p_platform, 10);
    return 0;
}

void VL53L7CX_SwapBuffer(uint8_t *buffer, uint16_t size)
{
    uint32_t i, tmp;
    for (i = 0; i < size; i += 4)
    {
        tmp = (buffer[i]   << 24)
            | (buffer[i+1] << 16)
            | (buffer[i+2] <<  8)
            | (buffer[i+3]);
        memcpy(&buffer[i], &tmp, 4);
    }
}

uint8_t VL53L7CX_WaitMs(VL53L7CX_Platform *p_platform, uint32_t TimeMs)
{
    HAL_Delay(TimeMs);
    return 0;
}