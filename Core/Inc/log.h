/**
  ******************************************************************************
  * @file           : log.h
  * @brief          : 统一日志库头文件
  * @author         : Antigravity AI
  * @version        : V1.0.0
  ******************************************************************************
  * @attention
  *
  * 统一日志库 - 为所有驱动提供标准化的调试输出接口
  *
  * 支持功能:
  *   - 多日志级别 (ERROR, WARN, INFO, DEBUG, VERBOSE)
  *   - 模块标签输出
  *   - 时间戳支持
  *   - 全局开关控制
  *   - 单独模块调试开关
  *
  * 使用方法:
  *   1. 包含此头文件: #include "log.h"
  *   2. 初始化日志: LOG_Init(&huart1);
  *   3. 使用日志宏:
  *      LOG_E("TAG", "Error message: %d", errorCode);
  *      LOG_W("TAG", "Warning message");
  *      LOG_I("TAG", "Info message");
  *      LOG_D("TAG", "Debug message");
  *      LOG_V("TAG", "Verbose message");
  *
  ******************************************************************************
  */

#ifndef __LOG_H
#define __LOG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* Exported defines ----------------------------------------------------------*/

/* 日志全局开关 (1:开启 0:关闭) */
#define LOG_ENABLE                      1

/* 日志级别定义 */
#define LOG_LEVEL_NONE                  0   /* 关闭所有日志 */
#define LOG_LEVEL_ERROR                 1   /* 仅错误 */
#define LOG_LEVEL_WARN                  2   /* 错误 + 警告 */
#define LOG_LEVEL_INFO                  3   /* 错误 + 警告 + 信息 */
#define LOG_LEVEL_DEBUG                 4   /* 错误 + 警告 + 信息 + 调试 */
#define LOG_LEVEL_VERBOSE               5   /* 所有日志 */

/* 当前日志级别 (可在此处修改默认级别) */
#define LOG_LEVEL                       LOG_LEVEL_DEBUG

/* 日志选项 */
#define LOG_COLOR_ENABLE                1   /* 启用颜色输出 (仅支持ANSI终端) */
#define LOG_TIMESTAMP_ENABLE            1   /* 启用时间戳 */
#define LOG_NEWLINE_AUTO                1   /* 自动添加换行符 */

/* 日志缓冲区大小 */
#define LOG_BUFFER_SIZE                 256

/* ANSI颜色码 */
#if LOG_COLOR_ENABLE
    #define LOG_COLOR_RESET             "\033[0m"
    #define LOG_COLOR_RED               "\033[31m"      /* 错误 - 红色 */
    #define LOG_COLOR_YELLOW            "\033[33m"      /* 警告 - 黄色 */
    #define LOG_COLOR_GREEN             "\033[32m"      /* 信息 - 绿色 */
    #define LOG_COLOR_CYAN              "\033[36m"      /* 调试 - 青色 */
    #define LOG_COLOR_WHITE             "\033[37m"      /* 详细 - 白色 */
#else
    #define LOG_COLOR_RESET             ""
    #define LOG_COLOR_RED               ""
    #define LOG_COLOR_YELLOW            ""
    #define LOG_COLOR_GREEN             ""
    #define LOG_COLOR_CYAN              ""
    #define LOG_COLOR_WHITE             ""
#endif

/* 日志级别前缀 */
#define LOG_PREFIX_ERROR                "[E]"
#define LOG_PREFIX_WARN                 "[W]"
#define LOG_PREFIX_INFO                 "[I]"
#define LOG_PREFIX_DEBUG                "[D]"
#define LOG_PREFIX_VERBOSE              "[V]"

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  日志句柄结构
  */
typedef struct {
    UART_HandleTypeDef *huart;          /* 日志输出UART */
    uint8_t initialized;                /* 初始化标志 */
    uint8_t enabled;                    /* 全局使能标志 */
    uint8_t level;                      /* 当前日志级别 */
} LOG_Handle_t;

/* Exported variables --------------------------------------------------------*/
extern LOG_Handle_t logHandle;

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  初始化日志模块
  * @param  huart: 用于日志输出的UART句柄
  * @retval None
  */
void LOG_Init(UART_HandleTypeDef *huart);

/**
  * @brief  反初始化日志模块
  * @retval None
  */
void LOG_DeInit(void);

/**
  * @brief  设置日志级别
  * @param  level: 日志级别 (LOG_LEVEL_xxx)
  * @retval None
  */
void LOG_SetLevel(uint8_t level);

/**
  * @brief  获取当前日志级别
  * @retval 当前日志级别
  */
uint8_t LOG_GetLevel(void);

/**
  * @brief  启用/禁用日志输出
  * @param  enable: 1=启用 0=禁用
  * @retval None
  */
void LOG_SetEnable(uint8_t enable);

/**
  * @brief  检查日志是否启用
  * @retval 1=启用 0=禁用
  */
uint8_t LOG_IsEnabled(void);

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
               const char *tag, const char *format, ...);

/**
  * @brief  原始输出 (无格式)
  * @param  format: 格式化字符串
  * @param  ...: 可变参数
  * @retval None
  */
void LOG_Raw(const char *format, ...);

/**
  * @brief  输出十六进制数据
  * @param  tag: 模块标签
  * @param  data: 数据指针
  * @param  len: 数据长度
  * @retval None
  */
void LOG_HexDump(const char *tag, const uint8_t *data, uint16_t len);

/* ========================= 日志宏定义 ========================= */

#if LOG_ENABLE

/* 错误日志 */
#if (LOG_LEVEL >= LOG_LEVEL_ERROR)
    #define LOG_E(tag, fmt, ...) \
        LOG_Print(LOG_LEVEL_ERROR, LOG_COLOR_RED, LOG_PREFIX_ERROR, tag, fmt, ##__VA_ARGS__)
#else
    #define LOG_E(tag, fmt, ...) ((void)0)
#endif

/* 警告日志 */
#if (LOG_LEVEL >= LOG_LEVEL_WARN)
    #define LOG_W(tag, fmt, ...) \
        LOG_Print(LOG_LEVEL_WARN, LOG_COLOR_YELLOW, LOG_PREFIX_WARN, tag, fmt, ##__VA_ARGS__)
#else
    #define LOG_W(tag, fmt, ...) ((void)0)
#endif

/* 信息日志 */
#if (LOG_LEVEL >= LOG_LEVEL_INFO)
    #define LOG_I(tag, fmt, ...) \
        LOG_Print(LOG_LEVEL_INFO, LOG_COLOR_GREEN, LOG_PREFIX_INFO, tag, fmt, ##__VA_ARGS__)
#else
    #define LOG_I(tag, fmt, ...) ((void)0)
#endif

/* 调试日志 */
#if (LOG_LEVEL >= LOG_LEVEL_DEBUG)
    #define LOG_D(tag, fmt, ...) \
        LOG_Print(LOG_LEVEL_DEBUG, LOG_COLOR_CYAN, LOG_PREFIX_DEBUG, tag, fmt, ##__VA_ARGS__)
#else
    #define LOG_D(tag, fmt, ...) ((void)0)
#endif

/* 详细日志 */
#if (LOG_LEVEL >= LOG_LEVEL_VERBOSE)
    #define LOG_V(tag, fmt, ...) \
        LOG_Print(LOG_LEVEL_VERBOSE, LOG_COLOR_WHITE, LOG_PREFIX_VERBOSE, tag, fmt, ##__VA_ARGS__)
#else
    #define LOG_V(tag, fmt, ...) ((void)0)
#endif

#else /* LOG_ENABLE == 0 */

#define LOG_E(tag, fmt, ...) ((void)0)
#define LOG_W(tag, fmt, ...) ((void)0)
#define LOG_I(tag, fmt, ...) ((void)0)
#define LOG_D(tag, fmt, ...) ((void)0)
#define LOG_V(tag, fmt, ...) ((void)0)

#endif /* LOG_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* __LOG_H */
