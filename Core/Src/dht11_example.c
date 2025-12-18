/**
  ******************************************************************************
  * @file           : dht11_example.c
  * @brief          : DHT11 温湿度传感器驱动使用示例
  * @author         : Antigravity AI
  * @version        : V1.0.0
  ******************************************************************************
  * @attention
  *
  * 此文件包含DHT11驱动库的使用示例代码
  * 将此代码添加到main.c中使用
  *
  ******************************************************************************
  */

/* 
   ========== 快速开始 ==========
   
   1. 在main.c中包含头文件:
      #include "dht11.h"
   
   2. 在main函数中初始化DHT11:
      DHT11_Init();
   
   3. 在while循环中读取数据:
      float temperature, humidity;
      if (DHT11_Read(&temperature, &humidity) == DHT11_OK) {
          // 使用温湿度数据
      }
   
   注意: DHT11采样间隔至少1秒!
*/

#include "dht11.h"
#include <stdio.h>

/* ========== 示例1: 基本使用 ========== */

/**
  * @brief  DHT11基本使用示例
  *         在main.c的USER CODE 2中调用初始化
  *         在while循环中调用读取函数
  */
void DHT11_Example_Basic(void)
{
    float temperature, humidity;
    DHT11_Status_t status;
    
    /* 初始化DHT11 (在main函数初始化部分调用一次) */
    status = DHT11_Init();
    if (status != DHT11_OK) {
        /* 初始化失败处理 */
        return;
    }
    
    /* 读取温湿度 (在while循环中调用) */
    /* 注意: 需要在while循环中调用，每次调用间隔至少1秒 */
    while (1) {
        /* 检查传感器是否就绪 */
        if (DHT11_IsReady()) {
            status = DHT11_Read(&temperature, &humidity);
            
            if (status == DHT11_OK) {
                /* 成功读取数据 */
                printf("Temperature: %.1f C, Humidity: %.1f %%\r\n", temperature, humidity);
            } else {
                /* 读取失败 */
                printf("DHT11 Error: %s\r\n", DHT11_GetStatusString(status));
            }
        }
        
        HAL_Delay(2000);  /* 每2秒读取一次 */
    }
}

/* ========== 示例2: 使用缓存的数据 ========== */

/**
  * @brief  使用缓存的温湿度数据
  *         如果读取失败或采样间隔不足，可以使用上次成功读取的数据
  */
void DHT11_Example_CachedData(void)
{
    float temperature, humidity;
    
    /* 初始化 */
    DHT11_Init();
    
    while (1) {
        /* 尝试读取新数据 */
        if (DHT11_IsReady()) {
            DHT11_Read(&temperature, &humidity);
        }
        
        /* 无论读取是否成功，都可以获取缓存的数据 */
        temperature = DHT11_GetTemperature();
        humidity = DHT11_GetHumidity();
        
        /* 检查最后一次读取状态 */
        if (DHT11_GetLastStatus() == DHT11_OK) {
            printf("Temperature: %.1f C, Humidity: %.1f %%\r\n", temperature, humidity);
        } else {
            printf("Using cached data (last read failed)\r\n");
            printf("Temperature: %.1f C, Humidity: %.1f %%\r\n", temperature, humidity);
        }
        
        HAL_Delay(1000);
    }
}

/* ========== 示例3: 读取原始数据 ========== */

/**
  * @brief  读取DHT11原始数据
  *         用于调试或需要整数数据的场景
  */
void DHT11_Example_RawData(void)
{
    DHT11_RawData_t rawData;
    DHT11_Status_t status;
    
    /* 初始化 */
    DHT11_Init();
    
    while (1) {
        /* 读取原始数据 */
        status = DHT11_ReadRaw(&rawData);
        
        if (status == DHT11_OK) {
            printf("Temperature: %d.%d C\r\n", rawData.temperature_int, rawData.temperature_dec);
            printf("Humidity: %d.%d %%\r\n", rawData.humidity_int, rawData.humidity_dec);
            printf("Checksum: 0x%02X\r\n", rawData.checksum);
        }
        
        HAL_Delay(2000);
    }
}

/* ========== 示例4: 温度报警 ========== */

/**
  * @brief  温度超限报警示例
  */
void DHT11_Example_TemperatureAlarm(void)
{
    float temperature, humidity;
    const float TEMP_HIGH_THRESHOLD = 30.0f;  /* 高温阈值 */
    const float TEMP_LOW_THRESHOLD = 10.0f;   /* 低温阈值 */
    
    /* 初始化 */
    DHT11_Init();
    
    while (1) {
        if (DHT11_Read(&temperature, &humidity) == DHT11_OK) {
            /* 检查温度阈值 */
            if (temperature > TEMP_HIGH_THRESHOLD) {
                /* 高温报警 */
                printf("WARNING: High temperature! %.1f C\r\n", temperature);
                /* 可以触发蜂鸣器或LED */
                /* HAL_GPIO_WritePin(ALARM_GPIO_Port, ALARM_Pin, GPIO_PIN_SET); */
            } else if (temperature < TEMP_LOW_THRESHOLD) {
                /* 低温报警 */
                printf("WARNING: Low temperature! %.1f C\r\n", temperature);
            } else {
                /* 温度正常 */
                printf("Temperature normal: %.1f C\r\n", temperature);
                /* HAL_GPIO_WritePin(ALARM_GPIO_Port, ALARM_Pin, GPIO_PIN_RESET); */
            }
        }
        
        HAL_Delay(5000);  /* 每5秒检测一次 */
    }
}

/* ========== 示例5: 结合ESP8266上传数据 ========== */

/**
  * @brief  结合ESP8266将温湿度数据上传到服务器
  */
void DHT11_Example_UploadData(void)
{
    float temperature, humidity;
    char dataBuffer[64];
    
    /* 假设ESP8266已经初始化并连接WiFi */
    /* ESP8266_Init(&huart3); */
    /* ESP8266_ConnectAP("SSID", "PASSWORD"); */
    /* ESP8266_Connect(ESP8266_TCP, "server_ip", 8000, NULL); */
    /* ESP8266_EnterTransparent(); */
    
    /* 初始化DHT11 */
    DHT11_Init();
    
    while (1) {
        if (DHT11_Read(&temperature, &humidity) == DHT11_OK) {
            /* 格式化数据 */
            snprintf(dataBuffer, sizeof(dataBuffer), 
                     "{\"temp\":%.1f,\"humi\":%.1f}\r\n", 
                     temperature, humidity);
            
            /* 通过ESP8266发送数据 */
            /* ESP8266_TransparentSend((uint8_t*)dataBuffer, strlen(dataBuffer)); */
            
            printf("Sent: %s", dataBuffer);
        }
        
        HAL_Delay(10000);  /* 每10秒上传一次 */
    }
}

/* ========== 示例6: 自定义引脚 ========== */

/**
  * @brief  使用自定义GPIO引脚
  */
void DHT11_Example_CustomPin(void)
{
    float temperature, humidity;
    
    /* 使用自定义引脚初始化 (例如: PA0) */
    /* 注意: 需要先在CubeMX中配置该引脚为GPIO输出 */
    DHT11_InitEx(GPIOA, GPIO_PIN_0);
    
    while (1) {
        if (DHT11_Read(&temperature, &humidity) == DHT11_OK) {
            printf("Temperature: %.1f C, Humidity: %.1f %%\r\n", temperature, humidity);
        }
        
        HAL_Delay(2000);
    }
}

/* 
   ========== main.c 集成示例 ==========
   
   在main.c中的集成步骤:
   
   1. 添加头文件包含 (在 USER CODE BEGIN Includes 区域):
   
      #include "dht11.h"
   
   2. 添加变量 (在 USER CODE BEGIN PV 区域):
   
      float g_temperature = 0.0f;
      float g_humidity = 0.0f;
   
   3. 初始化DHT11 (在 USER CODE BEGIN 2 区域):
   
      DHT11_Init();
   
   4. 在主循环中读取数据 (在 USER CODE BEGIN WHILE 区域):
   
      while (1) {
          if (DHT11_IsReady()) {
              if (DHT11_Read(&g_temperature, &g_humidity) == DHT11_OK) {
                  // 数据读取成功，可以使用g_temperature和g_humidity
              }
          }
          
          // 其他任务...
          
          HAL_Delay(100);  // 主循环延时，不影响DHT11采样间隔
      }
*/
