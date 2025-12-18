/**
  ******************************************************************************
  * @file           : esp8266_mqtt.h
  * @brief          : ESP8266 MQTT扩展库头文件
  * @author         : Antigravity AI
  * @version        : V1.0.0
  ******************************************************************************
  * @attention
  *
  * ESP8266 MQTT扩展库 - 基于AT指令的MQTT客户端实现
  * 依赖esp8266.h主驱动
  *
  * 支持功能:
  *   - MQTT用户配置 (Client ID, Username, Password)
  *   - MQTT连接/断开
  *   - 主题订阅/取消订阅
  *   - 消息发布 (字符串/原始数据)
  *   - 消息接收回调
  *   - 遗嘱消息(LWT)配置
  *   - 自动重连
  *
  * AT指令支持:
  *   - AT+MQTTUSERCFG: 配置MQTT用户参数
  *   - AT+MQTTCONNCFG: 配置MQTT连接属性
  *   - AT+MQTTCONN: 连接MQTT Broker
  *   - AT+MQTTPUB: 发布消息
  *   - AT+MQTTPUBRAW: 发布原始数据
  *   - AT+MQTTSUB: 订阅主题
  *   - AT+MQTTUNSUB: 取消订阅
  *   - AT+MQTTCLEAN: 清理MQTT连接
  *
  ******************************************************************************
  */

#ifndef __ESP8266_MQTT_H
#define __ESP8266_MQTT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "esp8266.h"

/* Exported defines ----------------------------------------------------------*/

/* MQTT配置 */
#define MQTT_LINK_ID                    0               /* MQTT Link ID (ESP8266只支持0) */
#define MQTT_CLIENT_ID_MAX_LEN          64              /* Client ID最大长度 */
#define MQTT_USERNAME_MAX_LEN           64              /* 用户名最大长度 */
#define MQTT_PASSWORD_MAX_LEN           64              /* 密码最大长度 */
#define MQTT_TOPIC_MAX_LEN              128             /* 主题最大长度 */
#define MQTT_MESSAGE_MAX_LEN            1024            /* 消息最大长度 */
#define MQTT_HOST_MAX_LEN               128             /* Broker地址最大长度 */

/* MQTT超时配置 */
#define MQTT_CONNECT_TIMEOUT            10000           /* 连接超时(ms) */
#define MQTT_PUBLISH_TIMEOUT            5000            /* 发布超时(ms) */
#define MQTT_SUBSCRIBE_TIMEOUT          5000            /* 订阅超时(ms) */
#define MQTT_DEFAULT_TIMEOUT            3000            /* 默认超时(ms) */

/* MQTT最大订阅主题数 */
#define MQTT_MAX_SUBSCRIPTIONS          8               /* 最大订阅主题数 */

/* 调试开关 */
#define MQTT_DEBUG_ENABLE               1               /* 1:开启调试输出 0:关闭 */

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  MQTT状态枚举
  */
typedef enum {
    MQTT_OK = 0,                        /* 操作成功 */
    MQTT_ERROR,                         /* 一般错误 */
    MQTT_TIMEOUT,                       /* 超时 */
    MQTT_NOT_INITIALIZED,               /* 未初始化 */
    MQTT_NOT_CONNECTED,                 /* 未连接 */
    MQTT_ALREADY_CONNECTED,             /* 已连接 */
    MQTT_CONNECT_FAIL,                  /* 连接失败 */
    MQTT_SUBSCRIBE_FAIL,                /* 订阅失败 */
    MQTT_PUBLISH_FAIL,                  /* 发布失败 */
    MQTT_INVALID_PARAM,                 /* 无效参数 */
    MQTT_BUFFER_FULL,                   /* 缓冲区满 */
    MQTT_WIFI_NOT_CONNECTED             /* WiFi未连接 */
} MQTT_Status_t;

/**
  * @brief  MQTT连接方案枚举
  */
typedef enum {
    MQTT_SCHEME_TCP = 1,                /* MQTT over TCP */
    MQTT_SCHEME_TLS_NO_CERT = 2,        /* MQTT over TLS (不校验证书) */
    MQTT_SCHEME_TLS_SERVER_CERT = 3,    /* MQTT over TLS (校验服务器证书) */
    MQTT_SCHEME_TLS_CLIENT_CERT = 4,    /* MQTT over TLS (提供客户端证书) */
    MQTT_SCHEME_TLS_BOTH_CERT = 5,      /* MQTT over TLS (双向认证) */
    MQTT_SCHEME_WS = 6,                 /* MQTT over WebSocket */
    MQTT_SCHEME_WSS_NO_CERT = 7         /* MQTT over WebSocket Secure */
} MQTT_Scheme_t;

/**
  * @brief  MQTT QoS等级枚举
  */
typedef enum {
    MQTT_QOS_0 = 0,                     /* 最多一次 (At most once) */
    MQTT_QOS_1 = 1,                     /* 至少一次 (At least once) */
    MQTT_QOS_2 = 2                      /* 恰好一次 (Exactly once) */
} MQTT_QoS_t;

/**
  * @brief  MQTT连接状态枚举
  */
typedef enum {
    MQTT_STATE_NOT_INIT = 0,            /* 未初始化 */
    MQTT_STATE_USER_SET = 1,            /* 用户配置已设置 */
    MQTT_STATE_CONN_SET = 2,            /* 连接配置已设置 */
    MQTT_STATE_DISCONNECTED = 3,        /* 已断开 */
    MQTT_STATE_CONNECTED = 4,           /* 已连接 */
    MQTT_STATE_CONN_NO_SUB = 5,         /* 已连接无订阅 */
    MQTT_STATE_CONN_WITH_SUB = 6        /* 已连接有订阅 */
} MQTT_State_t;

/**
  * @brief  MQTT用户配置结构
  */
typedef struct {
    MQTT_Scheme_t scheme;               /* 连接方案 */
    char clientId[MQTT_CLIENT_ID_MAX_LEN];  /* 客户端ID */
    char username[MQTT_USERNAME_MAX_LEN];   /* 用户名 */
    char password[MQTT_PASSWORD_MAX_LEN];   /* 密码 */
    uint8_t certKeyId;                  /* 证书密钥ID (TLS) */
    uint8_t caId;                       /* CA证书ID (TLS) */
    char path[64];                      /* WebSocket路径 */
} MQTT_UserConfig_t;

/**
  * @brief  MQTT连接配置结构
  */
typedef struct {
    uint16_t keepAlive;                 /* 心跳间隔(秒) */
    uint8_t disableCleanSession;        /* 禁用清除会话 */
    char lwtTopic[MQTT_TOPIC_MAX_LEN];  /* 遗嘱主题 */
    char lwtMessage[MQTT_MESSAGE_MAX_LEN]; /* 遗嘱消息 */
    MQTT_QoS_t lwtQos;                  /* 遗嘱QoS */
    uint8_t lwtRetain;                  /* 遗嘱保留标志 */
} MQTT_ConnConfig_t;

/**
  * @brief  MQTT Broker配置结构
  */
typedef struct {
    char host[MQTT_HOST_MAX_LEN];       /* Broker地址 */
    uint16_t port;                      /* Broker端口 */
    uint8_t reconnect;                  /* 自动重连 (0:禁用 1:启用) */
} MQTT_BrokerConfig_t;

/**
  * @brief  MQTT订阅信息结构
  */
typedef struct {
    char topic[MQTT_TOPIC_MAX_LEN];     /* 主题 */
    MQTT_QoS_t qos;                     /* QoS等级 */
    uint8_t active;                     /* 是否活跃 */
} MQTT_Subscription_t;

/**
  * @brief  MQTT消息结构
  */
typedef struct {
    char topic[MQTT_TOPIC_MAX_LEN];     /* 主题 */
    uint8_t data[MQTT_MESSAGE_MAX_LEN]; /* 消息数据 */
    uint16_t dataLen;                   /* 数据长度 */
    MQTT_QoS_t qos;                     /* QoS等级 */
    uint8_t retain;                     /* 保留标志 */
} MQTT_Message_t;

/**
  * @brief  MQTT句柄结构
  */
typedef struct {
    /* 配置信息 */
    MQTT_UserConfig_t userConfig;       /* 用户配置 */
    MQTT_ConnConfig_t connConfig;       /* 连接配置 */
    MQTT_BrokerConfig_t brokerConfig;   /* Broker配置 */
    
    /* 订阅列表 */
    MQTT_Subscription_t subscriptions[MQTT_MAX_SUBSCRIPTIONS];
    uint8_t subscriptionCount;          /* 当前订阅数量 */
    
    /* 状态信息 */
    MQTT_State_t state;                 /* MQTT状态 */
    uint8_t initialized;                /* 初始化标志 */
    uint8_t connected;                  /* 连接标志 */
    
    /* 统计信息 */
    uint32_t publishCount;              /* 发布计数 */
    uint32_t receiveCount;              /* 接收计数 */
    uint32_t reconnectCount;            /* 重连计数 */
    
    /* 回调函数 */
    void (*onConnected)(void);                              /* 连接成功回调 */
    void (*onDisconnected)(void);                           /* 断开连接回调 */
    void (*onMessageReceived)(MQTT_Message_t *message);     /* 消息接收回调 */
    void (*onPublishComplete)(const char *topic);           /* 发布完成回调 */
    void (*onSubscribed)(const char *topic);                /* 订阅成功回调 */
    void (*onUnsubscribed)(const char *topic);              /* 取消订阅回调 */
    void (*onError)(MQTT_Status_t error);                   /* 错误回调 */
    
} MQTT_Handle_t;

/* Exported variables --------------------------------------------------------*/
extern MQTT_Handle_t mqtt;

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  初始化和配置函数
  */
MQTT_Status_t MQTT_Init(void);
MQTT_Status_t MQTT_DeInit(void);

/**
  * @brief  用户配置函数
  */
MQTT_Status_t MQTT_SetUserConfig(const MQTT_UserConfig_t *config);
MQTT_Status_t MQTT_SetUserConfigSimple(const char *clientId, 
                                        const char *username, 
                                        const char *password);

/**
  * @brief  连接配置函数
  */
MQTT_Status_t MQTT_SetConnConfig(const MQTT_ConnConfig_t *config);
MQTT_Status_t MQTT_SetKeepAlive(uint16_t keepAlive);
MQTT_Status_t MQTT_SetLWT(const char *topic, const char *message, 
                           MQTT_QoS_t qos, uint8_t retain);

/**
  * @brief  Broker配置函数
  */
MQTT_Status_t MQTT_SetBrokerConfig(const MQTT_BrokerConfig_t *config);
MQTT_Status_t MQTT_SetBroker(const char *host, uint16_t port, uint8_t reconnect);

/**
  * @brief  连接控制函数
  */
MQTT_Status_t MQTT_Connect(void);
MQTT_Status_t MQTT_ConnectToBroker(const char *host, uint16_t port, 
                                    const char *clientId,
                                    const char *username, 
                                    const char *password);
MQTT_Status_t MQTT_Disconnect(void);
MQTT_Status_t MQTT_Reconnect(void);
MQTT_Status_t MQTT_Clean(void);

/**
  * @brief  订阅函数
  */
MQTT_Status_t MQTT_Subscribe(const char *topic, MQTT_QoS_t qos);
MQTT_Status_t MQTT_SubscribeMultiple(const char **topics, MQTT_QoS_t *qos, uint8_t count);
MQTT_Status_t MQTT_Unsubscribe(const char *topic);
MQTT_Status_t MQTT_UnsubscribeAll(void);
MQTT_Status_t MQTT_GetSubscriptions(MQTT_Subscription_t *list, uint8_t *count);

/**
  * @brief  发布函数
  */
MQTT_Status_t MQTT_Publish(const char *topic, const char *message, 
                            MQTT_QoS_t qos, uint8_t retain);
MQTT_Status_t MQTT_PublishData(const char *topic, const uint8_t *data, 
                                uint16_t len, MQTT_QoS_t qos, uint8_t retain);
MQTT_Status_t MQTT_PublishRaw(const char *topic, const uint8_t *data, 
                               uint16_t len, MQTT_QoS_t qos, uint8_t retain);
MQTT_Status_t MQTT_PublishF(const char *topic, MQTT_QoS_t qos, 
                             uint8_t retain, const char *format, ...);

/**
  * @brief  状态查询函数
  */
MQTT_State_t MQTT_GetState(void);
uint8_t MQTT_IsConnected(void);
uint8_t MQTT_IsInitialized(void);
MQTT_Status_t MQTT_QueryConnection(void);

/**
  * @brief  回调设置函数
  */
void MQTT_SetOnConnected(void (*callback)(void));
void MQTT_SetOnDisconnected(void (*callback)(void));
void MQTT_SetOnMessageReceived(void (*callback)(MQTT_Message_t *message));
void MQTT_SetOnPublishComplete(void (*callback)(const char *topic));
void MQTT_SetOnSubscribed(void (*callback)(const char *topic));
void MQTT_SetOnUnsubscribed(void (*callback)(const char *topic));
void MQTT_SetOnError(void (*callback)(MQTT_Status_t error));

/**
  * @brief  数据处理函数
  */
void MQTT_ProcessData(void);
void MQTT_ProcessMessage(const char *data, uint16_t len);

/**
  * @brief  调试函数
  */
void MQTT_DebugPrint(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* __ESP8266_MQTT_H */
