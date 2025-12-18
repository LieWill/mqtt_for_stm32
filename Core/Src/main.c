/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "esp8266.h" // esp8266驱动库
#include "dht11.h"   // dht11驱动库
#include "esp8266_mqtt.h" // esp8266的MQTT驱动库
#include "light_sensor.h" // 光敏传感器驱动库
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* MQTT Broker配置 - 请根据实际情况修改 */
#define MQTT_EXAMPLE_BROKER     "47.107.34.158"    /* 公共测试Broker */
#define MQTT_EXAMPLE_PORT       1883                 /* 默认MQTT端口 */
#define MQTT_EXAMPLE_CLIENT_ID  "STM32F407_Client"   /* 客户端ID */
#define MQTT_EXAMPLE_USERNAME   "stm32"                   /* 用户名(公共Broker可为空) */
#define MQTT_EXAMPLE_PASSWORD   "123456"                   /* 密码(公共Broker可为空) */

/* 主题配置 */
#define MQTT_TOPIC_SENSOR_DATA  "stm32/sensor/data"  /* 传感器数据发布主题 */
#define MQTT_TOPIC_CONTROL      "stm32/control"      /* 控制命令订阅主题 */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* MQTT回调函数声明 */
void OnMQTTConnected(void);
void OnMQTTDisconnected(void);
void OnMQTTMessageReceived(MQTT_Message_t *message);
void OnMQTTPublishComplete(const char *topic);
void OnMQTTError(MQTT_Status_t error);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_TIM13_Init();
  MX_USART3_UART_Init();
  MX_ADC3_Init();
  /* USER CODE BEGIN 2 */
	/* 初始化DHT11温湿度传感器 */
	DHT11_Init();
	HAL_UART_Transmit(&huart1, (uint8_t*)"DHT11 init finished\r\n", 22, 100);
	
	/* 初始化光敏传感器 (轮询模式) */
	if (LightSensor_Init() == LIGHT_SENSOR_OK) {
		HAL_UART_Transmit(&huart1, (uint8_t*)"LightSensor init OK\r\n", 22, 100);
	} else {
		HAL_UART_Transmit(&huart1, (uint8_t*)"LightSensor init failed!\r\n", 26, 100);
	}
	
	ESP8266_Status_t status;
    
    /* 初始化ESP8266 */
    status = ESP8266_Init(&huart3);
    if (status != ESP8266_OK) {
        ESP8266_DebugPrint("ESP8266 init failed!\r\n");
    }
    
    /* 设置WiFi模式为Station */
    ESP8266_SetWiFiMode(ESP8266_MODE_STA);
    
    /* 连接WiFi */
    status = ESP8266_ConnectAP("AK70", "204081011");
    if (status == ESP8266_OK) {
        ESP8266_DebugPrint("WiFi connected!\r\n");
        
        /* 获取IP地址 */
        ESP8266_IPInfo_t ipInfo;
        ESP8266_GetIPInfo(&ipInfo);
        ESP8266_DebugPrint("IP: %s\r\n", ipInfo.ip);
    } else {
        ESP8266_DebugPrint("WiFi connection failed!\r\n");
    }
	
	
	  MQTT_State_t ret = MQTT_Init();
    if (ret != MQTT_OK)
		{
			MQTT_DebugPrint("[MQTT] MQTT init faild!\r\n");
			return -1;
		}
    
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
    if (ret != MQTT_OK)
		{
			MQTT_DebugPrint("[MQTT] Set User Config faild!\r\n");
		}
 
    
    
    /* 6. 配置Broker */
    ret = MQTT_SetBroker(MQTT_EXAMPLE_BROKER, MQTT_EXAMPLE_PORT, 1);
		if (ret != MQTT_OK)
		{
			MQTT_DebugPrint("[MQTT] Connecting Broker faild!\r\n");
			////return -1;
		}
    
    /* 7. 连接 */
    ret = MQTT_Connect();
    if (ret == MQTT_OK) {
        MQTT_DebugPrint("[Example] Connected!\r\n");
    }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		//ESP8266_MainLoop();
		float temperature, humidity;
		char buffer[128];
		uint16_t light_value = 0;
    
		/* ========== 读取光敏传感器 ========== */
		/* LS1传感器特性：亮时ADC值小，暗时ADC值大，所以需要反转 */
		light_value = 4095 - LightSensor_GetValue();
		
		/* ========== 读取DHT11温湿度传感器 ========== */
    DHT11_Status_t dht_status = DHT11_Read(&temperature, &humidity);
		
		if (dht_status == DHT11_OK) {
			/* 构建包含所有传感器数据的JSON */
        snprintf(buffer, sizeof(buffer), 
                 "{\"temp\":%.1f,\"humi\":%.1f,\"light\":%d}", 
                 temperature, humidity, light_value);
        
        /* 调试输出 */
        HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
        HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, 100);
        
        /* 发布到MQTT */
        MQTT_Publish(MQTT_TOPIC_SENSOR_DATA, buffer, MQTT_QOS_0, 0);
    } else {
        /* DHT11读取失败，只发布光照数据 */
        snprintf(buffer, sizeof(buffer), "{\"light\":%d}", light_value);
        HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
        HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, 100);
        MQTT_Publish(MQTT_TOPIC_SENSOR_DATA, buffer, MQTT_QOS_0, 0);
    }
    
    HAL_Delay(5000);  /* 每5秒读取一次 */
		//HAL_UART_Transmit(&huart1,"hello world\n", 12, 100);
		//HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_SET);
		
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
	/* 退出透传模式 */
  // ESP8266_ExitTransparent();
  // ESP8266_DebugPrint("Exited transparent mode\r\n");
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/**
  * @brief  MQTT连接成功回调
  */
void OnMQTTConnected(void)
{
    MQTT_DebugPrint("[MQTT] Connected callback!\r\n");
}

/**
  * @brief  MQTT断开连接回调
  */
void OnMQTTDisconnected(void)
{
    MQTT_DebugPrint("[MQTT] Disconnected callback!\r\n");
}

/**
  * @brief  MQTT消息接收回调
  * @param  message: 接收到的消息
  */
void OnMQTTMessageReceived(MQTT_Message_t *message)
{
    if (!message) return;
    
    MQTT_DebugPrint("[MQTT] Message received!\r\n");
    MQTT_DebugPrint("  Topic: %s\r\n", message->topic);
    MQTT_DebugPrint("  Data: %s\r\n", message->data);
    
    /* 处理控制命令 - 可以根据需要扩展 */
    if (strcmp(message->topic, MQTT_TOPIC_CONTROL) == 0) {
        if (strstr((char *)message->data, "led_on")) {
            /* 打开LED */
            HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_SET);
        } else if (strstr((char *)message->data, "led_off")) {
            /* 关闭LED */
            HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);
        }
    }
}

/**
  * @brief  MQTT发布完成回调
  * @param  topic: 发布的主题
  */
void OnMQTTPublishComplete(const char *topic)
{
    MQTT_DebugPrint("[MQTT] Published to: %s\r\n", topic);
}

/**
  * @brief  MQTT错误回调
  * @param  error: 错误码
  */
void OnMQTTError(MQTT_Status_t error)
{
    MQTT_DebugPrint("[MQTT] Error: %d\r\n", error);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
