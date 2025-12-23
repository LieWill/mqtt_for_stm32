# 统一日志库 (Log Library)

## 简介

本日志库为STM32项目提供统一的调试输出接口，支持多种日志级别、时间戳、颜色输出等功能。所有驱动模块（ESP8266、MQTT、DHT11、光敏传感器等）均使用本库进行调试输出。

## 特性

- **多日志级别**: ERROR, WARN, INFO, DEBUG, VERBOSE
- **时间戳输出**: 可选显示毫秒时间戳
- **颜色输出**: 支持ANSI终端颜色（可选）
- **模块标签**: 每条日志带模块标识
- **全局控制**: 可运行时开关日志
- **统一接口**: 替代各驱动独立的调试函数

## 文件结构

```
Core/
├── Inc/
│   └── log.h          # 日志库头文件
└── Src/
    └── log.c          # 日志库实现
```

## 使用方法

### 1. 初始化

在 `main.c` 中初始化日志库：

```c
#include "log.h"

int main(void)
{
    // HAL和外设初始化...
    MX_USART1_UART_Init();
    
    // 初始化日志库
    LOG_Init(&huart1);
    
    // 其他代码...
}
```

### 2. 输出日志

使用日志宏输出不同级别的日志：

```c
// 错误日志 (红色)
LOG_E("MQTT", "Connection failed: %d", errorCode);

// 警告日志 (黄色)
LOG_W("ESP8266", "WiFi signal weak: %d dBm", rssi);

// 信息日志 (绿色)
LOG_I("MAIN", "System initialized successfully");

// 调试日志 (青色)
LOG_D("DHT11", "Read temperature: %.1f C", temp);

// 详细日志 (白色)
LOG_V("SENSOR", "Raw ADC value: %d", adcValue);
```

### 3. 输出格式

日志输出格式：
```
[时间戳] [级别][标签] 消息内容
```

示例：
```
[1234] [I][MAIN] System starting...
[1235] [D][ESP8266] AT command sent: AT+CWJAP
[5678] [E][MQTT] Connection timeout
```

## 配置选项

在 `log.h` 中可修改以下配置：

| 宏定义 | 默认值 | 描述 |
|--------|--------|------|
| `LOG_ENABLE` | 1 | 全局日志开关 |
| `LOG_LEVEL` | `LOG_LEVEL_DEBUG` | 编译时日志级别 |
| `LOG_COLOR_ENABLE` | 1 | ANSI颜色输出 |
| `LOG_TIMESTAMP_ENABLE` | 1 | 时间戳输出 |
| `LOG_NEWLINE_AUTO` | 1 | 自动换行 |
| `LOG_BUFFER_SIZE` | 256 | 日志缓冲区大小 |

## 日志级别

| 级别 | 宏 | 描述 |
|------|-----|------|
| 0 | `LOG_LEVEL_NONE` | 关闭所有日志 |
| 1 | `LOG_LEVEL_ERROR` | 仅错误 |
| 2 | `LOG_LEVEL_WARN` | 错误 + 警告 |
| 3 | `LOG_LEVEL_INFO` | 错误 + 警告 + 信息 |
| 4 | `LOG_LEVEL_DEBUG` | 错误 + 警告 + 信息 + 调试 |
| 5 | `LOG_LEVEL_VERBOSE` | 所有日志 |

## 运行时控制

```c
// 禁用日志
LOG_SetEnable(0);

// 启用日志
LOG_SetEnable(1);

// 修改日志级别
LOG_SetLevel(LOG_LEVEL_INFO);

// 获取当前级别
uint8_t level = LOG_GetLevel();
```

## 兼容性说明

各驱动模块仍保留原有的 `XXX_DebugPrint()` 函数接口，但内部已改为调用统一日志库：

- `ESP8266_DebugPrint()` → `LOG_Raw()`
- `MQTT_DebugPrint()` → `LOG_Raw()`
- `DHT11_DebugPrint()` → `LOG_Raw()`
- `LightSensor_DebugPrint()` → `LOG_Raw()`

这确保了向后兼容性，同时启用了统一的日志输出。

## 注意事项

1. **串口阻塞**: 日志使用阻塞式UART发送，高频日志可能影响实时性
2. **Flash占用**: 启用所有日志级别会增加代码体积
3. **调试模式**: 生产版本建议设置 `LOG_LEVEL` 为 `LOG_LEVEL_ERROR` 或 `LOG_LEVEL_NONE`

## 示例

```c
#include "log.h"

void Example_Function(void)
{
    LOG_I("EXAMPLE", "Function started");
    
    int result = SomeOperation();
    if (result < 0) {
        LOG_E("EXAMPLE", "Operation failed: %d", result);
        return;
    }
    
    LOG_D("EXAMPLE", "Operation result: %d", result);
    LOG_I("EXAMPLE", "Function completed");
}
```

## 版本历史

- V1.0.0 (2025-12-19): 初始版本
  - 多级别日志支持
  - 时间戳和颜色输出
  - 运行时控制接口
