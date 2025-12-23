/**
  ******************************************************************************
  * @file           : esp8266.h
  * @brief          : ESP8266 WiFi模块驱动头文件 (DMA版本)
  * @author         : Antigravity AI
  * @version        : V2.0.0
  ******************************************************************************
  * @attention
  *
  * ESP8266 WiFi模块 STM32 HAL库驱动
  * 使用DMA+IDLE中断实现高效数据收发
  *
  * 支持功能:
  *   - DMA发送和接收
  *   - 空闲中断接收不定长数据
  *   - AT指令通信
  *   - STA/AP/STA+AP 模式
  *   - TCP/UDP 客户端和服务器
  *   - HTTP GET/POST 请求
  *   - 透传模式
  *
  ******************************************************************************
  */

#ifndef __ESP8266_H
#define __ESP8266_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usart.h"
#include "log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* 日志TAG */
#define TAG_ESP8266                     "ESP8266"

/* Exported defines ----------------------------------------------------------*/

/* ESP8266 配置 */
#define ESP8266_RX_BUF_SIZE             2048            /* DMA接收缓冲区大小 */
#define ESP8266_TX_BUF_SIZE             1024            /* 发送缓冲区大小 */
#define ESP8266_DEFAULT_TIMEOUT         3000            /* 默认超时时间(ms) */
#define ESP8266_LONG_TIMEOUT            10000           /* 长超时时间(ms) */
#define ESP8266_CONNECT_TIMEOUT         15000           /* 连接超时时间(ms) */

/* 最大连接数 */
#define ESP8266_MAX_CONNECTIONS         5

/* 调试开关 */
#define ESP8266_DEBUG_ENABLE            1               /* 1:开启调试输出 0:关闭 */

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  ESP8266 状态枚举
  */
typedef enum {
    ESP8266_OK = 0,                     /* 操作成功 */
    ESP8266_ERROR,                      /* 一般错误 */
    ESP8266_TIMEOUT,                    /* 超时 */
    ESP8266_BUSY,                       /* 模块忙 */
    ESP8266_NOT_FOUND,                  /* 未找到 */
    ESP8266_CONNECT_FAIL,               /* 连接失败 */
    ESP8266_SEND_FAIL,                  /* 发送失败 */
    ESP8266_ALREADY_CONNECTED,          /* 已连接 */
    ESP8266_NOT_CONNECTED,              /* 未连接 */
    ESP8266_WIFI_DISCONNECT,            /* WiFi断开 */
    ESP8266_INVALID_PARAM               /* 无效参数 */
} ESP8266_Status_t;

/**
  * @brief  ESP8266 WiFi模式枚举
  */
typedef enum {
    ESP8266_MODE_STA = 1,               /* Station模式 */
    ESP8266_MODE_AP = 2,                /* SoftAP模式 */
    ESP8266_MODE_STA_AP = 3             /* Station + SoftAP模式 */
} ESP8266_WiFiMode_t;

/**
  * @brief  ESP8266 加密方式枚举
  */
typedef enum {
    ESP8266_ECN_OPEN = 0,               /* 开放网络 */
    ESP8266_ECN_WEP = 1,                /* WEP */
    ESP8266_ECN_WPA_PSK = 2,            /* WPA_PSK */
    ESP8266_ECN_WPA2_PSK = 3,           /* WPA2_PSK */
    ESP8266_ECN_WPA_WPA2_PSK = 4        /* WPA_WPA2_PSK */
} ESP8266_Encryption_t;

/**
  * @brief  ESP8266 连接类型枚举
  */
typedef enum {
    ESP8266_TCP = 0,                    /* TCP连接 */
    ESP8266_UDP = 1,                    /* UDP连接 */
    ESP8266_SSL = 2                     /* SSL连接 */
} ESP8266_ConnType_t;

/**
  * @brief  ESP8266 连接角色枚举
  */
typedef enum {
    ESP8266_ROLE_CLIENT = 0,            /* 客户端 */
    ESP8266_ROLE_SERVER = 1             /* 服务器 */
} ESP8266_ConnRole_t;

/**
  * @brief  ESP8266 接收数据结构
  */
typedef struct {
    uint8_t linkId;                     /* 连接ID (多连接模式) */
    uint16_t length;                    /* 数据长度 */
    uint8_t data[ESP8266_RX_BUF_SIZE];  /* 数据内容 */
} ESP8266_RxData_t;

/**
  * @brief  WiFi AP信息结构
  */
typedef struct {
    ESP8266_Encryption_t ecn;           /* 加密方式 */
    char ssid[33];                      /* SSID */
    int8_t rssi;                        /* 信号强度 */
    char mac[18];                       /* MAC地址 */
    uint8_t channel;                    /* 信道 */
} ESP8266_APInfo_t;

/**
  * @brief  IP信息结构
  */
typedef struct {
    char ip[16];                        /* IP地址 */
    char gateway[16];                   /* 网关地址 */
    char netmask[16];                   /* 子网掩码 */
} ESP8266_IPInfo_t;

/**
  * @brief  连接状态结构
  */
typedef struct {
    uint8_t linkId;                     /* 连接ID */
    ESP8266_ConnType_t type;            /* 连接类型 */
    ESP8266_ConnRole_t role;            /* 连接角色 */
    char remoteIp[16];                  /* 远程IP */
    uint16_t remotePort;                /* 远程端口 */
    uint16_t localPort;                 /* 本地端口 */
} ESP8266_ConnStatus_t;

/**
  * @brief  DMA接收缓冲区结构 (双缓冲)
  */
typedef struct {
    uint8_t buffer[ESP8266_RX_BUF_SIZE];    /* 接收缓冲区 */
    volatile uint16_t length;                /* 接收数据长度 */
    volatile uint8_t ready;                  /* 数据就绪标志 */
} ESP8266_DMA_RxBuffer_t;

/**
  * @brief  ESP8266 句柄结构
  */
typedef struct {
    UART_HandleTypeDef *huart;          /* UART句柄 */
    
    /* DMA接收缓冲区 */
    uint8_t dmaRxBuffer[ESP8266_RX_BUF_SIZE];   /* DMA接收缓冲区 */
    
    /* 处理缓冲区 */
    uint8_t rxBuffer[ESP8266_RX_BUF_SIZE];      /* 接收处理缓冲区 */
    volatile uint16_t rxLength;                  /* 接收数据长度 */
    volatile uint8_t rxComplete;                 /* 接收完成标志 */
    
    /* 发送缓冲区 */
    uint8_t txBuffer[ESP8266_TX_BUF_SIZE];
    volatile uint8_t txBusy;                     /* 发送忙标志 */
    
    /* 状态信息 */
    uint8_t initialized;                /* 初始化标志 */
    uint8_t wifiConnected;              /* WiFi连接状态 */
    uint8_t serverStarted;              /* 服务器启动状态 */
    uint8_t multiConnMode;              /* 多连接模式标志 */
    uint8_t transparentMode;            /* 透传模式标志 */
    ESP8266_WiFiMode_t wifiMode;        /* WiFi模式 */
    
    /* IP信息 */
    ESP8266_IPInfo_t ipInfo;            /* IP信息 */
    
    /* 回调函数 */
    void (*onDataReceived)(ESP8266_RxData_t *data);     /* 数据接收回调 */
    void (*onWifiConnected)(void);                       /* WiFi连接回调 */
    void (*onWifiDisconnected)(void);                    /* WiFi断开回调 */
    void (*onClientConnected)(uint8_t linkId);          /* 客户端连接回调 */
    void (*onClientDisconnected)(uint8_t linkId);       /* 客户端断开回调 */
    
} ESP8266_Handle_t;

/* Exported variables --------------------------------------------------------*/
extern ESP8266_Handle_t esp8266;

/* Exported functions --------------------------------------------------------*/

/* 初始化和基本控制 */
ESP8266_Status_t ESP8266_Init(UART_HandleTypeDef *huart);
ESP8266_Status_t ESP8266_DeInit(void);
ESP8266_Status_t ESP8266_Reset(void);
ESP8266_Status_t ESP8266_Test(void);
ESP8266_Status_t ESP8266_Restore(void);
ESP8266_Status_t ESP8266_SetEcho(uint8_t enable);
ESP8266_Status_t ESP8266_GetVersion(char *version, uint16_t maxLen);

/* WiFi模式设置 */
ESP8266_Status_t ESP8266_SetWiFiMode(ESP8266_WiFiMode_t mode);
ESP8266_Status_t ESP8266_GetWiFiMode(ESP8266_WiFiMode_t *mode);

/* Station模式操作 */
ESP8266_Status_t ESP8266_ConnectAP(const char *ssid, const char *password);
ESP8266_Status_t ESP8266_DisconnectAP(void);
ESP8266_Status_t ESP8266_GetAPInfo(ESP8266_APInfo_t *apInfo);
ESP8266_Status_t ESP8266_ScanAP(ESP8266_APInfo_t *apList, uint8_t maxCount, uint8_t *foundCount);
ESP8266_Status_t ESP8266_SetAutoConnect(uint8_t enable);

/* SoftAP模式操作 */
ESP8266_Status_t ESP8266_SetupAP(const char *ssid, const char *password, 
                                  uint8_t channel, ESP8266_Encryption_t ecn);
ESP8266_Status_t ESP8266_GetAPConfig(char *ssid, char *password, 
                                      uint8_t *channel, ESP8266_Encryption_t *ecn);

/* IP操作 */
ESP8266_Status_t ESP8266_GetIPInfo(ESP8266_IPInfo_t *ipInfo);
ESP8266_Status_t ESP8266_SetStationIP(const char *ip, const char *gateway, const char *netmask);
ESP8266_Status_t ESP8266_SetAPIP(const char *ip, const char *gateway, const char *netmask);
ESP8266_Status_t ESP8266_EnableDHCP(ESP8266_WiFiMode_t mode, uint8_t enable);
ESP8266_Status_t ESP8266_GetMac(ESP8266_WiFiMode_t mode, char *mac);
ESP8266_Status_t ESP8266_SetMac(ESP8266_WiFiMode_t mode, const char *mac);

/* TCP/UDP操作 */
ESP8266_Status_t ESP8266_SetMultiConn(uint8_t enable);
ESP8266_Status_t ESP8266_Connect(ESP8266_ConnType_t type, const char *host, 
                                  uint16_t port, uint8_t *linkId);
ESP8266_Status_t ESP8266_ConnectEx(uint8_t linkId, ESP8266_ConnType_t type, 
                                    const char *host, uint16_t port);
ESP8266_Status_t ESP8266_Close(uint8_t linkId);
ESP8266_Status_t ESP8266_CloseAll(void);
ESP8266_Status_t ESP8266_GetConnStatus(ESP8266_ConnStatus_t *status, uint8_t *count);

/* TCP服务器操作 */
ESP8266_Status_t ESP8266_StartServer(uint16_t port);
ESP8266_Status_t ESP8266_StopServer(void);
ESP8266_Status_t ESP8266_SetServerTimeout(uint16_t timeout);

/* 数据发送接收 (DMA) */
ESP8266_Status_t ESP8266_Send(uint8_t linkId, const uint8_t *data, uint16_t len);
ESP8266_Status_t ESP8266_SendString(uint8_t linkId, const char *str);
ESP8266_Status_t ESP8266_SendPrintf(uint8_t linkId, const char *format, ...);
ESP8266_Status_t ESP8266_SendDMA(const uint8_t *data, uint16_t len);

/* 透传模式 */
ESP8266_Status_t ESP8266_EnterTransparent(void);
ESP8266_Status_t ESP8266_ExitTransparent(void);
ESP8266_Status_t ESP8266_TransparentSend(const uint8_t *data, uint16_t len);

/* HTTP操作 */
ESP8266_Status_t ESP8266_HttpGet(const char *host, uint16_t port, const char *path,
                                  char *response, uint16_t maxLen);
ESP8266_Status_t ESP8266_HttpPost(const char *host, uint16_t port, const char *path,
                                   const char *contentType, const char *body,
                                   char *response, uint16_t maxLen);

/* PING操作 */
ESP8266_Status_t ESP8266_Ping(const char *host);

/* 底层通信函数 */
ESP8266_Status_t ESP8266_SendCommand(const char *cmd, const char *expectedResp, 
                                      uint32_t timeout);
ESP8266_Status_t ESP8266_SendCommandF(const char *expectedResp, uint32_t timeout,
                                       const char *format, ...);
void ESP8266_ClearBuffer(void);
uint8_t ESP8266_WaitForResponse(const char *response, uint32_t timeout);
uint8_t ESP8266_ContainsString(const char *str);
char* ESP8266_GetResponseBuffer(void);

/* 回调设置函数 */
void ESP8266_SetOnDataReceived(void (*callback)(ESP8266_RxData_t *data));
void ESP8266_SetOnWifiConnected(void (*callback)(void));
void ESP8266_SetOnWifiDisconnected(void (*callback)(void));
void ESP8266_SetOnClientConnected(void (*callback)(uint8_t linkId));
void ESP8266_SetOnClientDisconnected(void (*callback)(uint8_t linkId));

/* DMA和中断处理函数 */
void ESP8266_UART_IdleCallback(UART_HandleTypeDef *huart);
void ESP8266_DMA_RxCpltCallback(UART_HandleTypeDef *huart);
void ESP8266_DMA_TxCpltCallback(UART_HandleTypeDef *huart);
void ESP8266_StartDMAReceive(void);
void ESP8266_ProcessData(void);

/* 状态查询 */
uint8_t ESP8266_IsInitialized(void);
uint8_t ESP8266_IsWifiConnected(void);
uint8_t ESP8266_IsTxBusy(void);

/* 调试函数 */
void ESP8266_DebugPrint(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* __ESP8266_H */
