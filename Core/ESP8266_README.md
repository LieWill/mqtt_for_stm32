# ESP8266 WiFi模块 STM32 HAL驱动

## 📋 概述

本驱动为ESP8266 WiFi模块提供了完整的STM32 HAL库驱动支持，适用于STM32F4系列微控制器。

## ✨ 功能特性

- ✅ AT指令通信
- ✅ STA模式 (连接WiFi热点)
- ✅ AP模式 (创建WiFi热点)
- ✅ STA+AP混合模式
- ✅ TCP客户端/服务器
- ✅ UDP客户端/服务器
- ✅ HTTP GET/POST请求
- ✅ 透传模式
- ✅ WiFi扫描
- ✅ PING测试
- ✅ 事件回调机制
- ✅ 完善的错误处理

## 📁 文件结构

```
Core/
├── Inc/
│   └── esp8266.h           # 驱动头文件
└── Src/
    ├── esp8266.c           # 驱动源文件
    └── esp8266_example.c   # 使用示例
```

## 🔧 硬件连接

| ESP8266 | STM32F407 | 说明 |
|---------|-----------|------|
| VCC     | 3.3V      | 电源 |
| GND     | GND       | 地   |
| TX      | PB11      | USART3_RX |
| RX      | PB10      | USART3_TX |
| CH_PD   | 3.3V      | 使能(可接GPIO) |
| RST     | 3.3V      | 复位(可接GPIO) |

> ⚠️ **注意**: ESP8266工作电压为3.3V，请勿接5V电源！

## ⚙️ STM32CubeMX配置

### 1. USART3配置
- Mode: Asynchronous
- Baud Rate: 9600 (或115200，需与ESP8266一致)
- Word Length: 8 Bits
- Parity: None
- Stop Bits: 1

### 2. NVIC配置
- **必须使能** USART3 global interrupt
- Priority Group: 按需设置

### 3. 在.ioc文件中添加(如果未配置):
需要在CubeMX中打开NVIC设置，勾选USART3全局中断。

## 📖 快速开始

### 1. 包含头文件

```c
#include "esp8266.h"
```

### 2. 初始化

```c
// 初始化ESP8266
ESP8266_Init(&huart3);

// 连接WiFi
ESP8266_ConnectAP("YourSSID", "YourPassword");
```

### 3. 主循环处理

```c
while (1)
{
    // 处理ESP8266事件
    ESP8266_ProcessData();
    
    // 你的其他代码
}
```

## 📚 API参考

### 初始化和控制

| 函数 | 说明 |
|------|------|
| `ESP8266_Init()` | 初始化ESP8266 |
| `ESP8266_DeInit()` | 反初始化 |
| `ESP8266_Reset()` | 软复位 |
| `ESP8266_Restore()` | 恢复出厂设置 |

### WiFi操作

| 函数 | 说明 |
|------|------|
| `ESP8266_SetWiFiMode()` | 设置WiFi模式 |
| `ESP8266_ConnectAP()` | 连接WiFi热点 |
| `ESP8266_DisconnectAP()` | 断开WiFi |
| `ESP8266_ScanAP()` | 扫描WiFi热点 |
| `ESP8266_SetupAP()` | 设置AP热点 |

### TCP/UDP操作

| 函数 | 说明 |
|------|------|
| `ESP8266_Connect()` | 建立TCP/UDP连接 |
| `ESP8266_Close()` | 关闭连接 |
| `ESP8266_StartServer()` | 启动TCP服务器 |
| `ESP8266_StopServer()` | 停止TCP服务器 |
| `ESP8266_Send()` | 发送数据 |
| `ESP8266_SendString()` | 发送字符串 |

### HTTP操作

| 函数 | 说明 |
|------|------|
| `ESP8266_HttpGet()` | HTTP GET请求 |
| `ESP8266_HttpPost()` | HTTP POST请求 |

### 回调设置

| 函数 | 说明 |
|------|------|
| `ESP8266_SetOnDataReceived()` | 设置数据接收回调 |
| `ESP8266_SetOnWifiConnected()` | 设置WiFi连接回调 |
| `ESP8266_SetOnWifiDisconnected()` | 设置WiFi断开回调 |

## 🔍 使用示例

### TCP客户端

```c
// 连接到TCP服务器
ESP8266_Connect(ESP8266_TCP, "192.168.1.100", 8080, NULL);

// 发送数据
ESP8266_SendString(0, "Hello Server!");

// 关闭连接
ESP8266_Close(0);
```

### TCP服务器

```c
// 开启多连接模式
ESP8266_SetMultiConn(1);

// 启动服务器
ESP8266_StartServer(80);

// 设置回调处理客户端连接和数据
ESP8266_SetOnDataReceived(MyDataCallback);
```

### HTTP GET

```c
char response[512];
ESP8266_HttpGet("api.example.com", 80, "/data", response, sizeof(response));
```

### AP热点模式

```c
ESP8266_SetWiFiMode(ESP8266_MODE_AP);
ESP8266_SetupAP("MyESP8266", "password123", 6, ESP8266_ECN_WPA2_PSK);
```

## ⚡ 配置选项

在 `esp8266.h` 中可以修改以下配置:

```c
#define ESP8266_UART                huart3      // 使用的UART
#define ESP8266_RX_BUF_SIZE         1024        // 接收缓冲区大小
#define ESP8266_TX_BUF_SIZE         512         // 发送缓冲区大小
#define ESP8266_DEFAULT_TIMEOUT     3000        // 默认超时(ms)
```

## 🐛 调试

驱动内置调试输出功能，通过USART1输出调试信息:

```c
// 在esp8266.c中启用/禁用调试
#define ESP8266_DEBUG_ENABLE    1   // 1:开启 0:关闭
```

## ❓ 常见问题

### Q: ESP8266无响应?
A: 检查:
- 硬件连接是否正确
- 波特率是否一致
- CH_PD引脚是否拉高
- 电源是否稳定(ESP8266峰值电流300mA+)

### Q: 连接WiFi失败?
A: 检查:
- SSID和密码是否正确
- 信号强度是否足够
- 是否超时(可增加ESP8266_CONNECT_TIMEOUT)

### Q: 数据发送失败?
A: 检查:
- 连接是否建立成功
- 服务器是否正常运行
- 网络是否正常

---

# 🔌 ESP8266 MQTT扩展库

## � 概述

MQTT扩展库基于ESP8266基础驱动,提供完整的MQTT客户端功能。支持连接MQTT Broker、订阅主题、发布消息等所有MQTT核心操作。

## ✨ MQTT功能特性

- ✅ MQTT用户配置 (Client ID, Username, Password)
- ✅ 支持MQTT over TCP / TLS / WebSocket
- ✅ 连接/断开MQTT Broker
- ✅ 主题订阅/取消订阅 (最多8个主题)
- ✅ 消息发布 (字符串/原始数据)
- ✅ QoS 0/1/2 支持
- ✅ 遗嘱消息(LWT)配置
- ✅ 自动重连
- ✅ 消息接收回调
- ✅ 完善的状态管理

## 📁 MQTT文件结构

```
Core/
├── Inc/
│   ├── esp8266.h              # ESP8266基础驱动头文件
│   └── esp8266_mqtt.h         # MQTT扩展库头文件
└── Src/
    ├── esp8266.c              # ESP8266基础驱动
    ├── esp8266_mqtt.c         # MQTT扩展库实现
    └── esp8266_mqtt_example.c # MQTT使用示例
```

## 📖 MQTT快速开始

### 1. 包含头文件

```c
#include "esp8266.h"
#include "esp8266_mqtt.h"
```

### 2. 初始化并连接

```c
// 先初始化ESP8266并连接WiFi
ESP8266_Init(&huart3);
ESP8266_ConnectAP("YourSSID", "YourPassword");

// 初始化MQTT
MQTT_Init();

// 一站式连接到Broker
MQTT_ConnectToBroker("broker.emqx.io", 1883, "STM32_Client", "", "");
```

### 3. 订阅和发布

```c
// 订阅主题
MQTT_Subscribe("stm32/control", MQTT_QOS_1);

// 发布消息
MQTT_Publish("stm32/data", "{\"temp\":25.5}", MQTT_QOS_0, 0);

// 格式化发布
MQTT_PublishF("stm32/sensor", MQTT_QOS_0, 0, "{\"temp\":%.1f}", temperature);
```

### 4. 消息接收处理

```c
// 设置消息接收回调
void OnMessageReceived(MQTT_Message_t *msg) {
    printf("Topic: %s\n", msg->topic);
    printf("Data: %s\n", msg->data);
}

MQTT_SetOnMessageReceived(OnMessageReceived);

// 主循环中处理
while (1) {
    ESP8266_ProcessData();
    MQTT_ProcessData();
    HAL_Delay(10);
}
```

## 📚 MQTT API参考

### 初始化和配置

| 函数 | 说明 |
|------|------|
| `MQTT_Init()` | 初始化MQTT模块 |
| `MQTT_DeInit()` | 反初始化 |
| `MQTT_SetUserConfigSimple()` | 简化用户配置 |
| `MQTT_SetUserConfig()` | 完整用户配置 |
| `MQTT_SetConnConfig()` | 连接配置 |
| `MQTT_SetKeepAlive()` | 设置心跳间隔 |
| `MQTT_SetLWT()` | 设置遗嘱消息 |
| `MQTT_SetBroker()` | 设置Broker地址 |

### 连接控制

| 函数 | 说明 |
|------|------|
| `MQTT_Connect()` | 连接Broker |
| `MQTT_ConnectToBroker()` | 一站式连接 |
| `MQTT_Disconnect()` | 断开连接 |
| `MQTT_Reconnect()` | 重新连接 |
| `MQTT_Clean()` | 清理资源 |

### 订阅操作

| 函数 | 说明 |
|------|------|
| `MQTT_Subscribe()` | 订阅主题 |
| `MQTT_SubscribeMultiple()` | 订阅多个主题 |
| `MQTT_Unsubscribe()` | 取消订阅 |
| `MQTT_UnsubscribeAll()` | 取消所有订阅 |
| `MQTT_GetSubscriptions()` | 获取订阅列表 |

### 发布操作

| 函数 | 说明 |
|------|------|
| `MQTT_Publish()` | 发布字符串消息 |
| `MQTT_PublishData()` | 发布二进制数据 |
| `MQTT_PublishRaw()` | 发布原始数据 |
| `MQTT_PublishF()` | 格式化发布 |

### 状态查询

| 函数 | 说明 |
|------|------|
| `MQTT_IsConnected()` | 检查连接状态 |
| `MQTT_IsInitialized()` | 检查初始化状态 |
| `MQTT_GetState()` | 获取详细状态 |
| `MQTT_QueryConnection()` | 查询并更新状态 |

### 回调设置

| 函数 | 说明 |
|------|------|
| `MQTT_SetOnConnected()` | 连接成功回调 |
| `MQTT_SetOnDisconnected()` | 断开连接回调 |
| `MQTT_SetOnMessageReceived()` | 消息接收回调 |
| `MQTT_SetOnPublishComplete()` | 发布完成回调 |
| `MQTT_SetOnSubscribed()` | 订阅成功回调 |
| `MQTT_SetOnError()` | 错误回调 |

## 🔍 MQTT使用示例

### 基础连接示例

```c
MQTT_Status_t ret;

// 初始化
MQTT_Init();

// 设置回调
MQTT_SetOnConnected(OnConnected);
MQTT_SetOnMessageReceived(OnMessage);

// 连接到公共Broker
ret = MQTT_ConnectToBroker(
    "broker.emqx.io",   // Broker地址
    1883,               // 端口
    "STM32_Device",     // 客户端ID
    NULL,               // 用户名(可选)
    NULL                // 密码(可选)
);

if (ret == MQTT_OK) {
    MQTT_Subscribe("stm32/cmd", MQTT_QOS_1);
}
```

### 高级配置示例

```c
// 详细配置
MQTT_UserConfig_t userCfg = {
    .scheme = MQTT_SCHEME_TCP,
    .clientId = "STM32_Device_001",
    .username = "user",
    .password = "pass123"
};
MQTT_SetUserConfig(&userCfg);

// 设置遗嘱消息 (设备离线时自动发布)
MQTT_SetLWT("device/status", "offline", MQTT_QOS_1, 1);

// 设置心跳60秒
MQTT_SetKeepAlive(60);

// 配置Broker并连接
MQTT_SetBroker("mqtt.example.com", 1883, 1);
MQTT_Connect();
```

### 传感器数据上报示例

```c
// 定时上报温湿度数据
void UploadSensorData(void) {
    static uint32_t lastTime = 0;
    
    if (HAL_GetTick() - lastTime >= 5000) {  // 每5秒
        lastTime = HAL_GetTick();
        
        if (MQTT_IsConnected()) {
            float temp = ReadTemperature();
            float humi = ReadHumidity();
            
            MQTT_PublishF("sensor/data", MQTT_QOS_0, 0,
                "{\"temp\":%.1f,\"humi\":%.1f,\"ts\":%lu}",
                temp, humi, HAL_GetTick());
        }
    }
}
```

## ⚙️ MQTT配置选项

在 `esp8266_mqtt.h` 中可以修改:

```c
#define MQTT_CLIENT_ID_MAX_LEN   64    // Client ID最大长度
#define MQTT_TOPIC_MAX_LEN       128   // 主题最大长度
#define MQTT_MESSAGE_MAX_LEN     1024  // 消息最大长度
#define MQTT_MAX_SUBSCRIPTIONS   8     // 最大订阅主题数
#define MQTT_CONNECT_TIMEOUT     10000 // 连接超时(ms)
#define MQTT_DEBUG_ENABLE        1     // 调试开关
```

## 🌐 常用MQTT Broker

### 公共测试Broker

| Broker | 地址 | 端口 | 说明 |
|--------|------|------|------|
| EMQX | broker.emqx.io | 1883 | 稳定推荐 |
| HiveMQ | broker.hivemq.com | 1883 | 速度快 |
| Mosquitto | test.mosquitto.org | 1883 | 开源经典 |

> ⚠️ **注意**: 公共Broker仅用于测试,不要传输敏感数据!

## ❓ MQTT常见问题

### Q: MQTT连接失败?
A: 检查:
- WiFi是否已连接
- Broker地址和端口是否正确
- 防火墙是否阻止1883端口
- Client ID是否重复

### Q: 消息发布失败?
A: 检查:
- MQTT是否已连接 (`MQTT_IsConnected()`)
- 主题和消息格式是否正确
- 消息长度是否超限

### Q: 收不到订阅消息?
A: 检查:
- 订阅是否成功
- 主循环中是否调用 `MQTT_ProcessData()`
- 回调函数是否正确设置

---

## �📝 版本历史

- **V2.0.0** - MQTT扩展版本
  - 新增MQTT客户端支持
  - 支持订阅/发布
  - 消息回调机制
  - 遗嘱消息支持
  - 自动重连

- **V1.0.0** - 初始版本
  - 基本AT指令支持
  - WiFi STA/AP模式
  - TCP/UDP客户端和服务器
  - HTTP GET/POST
  - 回调机制

## 📄 许可证

本项目采用 MIT 许可证。

---
*由 Antigravity AI 生成*
