/**
  ******************************************************************************
  * @file           : esp8266_example.c
  * @brief          : ESP8266驱动使用示例
  * @author         : Antigravity AI
  * @version        : V1.0.0
  ******************************************************************************
  * @attention
  *
  * 本文件提供ESP8266驱动的使用示例代码
  * 包括WiFi连接、TCP客户端、TCP服务器、HTTP请求等功能演示
  *
  * 使用前请确保:
  * 1. 在STM32CubeMX中使能USART3的NVIC中断
  * 2. ESP8266模块正确连接到USART3 (PB10-TX, PB11-RX)
  * 3. ESP8266波特率设置为9600 (或修改esp8266.h中的配置)
  *
  ******************************************************************************
  */

/* 
================================================================================
                            使用说明
================================================================================

1. 硬件连接:
   ESP8266     STM32F407
   ---------   ---------
   VCC    ->   3.3V
   GND    ->   GND
   TX     ->   PB11 (USART3_RX)
   RX     ->   PB10 (USART3_TX)
   CH_PD  ->   3.3V (或通过GPIO控制)
   RST    ->   3.3V (或通过GPIO控制)

2. STM32CubeMX配置:
   - USART3: 异步模式, 9600波特率 (或115200, 需修改ESP8266模块默认波特率)
   - NVIC: 使能USART3全局中断

3. 在main.c中使用:
   - 包含头文件: #include "esp8266.h"
   - 在main函数中调用初始化和操作函数

================================================================================
*/

#include "esp8266.h"

/* 配置信息 - 请根据实际情况修改 */
#define WIFI_SSID       "YourWiFiSSID"      /* WiFi名称 */
#define WIFI_PASSWORD   "YourWiFiPassword"   /* WiFi密码 */

#define TCP_SERVER_HOST "192.168.1.100"      /* TCP服务器地址 */
#define TCP_SERVER_PORT 8080                  /* TCP服务器端口 */

#define LOCAL_SERVER_PORT 80                  /* 本地服务器端口 */

/* 回调函数声明 */
static void OnDataReceived(ESP8266_RxData_t *data);
static void OnWifiConnected(void);
static void OnWifiDisconnected(void);
static void OnClientConnected(uint8_t linkId);
static void OnClientDisconnected(uint8_t linkId);

/* ============================================================================
 * 示例1: 基本初始化和WiFi连接
 * ============================================================================ */
void Example_BasicInit(void)
{
    ESP8266_Status_t status;
    
    /* 初始化ESP8266 */
    //status = ESP8266_Init(&huart3);
    if (status != ESP8266_OK) {
        ESP8266_DebugPrint("ESP8266 init failed!\r\n");
        return;
    }
    
    /* 设置回调函数 */
    ESP8266_SetOnDataReceived(OnDataReceived);
    ESP8266_SetOnWifiConnected(OnWifiConnected);
    ESP8266_SetOnWifiDisconnected(OnWifiDisconnected);
    
    /* 设置WiFi模式为Station */
    ESP8266_SetWiFiMode(ESP8266_MODE_STA);
    
    /* 连接WiFi */
    status = ESP8266_ConnectAP(WIFI_SSID, WIFI_PASSWORD);
    if (status == ESP8266_OK) {
        ESP8266_DebugPrint("WiFi connected!\r\n");
        
        /* 获取IP地址 */
        ESP8266_IPInfo_t ipInfo;
        ESP8266_GetIPInfo(&ipInfo);
        ESP8266_DebugPrint("IP: %s\r\n", ipInfo.ip);
    } else {
        ESP8266_DebugPrint("WiFi connection failed!\r\n");
    }
}

/* ============================================================================
 * 示例2: TCP客户端 - 连接服务器并发送数据
 * ============================================================================ */
void Example_TCPClient(void)
{
    ESP8266_Status_t status;
    
    /* 确保WiFi已连接 */
    if (!ESP8266_IsWifiConnected()) {
        ESP8266_DebugPrint("WiFi not connected!\r\n");
        return;
    }
    
    /* 建立TCP连接 */
    status = ESP8266_Connect(ESP8266_TCP, TCP_SERVER_HOST, TCP_SERVER_PORT, NULL);
    if (status != ESP8266_OK) {
        ESP8266_DebugPrint("TCP connection failed!\r\n");
        return;
    }
    
    /* 发送数据 */
    ESP8266_SendString(0, "Hello from STM32!\r\n");
    
    /* 格式化发送 */
    ESP8266_SendPrintf(0, "Temperature: %.2f\r\n", 25.5);
    
    /* 发送原始数据 */
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    ESP8266_Send(0, data, sizeof(data));
    
    /* 关闭连接 */
    HAL_Delay(1000);
    ESP8266_Close(0);
}

/* ============================================================================
 * 示例3: TCP服务器 - 等待客户端连接
 * ============================================================================ */
void Example_TCPServer(void)
{
    ESP8266_Status_t status;
    
    /* 确保WiFi已连接 */
    if (!ESP8266_IsWifiConnected()) {
        ESP8266_DebugPrint("WiFi not connected!\r\n");
        return;
    }
    
    /* 设置回调函数 */
    ESP8266_SetOnClientConnected(OnClientConnected);
    ESP8266_SetOnClientDisconnected(OnClientDisconnected);
    ESP8266_SetOnDataReceived(OnDataReceived);
    
    /* 设置多连接模式 (服务器必须) */
    ESP8266_SetMultiConn(1);
    
    /* 启动TCP服务器 */
    status = ESP8266_StartServer(LOCAL_SERVER_PORT);
    if (status != ESP8266_OK) {
        ESP8266_DebugPrint("Server start failed!\r\n");
        return;
    }
    
    ESP8266_DebugPrint("Server started on port %d\r\n", LOCAL_SERVER_PORT);
    
    /* 设置服务器超时 (0-7200秒, 0表示永不超时) */
    ESP8266_SetServerTimeout(180);
}

/* ============================================================================
 * 示例4: HTTP GET请求
 * ============================================================================ */
void Example_HttpGet(void)
{
    char response[512];
    ESP8266_Status_t status;
    
    /* 确保WiFi已连接 */
    if (!ESP8266_IsWifiConnected()) {
        ESP8266_DebugPrint("WiFi not connected!\r\n");
        return;
    }
    
    /* 发送HTTP GET请求 */
    status = ESP8266_HttpGet("httpbin.org", 80, "/get", response, sizeof(response));
    
    if (status == ESP8266_OK) {
        ESP8266_DebugPrint("Response:\r\n%s\r\n", response);
    } else {
        ESP8266_DebugPrint("HTTP GET failed!\r\n");
    }
}

/* ============================================================================
 * 示例5: HTTP POST请求
 * ============================================================================ */
void Example_HttpPost(void)
{
    char response[512];
    ESP8266_Status_t status;
    
    /* 确保WiFi已连接 */
    if (!ESP8266_IsWifiConnected()) {
        ESP8266_DebugPrint("WiFi not connected!\r\n");
        return;
    }
    
    /* 准备POST数据 */
    const char *body = "{\"name\":\"STM32\",\"value\":123}";
    
    /* 发送HTTP POST请求 */
    status = ESP8266_HttpPost("httpbin.org", 80, "/post",
                               "application/json", body,
                               response, sizeof(response));
    
    if (status == ESP8266_OK) {
        ESP8266_DebugPrint("Response:\r\n%s\r\n", response);
    } else {
        ESP8266_DebugPrint("HTTP POST failed!\r\n");
    }
}

/* ============================================================================
 * 示例6: 扫描WiFi热点
 * ============================================================================ */
void Example_ScanAP(void)
{
    ESP8266_APInfo_t apList[10];
    uint8_t foundCount;
    
    ESP8266_Status_t status = ESP8266_ScanAP(apList, 10, &foundCount);
    
    if (status == ESP8266_OK) {
        ESP8266_DebugPrint("Found %d access points:\r\n", foundCount);
        
        for (uint8_t i = 0; i < foundCount; i++) {
            ESP8266_DebugPrint("%d. SSID: %s, RSSI: %d, CH: %d\r\n",
                               i + 1, apList[i].ssid, apList[i].rssi, apList[i].channel);
        }
    } else {
        ESP8266_DebugPrint("Scan failed!\r\n");
    }
}

/* ============================================================================
 * 示例7: 设置为AP模式 (热点模式)
 * ============================================================================ */
void Example_APMode(void)
{
    ESP8266_Status_t status;
    
    /* 设置WiFi模式为AP */
    ESP8266_SetWiFiMode(ESP8266_MODE_AP);
    
    /* 设置AP参数 */
    status = ESP8266_SetupAP("ESP8266_AP", "12345678", 6, ESP8266_ECN_WPA2_PSK);
    
    if (status == ESP8266_OK) {
        ESP8266_DebugPrint("AP mode configured!\r\n");
        ESP8266_DebugPrint("SSID: ESP8266_AP\r\n");
        ESP8266_DebugPrint("Password: 12345678\r\n");
        
        /* 启动服务器接受连接 */
        ESP8266_SetMultiConn(1);
        ESP8266_StartServer(80);
    } else {
        ESP8266_DebugPrint("AP setup failed!\r\n");
    }
}

/* ============================================================================
 * 示例8: 透传模式
 * ============================================================================ */
void Example_TransparentMode(void)
{
    ESP8266_Status_t status;
    
    /* 确保WiFi已连接 */
    if (!ESP8266_IsWifiConnected()) {
        ESP8266_DebugPrint("WiFi not connected!\r\n");
        return;
    }
    
    /* 必须关闭多连接模式 */
    ESP8266_SetMultiConn(0);
    
    /* 建立TCP连接 */
    status = ESP8266_Connect(ESP8266_TCP, TCP_SERVER_HOST, TCP_SERVER_PORT, NULL);
    if (status != ESP8266_OK) {
        ESP8266_DebugPrint("TCP connection failed!\r\n");
        return;
    }
    
    /* 进入透传模式 */
    status = ESP8266_EnterTransparent();
    if (status != ESP8266_OK) {
        ESP8266_DebugPrint("Enter transparent mode failed!\r\n");
        return;
    }
    
    ESP8266_DebugPrint("Entered transparent mode\r\n");
    
    /* 在透传模式下直接发送数据 */
    ESP8266_TransparentSend((uint8_t *)"Hello", 5);
    
    HAL_Delay(5000);
    
    /* 退出透传模式 */
    ESP8266_ExitTransparent();
    ESP8266_DebugPrint("Exited transparent mode\r\n");
}

/* ============================================================================
 * 示例9: PING测试
 * ============================================================================ */
void Example_Ping(void)
{
    ESP8266_Status_t status;
    
    /* 确保WiFi已连接 */
    if (!ESP8266_IsWifiConnected()) {
        ESP8266_DebugPrint("WiFi not connected!\r\n");
        return;
    }
    
    /* Ping测试 */
    status = ESP8266_Ping("www.baidu.com");
    
    if (status == ESP8266_OK) {
        ESP8266_DebugPrint("Ping successful!\r\n");
    } else {
        ESP8266_DebugPrint("Ping failed!\r\n");
    }
}

/* ============================================================================
 * 回调函数实现
 * ============================================================================ */

/**
  * @brief  数据接收回调
  */
static void OnDataReceived(ESP8266_RxData_t *data)
{
    ESP8266_DebugPrint("Received from link %d, len %d: %s\r\n",
                       data->linkId, data->length, data->data);
    
    /* 在这里处理接收到的数据 */
    /* 例如: 回复客户端 */
    ESP8266_SendPrintf(data->linkId, "Received: %s\r\n", data->data);
}

/**
  * @brief  WiFi连接成功回调
  */
static void OnWifiConnected(void)
{
    ESP8266_DebugPrint("WiFi Connected!\r\n");
    
    /* WiFi连接后可以进行的操作 */
    /* 例如: 获取IP地址, 连接服务器等 */
}

/**
  * @brief  WiFi断开回调
  */
static void OnWifiDisconnected(void)
{
    ESP8266_DebugPrint("WiFi Disconnected!\r\n");
    
    /* WiFi断开后可以尝试重连 */
    /* ESP8266_ConnectAP(WIFI_SSID, WIFI_PASSWORD); */
}

/**
  * @brief  客户端连接回调 (服务器模式)
  */
static void OnClientConnected(uint8_t linkId)
{
    ESP8266_DebugPrint("Client %d connected!\r\n", linkId);
    
    /* 向新连接的客户端发送欢迎消息 */
    ESP8266_SendPrintf(linkId, "Welcome to STM32 Server!\r\n");
}

/**
  * @brief  客户端断开回调 (服务器模式)
  */
static void OnClientDisconnected(uint8_t linkId)
{
    ESP8266_DebugPrint("Client %d disconnected!\r\n", linkId);
}

/* ============================================================================
 * 主循环处理函数
 * ============================================================================ */

/**
  * @brief  ESP8266 主循环处理
  * @note   需要在main.c的while(1)循环中周期性调用
  */
void ESP8266_MainLoop(void)
{
    /* 处理接收到的数据和事件 */
    ESP8266_ProcessData();
    
    /* 可以添加其他周期性任务 */
    /* 例如: 定时发送心跳包, 检查连接状态等 */
}

/* ============================================================================
 * 在main.c中的使用示例
 * ============================================================================
 
在main.c中的使用方式:

#include "esp8266.h"

// 在 USER CODE BEGIN Includes 区域添加
#include "esp8266.h"

// 在 main() 函数中:
int main(void)
{
    // ... 初始化代码 ...
    
    // USER CODE BEGIN 2
    
    // 初始化ESP8266
    ESP8266_Init(&huart3);
    
    // 连接WiFi
    ESP8266_ConnectAP("YourSSID", "YourPassword");
    
    // USER CODE END 2
    
    while (1)
    {
        // USER CODE BEGIN WHILE
        
        // 处理ESP8266事件
        ESP8266_ProcessData();
        
        // 你的其他代码
        
        // USER CODE END WHILE
    }
}

================================================================================
*/
