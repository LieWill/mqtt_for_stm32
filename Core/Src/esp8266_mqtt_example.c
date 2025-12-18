/**
  ******************************************************************************
  * @file           : esp8266_mqtt_example.c
  * @brief          : ESP8266 MQTT扩展库使用示例
  * @author         : Antigravity AI
  * @version        : V1.0.0
  ******************************************************************************
  * @attention
  *
  * 本文件包含ESP8266 MQTT库的使用示例
  * 演示如何连接MQTT Broker、订阅主题和发布消息
  *
  * 使用步骤:
  *   1. 在main.c中包含esp8266_mqtt.h
  *   2. 初始化ESP8266和WiFi连接
  *   3. 调用MQTT_Init()初始化MQTT模块
  *   4. 配置并连接到MQTT Broker
  *   5. 订阅感兴趣的主题
  *   6. 发布消息
  *   7. 在主循环中调用MQTT_ProcessData()处理接收消息
  *
  ******************************************************************************
  */

#include "esp8266_mqtt.h"

/* ========================= 示例配置 ========================= */

/* MQTT Broker配置 - 请根据实际情况修改 */
#define MQTT_EXAMPLE_BROKER     "broker.emqx.io"    /* 公共测试Broker */
#define MQTT_EXAMPLE_PORT       1883                 /* 默认MQTT端口 */
#define MQTT_EXAMPLE_CLIENT_ID  "STM32F407_Client"   /* 客户端ID */
#define MQTT_EXAMPLE_USERNAME   ""                   /* 用户名(公共Broker可为空) */
#define MQTT_EXAMPLE_PASSWORD   ""                   /* 密码(公共Broker可为空) */

/* 主题配置 */
#define MQTT_TOPIC_SENSOR_DATA  "stm32/sensor/data"  /* 传感器数据发布主题 */
#define MQTT_TOPIC_CONTROL      "stm32/control"      /* 控制命令订阅主题 */
#define MQTT_TOPIC_STATUS       "stm32/status"       /* 状态主题 */
#define MQTT_TOPIC_LWT          "stm32/lwt"          /* 遗嘱主题 */

/* ========================= 回调函数 ========================= */

/**
  * @brief  MQTT连接成功回调
  */
void OnMQTTConnected(void)
{
    /* 这里可以在连接成功后自动订阅主题 */
    MQTT_DebugPrint("[Example] MQTT Connected! Subscribing topics...\r\n");
    
    /* 订阅控制命令主题 */
    MQTT_Subscribe(MQTT_TOPIC_CONTROL, MQTT_QOS_1);
    
    /* 发布上线状态 */
    MQTT_Publish(MQTT_TOPIC_STATUS, "online", MQTT_QOS_1, 1);
}

/**
  * @brief  MQTT断开连接回调
  */
void OnMQTTDisconnected(void)
{
    MQTT_DebugPrint("[Example] MQTT Disconnected!\r\n");
    
    /* 可以在这里设置状态标志,在主循环中尝试重连 */
}

/**
  * @brief  MQTT消息接收回调
  * @param  message: 接收到的消息
  */
void OnMQTTMessageReceived(MQTT_Message_t *message)
{
    if (!message) return;
    
    MQTT_DebugPrint("[Example] Message received!\r\n");
    MQTT_DebugPrint("  Topic: %s\r\n", message->topic);
    MQTT_DebugPrint("  Data: %s\r\n", message->data);
    MQTT_DebugPrint("  Length: %d\r\n", message->dataLen);
    
    /* 处理控制命令 */
    if (strcmp(message->topic, MQTT_TOPIC_CONTROL) == 0) {
        /* 示例: 解析JSON格式的控制命令 */
        if (strstr((char *)message->data, "\"cmd\":\"led_on\"")) {
            /* 打开LED */
            MQTT_DebugPrint("[Example] Command: LED ON\r\n");
            /* HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET); */
        }
        else if (strstr((char *)message->data, "\"cmd\":\"led_off\"")) {
            /* 关闭LED */
            MQTT_DebugPrint("[Example] Command: LED OFF\r\n");
            /* HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET); */
        }
        else if (strstr((char *)message->data, "\"cmd\":\"get_status\"")) {
            /* 回复状态 */
            MQTT_Publish(MQTT_TOPIC_STATUS, "{\"status\":\"running\"}", MQTT_QOS_0, 0);
        }
    }
}

/**
  * @brief  MQTT发布完成回调
  * @param  topic: 发布的主题
  */
void OnMQTTPublishComplete(const char *topic)
{
    MQTT_DebugPrint("[Example] Published to: %s\r\n", topic);
}

/**
  * @brief  MQTT错误回调
  * @param  error: 错误码
  */
void OnMQTTError(MQTT_Status_t error)
{
    MQTT_DebugPrint("[Example] MQTT Error: %d\r\n", error);
}

/* ========================= 示例函数 ========================= */

/**
  * @brief  MQTT基础示例 - 一站式连接
  * @note   这是最简单的使用方式
  */
void MQTT_Example_BasicConnect(void)
{
    MQTT_Status_t ret;
    
    /* 1. 初始化MQTT模块 (需先确保ESP8266和WiFi已初始化) */
    ret = MQTT_Init();
    if (ret != MQTT_OK) {
        MQTT_DebugPrint("[Example] MQTT Init failed!\r\n");
        return;
    }
    
    /* 2. 设置回调函数 */
    MQTT_SetOnConnected(OnMQTTConnected);
    MQTT_SetOnDisconnected(OnMQTTDisconnected);
    MQTT_SetOnMessageReceived(OnMQTTMessageReceived);
    MQTT_SetOnPublishComplete(OnMQTTPublishComplete);
    MQTT_SetOnError(OnMQTTError);
    
    /* 3. 一站式连接到Broker */
    ret = MQTT_ConnectToBroker(
        MQTT_EXAMPLE_BROKER,
        MQTT_EXAMPLE_PORT,
        MQTT_EXAMPLE_CLIENT_ID,
        MQTT_EXAMPLE_USERNAME,
        MQTT_EXAMPLE_PASSWORD
    );
    
    if (ret == MQTT_OK) {
        MQTT_DebugPrint("[Example] Connected to MQTT Broker!\r\n");
    } else {
        MQTT_DebugPrint("[Example] Failed to connect! Error: %d\r\n", ret);
    }
}

/**
  * @brief  MQTT高级示例 - 分步配置连接
  * @note   这种方式可以进行更详细的配置
  */
void MQTT_Example_AdvancedConnect(void)
{
    MQTT_Status_t ret;
    
    /* 1. 初始化MQTT模块 */
    ret = MQTT_Init();
    if (ret != MQTT_OK) return;
    
    /* 2. 设置回调函数 */
    MQTT_SetOnConnected(OnMQTTConnected);
    MQTT_SetOnDisconnected(OnMQTTDisconnected);
    MQTT_SetOnMessageReceived(OnMQTTMessageReceived);
    
    /* 3. 配置用户参数 */
    MQTT_UserConfig_t userConfig = {
        .scheme = MQTT_SCHEME_TCP,
        .clientId = MQTT_EXAMPLE_CLIENT_ID,
        .username = MQTT_EXAMPLE_USERNAME,
        .password = MQTT_EXAMPLE_PASSWORD,
        .certKeyId = 0,
        .caId = 0,
        .path = ""
    };
    ret = MQTT_SetUserConfig(&userConfig);
    if (ret != MQTT_OK) return;
    
    /* 4. 配置连接参数 (可选) */
    ret = MQTT_SetKeepAlive(60);  /* 60秒心跳 */
    
    /* 5. 设置遗嘱消息 (可选) */
    ret = MQTT_SetLWT(MQTT_TOPIC_LWT, "offline", MQTT_QOS_1, 1);
    
    /* 6. 配置Broker */
    ret = MQTT_SetBroker(MQTT_EXAMPLE_BROKER, MQTT_EXAMPLE_PORT, 1);
    if (ret != MQTT_OK) return;
    
    /* 7. 连接 */
    ret = MQTT_Connect();
    if (ret == MQTT_OK) {
        MQTT_DebugPrint("[Example] Connected!\r\n");
    }
}

/**
  * @brief  发布传感器数据示例
  * @param  temperature: 温度值
  * @param  humidity: 湿度值
  */
void MQTT_Example_PublishSensorData(float temperature, float humidity)
{
    if (!MQTT_IsConnected()) {
        MQTT_DebugPrint("[Example] Not connected, cannot publish!\r\n");
        return;
    }
    
    /* 方式1: 使用格式化发布 */
    MQTT_PublishF(MQTT_TOPIC_SENSOR_DATA, MQTT_QOS_0, 0,
        "{\"temp\":%.1f,\"humi\":%.1f}", temperature, humidity);
    
    /* 方式2: 手动构建消息 */
    /*
    char msg[128];
    snprintf(msg, sizeof(msg), "{\"temp\":%.1f,\"humi\":%.1f}", temperature, humidity);
    MQTT_Publish(MQTT_TOPIC_SENSOR_DATA, msg, MQTT_QOS_0, 0);
    */
}

/**
  * @brief  订阅多个主题示例
  */
void MQTT_Example_SubscribeMultiple(void)
{
    const char *topics[] = {
        "stm32/control",
        "stm32/config",
        "stm32/ota"
    };
    
    MQTT_QoS_t qos[] = {
        MQTT_QOS_1,
        MQTT_QOS_0,
        MQTT_QOS_2
    };
    
    MQTT_SubscribeMultiple(topics, qos, 3);
}

/**
  * @brief  主循环中调用此函数处理MQTT消息
  * @note   需要在while(1)主循环中周期性调用
  */
void MQTT_Example_MainLoop(void)
{
    /* 处理ESP8266数据 */
    ESP8266_ProcessData();
    
    /* 处理MQTT数据 */
    MQTT_ProcessData();
    
    /* 可以在这里添加定时发布逻辑 */
    static uint32_t lastPublishTime = 0;
    if (HAL_GetTick() - lastPublishTime >= 10000) {  /* 每10秒 */
        lastPublishTime = HAL_GetTick();
        
        if (MQTT_IsConnected()) {
            /* 发布心跳或状态 */
            MQTT_Publish(MQTT_TOPIC_STATUS, "heartbeat", MQTT_QOS_0, 0);
        }
    }
    
    /* 检查连接状态,必要时重连 */
    static uint32_t lastCheckTime = 0;
    if (HAL_GetTick() - lastCheckTime >= 30000) {  /* 每30秒检查一次 */
        lastCheckTime = HAL_GetTick();
        
        if (!MQTT_IsConnected() && ESP8266_IsWifiConnected()) {
            MQTT_DebugPrint("[Example] MQTT disconnected, trying reconnect...\r\n");
            MQTT_Reconnect();
        }
    }
}

/* ========================= 完整使用示例 ========================= */

/**
  * @brief  完整的MQTT使用示例
  * @note   这是一个完整的使用流程示例,展示如何集成到main.c中
  * 
  * 在main.c中的使用方式:
  * 
  * ---------------------- main.c ----------------------
  * 
  * #include "esp8266.h"
  * #include "esp8266_mqtt.h"
  * 
  * // WiFi配置
  * #define WIFI_SSID     "YourWiFiSSID"
  * #define WIFI_PASSWORD "YourWiFiPassword"
  * 
  * int main(void)
  * {
  *     // HAL初始化...
  *     HAL_Init();
  *     SystemClock_Config();
  *     MX_GPIO_Init();
  *     MX_DMA_Init();
  *     MX_USART1_UART_Init();  // 调试串口
  *     MX_USART2_UART_Init();  // ESP8266串口
  *     
  *     // 初始化ESP8266
  *     if (ESP8266_Init(&huart2) != ESP8266_OK) {
  *         printf("ESP8266 Init failed!\r\n");
  *         Error_Handler();
  *     }
  *     
  *     // 连接WiFi
  *     if (ESP8266_ConnectAP(WIFI_SSID, WIFI_PASSWORD) != ESP8266_OK) {
  *         printf("WiFi connect failed!\r\n");
  *         Error_Handler();
  *     }
  *     
  *     // 初始化并连接MQTT
  *     MQTT_Example_BasicConnect();
  *     
  *     // 主循环
  *     while (1)
  *     {
  *         // 处理MQTT消息
  *         MQTT_Example_MainLoop();
  *         
  *         // 其他任务...
  *         
  *         HAL_Delay(10);
  *     }
  * }
  * 
  * ---------------------- END ----------------------
  */

/* ========================= 常用Broker列表 ========================= */
/*
 * 公共测试Broker (仅用于测试,不要传输敏感数据):
 *
 * 1. EMQX Public Broker
 *    Host: broker.emqx.io
 *    Port: 1883 (TCP), 8083 (WebSocket)
 *    TLS Port: 8883 (TCP), 8084 (WebSocket)
 *
 * 2. HiveMQ Public Broker
 *    Host: broker.hivemq.com
 *    Port: 1883 (TCP), 8000 (WebSocket)
 *
 * 3. Eclipse Mosquitto Test Server
 *    Host: test.mosquitto.org
 *    Port: 1883 (TCP), 8080 (WebSocket)
 *    TLS Port: 8883, 8081
 *
 * 4. CloudMQTT (需注册)
 *    Host: 根据账户不同
 *    需要用户名和密码
 *
 * 私有Broker搭建推荐:
 * - Mosquitto: 轻量级,适合嵌入式
 * - EMQX: 功能强大,适合企业应用
 * - RabbitMQ + MQTT插件
 */
