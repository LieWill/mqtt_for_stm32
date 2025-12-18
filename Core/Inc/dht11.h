/**
  ******************************************************************************
  * @file           : dht11.h
  * @brief          : DHT11 温湿度传感器驱动头文件
  * @author         : Antigravity AI
  * @version        : V1.0.0
  ******************************************************************************
  * @attention
  *
  * DHT11 温湿度传感器 STM32 HAL库驱动
  * 使用DWT周期计数器实现精确微秒延时
  *
  * 支持功能:
  *   - 读取温度 (精度: 1°C, 范围: 0-50°C)
  *   - 读取湿度 (精度: 1%RH, 范围: 20-90%RH)
  *   - 数据校验 (8位校验和)
  *   - 错误检测和处理
  *
  * 硬件连接:
  *   - DATA (PG9) -> STM32 GPIO (需配置为输出模式)
  *   - VCC -> 3.3V 或 5V
  *   - GND -> GND
  *
  ******************************************************************************
  */

#ifndef __DHT11_H
#define __DHT11_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>

/* Exported defines ----------------------------------------------------------*/

/* DHT11 配置 - 使用CubeMX生成的引脚定义 */
#ifndef DHT11_Pin
#define DHT11_Pin           GPIO_PIN_9
#endif

#ifndef DHT11_GPIO_Port
#define DHT11_GPIO_Port     GPIOG
#endif

/* 时序参数 (微秒) */
#define DHT11_START_SIGNAL_LOW_US       18000   /* 主机起始信号低电平时间 (至少18ms) */
#define DHT11_START_SIGNAL_HIGH_US      40      /* 主机释放总线等待时间 (20-40us) */
#define DHT11_RESPONSE_TIMEOUT_US       200     /* 响应超时时间 */
#define DHT11_BIT_TIMEOUT_US            200     /* 位超时时间 */

/* 数据位判断阈值 */
#define DHT11_BIT_THRESHOLD_US          40      /* 高电平持续时间阈值 (26-28us=0, 70us=1) */

/* 采样间隔 (DHT11最小采样间隔为1秒) */
#define DHT11_MIN_SAMPLE_INTERVAL_MS    1000

/* 调试开关 */
#define DHT11_DEBUG_ENABLE              0       /* 1:开启调试输出 0:关闭 */

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  DHT11 状态枚举
  */
typedef enum {
    DHT11_OK = 0,               /* 操作成功 */
    DHT11_ERROR,                /* 一般错误 */
    DHT11_ERROR_TIMEOUT,        /* 超时错误 */
    DHT11_ERROR_CHECKSUM,       /* 校验和错误 */
    DHT11_ERROR_NO_RESPONSE,    /* 传感器无响应 */
    DHT11_ERROR_NOT_READY,      /* 采样间隔不足 */
    DHT11_ERROR_INVALID_DATA    /* 数据无效 */
} DHT11_Status_t;

/**
  * @brief  DHT11 数据结构
  */
typedef struct {
    uint8_t humidity_int;       /* 湿度整数部分 */
    uint8_t humidity_dec;       /* 湿度小数部分 (DHT11始终为0) */
    uint8_t temperature_int;    /* 温度整数部分 */
    uint8_t temperature_dec;    /* 温度小数部分 (DHT11始终为0) */
    uint8_t checksum;           /* 校验和 */
} DHT11_RawData_t;

/**
  * @brief  DHT11 处理后的数据结构
  */
typedef struct {
    float temperature;          /* 温度 (°C) */
    float humidity;             /* 湿度 (%RH) */
    uint32_t lastReadTime;      /* 最后读取时间 (ms) */
    DHT11_Status_t lastStatus;  /* 最后一次读取状态 */
} DHT11_Data_t;

/**
  * @brief  DHT11 句柄结构
  */
typedef struct {
    GPIO_TypeDef *port;         /* GPIO端口 */
    uint16_t pin;               /* GPIO引脚 */
    DHT11_Data_t data;          /* 传感器数据 */
    uint8_t initialized;        /* 初始化标志 */
} DHT11_Handle_t;

/* Exported variables --------------------------------------------------------*/
extern DHT11_Handle_t dht11;

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  初始化DHT11传感器
  * @note   使用CubeMX配置的默认引脚 (PG9)
  * @retval DHT11_Status_t 操作状态
  */
DHT11_Status_t DHT11_Init(void);

/**
  * @brief  使用自定义引脚初始化DHT11传感器
  * @param  port: GPIO端口指针
  * @param  pin: GPIO引脚号
  * @retval DHT11_Status_t 操作状态
  */
DHT11_Status_t DHT11_InitEx(GPIO_TypeDef *port, uint16_t pin);

/**
  * @brief  读取DHT11传感器数据
  * @param  temperature: 温度输出指针 (可为NULL)
  * @param  humidity: 湿度输出指针 (可为NULL)
  * @retval DHT11_Status_t 操作状态
  */
DHT11_Status_t DHT11_Read(float *temperature, float *humidity);

/**
  * @brief  读取DHT11传感器原始数据
  * @param  rawData: 原始数据输出指针
  * @retval DHT11_Status_t 操作状态
  */
DHT11_Status_t DHT11_ReadRaw(DHT11_RawData_t *rawData);

/**
  * @brief  获取最后一次读取的温度
  * @retval 温度值 (°C)
  */
float DHT11_GetTemperature(void);

/**
  * @brief  获取最后一次读取的湿度
  * @retval 湿度值 (%RH)
  */
float DHT11_GetHumidity(void);

/**
  * @brief  获取最后一次读取的状态
  * @retval DHT11_Status_t 状态
  */
DHT11_Status_t DHT11_GetLastStatus(void);

/**
  * @brief  检查传感器是否就绪 (采样间隔已满足)
  * @retval 1:就绪 0:未就绪
  */
uint8_t DHT11_IsReady(void);

/**
  * @brief  检查传感器是否已初始化
  * @retval 1:已初始化 0:未初始化
  */
uint8_t DHT11_IsInitialized(void);

/**
  * @brief  获取状态字符串
  * @param  status: 状态值
  * @retval 状态描述字符串
  */
const char* DHT11_GetStatusString(DHT11_Status_t status);

/**
  * @brief  调试打印函数
  * @param  format: 格式化字符串
  * @param  ...: 可变参数
  */
void DHT11_DebugPrint(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* __DHT11_H */
