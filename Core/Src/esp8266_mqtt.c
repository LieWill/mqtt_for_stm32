/**
  ******************************************************************************
  * @file           : esp8266_mqtt.c
  * @brief          : ESP8266 MQTT扩展库源文件
  * @author         : Antigravity AI
  * @version        : V1.0.0
  ******************************************************************************
  * @attention
  *
  * ESP8266 MQTT扩展库实现
  * 基于ESP8266 AT指令集实现MQTT客户端功能
  *
  * AT指令参考:
  *   AT+MQTTUSERCFG=<LinkID>,<scheme>,"<client_id>","<username>","<password>",<cert_key_ID>,<CA_ID>,"<path>"
  *   AT+MQTTCONNCFG=<LinkID>,<keepalive>,<disable_clean_session>,"<lwt_topic>","<lwt_msg>",<lwt_qos>,<lwt_retain>
  *   AT+MQTTCONN=<LinkID>,"<host>",<port>,<reconnect>
  *   AT+MQTTCONN?
  *   AT+MQTTSUB=<LinkID>,"<topic>",<qos>
  *   AT+MQTTUNSUB=<LinkID>,"<topic>"
  *   AT+MQTTPUB=<LinkID>,"<topic>","<data>",<qos>,<retain>
  *   AT+MQTTPUBRAW=<LinkID>,"<topic>",<length>,<qos>,<retain>
  *   AT+MQTTCLEAN=<LinkID>
  *
  ******************************************************************************
  */

#include "esp8266_mqtt.h"

/* Private variables ---------------------------------------------------------*/
MQTT_Handle_t mqtt;

/* Private function prototypes -----------------------------------------------*/
static void MQTT_Delay(uint32_t ms);
static MQTT_Status_t MQTT_ParseSubMessage(const char *data);
static void MQTT_AddSubscription(const char *topic, MQTT_QoS_t qos);
static void MQTT_RemoveSubscription(const char *topic);

/* Debug print - 使用统一日志库 */
void MQTT_DebugPrint(const char *format, ...)
{
#if MQTT_DEBUG_ENABLE
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    /* 使用LOG_Raw以兼容原有格式 */
    LOG_Raw("%s", buf);
#endif
}

static void MQTT_Delay(uint32_t ms) 
{ 
    HAL_Delay(ms); 
}

/**
  * @brief  初始化MQTT模块
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_Init(void)
{
    MQTT_DebugPrint("[MQTT] Initializing...\r\n");
    
    /* 检查ESP8266是否已初始化 */
    if (!ESP8266_IsInitialized()) {
         MQTT_DebugPrint("[MQTT] ESP8266 not initialized!\r\n");
         return MQTT_NOT_INITIALIZED;
    }
    
    /* 清空MQTT句柄 */
    memset(&mqtt, 0, sizeof(MQTT_Handle_t));
    
    /* 设置默认值 */
    mqtt.userConfig.scheme = MQTT_SCHEME_TCP;
    mqtt.connConfig.keepAlive = 120;
    mqtt.connConfig.disableCleanSession = 0;
    mqtt.brokerConfig.port = 1883;
    mqtt.brokerConfig.reconnect = 1;
    
    mqtt.state = MQTT_STATE_NOT_INIT;
    mqtt.initialized = 1;
    
    MQTT_DebugPrint("[MQTT] Init OK\r\n");
    return MQTT_OK;
}

/**
  * @brief  反初始化MQTT模块
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_DeInit(void)
{
    if (mqtt.connected) {
        MQTT_Disconnect();
    }
    MQTT_Clean();
    memset(&mqtt, 0, sizeof(MQTT_Handle_t));
    return MQTT_OK;
}

/**
  * @brief  设置MQTT用户配置
  * @param  config: 用户配置结构指针
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_SetUserConfig(const MQTT_UserConfig_t *config)
{
    if (!mqtt.initialized) return MQTT_NOT_INITIALIZED;
    if (!config) return MQTT_INVALID_PARAM;
    
    MQTT_DebugPrint("[MQTT] Setting user config...\r\n");
    
    /* 发送AT+MQTTUSERCFG指令 */
    ESP8266_Status_t ret = ESP8266_SendCommandF("OK", MQTT_DEFAULT_TIMEOUT,
        "AT+MQTTUSERCFG=%d,%d,\"%s\",\"%s\",\"%s\",%d,%d,\"%s\"\r\n",
        MQTT_LINK_ID,
        config->scheme,
        config->clientId,
        config->username,
        config->password,
        config->certKeyId,
        config->caId,
        config->path);
    
    if (ret != ESP8266_OK) {
        MQTT_DebugPrint("[MQTT] User config failed!\r\n");
        return MQTT_ERROR;
    }
    
    /* 保存配置 */
    memcpy(&mqtt.userConfig, config, sizeof(MQTT_UserConfig_t));
    mqtt.state = MQTT_STATE_USER_SET;
    
    MQTT_DebugPrint("[MQTT] User config OK\r\n");
    return MQTT_OK;
}

/**
  * @brief  简化的用户配置设置
  * @param  clientId: 客户端ID
  * @param  username: 用户名 (可为NULL)
  * @param  password: 密码 (可为NULL)
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_SetUserConfigSimple(const char *clientId, 
                                        const char *username, 
                                        const char *password)
{
    if (!mqtt.initialized) return MQTT_NOT_INITIALIZED;
    if (!clientId) return MQTT_INVALID_PARAM;
    
    MQTT_DebugPrint("[MQTT] Setting user config (simple)...\r\n");
    
    /* 发送AT+MQTTUSERCFG指令 - 使用TCP方案 */
    ESP8266_Status_t ret = ESP8266_SendCommandF("OK", MQTT_DEFAULT_TIMEOUT,
        "AT+MQTTUSERCFG=%d,%d,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
        MQTT_LINK_ID,
        MQTT_SCHEME_TCP,
        clientId,
        username ? username : "",
        password ? password : "");
    
    if (ret != ESP8266_OK) {
        MQTT_DebugPrint("[MQTT] User config failed!\r\n");
        return MQTT_ERROR;
    }
    
    /* 保存配置 */
    mqtt.userConfig.scheme = MQTT_SCHEME_TCP;
    strncpy(mqtt.userConfig.clientId, clientId, MQTT_CLIENT_ID_MAX_LEN - 1);
    if (username) strncpy(mqtt.userConfig.username, username, MQTT_USERNAME_MAX_LEN - 1);
    if (password) strncpy(mqtt.userConfig.password, password, MQTT_PASSWORD_MAX_LEN - 1);
    mqtt.state = MQTT_STATE_USER_SET;
    
    MQTT_DebugPrint("[MQTT] User config OK\r\n");
    return MQTT_OK;
}

/**
  * @brief  设置MQTT连接配置
  * @param  config: 连接配置结构指针
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_SetConnConfig(const MQTT_ConnConfig_t *config)
{
    if (!mqtt.initialized) return MQTT_NOT_INITIALIZED;
    if (!config) return MQTT_INVALID_PARAM;
    
    MQTT_DebugPrint("[MQTT] Setting conn config...\r\n");
    
    /* 发送AT+MQTTCONNCFG指令 */
    ESP8266_Status_t ret = ESP8266_SendCommandF("OK", MQTT_DEFAULT_TIMEOUT,
        "AT+MQTTCONNCFG=%d,%d,%d,\"%s\",\"%s\",%d,%d\r\n",
        MQTT_LINK_ID,
        config->keepAlive,
        config->disableCleanSession,
        config->lwtTopic,
        config->lwtMessage,
        config->lwtQos,
        config->lwtRetain);
    
    if (ret != ESP8266_OK) {
        MQTT_DebugPrint("[MQTT] Conn config failed!\r\n");
        return MQTT_ERROR;
    }
    
    /* 保存配置 */
    memcpy(&mqtt.connConfig, config, sizeof(MQTT_ConnConfig_t));
    mqtt.state = MQTT_STATE_CONN_SET;
    
    MQTT_DebugPrint("[MQTT] Conn config OK\r\n");
    return MQTT_OK;
}

/**
  * @brief  设置心跳间隔
  * @param  keepAlive: 心跳间隔(秒), 范围0-7200
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_SetKeepAlive(uint16_t keepAlive)
{
    if (keepAlive > 7200) keepAlive = 7200;
    
    mqtt.connConfig.keepAlive = keepAlive;
    
    /* 如果已配置,重新发送配置 */
    if (mqtt.state >= MQTT_STATE_USER_SET) {
        return MQTT_SetConnConfig(&mqtt.connConfig);
    }
    
    return MQTT_OK;
}

/**
  * @brief  设置遗嘱消息(LWT)
  * @param  topic: 遗嘱主题
  * @param  message: 遗嘱消息
  * @param  qos: QoS等级
  * @param  retain: 保留标志
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_SetLWT(const char *topic, const char *message, 
                           MQTT_QoS_t qos, uint8_t retain)
{
    if (!topic || !message) return MQTT_INVALID_PARAM;
    
    strncpy(mqtt.connConfig.lwtTopic, topic, MQTT_TOPIC_MAX_LEN - 1);
    strncpy(mqtt.connConfig.lwtMessage, message, MQTT_MESSAGE_MAX_LEN - 1);
    mqtt.connConfig.lwtQos = qos;
    mqtt.connConfig.lwtRetain = retain ? 1 : 0;
    
    /* 如果已配置,重新发送配置 */
    if (mqtt.state >= MQTT_STATE_USER_SET) {
        return MQTT_SetConnConfig(&mqtt.connConfig);
    }
    
    return MQTT_OK;
}

/**
  * @brief  设置Broker配置
  * @param  config: Broker配置结构指针
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_SetBrokerConfig(const MQTT_BrokerConfig_t *config)
{
    if (!config) return MQTT_INVALID_PARAM;
    
    memcpy(&mqtt.brokerConfig, config, sizeof(MQTT_BrokerConfig_t));
    return MQTT_OK;
}

/**
  * @brief  设置Broker (简化版)
  * @param  host: Broker地址
  * @param  port: 端口号
  * @param  reconnect: 自动重连 (0:禁用 1:启用)
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_SetBroker(const char *host, uint16_t port, uint8_t reconnect)
{
    if (!host) return MQTT_INVALID_PARAM;
    
    strncpy(mqtt.brokerConfig.host, host, MQTT_HOST_MAX_LEN - 1);
    mqtt.brokerConfig.port = port;
    mqtt.brokerConfig.reconnect = reconnect ? 1 : 0;
    
    return MQTT_OK;
}

/**
  * @brief  连接到MQTT Broker (使用已配置的参数)
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_Connect(void)
{
    //if (!mqtt.initialized) return MQTT_NOT_INITIALIZED;
//    if (!ESP8266_IsWifiConnected()) {
//        MQTT_DebugPrint("[MQTT] WiFi not connected!\r\n");
//        return MQTT_WIFI_NOT_CONNECTED;
//    }
//    
//    if (mqtt.state < MQTT_STATE_USER_SET) {
//        MQTT_DebugPrint("[MQTT] User config not set!\r\n");
//        return MQTT_ERROR;
//    }
//    
//    if (strlen(mqtt.brokerConfig.host) == 0) {
//        MQTT_DebugPrint("[MQTT] Broker not configured!\r\n");
//        return MQTT_INVALID_PARAM;
//    }
    
    MQTT_DebugPrint("[MQTT] Connecting to %s:%d...\r\n", 
                    mqtt.brokerConfig.host, mqtt.brokerConfig.port);
    
    /* 发送AT+MQTTCONN指令 */
    ESP8266_Status_t ret = ESP8266_SendCommandF("OK", MQTT_CONNECT_TIMEOUT,
        "AT+MQTTCONN=%d,\"%s\",%d,%d\r\n",
        MQTT_LINK_ID,
        mqtt.brokerConfig.host,
        mqtt.brokerConfig.port,
        mqtt.brokerConfig.reconnect);
    
    if (ret != ESP8266_OK) {
        MQTT_DebugPrint("[MQTT] Connect failed!\r\n");
        mqtt.connected = 0;
        mqtt.state = MQTT_STATE_DISCONNECTED;
        if (mqtt.onError) mqtt.onError(MQTT_CONNECT_FAIL);
        return MQTT_CONNECT_FAIL;
    }
    
    mqtt.connected = 1;
    mqtt.state = MQTT_STATE_CONN_NO_SUB;
    
    MQTT_DebugPrint("[MQTT] Connected!\r\n");
    if (mqtt.onConnected) mqtt.onConnected();
    
    return MQTT_OK;
}

/**
  * @brief  一站式连接到MQTT Broker
  * @param  host: Broker地址
  * @param  port: 端口号
  * @param  clientId: 客户端ID
  * @param  username: 用户名 (可为NULL)
  * @param  password: 密码 (可为NULL)
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_ConnectToBroker(const char *host, uint16_t port, 
                                    const char *clientId,
                                    const char *username, 
                                    const char *password)
{
    MQTT_Status_t ret;
    
    /* 设置用户配置 */
    ret = MQTT_SetUserConfigSimple(clientId, username, password);
    if (ret != MQTT_OK) return ret;
    
    /* 设置Broker */
    ret = MQTT_SetBroker(host, port, 1);  /* 启用自动重连 */
    if (ret != MQTT_OK) return ret;
    
    /* 连接 */
    return MQTT_Connect();
}

/**
  * @brief  断开MQTT连接
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_Disconnect(void)
{
    if (!mqtt.initialized) return MQTT_NOT_INITIALIZED;
    
    MQTT_DebugPrint("[MQTT] Disconnecting...\r\n");
    
    /* 使用AT+MQTTCLEAN断开连接 */
    ESP8266_Status_t ret = ESP8266_SendCommandF("OK", MQTT_DEFAULT_TIMEOUT,
        "AT+MQTTCLEAN=%d\r\n", MQTT_LINK_ID);
    
    mqtt.connected = 0;
    mqtt.state = MQTT_STATE_DISCONNECTED;
    
    /* 清除订阅列表 */
    memset(mqtt.subscriptions, 0, sizeof(mqtt.subscriptions));
    mqtt.subscriptionCount = 0;
    
    MQTT_DebugPrint("[MQTT] Disconnected\r\n");
    if (mqtt.onDisconnected) mqtt.onDisconnected();
    
    return (ret == ESP8266_OK) ? MQTT_OK : MQTT_ERROR;
}

/**
  * @brief  重新连接MQTT
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_Reconnect(void)
{
    MQTT_DebugPrint("[MQTT] Reconnecting...\r\n");
    mqtt.reconnectCount++;
    
    /* 先清理之前的连接 */
    MQTT_Clean();
    MQTT_Delay(1000);
    
    /* 重新配置并连接 */
    MQTT_Status_t ret = MQTT_SetUserConfigSimple(
        mqtt.userConfig.clientId,
        mqtt.userConfig.username,
        mqtt.userConfig.password);
    
    if (ret != MQTT_OK) return ret;
    
    return MQTT_Connect();
}

/**
  * @brief  清理MQTT资源
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_Clean(void)
{
    ESP8266_Status_t ret = ESP8266_SendCommandF("OK", MQTT_DEFAULT_TIMEOUT,
        "AT+MQTTCLEAN=%d\r\n", MQTT_LINK_ID);
    
    mqtt.connected = 0;
    mqtt.state = MQTT_STATE_NOT_INIT;
    
    return (ret == ESP8266_OK) ? MQTT_OK : MQTT_ERROR;
}

/**
  * @brief  订阅主题
  * @param  topic: 主题名称
  * @param  qos: QoS等级
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_Subscribe(const char *topic, MQTT_QoS_t qos)
{
    if (!mqtt.initialized) return MQTT_NOT_INITIALIZED;
    if (!mqtt.connected) return MQTT_NOT_CONNECTED;
    if (!topic) return MQTT_INVALID_PARAM;
    if (mqtt.subscriptionCount >= MQTT_MAX_SUBSCRIPTIONS) return MQTT_BUFFER_FULL;
    
    MQTT_DebugPrint("[MQTT] Subscribing to: %s (QoS%d)\r\n", topic, qos);
    
    /* 发送AT+MQTTSUB指令 */
    ESP8266_Status_t ret = ESP8266_SendCommandF("OK", MQTT_SUBSCRIBE_TIMEOUT,
        "AT+MQTTSUB=%d,\"%s\",%d\r\n",
        MQTT_LINK_ID, topic, qos);
    
    if (ret != ESP8266_OK) {
        MQTT_DebugPrint("[MQTT] Subscribe failed!\r\n");
        return MQTT_SUBSCRIBE_FAIL;
    }
    
    /* 添加到订阅列表 */
    MQTT_AddSubscription(topic, qos);
    mqtt.state = MQTT_STATE_CONN_WITH_SUB;
    
    MQTT_DebugPrint("[MQTT] Subscribed OK\r\n");
    if (mqtt.onSubscribed) mqtt.onSubscribed(topic);
    
    return MQTT_OK;
}

/**
  * @brief  订阅多个主题
  * @param  topics: 主题数组
  * @param  qos: QoS数组
  * @param  count: 主题数量
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_SubscribeMultiple(const char **topics, MQTT_QoS_t *qos, uint8_t count)
{
    if (!topics || !qos || count == 0) return MQTT_INVALID_PARAM;
    
    MQTT_Status_t ret;
    for (uint8_t i = 0; i < count; i++) {
        ret = MQTT_Subscribe(topics[i], qos[i]);
        if (ret != MQTT_OK) return ret;
        MQTT_Delay(100);  /* 间隔一下避免命令过快 */
    }
    
    return MQTT_OK;
}

/**
  * @brief  取消订阅主题
  * @param  topic: 主题名称
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_Unsubscribe(const char *topic)
{
    if (!mqtt.initialized) return MQTT_NOT_INITIALIZED;
    if (!mqtt.connected) return MQTT_NOT_CONNECTED;
    if (!topic) return MQTT_INVALID_PARAM;
    
    MQTT_DebugPrint("[MQTT] Unsubscribing from: %s\r\n", topic);
    
    /* 发送AT+MQTTUNSUB指令 */
    ESP8266_Status_t ret = ESP8266_SendCommandF("OK", MQTT_DEFAULT_TIMEOUT,
        "AT+MQTTUNSUB=%d,\"%s\"\r\n",
        MQTT_LINK_ID, topic);
    
    if (ret != ESP8266_OK) {
        MQTT_DebugPrint("[MQTT] Unsubscribe failed!\r\n");
        return MQTT_ERROR;
    }
    
    /* 从订阅列表移除 */
    MQTT_RemoveSubscription(topic);
    
    MQTT_DebugPrint("[MQTT] Unsubscribed OK\r\n");
    if (mqtt.onUnsubscribed) mqtt.onUnsubscribed(topic);
    
    return MQTT_OK;
}

/**
  * @brief  取消所有订阅
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_UnsubscribeAll(void)
{
    MQTT_Status_t ret;
    
    for (uint8_t i = 0; i < MQTT_MAX_SUBSCRIPTIONS; i++) {
        if (mqtt.subscriptions[i].active) {
            ret = MQTT_Unsubscribe(mqtt.subscriptions[i].topic);
            if (ret != MQTT_OK) return ret;
            MQTT_Delay(100);
        }
    }
    
    return MQTT_OK;
}

/**
  * @brief  获取订阅列表
  * @param  list: 订阅列表输出缓冲区
  * @param  count: 输出订阅数量
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_GetSubscriptions(MQTT_Subscription_t *list, uint8_t *count)
{
    if (!list || !count) return MQTT_INVALID_PARAM;
    
    *count = 0;
    for (uint8_t i = 0; i < MQTT_MAX_SUBSCRIPTIONS; i++) {
        if (mqtt.subscriptions[i].active) {
            memcpy(&list[*count], &mqtt.subscriptions[i], sizeof(MQTT_Subscription_t));
            (*count)++;
        }
    }
    
    return MQTT_OK;
}

/**
  * @brief  发布字符串消息
  * @param  topic: 主题名称
  * @param  message: 消息内容
  * @param  qos: QoS等级
  * @param  retain: 保留标志
  * @note   使用MQTTPUBRAW发送原始数据，避免特殊字符转义问题
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_Publish(const char *topic, const char *message, 
                            MQTT_QoS_t qos, uint8_t retain)
{
//    if (!mqtt.initialized) return MQTT_NOT_INITIALIZED;
//    if (!mqtt.connected) return MQTT_NOT_CONNECTED;
    if (!topic || !message) return MQTT_INVALID_PARAM;
    
    uint16_t len = strlen(message);
    
    MQTT_DebugPrint("[MQTT] Publishing to %s (%d bytes): %s\r\n", topic, len, message);
    
    /* 使用AT+MQTTPUBRAW发送原始数据，避免转义问题 */
    ESP8266_Status_t ret = ESP8266_SendCommandF(">", MQTT_DEFAULT_TIMEOUT,
        "AT+MQTTPUBRAW=%d,\"%s\",%d,%d,%d\r\n",
        MQTT_LINK_ID, topic, len, qos, retain ? 1 : 0);
    
    if (ret != ESP8266_OK) {
        MQTT_DebugPrint("[MQTT] PUBRAW prepare failed!\r\n");
        return MQTT_PUBLISH_FAIL;
    }
    
    /* 发送实际数据 */
    ESP8266_ClearBuffer();
    ret = ESP8266_SendDMA((uint8_t *)message, len);
    if (ret != ESP8266_OK) {
        MQTT_DebugPrint("[MQTT] Data send failed!\r\n");
        return MQTT_PUBLISH_FAIL;
    }
    
    /* 等待发送完成 - 检查多种可能的响应 */
    uint32_t startTick = HAL_GetTick();
    while (HAL_GetTick() - startTick < MQTT_PUBLISH_TIMEOUT) {
        if (ESP8266_ContainsString("+MQTTPUB:OK") || 
            ESP8266_ContainsString("OK")) {
            mqtt.publishCount++;
            MQTT_DebugPrint("[MQTT] Publish OK\r\n");
            if (mqtt.onPublishComplete) mqtt.onPublishComplete(topic);
            return MQTT_OK;
        }
        if (ESP8266_ContainsString("ERROR") || 
            ESP8266_ContainsString("FAIL")) {
            MQTT_DebugPrint("[MQTT] Publish failed!\r\n");
            return MQTT_PUBLISH_FAIL;
        }
        HAL_Delay(10);
    }
    
    MQTT_DebugPrint("[MQTT] Publish timeout!\r\n");
    return MQTT_TIMEOUT;
}

/**
  * @brief  发布二进制数据
  * @param  topic: 主题名称
  * @param  data: 数据指针
  * @param  len: 数据长度
  * @param  qos: QoS等级
  * @param  retain: 保留标志
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_PublishData(const char *topic, const uint8_t *data, 
                                uint16_t len, MQTT_QoS_t qos, uint8_t retain)
{
    if (!mqtt.initialized) return MQTT_NOT_INITIALIZED;
    if (!mqtt.connected) return MQTT_NOT_CONNECTED;
    if (!topic || !data || len == 0) return MQTT_INVALID_PARAM;
    
    /* 对于短数据,转换为字符串发送 */
    if (len < MQTT_MESSAGE_MAX_LEN) {
        char msgBuf[MQTT_MESSAGE_MAX_LEN];
        memcpy(msgBuf, data, len);
        msgBuf[len] = '\0';
        return MQTT_Publish(topic, msgBuf, qos, retain);
    }
    
    /* 长数据使用MQTTPUBRAW */
    return MQTT_PublishRaw(topic, data, len, qos, retain);
}

/**
  * @brief  发布原始数据 (使用MQTTPUBRAW)
  * @param  topic: 主题名称
  * @param  data: 数据指针
  * @param  len: 数据长度
  * @param  qos: QoS等级
  * @param  retain: 保留标志
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_PublishRaw(const char *topic, const uint8_t *data, 
                               uint16_t len, MQTT_QoS_t qos, uint8_t retain)
{
//    if (!mqtt.initialized) return MQTT_NOT_INITIALIZED;
//    if (!mqtt.connected) return MQTT_NOT_CONNECTED;
    if (!topic || !data || len == 0) return MQTT_INVALID_PARAM;
    
    MQTT_DebugPrint("[MQTT] Publishing RAW to %s (%d bytes)\r\n", topic, len);
    
    /* 发送AT+MQTTPUBRAW指令 */
    ESP8266_Status_t ret = ESP8266_SendCommandF(">", MQTT_DEFAULT_TIMEOUT,
        "AT+MQTTPUBRAW=%d,\"%s\",%d,%d,%d\r\n",
        MQTT_LINK_ID, topic, len, qos, retain ? 1 : 0);
    
    if (ret != ESP8266_OK) {
        MQTT_DebugPrint("[MQTT] PUBRAW prepare failed!\r\n");
        return MQTT_PUBLISH_FAIL;
    }
    
    /* 发送数据 */
    ESP8266_ClearBuffer();
    ret = ESP8266_SendDMA(data, len);
    if (ret != ESP8266_OK) {
        return MQTT_PUBLISH_FAIL;
    }
    
    /* 等待发送完成 */
    if (!ESP8266_WaitForResponse("+MQTTPUB:OK", MQTT_PUBLISH_TIMEOUT)) {
        MQTT_DebugPrint("[MQTT] PUBRAW failed!\r\n");
        return MQTT_PUBLISH_FAIL;
    }
    
    mqtt.publishCount++;
    MQTT_DebugPrint("[MQTT] PUBRAW OK\r\n");
    if (mqtt.onPublishComplete) mqtt.onPublishComplete(topic);
    
    return MQTT_OK;
}

/**
  * @brief  格式化发布消息
  * @param  topic: 主题名称
  * @param  qos: QoS等级
  * @param  retain: 保留标志
  * @param  format: 格式化字符串
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_PublishF(const char *topic, MQTT_QoS_t qos, 
                             uint8_t retain, const char *format, ...)
{
    char buf[MQTT_MESSAGE_MAX_LEN];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    
    return MQTT_Publish(topic, buf, qos, retain);
}

/**
  * @brief  获取MQTT状态
  * @retval MQTT_State_t
  */
MQTT_State_t MQTT_GetState(void)
{
    return mqtt.state;
}

/**
  * @brief  检查是否已连接
  * @retval uint8_t: 1=已连接, 0=未连接
  */
uint8_t MQTT_IsConnected(void)
{
    return mqtt.connected;
}

/**
  * @brief  检查是否已初始化
  * @retval uint8_t: 1=已初始化, 0=未初始化
  */
uint8_t MQTT_IsInitialized(void)
{
    return mqtt.initialized;
}

/**
  * @brief  查询并更新连接状态
  * @retval MQTT_Status_t
  */
MQTT_Status_t MQTT_QueryConnection(void)
{
    ESP8266_Status_t ret = ESP8266_SendCommand("AT+MQTTCONN?\r\n", "OK", MQTT_DEFAULT_TIMEOUT);
    
    if (ret != ESP8266_OK) {
        return MQTT_ERROR;
    }
    
    /* 解析响应 +MQTTCONN:<LinkID>,<state>,<scheme>,"<host>",<port>,"<path>",<reconnect> */
    char *ptr = strstr(ESP8266_GetResponseBuffer(), "+MQTTCONN:");
    if (ptr) {
        ptr += 10;
        /* 跳过LinkID */
        ptr = strchr(ptr, ',');
        if (ptr) {
            ptr++;
            int state = atoi(ptr);
            mqtt.state = (MQTT_State_t)state;
            mqtt.connected = (state >= 4);
        }
    }
    
    return MQTT_OK;
}

/**
  * @brief  设置连接成功回调
  */
void MQTT_SetOnConnected(void (*callback)(void))
{
    mqtt.onConnected = callback;
}

/**
  * @brief  设置断开连接回调
  */
void MQTT_SetOnDisconnected(void (*callback)(void))
{
    mqtt.onDisconnected = callback;
}

/**
  * @brief  设置消息接收回调
  */
void MQTT_SetOnMessageReceived(void (*callback)(MQTT_Message_t *message))
{
    mqtt.onMessageReceived = callback;
}

/**
  * @brief  设置发布完成回调
  */
void MQTT_SetOnPublishComplete(void (*callback)(const char *topic))
{
    mqtt.onPublishComplete = callback;
}

/**
  * @brief  设置订阅成功回调
  */
void MQTT_SetOnSubscribed(void (*callback)(const char *topic))
{
    mqtt.onSubscribed = callback;
}

/**
  * @brief  设置取消订阅回调
  */
void MQTT_SetOnUnsubscribed(void (*callback)(const char *topic))
{
    mqtt.onUnsubscribed = callback;
}

/**
  * @brief  设置错误回调
  */
void MQTT_SetOnError(void (*callback)(MQTT_Status_t error))
{
    mqtt.onError = callback;
}

/**
  * @brief  解析订阅消息
  * @param  data: 原始数据
  * @retval MQTT_Status_t
  */
static MQTT_Status_t MQTT_ParseSubMessage(const char *data)
{
    /* 解析 +MQTTSUBRECV:<LinkID>,"<topic>",<data_length>,<data> */
    if (!data) return MQTT_ERROR;
    
    MQTT_DebugPrint("[MQTT] Parsing SUBRECV message...\r\n");
    
    char *ptr = strstr(data, "+MQTTSUBRECV:");
    if (!ptr) return MQTT_ERROR;
    
    ptr += 13;  /* 跳过 "+MQTTSUBRECV:" */
    
    MQTT_Message_t msg;
    memset(&msg, 0, sizeof(MQTT_Message_t));
    
    /* 跳过LinkID和逗号 */
    ptr = strchr(ptr, ',');
    if (!ptr) return MQTT_ERROR;
    ptr++;
    
    /* 提取主题 */
    if (*ptr == '"') {
        ptr++;
        char *end = strchr(ptr, '"');
        if (end) {
            int len = end - ptr;
            if (len >= MQTT_TOPIC_MAX_LEN) len = MQTT_TOPIC_MAX_LEN - 1;
            strncpy(msg.topic, ptr, len);
            ptr = end + 1;
        }
    }
    
    /* 跳过逗号,获取长度 */
    ptr = strchr(ptr, ',');
    if (!ptr) return MQTT_ERROR;
    ptr++;
    
    msg.dataLen = atoi(ptr);
    
    /* 跳到数据部分 */
    ptr = strchr(ptr, ',');
    if (!ptr) return MQTT_ERROR;
    ptr++;
    
    /* 复制数据 */
    uint16_t copyLen = msg.dataLen;
    if (copyLen >= MQTT_MESSAGE_MAX_LEN) copyLen = MQTT_MESSAGE_MAX_LEN - 1;
    memcpy(msg.data, ptr, copyLen);
    msg.data[copyLen] = '\0';
    
    MQTT_DebugPrint("[MQTT] Received: topic=%s, len=%d, data=%s\r\n", 
                    msg.topic, msg.dataLen, msg.data);
    
    /* 调用回调 */
    mqtt.receiveCount++;
    if (mqtt.onMessageReceived) {
        MQTT_DebugPrint("[MQTT] Calling onMessageReceived callback\r\n");
        mqtt.onMessageReceived(&msg);
    } else {
        MQTT_DebugPrint("[MQTT] WARNING: onMessageReceived callback is NULL!\r\n");
    }
    
    return MQTT_OK;
}

/**
  * @brief  处理MQTT数据 (在主循环中调用)
  */
void MQTT_ProcessData(void)
{
    if (!mqtt.initialized) return;
    
    /* 优先处理异步接收到的订阅消息 */
    if (mqtt.msgPending) {
        mqtt.msgPending = 0;  /* 清除标志 */
        MQTT_ParseSubMessage((char *)mqtt.msgBuffer);
    }
    
    char *respBuf = ESP8266_GetResponseBuffer();
    
    /* 检查连接状态变化 */
    if (strstr(respBuf, "+MQTTDISCONNECTED:")) {
        mqtt.connected = 0;
        mqtt.state = MQTT_STATE_DISCONNECTED;
        MQTT_DebugPrint("[MQTT] Disconnected event\r\n");
        if (mqtt.onDisconnected) mqtt.onDisconnected();
    }
    
    if (strstr(respBuf, "+MQTTCONNECTED:")) {
        mqtt.connected = 1;
        mqtt.state = MQTT_STATE_CONNECTED;
        MQTT_DebugPrint("[MQTT] Connected event\r\n");
        if (mqtt.onConnected) mqtt.onConnected();
    }
    
    /* 检查订阅消息 (同步方式，作为备用) */
    if (strstr(respBuf, "+MQTTSUBRECV:")) {
        MQTT_ParseSubMessage(respBuf);
    }
}

/**
  * @brief  处理接收到的MQTT消息
  * @param  data: 消息数据
  * @param  len: 数据长度
  */
void MQTT_ProcessMessage(const char *data, uint16_t len)
{
    if (!data || len == 0) return;
    
    /* 检查订阅消息 */
    if (strstr(data, "+MQTTSUBRECV:")) {
        MQTT_ParseSubMessage(data);
    }
}

/**
  * @brief  添加订阅到列表
  */
static void MQTT_AddSubscription(const char *topic, MQTT_QoS_t qos)
{
    /* 检查是否已存在 */
    for (uint8_t i = 0; i < MQTT_MAX_SUBSCRIPTIONS; i++) {
        if (mqtt.subscriptions[i].active && 
            strcmp(mqtt.subscriptions[i].topic, topic) == 0) {
            /* 更新QoS */
            mqtt.subscriptions[i].qos = qos;
            return;
        }
    }
    
    /* 添加新订阅 */
    for (uint8_t i = 0; i < MQTT_MAX_SUBSCRIPTIONS; i++) {
        if (!mqtt.subscriptions[i].active) {
            strncpy(mqtt.subscriptions[i].topic, topic, MQTT_TOPIC_MAX_LEN - 1);
            mqtt.subscriptions[i].qos = qos;
            mqtt.subscriptions[i].active = 1;
            mqtt.subscriptionCount++;
            return;
        }
    }
}

/**
  * @brief  从列表移除订阅
  */
static void MQTT_RemoveSubscription(const char *topic)
{
    for (uint8_t i = 0; i < MQTT_MAX_SUBSCRIPTIONS; i++) {
        if (mqtt.subscriptions[i].active && 
            strcmp(mqtt.subscriptions[i].topic, topic) == 0) {
            memset(&mqtt.subscriptions[i], 0, sizeof(MQTT_Subscription_t));
            mqtt.subscriptionCount--;
            return;
        }
    }
}
