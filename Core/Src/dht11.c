/**
  ******************************************************************************
  * @file           : dht11.c
  * @brief          : DHT11 温湿度传感器驱动源文件
  * @author         : Antigravity AI
  * @version        : V1.0.0
  ******************************************************************************
  * @attention
  *
  * DHT11通信时序:
  * 1. 主机发送起始信号 (拉低18ms以上，然后释放)
  * 2. DHT11响应信号 (拉低80us，然后拉高80us)
  * 3. DHT11发送40位数据:
  *    - 每位数据以50us低电平开始
  *    - 高电平持续26-28us表示'0'
  *    - 高电平持续70us表示'1'
  * 4. 数据格式: 8bit湿度整数 + 8bit湿度小数 + 8bit温度整数 + 8bit温度小数 + 8bit校验和
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "dht11.h"
#include <stdio.h>
#include <stdarg.h>

/* Private defines -----------------------------------------------------------*/

/* DWT寄存器地址 (用于微秒延时) */
#define DWT_CONTROL     (*(volatile uint32_t*)0xE0001000)
#define DWT_CYCCNT      (*(volatile uint32_t*)0xE0001004)
#define DWT_LAR         (*(volatile uint32_t*)0xE0001FB0)
#define SCB_DEMCR       (*(volatile uint32_t*)0xE000EDFC)

/* Private variables ---------------------------------------------------------*/

/* DHT11句柄实例 */
DHT11_Handle_t dht11 = {0};

/* 系统时钟频率 (用于计算延时) */
static uint32_t dwt_us_tick = 0;

/* Private function prototypes -----------------------------------------------*/
static void DHT11_DelayInit(void);
static void DHT11_DelayUs(uint32_t us);
static void DHT11_SetPinOutput(void);
static void DHT11_SetPinInput(void);
static void DHT11_SetPinHigh(void);
static void DHT11_SetPinLow(void);
static uint8_t DHT11_ReadPin(void);
static DHT11_Status_t DHT11_WaitForLevel(uint8_t level, uint32_t timeout_us, uint32_t *duration);
static uint8_t DHT11_GetPinNumber(uint16_t pin);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  初始化DWT微秒延时
  */
static void DHT11_DelayInit(void)
{
    /* 使能DWT */
    SCB_DEMCR |= 0x01000000;
    
    /* 解锁DWT (Cortex-M4需要) */
    DWT_LAR = 0xC5ACCE55;
    
    /* 重置周期计数器 */
    DWT_CYCCNT = 0;
    
    /* 使能周期计数器 */
    DWT_CONTROL |= 1;
    
    /* 计算每微秒的时钟周期数 */
    dwt_us_tick = SystemCoreClock / 1000000;
}

/**
  * @brief  微秒延时函数 (使用DWT周期计数器)
  * @param  us: 延时微秒数
  */
static void DHT11_DelayUs(uint32_t us)
{
    uint32_t startTick = DWT_CYCCNT;
    uint32_t delayTicks = us * dwt_us_tick;
    
    while ((DWT_CYCCNT - startTick) < delayTicks);
}

/**
  * @brief  设置DHT11数据引脚为输出模式
  */
static void DHT11_SetPinOutput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    GPIO_InitStruct.Pin = dht11.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    
    HAL_GPIO_Init(dht11.port, &GPIO_InitStruct);
}

/**
  * @brief  设置DHT11数据引脚为输入模式
  */
static void DHT11_SetPinInput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    GPIO_InitStruct.Pin = dht11.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;     /* 使用内部上拉 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    
    HAL_GPIO_Init(dht11.port, &GPIO_InitStruct);
}

/**
  * @brief  设置DHT11数据引脚为高电平
  */
static void DHT11_SetPinHigh(void)
{
    HAL_GPIO_WritePin(dht11.port, dht11.pin, GPIO_PIN_SET);
}

/**
  * @brief  设置DHT11数据引脚为低电平
  */
static void DHT11_SetPinLow(void)
{
    HAL_GPIO_WritePin(dht11.port, dht11.pin, GPIO_PIN_RESET);
}

/**
  * @brief  读取DHT11数据引脚电平
  * @retval 引脚电平 (0或1)
  */
static uint8_t DHT11_ReadPin(void)
{
    return (HAL_GPIO_ReadPin(dht11.port, dht11.pin) == GPIO_PIN_SET) ? 1 : 0;
}

/**
  * @brief  等待引脚电平变化
  * @param  level: 等待的电平 (0或1)
  * @param  timeout_us: 超时时间 (微秒)
  * @param  duration: 输出持续时间 (微秒)，可为NULL
  * @retval DHT11_Status_t 操作状态
  */
static DHT11_Status_t DHT11_WaitForLevel(uint8_t level, uint32_t timeout_us, uint32_t *duration)
{
    uint32_t startTick = DWT_CYCCNT;
    uint32_t timeoutTicks = timeout_us * dwt_us_tick;
    
    /* 
     * 如果需要测量持续时间 (duration != NULL):
     * 测量的是当前电平(与目标相反)的持续时间
     * 例如: WaitForLevel(0, ..., &dur) 会测量高电平持续多久才变低
     */
    if (duration != NULL) {
        /* 等待电平变化到目标电平，同时测量当前电平持续时间 */
        while (DHT11_ReadPin() != level) {
            if ((DWT_CYCCNT - startTick) > timeoutTicks) {
                return DHT11_ERROR_TIMEOUT;
            }
        }
        /* 计算从开始到电平变化的时间 */
        *duration = (DWT_CYCCNT - startTick) / dwt_us_tick;
    } else {
        /* 只等待电平变化，不测量时间 */
        while (DHT11_ReadPin() != level) {
            if ((DWT_CYCCNT - startTick) > timeoutTicks) {
                return DHT11_ERROR_TIMEOUT;
            }
        }
    }
    
    return DHT11_OK;
}

/**
  * @brief  获取GPIO引脚编号 (替代__builtin_ctz，兼容所有编译器)
  * @param  pin: GPIO引脚定义 (如GPIO_PIN_9)
  * @retval 引脚编号 (0-15)
  */
static uint8_t DHT11_GetPinNumber(uint16_t pin)
{
    uint8_t n = 0;
    if (pin == 0) return 0;
    while ((pin & 1) == 0) {
        pin >>= 1;
        n++;
    }
    return n;
}

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  初始化DHT11传感器 (使用默认引脚)
  */
DHT11_Status_t DHT11_Init(void)
{
    return DHT11_InitEx(DHT11_GPIO_Port, DHT11_Pin);
}

/**
  * @brief  使用自定义引脚初始化DHT11传感器
  */
DHT11_Status_t DHT11_InitEx(GPIO_TypeDef *port, uint16_t pin)
{
    if (port == NULL) {
        return DHT11_ERROR;
    }
    
    /* 保存引脚配置 */
    dht11.port = port;
    dht11.pin = pin;
    
    /* 初始化微秒延时 */
    DHT11_DelayInit();
    
    /* 设置引脚为输出模式并拉高 */
    DHT11_SetPinOutput();
    DHT11_SetPinHigh();
    
    /* 初始化数据 */
    dht11.data.temperature = 0.0f;
    dht11.data.humidity = 0.0f;
    dht11.data.lastReadTime = 0;
    dht11.data.lastStatus = DHT11_OK;
    
    /* 标记已初始化 */
    dht11.initialized = 1;
    
    /* 等待DHT11上电稳定 (至少1秒) */
    HAL_Delay(1000);
    
#if DHT11_DEBUG_ENABLE
    DHT11_DebugPrint("DHT11 initialized on GPIO%c Pin%d\r\n", 
                     'A' + ((uint32_t)(port - GPIOA) / ((uint32_t)GPIOB - (uint32_t)GPIOA)), 
                     DHT11_GetPinNumber(pin));
#endif
    
    return DHT11_OK;
}

/**
  * @brief  读取DHT11传感器数据
  */
DHT11_Status_t DHT11_Read(float *temperature, float *humidity)
{
    DHT11_RawData_t rawData;
    DHT11_Status_t status;
    
    /* 读取原始数据 */
    status = DHT11_ReadRaw(&rawData);
    
    if (status == DHT11_OK) {
        /* 转换为浮点数 */
        dht11.data.temperature = (float)rawData.temperature_int + (float)rawData.temperature_dec * 0.1f;
        dht11.data.humidity = (float)rawData.humidity_int + (float)rawData.humidity_dec * 0.1f;
        dht11.data.lastReadTime = HAL_GetTick();
        
        /* 输出结果 */
        if (temperature != NULL) {
            *temperature = dht11.data.temperature;
        }
        if (humidity != NULL) {
            *humidity = dht11.data.humidity;
        }
    }
    
    dht11.data.lastStatus = status;
    return status;
}

/**
  * @brief  读取DHT11传感器原始数据
  */
DHT11_Status_t DHT11_ReadRaw(DHT11_RawData_t *rawData)
{
    uint8_t data[5] = {0};
    uint8_t i, j;
    uint32_t highDuration;
    DHT11_Status_t status;
    
    if (!dht11.initialized) {
        return DHT11_ERROR;
    }
    
    if (rawData == NULL) {
        return DHT11_ERROR;
    }
    
    /* 检查采样间隔 */
    uint32_t currentTick = HAL_GetTick();
    if ((currentTick - dht11.data.lastReadTime) < DHT11_MIN_SAMPLE_INTERVAL_MS && 
        dht11.data.lastReadTime != 0) {
#if DHT11_DEBUG_ENABLE
        DHT11_DebugPrint("DHT11: Sampling too fast, please wait\r\n");
#endif
        return DHT11_ERROR_NOT_READY;
    }
    
    /* ========== 第1步: 主机发送起始信号 ========== */
    
    /* 设置为输出模式 */
    DHT11_SetPinOutput();
    
    /* 拉低总线至少18ms */
    DHT11_SetPinLow();
    HAL_Delay(20);  /* 20ms确保足够长 */
    
    /* 释放总线 (拉高) */
    DHT11_SetPinHigh();
    DHT11_DelayUs(DHT11_START_SIGNAL_HIGH_US);
    
    /* 切换为输入模式 */
    DHT11_SetPinInput();
    
    /* ========== 第2步: 等待DHT11响应 ========== */
    
    /* 等待DHT11拉低 (响应信号开始) */
    status = DHT11_WaitForLevel(0, DHT11_RESPONSE_TIMEOUT_US, NULL);
    if (status != DHT11_OK) {
#if DHT11_DEBUG_ENABLE
        DHT11_DebugPrint("DHT11: No response (low)\r\n");
#endif
        DHT11_SetPinOutput();
        DHT11_SetPinHigh();
        return DHT11_ERROR_NO_RESPONSE;
    }
    
    /* 等待DHT11拉高 (响应信号结束) */
    status = DHT11_WaitForLevel(1, DHT11_RESPONSE_TIMEOUT_US, NULL);
    if (status != DHT11_OK) {
#if DHT11_DEBUG_ENABLE
        DHT11_DebugPrint("DHT11: No response (high)\r\n");
#endif
        DHT11_SetPinOutput();
        DHT11_SetPinHigh();
        return DHT11_ERROR_NO_RESPONSE;
    }
    
    /* 等待DHT11准备发送数据 (拉低表示第一个bit开始) */
    status = DHT11_WaitForLevel(0, DHT11_RESPONSE_TIMEOUT_US, NULL);
    if (status != DHT11_OK) {
#if DHT11_DEBUG_ENABLE
        DHT11_DebugPrint("DHT11: Data start failed\r\n");
#endif
        DHT11_SetPinOutput();
        DHT11_SetPinHigh();
        return DHT11_ERROR_NO_RESPONSE;
    }
    
    /* ========== 第3步: 读取40位数据 ========== */
    
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 8; j++) {
            /* 等待高电平开始 (每个bit以50us低电平开始) */
            status = DHT11_WaitForLevel(1, DHT11_BIT_TIMEOUT_US, NULL);
            if (status != DHT11_OK) {
#if DHT11_DEBUG_ENABLE
                DHT11_DebugPrint("DHT11: Bit timeout (waiting high) at byte %d bit %d\r\n", i, j);
#endif
                DHT11_SetPinOutput();
                DHT11_SetPinHigh();
                return DHT11_ERROR_TIMEOUT;
            }
            
            /* 测量高电平持续时间 */
            status = DHT11_WaitForLevel(0, DHT11_BIT_TIMEOUT_US, &highDuration);
            if (status != DHT11_OK) {
#if DHT11_DEBUG_ENABLE
                DHT11_DebugPrint("DHT11: Bit timeout (measuring) at byte %d bit %d\r\n", i, j);
#endif
                DHT11_SetPinOutput();
                DHT11_SetPinHigh();
                return DHT11_ERROR_TIMEOUT;
            }
            
            /* 根据高电平持续时间判断数据位 */
            data[i] <<= 1;
            if (highDuration > DHT11_BIT_THRESHOLD_US) {
                data[i] |= 1;   /* 高电平长于阈值，数据位为1 */
            }
        }
    }
    
    /* ========== 第4步: 恢复总线状态 ========== */
    DHT11_SetPinOutput();
    DHT11_SetPinHigh();
    
    /* ========== 第5步: 校验数据 ========== */
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    
    if (checksum != data[4]) {
#if DHT11_DEBUG_ENABLE
        DHT11_DebugPrint("DHT11: Checksum error (calc: 0x%02X, recv: 0x%02X)\r\n", checksum, data[4]);
        DHT11_DebugPrint("DHT11: Raw data: %02X %02X %02X %02X %02X\r\n", 
                         data[0], data[1], data[2], data[3], data[4]);
#endif
        return DHT11_ERROR_CHECKSUM;
    }
    
    /* ========== 第6步: 填充输出数据 ========== */
    rawData->humidity_int = data[0];
    rawData->humidity_dec = data[1];
    rawData->temperature_int = data[2];
    rawData->temperature_dec = data[3];
    rawData->checksum = data[4];
    
#if DHT11_DEBUG_ENABLE
    DHT11_DebugPrint("DHT11: Temp=%d.%d C, Humi=%d.%d %%\r\n", 
                     data[2], data[3], data[0], data[1]);
#endif
    
    return DHT11_OK;
}

/**
  * @brief  获取最后一次读取的温度
  */
float DHT11_GetTemperature(void)
{
    return dht11.data.temperature;
}

/**
  * @brief  获取最后一次读取的湿度
  */
float DHT11_GetHumidity(void)
{
    return dht11.data.humidity;
}

/**
  * @brief  获取最后一次读取的状态
  */
DHT11_Status_t DHT11_GetLastStatus(void)
{
    return dht11.data.lastStatus;
}

/**
  * @brief  检查传感器是否就绪
  */
uint8_t DHT11_IsReady(void)
{
    uint32_t currentTick = HAL_GetTick();
    return ((currentTick - dht11.data.lastReadTime) >= DHT11_MIN_SAMPLE_INTERVAL_MS) || 
           (dht11.data.lastReadTime == 0);
}

/**
  * @brief  检查传感器是否已初始化
  */
uint8_t DHT11_IsInitialized(void)
{
    return dht11.initialized;
}

/**
  * @brief  获取状态字符串
  */
const char* DHT11_GetStatusString(DHT11_Status_t status)
{
    switch (status) {
        case DHT11_OK:
            return "OK";
        case DHT11_ERROR:
            return "General Error";
        case DHT11_ERROR_TIMEOUT:
            return "Timeout Error";
        case DHT11_ERROR_CHECKSUM:
            return "Checksum Error";
        case DHT11_ERROR_NO_RESPONSE:
            return "No Response";
        case DHT11_ERROR_NOT_READY:
            return "Not Ready (sampling too fast)";
        case DHT11_ERROR_INVALID_DATA:
            return "Invalid Data";
        default:
            return "Unknown Error";
    }
}

/**
  * @brief  调试打印函数 - 使用统一日志库
  */
void DHT11_DebugPrint(const char *format, ...)
{
#if DHT11_DEBUG_ENABLE
    char buffer[128];
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    /* 使用LOG_Raw以兼容原有格式 */
    LOG_Raw("%s", buffer);
#else
    (void)format;   /* 避免未使用警告 */
#endif
}
