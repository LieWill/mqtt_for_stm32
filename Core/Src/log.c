/**
  ******************************************************************************
  * @file           : log.c
  * @brief          : 统一日志库源文件
  * @author         : Antigravity AI
  * @version        : V1.0.0
  ******************************************************************************
  * @attention
  *
  * 统一日志库实现
  * 为所有驱动提供标准化的调试输出接口
  *
  ******************************************************************************
  */

#include "log.h"

/* Private variables ---------------------------------------------------------*/
LOG_Handle_t logHandle = {0};

/* Private function prototypes -----------------------------------------------*/
static uint32_t LOG_GetTimestamp(void);

/**
  * @brief  初始化日志模块
  * @param  huart: 用于日志输出的UART句柄
  * @retval None
  */
void LOG_Init(UART_HandleTypeDef *huart)
{
    if (!huart) return;
    
    logHandle.huart = huart;
    logHandle.enabled = 1;
    logHandle.level = LOG_LEVEL;
    logHandle.initialized = 1;
    
    /* 打印初始化信息 */
    LOG_I("LOG", "Log system initialized (Level: %d)", logHandle.level);
}

/**
  * @brief  反初始化日志模块
  * @retval None
  */
void LOG_DeInit(void)
{
    logHandle.initialized = 0;
    logHandle.enabled = 0;
    logHandle.huart = NULL;
}

/**
  * @brief  设置日志级别
  * @param  level: 日志级别 (LOG_LEVEL_xxx)
  * @retval None
  */
void LOG_SetLevel(uint8_t level)
{
    if (level <= LOG_LEVEL_VERBOSE) {
        logHandle.level = level;
    }
}

/**
  * @brief  获取当前日志级别
  * @retval 当前日志级别
  */
uint8_t LOG_GetLevel(void)
{
    return logHandle.level;
}

/**
  * @brief  启用/禁用日志输出
  * @param  enable: 1=启用 0=禁用
  * @retval None
  */
void LOG_SetEnable(uint8_t enable)
{
    logHandle.enabled = enable ? 1 : 0;
}

/**
  * @brief  检查日志是否启用
  * @retval 1=启用 0=禁用
  */
uint8_t LOG_IsEnabled(void)
{
    return logHandle.enabled;
}

/**
  * @brief  获取时间戳 (毫秒)
  * @retval 当前时间戳
  */
static uint32_t LOG_GetTimestamp(void)
{
    return HAL_GetTick();
}

/**
  * @brief  格式化日志输出 (底层函数)
  * @param  level: 日志级别
  * @param  color: 颜色代码
  * @param  prefix: 级别前缀
  * @param  tag: 模块标签
  * @param  format: 格式化字符串
  * @param  ...: 可变参数
  * @retval None
  */
void LOG_Print(uint8_t level, const char *color, const char *prefix, 
               const char *tag, const char *format, ...)
{
    /* 检查初始化和使能状态 */
    if (!logHandle.initialized || !logHandle.enabled) return;
    if (!logHandle.huart) return;
    
    /* 检查日志级别 */
    if (level > logHandle.level) return;
    
    char buffer[LOG_BUFFER_SIZE];
    int offset = 0;
    
#if LOG_COLOR_ENABLE
    /* 添加颜色代码 */
    offset += snprintf(buffer + offset, LOG_BUFFER_SIZE - offset, "%s", color);
#endif
    
#if LOG_TIMESTAMP_ENABLE
    /* 添加时间戳 */
    uint32_t timestamp = LOG_GetTimestamp();
    offset += snprintf(buffer + offset, LOG_BUFFER_SIZE - offset, 
                       "[%lu] ", timestamp);
#endif
    
    /* 添加级别前缀和标签 */
    offset += snprintf(buffer + offset, LOG_BUFFER_SIZE - offset, 
                       "%s[%s] ", prefix, tag);
    
    /* 添加用户消息 */
    va_list args;
    va_start(args, format);
    offset += vsnprintf(buffer + offset, LOG_BUFFER_SIZE - offset, format, args);
    va_end(args);
    
#if LOG_COLOR_ENABLE
    /* 重置颜色 */
    offset += snprintf(buffer + offset, LOG_BUFFER_SIZE - offset, "%s", LOG_COLOR_RESET);
#endif
    
#if LOG_NEWLINE_AUTO
    /* 添加换行符 */
    offset += snprintf(buffer + offset, LOG_BUFFER_SIZE - offset, "\r\n");
#endif
    
    /* 输出到UART */
    HAL_UART_Transmit(logHandle.huart, (uint8_t *)buffer, offset, HAL_MAX_DELAY);
}

/**
  * @brief  原始输出 (无格式)
  * @param  format: 格式化字符串
  * @param  ...: 可变参数
  * @retval None
  */
void LOG_Raw(const char *format, ...)
{
    if (!logHandle.initialized || !logHandle.enabled) return;
    if (!logHandle.huart) return;
    
    char buffer[LOG_BUFFER_SIZE];
    
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, LOG_BUFFER_SIZE, format, args);
    va_end(args);
    
    HAL_UART_Transmit(logHandle.huart, (uint8_t *)buffer, len, HAL_MAX_DELAY);
}

/**
  * @brief  输出十六进制数据
  * @param  tag: 模块标签
  * @param  data: 数据指针
  * @param  len: 数据长度
  * @retval None
  */
void LOG_HexDump(const char *tag, const uint8_t *data, uint16_t len)
{
    if (!logHandle.initialized || !logHandle.enabled) return;
    if (!logHandle.huart || !data || len == 0) return;
    
    LOG_D(tag, "HexDump (%d bytes):", len);
    
    char line[80];
    int offset = 0;
    
    for (uint16_t i = 0; i < len; i++) {
        if (i % 16 == 0) {
            if (i > 0) {
                /* 输出上一行 */
                LOG_Raw("%s\r\n", line);
            }
            offset = snprintf(line, sizeof(line), "  %04X: ", i);
        }
        
        offset += snprintf(line + offset, sizeof(line) - offset, "%02X ", data[i]);
    }
    
    /* 输出最后一行 */
    if (offset > 0) {
        LOG_Raw("%s\r\n", line);
    }
}
