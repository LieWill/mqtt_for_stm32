# DHT11 温湿度传感器驱动库

## 概述

这是一个用于STM32F407的DHT11温湿度传感器HAL库驱动，使用DWT周期计数器实现精确的微秒级延时。

## 特性

- ✅ 精确的微秒延时（使用DWT周期计数器）
- ✅ 完整的DHT11通信时序实现
- ✅ 8位校验和数据验证
- ✅ 错误检测和处理
- ✅ 采样间隔保护（防止过快读取）
- ✅ 支持自定义GPIO引脚
- ✅ 调试输出支持

## 硬件连接

| DHT11引脚 | STM32引脚 |
|-----------|-----------|
| VCC       | 3.3V/5V   |
| DATA      | PG9       |
| GND       | GND       |

> 注意：已在CubeMX中将PG9配置为GPIO输出模式

## DHT11 规格

| 参数 | 范围 | 精度 |
|------|------|------|
| 温度 | 0-50°C | ±2°C |
| 湿度 | 20-90%RH | ±5%RH |
| 采样周期 | ≥1秒 | - |

## 快速开始

### 1. 包含头文件

在 `main.c` 中添加：

```c
/* USER CODE BEGIN Includes */
#include "dht11.h"
/* USER CODE END Includes */
```

### 2. 初始化传感器

在 `main()` 函数中初始化：

```c
/* USER CODE BEGIN 2 */
DHT11_Init();
/* USER CODE END 2 */
```

### 3. 读取温湿度数据

在主循环中读取数据：

```c
/* USER CODE BEGIN WHILE */
while (1) {
    float temperature, humidity;
    
    if (DHT11_Read(&temperature, &humidity) == DHT11_OK) {
        // 使用温度和湿度数据
    }
    
    HAL_Delay(2000);  // 每2秒读取一次
    /* USER CODE END WHILE */
}
```

## API 参考

### 初始化函数

#### `DHT11_Init()`
使用默认引脚(PG9)初始化DHT11传感器。

```c
DHT11_Status_t DHT11_Init(void);
```

#### `DHT11_InitEx()`
使用自定义引脚初始化DHT11传感器。

```c
DHT11_Status_t DHT11_InitEx(GPIO_TypeDef *port, uint16_t pin);
```

### 数据读取函数

#### `DHT11_Read()`
读取温湿度数据。

```c
DHT11_Status_t DHT11_Read(float *temperature, float *humidity);
```

**参数：**
- `temperature`: 温度输出指针（可为NULL）
- `humidity`: 湿度输出指针（可为NULL）

**返回值：**
- `DHT11_OK`: 读取成功
- `DHT11_ERROR_*`: 各种错误

#### `DHT11_ReadRaw()`
读取原始数据（整数形式）。

```c
DHT11_Status_t DHT11_ReadRaw(DHT11_RawData_t *rawData);
```

### 辅助函数

#### `DHT11_GetTemperature()`
获取最后一次成功读取的温度。

```c
float DHT11_GetTemperature(void);
```

#### `DHT11_GetHumidity()`
获取最后一次成功读取的湿度。

```c
float DHT11_GetHumidity(void);
```

#### `DHT11_IsReady()`
检查传感器是否就绪（采样间隔已满足）。

```c
uint8_t DHT11_IsReady(void);
```

#### `DHT11_GetStatusString()`
获取状态描述字符串。

```c
const char* DHT11_GetStatusString(DHT11_Status_t status);
```

## 状态码

| 状态码 | 描述 |
|--------|------|
| `DHT11_OK` | 操作成功 |
| `DHT11_ERROR` | 一般错误 |
| `DHT11_ERROR_TIMEOUT` | 通信超时 |
| `DHT11_ERROR_CHECKSUM` | 校验和错误 |
| `DHT11_ERROR_NO_RESPONSE` | 传感器无响应 |
| `DHT11_ERROR_NOT_READY` | 采样间隔不足 |
| `DHT11_ERROR_INVALID_DATA` | 数据无效 |

## 使用示例

### 基本使用

```c
#include "dht11.h"

int main(void)
{
    // HAL初始化...
    
    DHT11_Init();
    
    while (1) {
        float temp, humi;
        
        if (DHT11_Read(&temp, &humi) == DHT11_OK) {
            printf("Temp: %.1f C, Humi: %.1f %%\r\n", temp, humi);
        }
        
        HAL_Delay(2000);
    }
}
```

### 温度报警

```c
const float TEMP_THRESHOLD = 30.0f;

if (DHT11_Read(&temp, &humi) == DHT11_OK) {
    if (temp > TEMP_THRESHOLD) {
        // 触发高温报警
        HAL_GPIO_WritePin(ALARM_GPIO_Port, ALARM_Pin, GPIO_PIN_SET);
    }
}
```

### 结合ESP8266上传数据

```c
if (DHT11_Read(&temp, &humi) == DHT11_OK) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), 
             "{\"temp\":%.1f,\"humi\":%.1f}\r\n", temp, humi);
    ESP8266_TransparentSend((uint8_t*)buffer, strlen(buffer));
}
```

## 调试

开启调试输出（在 `dht11.h` 中）：

```c
#define DHT11_DEBUG_ENABLE  1
```

调试信息将通过UART1输出。

## DHT11 通信时序

```
主机发送起始信号:
    ___      ____________________
       |____|                    (低电平至少18ms，然后释放)

DHT11响应信号:
           ___      ___
       ___|   |____|   |________  (80us低 + 80us高)

数据位 '0':
    ___      ___
       |____|   |________________  (50us低 + 26-28us高)

数据位 '1':
    ___      _______
       |____|       |____________  (50us低 + 70us高)
```

## 注意事项

1. **采样间隔**：DHT11最小采样间隔为1秒，过快读取会返回`DHT11_ERROR_NOT_READY`
2. **上电延时**：传感器上电后需要等待1秒稳定
3. **总线需上拉**：如果没有外部上拉电阻，驱动会使用内部上拉
4. **中断保护**：读取过程中时序敏感，高优先级中断可能影响读取

## 文件结构

```
Core/
├── Inc/
│   └── dht11.h          # 驱动头文件
└── Src/
    ├── dht11.c          # 驱动源文件
    └── dht11_example.c  # 使用示例
```

## 版本历史

- **V1.0.0** (2025-12-16)
  - 初始版本
  - 支持温湿度读取
  - 数据校验和验证
  - DWT微秒延时

## 作者

Antigravity AI

## 许可证

MIT License
