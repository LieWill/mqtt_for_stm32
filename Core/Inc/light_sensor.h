/**
  ******************************************************************************
  * @file           : light_sensor.h
  * @brief          : 光敏传感器驱动头文件 (轮询模式)
  * @author         : Antigravity AI
  * @version        : V1.1.0
  * @date           : 2025-12-17
  ******************************************************************************
  * @attention
  *
  * 本驱动使用ADC3通道5 (PF7引脚) 采集光敏传感器模拟信号
  * 使用轮询模式读取，简单可靠
  *
  * 硬件连接:
  *   光敏传感器 VCC -> 3.3V/5V
  *   光敏传感器 GND -> GND
  *   光敏传感器 AO  -> PF7 (ADC3_IN5)
  *
  * 使用方法:
  *   1. 调用 LightSensor_Init() 初始化
  *   2. 调用 LightSensor_GetValue() 获取ADC原始值 (会自动读取)
  *   3. 使用 LightSensor_GetPercent() 获取光照百分比
  *
  ******************************************************************************
  */

#ifndef __LIGHT_SENSOR_H__
#define __LIGHT_SENSOR_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"

/* Exported defines ----------------------------------------------------------*/

/* 调试开关 */
#define LIGHT_SENSOR_DEBUG_ENABLE   0

/* ADC参考电压 (mV) */
#define LIGHT_SENSOR_VREF_MV        3300

/* ADC分辨率 */
#define LIGHT_SENSOR_ADC_MAX        4095    /* 12位ADC */

/* 光照等级阈值 (ADC值，可根据实际传感器调整) */
#define LIGHT_LEVEL_DARK_THRESHOLD      500     /* 暗环境阈值 */
#define LIGHT_LEVEL_DIM_THRESHOLD       1500    /* 昏暗环境阈值 */
#define LIGHT_LEVEL_NORMAL_THRESHOLD    2500    /* 正常光线阈值 */
#define LIGHT_LEVEL_BRIGHT_THRESHOLD    3500    /* 明亮环境阈值 */

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  光敏传感器状态枚举
  */
typedef enum {
    LIGHT_SENSOR_OK = 0,            /**< 操作成功 */
    LIGHT_SENSOR_ERROR,             /**< 通用错误 */
    LIGHT_SENSOR_NOT_INITIALIZED,   /**< 未初始化 */
    LIGHT_SENSOR_TIMEOUT,           /**< 操作超时 */
    LIGHT_SENSOR_DMA_ERROR          /**< DMA错误 */
} LightSensor_Status_t;

/**
  * @brief  光照等级枚举
  */
typedef enum {
    LIGHT_LEVEL_DARK = 0,           /**< 非常暗 (夜间/封闭空间) */
    LIGHT_LEVEL_DIM,                /**< 昏暗 (室内弱光) */
    LIGHT_LEVEL_NORMAL,             /**< 正常 (室内日光) */
    LIGHT_LEVEL_BRIGHT,             /**< 明亮 (阳光直射) */
    LIGHT_LEVEL_VERY_BRIGHT         /**< 非常亮 (强光) */
} LightSensor_LightLevel_t;

/**
  * @brief  光敏传感器句柄结构体
  */
typedef struct {
    ADC_HandleTypeDef *hadc;        /**< ADC句柄指针 */
    uint16_t current_value;         /**< 当前ADC值 */
    uint16_t filtered_value;        /**< 滤波后的ADC值 */
    uint32_t last_update_tick;      /**< 最后更新时间 */
    uint8_t is_initialized;         /**< 初始化标志 */
    uint8_t dma_running;            /**< DMA运行标志 (保留) */
    uint8_t conversion_complete;    /**< 转换完成标志 */
} LightSensor_Handle_t;

/* Exported variables --------------------------------------------------------*/
extern LightSensor_Handle_t lightSensor;

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  初始化光敏传感器驱动
  * @retval LightSensor_Status_t 操作状态
  */
LightSensor_Status_t LightSensor_Init(void);

/**
  * @brief  读取一次ADC值
  * @retval LightSensor_Status_t 操作状态
  */
LightSensor_Status_t LightSensor_Read(void);

/**
  * @brief  获取当前ADC原始值 (0-4095，会自动读取一次)
  * @retval uint16_t ADC原始值
  */
uint16_t LightSensor_GetValue(void);

/**
  * @brief  获取光照百分比 (0-100%)
  * @retval uint8_t 光照百分比
  */
uint8_t LightSensor_GetPercent(void);

/**
  * @brief  获取电压值 (毫伏)
  * @retval uint32_t 电压值 (mV)
  */
uint32_t LightSensor_GetVoltage_mV(void);

/**
  * @brief  获取光照等级
  * @retval LightSensor_LightLevel_t 光照等级
  */
LightSensor_LightLevel_t LightSensor_GetLightLevel(void);

/**
  * @brief  获取光照等级字符串描述
  * @param  level 光照等级
  * @retval const char* 等级描述字符串
  */
const char* LightSensor_GetLevelString(LightSensor_LightLevel_t level);

/**
  * @brief  检查是否已初始化
  * @retval uint8_t 1=已初始化, 0=未初始化
  */
uint8_t LightSensor_IsInitialized(void);

/**
  * @brief  调试打印函数
  */
void LightSensor_DebugPrint(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* __LIGHT_SENSOR_H__ */

