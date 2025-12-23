# STM32F407 MQTT IoT 物联网项目

![STM32](https://img.shields.io/badge/MCU-STM32F407-blue)
![MQTT](https://img.shields.io/badge/Protocol-MQTT-green)
![HAL](https://img.shields.io/badge/Library-HAL-orange)
![License](https://img.shields.io/badge/License-MIT-yellow)

基于 **STM32F407** 微控制器的 MQTT 物联网传感器数据采集与远程控制系统。通过 ESP8266 WiFi 模块实现网络连接，支持温湿度、光照传感器数据上报以及远程 LED 和蜂鸣器控制。

---

## ✨ 功能特性

### 🌡️ 传感器数据采集
- **DHT11 温湿度传感器**: 读取环境温度 (0-50°C) 和湿度 (20-90%RH)
- **光敏传感器**: 通过 ADC 采集光照强度 (12位分辨率)

### 📡 网络通信
- **ESP8266 WiFi 模块**: 支持 Station/AP/混合模式
- **MQTT 协议**: 支持 QoS 0/1/2，消息发布/订阅
- **DMA + IDLE 中断**: 高效的串口数据收发

### 🎮 远程控制
- **4路 LED 控制**: LED1~LED4 独立控制
- **蜂鸣器控制**: BEEP 开关控制
- **JSON 格式命令**: 支持组合控制命令

### 📝 日志系统
- **多级别日志**: ERROR/WARN/INFO/DEBUG/VERBOSE
- **彩色输出**: 支持 ANSI 终端颜色
- **时间戳**: 可选时间戳输出

---

## 📁 项目结构

```
.
├── Core/
│   ├── Inc/                    # 头文件目录
│   │   ├── main.h              # 主程序头文件
│   │   ├── esp8266.h           # ESP8266 WiFi 驱动
│   │   ├── esp8266_mqtt.h      # ESP8266 MQTT 扩展库
│   │   ├── dht11.h             # DHT11 温湿度传感器驱动
│   │   ├── light_sensor.h      # 光敏传感器驱动
│   │   ├── log.h               # 统一日志库
│   │   └── ...
│   └── Src/                    # 源文件目录
│       ├── main.c              # 主程序入口
│       ├── esp8266.c           # ESP8266 驱动实现
│       ├── esp8266_mqtt.c      # MQTT 功能实现
│       ├── dht11.c             # DHT11 驱动实现
│       ├── light_sensor.c      # 光敏传感器驱动实现
│       ├── log.c               # 日志库实现
│       ├── *_example.c         # 各模块使用示例
│       └── ...
├── Drivers/                    # STM32 HAL 驱动库
├── MDK-ARM/                    # Keil MDK 工程文件
├── two.ioc                     # STM32CubeMX 配置文件
└── README.md                   # 项目说明文档
```

---

## 🔧 硬件需求

### 主控板
- **STM32F407** 开发板 (如 STM32F407ZGT6)
- 系统时钟: 168MHz

### 传感器模块
| 模块 | 引脚连接 | 说明 |
|------|----------|------|
| DHT11 | PG9 | 温湿度传感器，DATA 引脚 |
| 光敏传感器 | PF7 (ADC3_CH5) | ADC 模拟输入 |

### 通信模块
| 模块 | 引脚连接 | 说明 |
|------|----------|------|
| ESP8266 | USART3 (TX/RX) | WiFi 模块 |
| 调试串口 | USART1 | 日志输出 |

### 执行器
| 设备 | 引脚 | 说明 |
|------|------|------|
| LED1 | PF9 | 状态指示灯 1 |
| LED2 | PF10 | 状态指示灯 2 |
| LED3 | PE13 | 状态指示灯 3 |
| LED4 | PE14 | 状态指示灯 4 |
| BEEP | PF8 | 蜂鸣器 |

---

## 🛠️ 开发环境

- **IDE**: Keil MDK-ARM 5.x
- **配置工具**: STM32CubeMX
- **HAL库版本**: STM32Cube FW_F4 V1.x
- **编译器**: ARM Compiler 6 或 ARM Compiler 5

---

## 🚀 快速开始

### 1. 克隆项目

```bash
git clone https://github.com/LieWill/mqtt_for_stm32.git
cd mqtt_for_stm32
```

### 2. 配置 WiFi 和 MQTT

编辑 `Core/Src/main.c` 中的配置参数：

```c
/* WiFi 配置 */
ESP8266_ConnectAP("YourWiFiSSID", "YourWiFiPassword");

/* MQTT Broker 配置 */
#define MQTT_EXAMPLE_BROKER     "your.mqtt.broker.com"
#define MQTT_EXAMPLE_PORT       1883
#define MQTT_EXAMPLE_CLIENT_ID  "STM32F407_Client"
#define MQTT_EXAMPLE_USERNAME   "username"      // 可选
#define MQTT_EXAMPLE_PASSWORD   "password"      // 可选

/* 主题配置 */
#define MQTT_TOPIC_SENSOR_DATA  "stm32/sensor/data"
#define MQTT_TOPIC_CONTROL      "stm32/control"
```

### 3. 编译与烧录

1. 使用 Keil MDK 打开 `MDK-ARM/` 目录下的工程文件
2. 编译项目 (F7)
3. 连接 ST-Link 烧录器
4. 下载固件到目标板 (F8)

### 4. 运行与测试

连接串口调试助手 (波特率 115200) 查看日志输出：

```
[I][MAIN] System starting...
[I][MAIN] DHT11 initialized
[I][MAIN] LightSensor initialized
[I][ESP8266] WiFi connected!
[I][ESP8266] IP: 192.168.1.100
[I][MQTT] Connected to broker!
[I][MQTT] Subscribed to stm32/control
```

---

## 📊 MQTT 消息格式

### 传感器数据上报

**主题**: `stm32/sensor/data`

**格式**:
```json
{
    "temp": 25.5,
    "humi": 60.0,
    "light": 2048
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `temp` | float | 温度 (°C) |
| `humi` | float | 湿度 (%RH) |
| `light` | int | 光照强度 (0-4095，越大越亮) |

### 控制命令下发

**主题**: `stm32/control`

**格式示例**:
```json
// 单独控制
{"led1": true}
{"led2": false}
{"beep": true}

// 组合控制
{"led1": true, "led2": false, "led3": true, "led4": false, "beep": false}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `led1` | bool | LED1 状态 (true=亮, false=灭) |
| `led2` | bool | LED2 状态 |
| `led3` | bool | LED3 状态 |
| `led4` | bool | LED4 状态 |
| `beep` | bool | 蜂鸣器状态 (true=响, false=停) |

---

## 📚 驱动模块说明

### ESP8266 WiFi 驱动

提供完整的 ESP8266 AT 指令封装，支持：
- WiFi Station/AP 模式配置
- TCP/UDP 连接管理
- DMA 收发 + IDLE 中断
- 回调事件机制

```c
// 初始化
ESP8266_Init(&huart3);

// 设置模式并连接
ESP8266_SetWiFiMode(ESP8266_MODE_STA);
ESP8266_ConnectAP("SSID", "Password");

// 获取 IP
ESP8266_IPInfo_t ipInfo;
ESP8266_GetIPInfo(&ipInfo);
```

### ESP8266 MQTT 扩展库

基于 ESP8266 AT 固件的 MQTT 功能封装：
- 连接/断开 MQTT Broker
- 发布/订阅主题
- QoS 0/1/2 支持
- 遗嘱消息配置

```c
// 初始化
MQTT_Init();

// 配置并连接
MQTT_SetUserConfig(&userConfig);
MQTT_SetBroker("broker.mqtt.com", 1883, 1);
MQTT_Connect();

// 订阅与发布
MQTT_Subscribe("topic", MQTT_QOS_1);
MQTT_Publish("topic", "message", MQTT_QOS_0, 0);
```

### DHT11 温湿度传感器驱动

单总线协议实现，特性：
- 精确微秒延时 (DWT 计数器)
- 数据校验和验证
- 最小采样间隔保护 (1秒)

```c
// 初始化
DHT11_Init();

// 读取数据
float temp, humi;
DHT11_Read(&temp, &humi);
```

### 光敏传感器驱动

ADC 轮询模式读取：
- 12位分辨率 (0-4095)
- 光照等级判断
- 电压值转换

```c
// 初始化
LightSensor_Init();

// 读取 ADC 值
uint16_t value = LightSensor_GetValue();

// 获取光照等级
LightSensor_LightLevel_t level = LightSensor_GetLightLevel();
```

### 统一日志库

标准化调试输出接口：
```c
// 初始化
LOG_Init(&huart1);

// 使用日志宏
LOG_E("TAG", "Error: %d", errorCode);   // 错误 (红色)
LOG_W("TAG", "Warning message");         // 警告 (黄色)
LOG_I("TAG", "Info message");            // 信息 (绿色)
LOG_D("TAG", "Debug message");           // 调试 (青色)
LOG_V("TAG", "Verbose message");         // 详细 (白色)
```

---

## ⚙️ 配置选项

### 日志级别配置

编辑 `Core/Inc/log.h`：

```c
#define LOG_ENABLE              1               // 全局开关
#define LOG_LEVEL               LOG_LEVEL_DEBUG // 日志级别
#define LOG_COLOR_ENABLE        1               // 颜色输出
#define LOG_TIMESTAMP_ENABLE    1               // 时间戳
```

### 传感器调试开关

```c
// dht11.h
#define DHT11_DEBUG_ENABLE      0   // DHT11 调试输出

// esp8266.h
#define ESP8266_DEBUG_ENABLE    1   // ESP8266 调试输出

// esp8266_mqtt.h
#define MQTT_DEBUG_ENABLE       1   // MQTT 调试输出

// light_sensor.h
#define LIGHT_SENSOR_DEBUG_ENABLE 0 // 光敏传感器调试输出
```

---

## 🔌 引脚分配总览

| 功能 | 引脚 | 外设 | 备注 |
|------|------|------|------|
| ESP8266 TX | - | USART3_RX | WiFi 模块发送 |
| ESP8266 RX | - | USART3_TX | WiFi 模块接收 |
| 调试串口 TX | - | USART1_TX | 日志输出 |
| 调试串口 RX | - | USART1_RX | 命令输入 |
| DHT11 DATA | PG9 | GPIO | 温湿度传感器 |
| 光敏传感器 | PF7 | ADC3_CH5 | 模拟输入 |
| LED1 | PF9 | GPIO | 输出 |
| LED2 | PF10 | GPIO | 输出 |
| LED3 | PE13 | GPIO | 输出 |
| LED4 | PE14 | GPIO | 输出 |
| BEEP | PF8 | GPIO | 蜂鸣器输出 |

---

## 📈 性能参数

| 参数 | 值 |
|------|-----|
| 系统时钟 | 168 MHz |
| 传感器采样周期 | 5 秒 |
| MQTT 心跳间隔 | 120 秒 |
| ESP8266 通信波特率 | 115200 bps |
| 调试串口波特率 | 115200 bps |
| DMA 接收缓冲区 | 2048 字节 |

---

## 🐛 故障排除

### 常见问题

1. **WiFi 连接失败**
   - 检查 SSID 和密码是否正确
   - 确认 ESP8266 AT 固件版本 (推荐 V2.0.0+)
   - 检查串口连接和波特率

2. **MQTT 连接失败**
   - 验证 Broker 地址和端口
   - 检查网络防火墙设置
   - 确认 Client ID 唯一性

3. **DHT11 读取失败**
   - 确保采样间隔 >= 1 秒
   - 检查 DATA 引脚连接
   - 验证电源电压 (3.3V/5V)

4. **日志无输出**
   - 检查 `LOG_ENABLE` 宏定义
   - 确认 USART1 初始化正确
   - 验证串口波特率设置

---

## 📜 版本历史

| 版本 | 日期 | 更新内容 |
|------|------|----------|
| V1.0.0 | 2025-12-23 | 初始版本：基础传感器采集与 MQTT 通信 |

---

## 📄 许可证

本项目采用 **MIT License** 开源许可证。详见 [LICENSE](LICENSE) 文件。

---

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

---

## 📧 联系方式

- **作者**: LieWill
- **GitHub**: [https://github.com/LieWill](https://github.com/LieWill)

---

<p align="center">
  <b>⭐ 如果这个项目对你有帮助，请给个 Star 支持一下！ ⭐</b>
</p>
