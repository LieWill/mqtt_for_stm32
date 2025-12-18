/**
  ******************************************************************************
  * @file           : esp8266.c
  * @brief          : ESP8266 WiFi模块驱动源文件 (DMA版本)
  * @version        : V2.0.0
  ******************************************************************************
  */

#include "esp8266.h"

/* Private variables ---------------------------------------------------------*/
ESP8266_Handle_t esp8266;
extern UART_HandleTypeDef huart1;  /* 调试输出UART */

/* Private function prototypes -----------------------------------------------*/
static void ESP8266_Delay(uint32_t ms);
static uint8_t ESP8266_ParseIPD(ESP8266_RxData_t *rxData);

/* Debug print */
void ESP8266_DebugPrint(const char *format, ...)
{
#if ESP8266_DEBUG_ENABLE
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), HAL_MAX_DELAY);
#endif
}

static void ESP8266_Delay(uint32_t ms) { HAL_Delay(ms); }

/* DMA发送 */
ESP8266_Status_t ESP8266_SendDMA(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0) return ESP8266_INVALID_PARAM;
    
    uint32_t timeout = HAL_GetTick();
    while (esp8266.txBusy) {
        if (HAL_GetTick() - timeout > 1000) return ESP8266_TIMEOUT;
        ESP8266_Delay(1);
    }
    
    esp8266.txBusy = 1;
    if (HAL_UART_Transmit_DMA(esp8266.huart, (uint8_t *)data, len) != HAL_OK) {
        esp8266.txBusy = 0;
        return ESP8266_ERROR;
    }
    
    timeout = HAL_GetTick();
    while (esp8266.txBusy) {
        if (HAL_GetTick() - timeout > 5000) {
            esp8266.txBusy = 0;
            return ESP8266_TIMEOUT;
        }
        ESP8266_Delay(1);
    }
    return ESP8266_OK;
}

/* 启动DMA接收 */
void ESP8266_StartDMAReceive(void)
{
    HAL_UARTEx_ReceiveToIdle_DMA(esp8266.huart, esp8266.dmaRxBuffer, ESP8266_RX_BUF_SIZE);
    __HAL_DMA_DISABLE_IT(esp8266.huart->hdmarx, DMA_IT_HT);
}

/* IDLE中断回调 */
void ESP8266_UART_IdleCallback(UART_HandleTypeDef *huart)
{
    if (huart != esp8266.huart) return;
    
    uint16_t len = ESP8266_RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(huart->hdmarx);
    if (len > 0 && len < ESP8266_RX_BUF_SIZE) {
        memcpy(esp8266.rxBuffer, esp8266.dmaRxBuffer, len);
        esp8266.rxBuffer[len] = '\0';
        esp8266.rxLength = len;
        esp8266.rxComplete = 1;
    }
    ESP8266_StartDMAReceive();
}

/* HAL回调 - 接收完成/IDLE */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart == esp8266.huart && Size > 0) {
        memcpy(esp8266.rxBuffer, esp8266.dmaRxBuffer, Size);
        esp8266.rxBuffer[Size] = '\0';
        esp8266.rxLength = Size;
        esp8266.rxComplete = 1;
        ESP8266_StartDMAReceive();
    }
}

/* DMA发送完成回调 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == esp8266.huart) {
        esp8266.txBusy = 0;
    }
}

/* 初始化 */
ESP8266_Status_t ESP8266_Init(UART_HandleTypeDef *huart)
{
    ESP8266_DebugPrint("[ESP8266] DMA Init...\r\n");
    
    memset(&esp8266, 0, sizeof(ESP8266_Handle_t));
    esp8266.huart = huart;
    
    ESP8266_StartDMAReceive();
    ESP8266_Delay(1000);
    
    if (ESP8266_Test() != ESP8266_OK) {
        ESP8266_ExitTransparent();
        ESP8266_Delay(500);
        if (ESP8266_Test() != ESP8266_OK) {
            ESP8266_DebugPrint("[ESP8266] Init failed\r\n");
            return ESP8266_ERROR;
        }
    }
    
    ESP8266_SetEcho(0);
    ESP8266_SetWiFiMode(ESP8266_MODE_STA);
    esp8266.initialized = 1;
    ESP8266_DebugPrint("[ESP8266] Init OK\r\n");
    return ESP8266_OK;
}

ESP8266_Status_t ESP8266_DeInit(void) {
    HAL_UART_DMAStop(esp8266.huart);
    esp8266.initialized = 0;
    return ESP8266_OK;
}

ESP8266_Status_t ESP8266_Reset(void) {
    ESP8266_Status_t ret = ESP8266_SendCommand("AT+RST\r\n", "ready", ESP8266_LONG_TIMEOUT);
    if (ret == ESP8266_OK) { ESP8266_Delay(2000); esp8266.wifiConnected = 0; }
    return ret;
}

ESP8266_Status_t ESP8266_Test(void) {
    return ESP8266_SendCommand("AT\r\n", "OK", ESP8266_DEFAULT_TIMEOUT);
}

ESP8266_Status_t ESP8266_Restore(void) {
    ESP8266_Status_t ret = ESP8266_SendCommand("AT+RESTORE\r\n", "ready", ESP8266_LONG_TIMEOUT);
    if (ret == ESP8266_OK) { ESP8266_Delay(2000); esp8266.wifiConnected = 0; }
    return ret;
}

ESP8266_Status_t ESP8266_SetEcho(uint8_t enable) {
    char cmd[16];
    snprintf(cmd, sizeof(cmd), "ATE%d\r\n", enable ? 1 : 0);
    return ESP8266_SendCommand(cmd, "OK", ESP8266_DEFAULT_TIMEOUT);
}

ESP8266_Status_t ESP8266_GetVersion(char *version, uint16_t maxLen) {
    ESP8266_Status_t ret = ESP8266_SendCommand("AT+GMR\r\n", "OK", ESP8266_DEFAULT_TIMEOUT);
    if (ret == ESP8266_OK && version) {
        strncpy(version, (char *)esp8266.rxBuffer, maxLen - 1);
        version[maxLen - 1] = '\0';
    }
    return ret;
}

ESP8266_Status_t ESP8266_SetWiFiMode(ESP8266_WiFiMode_t mode) {
    ESP8266_Status_t ret = ESP8266_SendCommandF("OK", ESP8266_DEFAULT_TIMEOUT, "AT+CWMODE=%d\r\n", mode);
    if (ret == ESP8266_OK) esp8266.wifiMode = mode;
    return ret;
}

ESP8266_Status_t ESP8266_GetWiFiMode(ESP8266_WiFiMode_t *mode) {
    ESP8266_Status_t ret = ESP8266_SendCommand("AT+CWMODE?\r\n", "OK", ESP8266_DEFAULT_TIMEOUT);
    if (ret == ESP8266_OK && mode) {
        char *ptr = strstr((char *)esp8266.rxBuffer, "+CWMODE:");
        if (ptr) *mode = (ESP8266_WiFiMode_t)atoi(ptr + 8);
    }
    return ret;
}

ESP8266_Status_t ESP8266_ConnectAP(const char *ssid, const char *password) {
    if (!ssid) return ESP8266_INVALID_PARAM;
    ESP8266_DebugPrint("[ESP8266] Connecting: %s\r\n", ssid);
    
    ESP8266_Status_t ret;
    if (password && strlen(password) > 0)
        ret = ESP8266_SendCommandF("OK", ESP8266_CONNECT_TIMEOUT, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    else
        ret = ESP8266_SendCommandF("OK", ESP8266_CONNECT_TIMEOUT, "AT+CWJAP=\"%s\",\"\"\r\n", ssid);
    
    if (ret == ESP8266_OK) {
        esp8266.wifiConnected = 1;
        if (esp8266.onWifiConnected) esp8266.onWifiConnected();
        ESP8266_GetIPInfo(&esp8266.ipInfo);
    } else {
        esp8266.wifiConnected = 0;
        if (ESP8266_ContainsString("FAIL")) return ESP8266_CONNECT_FAIL;
    }
    return ret;
}

ESP8266_Status_t ESP8266_DisconnectAP(void) {
    ESP8266_Status_t ret = ESP8266_SendCommand("AT+CWQAP\r\n", "OK", ESP8266_DEFAULT_TIMEOUT);
    if (ret == ESP8266_OK) {
        esp8266.wifiConnected = 0;
        if (esp8266.onWifiDisconnected) esp8266.onWifiDisconnected();
    }
    return ret;
}

ESP8266_Status_t ESP8266_GetAPInfo(ESP8266_APInfo_t *apInfo) {
    if (!apInfo) return ESP8266_INVALID_PARAM;
    return ESP8266_SendCommand("AT+CWJAP?\r\n", "OK", ESP8266_DEFAULT_TIMEOUT);
}

ESP8266_Status_t ESP8266_ScanAP(ESP8266_APInfo_t *apList, uint8_t maxCount, uint8_t *foundCount) {
    if (!apList || !foundCount) return ESP8266_INVALID_PARAM;
    *foundCount = 0;
    return ESP8266_SendCommand("AT+CWLAP\r\n", "OK", ESP8266_LONG_TIMEOUT);
}

ESP8266_Status_t ESP8266_SetAutoConnect(uint8_t enable) {
    return ESP8266_SendCommandF("OK", ESP8266_DEFAULT_TIMEOUT, "AT+CWAUTOCONN=%d\r\n", enable ? 1 : 0);
}

ESP8266_Status_t ESP8266_SetupAP(const char *ssid, const char *password, uint8_t channel, ESP8266_Encryption_t ecn) {
    if (!ssid) return ESP8266_INVALID_PARAM;
    return ESP8266_SendCommandF("OK", ESP8266_DEFAULT_TIMEOUT, "AT+CWSAP=\"%s\",\"%s\",%d,%d\r\n",
                                ssid, password ? password : "", channel, ecn);
}

ESP8266_Status_t ESP8266_GetAPConfig(char *ssid, char *password, uint8_t *channel, ESP8266_Encryption_t *ecn) {
    return ESP8266_SendCommand("AT+CWSAP?\r\n", "OK", ESP8266_DEFAULT_TIMEOUT);
}

ESP8266_Status_t ESP8266_GetIPInfo(ESP8266_IPInfo_t *ipInfo) {
    if (!ipInfo) return ESP8266_INVALID_PARAM;
    ESP8266_Status_t ret = ESP8266_SendCommand("AT+CIFSR\r\n", "OK", ESP8266_DEFAULT_TIMEOUT);
    if (ret == ESP8266_OK) {
        char *ptr = strstr((char *)esp8266.rxBuffer, "STAIP,\"");
        if (ptr) {
            ptr += 7;
            char *end = strchr(ptr, '"');
            if (end) { int len = end - ptr; if (len > 15) len = 15; strncpy(ipInfo->ip, ptr, len); ipInfo->ip[len] = '\0'; }
        }
    }
    return ret;
}

ESP8266_Status_t ESP8266_SetStationIP(const char *ip, const char *gateway, const char *netmask) {
    if (!ip) return ESP8266_INVALID_PARAM;
    if (gateway && netmask)
        return ESP8266_SendCommandF("OK", ESP8266_DEFAULT_TIMEOUT, "AT+CIPSTA=\"%s\",\"%s\",\"%s\"\r\n", ip, gateway, netmask);
    return ESP8266_SendCommandF("OK", ESP8266_DEFAULT_TIMEOUT, "AT+CIPSTA=\"%s\"\r\n", ip);
}

ESP8266_Status_t ESP8266_SetAPIP(const char *ip, const char *gateway, const char *netmask) {
    if (!ip) return ESP8266_INVALID_PARAM;
    if (gateway && netmask)
        return ESP8266_SendCommandF("OK", ESP8266_DEFAULT_TIMEOUT, "AT+CIPAP=\"%s\",\"%s\",\"%s\"\r\n", ip, gateway, netmask);
    return ESP8266_SendCommandF("OK", ESP8266_DEFAULT_TIMEOUT, "AT+CIPAP=\"%s\"\r\n", ip);
}

ESP8266_Status_t ESP8266_EnableDHCP(ESP8266_WiFiMode_t mode, uint8_t enable) {
    return ESP8266_SendCommandF("OK", ESP8266_DEFAULT_TIMEOUT, "AT+CWDHCP=%d,%d\r\n", mode, enable ? 1 : 0);
}

ESP8266_Status_t ESP8266_GetMac(ESP8266_WiFiMode_t mode, char *mac) {
    if (!mac) return ESP8266_INVALID_PARAM;
    return ESP8266_SendCommand(mode == ESP8266_MODE_STA ? "AT+CIPSTAMAC?\r\n" : "AT+CIPAPMAC?\r\n", "OK", ESP8266_DEFAULT_TIMEOUT);
}

ESP8266_Status_t ESP8266_SetMac(ESP8266_WiFiMode_t mode, const char *mac) {
    if (!mac) return ESP8266_INVALID_PARAM;
    return ESP8266_SendCommandF("OK", ESP8266_DEFAULT_TIMEOUT, "%s=\"%s\"\r\n",
                                mode == ESP8266_MODE_STA ? "AT+CIPSTAMAC" : "AT+CIPAPMAC", mac);
}

ESP8266_Status_t ESP8266_SetMultiConn(uint8_t enable) {
    ESP8266_Status_t ret = ESP8266_SendCommandF("OK", ESP8266_DEFAULT_TIMEOUT, "AT+CIPMUX=%d\r\n", enable ? 1 : 0);
    if (ret == ESP8266_OK) esp8266.multiConnMode = enable;
    return ret;
}

ESP8266_Status_t ESP8266_Connect(ESP8266_ConnType_t type, const char *host, uint16_t port, uint8_t *linkId) {
    if (!host) return ESP8266_INVALID_PARAM;
    const char *typeStr = type == ESP8266_TCP ? "TCP" : (type == ESP8266_UDP ? "UDP" : "SSL");
    ESP8266_Status_t ret = ESP8266_SendCommandF("OK", ESP8266_CONNECT_TIMEOUT, "AT+CIPSTART=\"%s\",\"%s\",%d\r\n", typeStr, host, port);
    if (ret == ESP8266_OK || ESP8266_ContainsString("CONNECT")) { if (linkId) *linkId = 0; return ESP8266_OK; }
    if (ESP8266_ContainsString("ALREADY")) return ESP8266_ALREADY_CONNECTED;
    return ESP8266_CONNECT_FAIL;
}

ESP8266_Status_t ESP8266_ConnectEx(uint8_t linkId, ESP8266_ConnType_t type, const char *host, uint16_t port) {
    if (!host || linkId >= ESP8266_MAX_CONNECTIONS) return ESP8266_INVALID_PARAM;
    const char *typeStr = type == ESP8266_TCP ? "TCP" : (type == ESP8266_UDP ? "UDP" : "SSL");
    ESP8266_Status_t ret = ESP8266_SendCommandF("OK", ESP8266_CONNECT_TIMEOUT, "AT+CIPSTART=%d,\"%s\",\"%s\",%d\r\n", linkId, typeStr, host, port);
    if (ret == ESP8266_OK || ESP8266_ContainsString("CONNECT")) return ESP8266_OK;
    if (ESP8266_ContainsString("ALREADY")) return ESP8266_ALREADY_CONNECTED;
    return ESP8266_CONNECT_FAIL;
}

ESP8266_Status_t ESP8266_Close(uint8_t linkId) {
    if (esp8266.multiConnMode)
        return ESP8266_SendCommandF("OK", ESP8266_DEFAULT_TIMEOUT, "AT+CIPCLOSE=%d\r\n", linkId);
    return ESP8266_SendCommand("AT+CIPCLOSE\r\n", "OK", ESP8266_DEFAULT_TIMEOUT);
}

ESP8266_Status_t ESP8266_CloseAll(void) {
    return ESP8266_SendCommand("AT+CIPCLOSE=5\r\n", "OK", ESP8266_DEFAULT_TIMEOUT);
}

ESP8266_Status_t ESP8266_GetConnStatus(ESP8266_ConnStatus_t *status, uint8_t *count) {
    if (!count) return ESP8266_INVALID_PARAM;
    *count = 0;
    return ESP8266_SendCommand("AT+CIPSTATUS\r\n", "OK", ESP8266_DEFAULT_TIMEOUT);
}

ESP8266_Status_t ESP8266_StartServer(uint16_t port) {
    if (!esp8266.multiConnMode) ESP8266_SetMultiConn(1);
    ESP8266_Status_t ret = ESP8266_SendCommandF("OK", ESP8266_DEFAULT_TIMEOUT, "AT+CIPSERVER=1,%d\r\n", port);
    if (ret == ESP8266_OK) esp8266.serverStarted = 1;
    return ret;
}

ESP8266_Status_t ESP8266_StopServer(void) {
    ESP8266_Status_t ret = ESP8266_SendCommand("AT+CIPSERVER=0\r\n", "OK", ESP8266_DEFAULT_TIMEOUT);
    if (ret == ESP8266_OK) esp8266.serverStarted = 0;
    return ret;
}

ESP8266_Status_t ESP8266_SetServerTimeout(uint16_t timeout) {
    if (timeout > 7200) timeout = 7200;
    return ESP8266_SendCommandF("OK", ESP8266_DEFAULT_TIMEOUT, "AT+CIPSTO=%d\r\n", timeout);
}

ESP8266_Status_t ESP8266_Send(uint8_t linkId, const uint8_t *data, uint16_t len) {
    if (!data || len == 0) return ESP8266_INVALID_PARAM;
    
    ESP8266_Status_t ret;
    if (esp8266.multiConnMode)
        ret = ESP8266_SendCommandF(">", ESP8266_DEFAULT_TIMEOUT, "AT+CIPSEND=%d,%d\r\n", linkId, len);
    else
        ret = ESP8266_SendCommandF(">", ESP8266_DEFAULT_TIMEOUT, "AT+CIPSEND=%d\r\n", len);
    
    if (ret != ESP8266_OK) return ESP8266_SEND_FAIL;
    
    ESP8266_ClearBuffer();
    ESP8266_SendDMA(data, len);
    
    if (ESP8266_WaitForResponse("SEND OK", ESP8266_DEFAULT_TIMEOUT)) return ESP8266_OK;
    return ESP8266_SEND_FAIL;
}

ESP8266_Status_t ESP8266_SendString(uint8_t linkId, const char *str) {
    if (!str) return ESP8266_INVALID_PARAM;
    return ESP8266_Send(linkId, (uint8_t *)str, strlen(str));
}

ESP8266_Status_t ESP8266_SendPrintf(uint8_t linkId, const char *format, ...) {
    char buf[ESP8266_TX_BUF_SIZE];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    return len > 0 ? ESP8266_Send(linkId, (uint8_t *)buf, len) : ESP8266_INVALID_PARAM;
}

ESP8266_Status_t ESP8266_EnterTransparent(void) {
    if (esp8266.multiConnMode) ESP8266_SetMultiConn(0);
    ESP8266_Status_t ret = ESP8266_SendCommand("AT+CIPMODE=1\r\n", "OK", ESP8266_DEFAULT_TIMEOUT);
    if (ret != ESP8266_OK) return ret;
    ret = ESP8266_SendCommand("AT+CIPSEND\r\n", ">", ESP8266_DEFAULT_TIMEOUT);
    if (ret == ESP8266_OK) esp8266.transparentMode = 1;
    return ret;
}

ESP8266_Status_t ESP8266_ExitTransparent(void) {
    ESP8266_Delay(1000);
    HAL_UART_Transmit(esp8266.huart, (uint8_t *)"+++", 3, HAL_MAX_DELAY);
    ESP8266_Delay(1000);
    esp8266.transparentMode = 0;
    return ESP8266_SendCommand("AT+CIPMODE=0\r\n", "OK", ESP8266_DEFAULT_TIMEOUT);
}

ESP8266_Status_t ESP8266_TransparentSend(const uint8_t *data, uint16_t len) {
    if (!esp8266.transparentMode || !data || len == 0) return ESP8266_ERROR;
    return ESP8266_SendDMA(data, len);
}

ESP8266_Status_t ESP8266_HttpGet(const char *host, uint16_t port, const char *path, char *response, uint16_t maxLen) {
    if (!host || !path) return ESP8266_INVALID_PARAM;
    
    ESP8266_Status_t ret = ESP8266_Connect(ESP8266_TCP, host, port, NULL);
    if (ret != ESP8266_OK && ret != ESP8266_ALREADY_CONNECTED) return ret;
    
    char request[512];
    int len = snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
    ret = ESP8266_Send(0, (uint8_t *)request, len);
    
    ESP8266_Delay(2000);
    if (response && esp8266.rxLength > 0) {
        uint16_t copyLen = esp8266.rxLength < maxLen - 1 ? esp8266.rxLength : maxLen - 1;
        memcpy(response, esp8266.rxBuffer, copyLen);
        response[copyLen] = '\0';
    }
    
    ESP8266_Close(0);
    return ret;
}

ESP8266_Status_t ESP8266_HttpPost(const char *host, uint16_t port, const char *path, const char *contentType, const char *body, char *response, uint16_t maxLen) {
    if (!host || !path) return ESP8266_INVALID_PARAM;
    
    ESP8266_Status_t ret = ESP8266_Connect(ESP8266_TCP, host, port, NULL);
    if (ret != ESP8266_OK && ret != ESP8266_ALREADY_CONNECTED) return ret;
    
    uint16_t bodyLen = body ? strlen(body) : 0;
    char request[1024];
    int len = snprintf(request, sizeof(request),
        "POST %s HTTP/1.1\r\nHost: %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s",
        path, host, contentType ? contentType : "application/x-www-form-urlencoded", bodyLen, body ? body : "");
    ret = ESP8266_Send(0, (uint8_t *)request, len);
    
    ESP8266_Delay(2000);
    if (response && esp8266.rxLength > 0) {
        uint16_t copyLen = esp8266.rxLength < maxLen - 1 ? esp8266.rxLength : maxLen - 1;
        memcpy(response, esp8266.rxBuffer, copyLen);
        response[copyLen] = '\0';
    }
    
    ESP8266_Close(0);
    return ret;
}

ESP8266_Status_t ESP8266_Ping(const char *host) {
    if (!host) return ESP8266_INVALID_PARAM;
    return ESP8266_SendCommandF("OK", ESP8266_LONG_TIMEOUT, "AT+PING=\"%s\"\r\n", host);
}

/* 底层AT命令发送 */
ESP8266_Status_t ESP8266_SendCommand(const char *cmd, const char *expectedResp, uint32_t timeout) {
    if (!cmd) return ESP8266_INVALID_PARAM;
    
    ESP8266_ClearBuffer();
    ESP8266_SendDMA((uint8_t *)cmd, strlen(cmd));
    
    if (!expectedResp) return ESP8266_OK;
    if (ESP8266_WaitForResponse(expectedResp, timeout)) return ESP8266_OK;
    if (ESP8266_ContainsString("ERROR")) return ESP8266_ERROR;
    if (ESP8266_ContainsString("BUSY")) return ESP8266_BUSY;
    return ESP8266_TIMEOUT;
}

ESP8266_Status_t ESP8266_SendCommandF(const char *expectedResp, uint32_t timeout, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf((char *)esp8266.txBuffer, ESP8266_TX_BUF_SIZE, format, args);
    va_end(args);
    return ESP8266_SendCommand((char *)esp8266.txBuffer, expectedResp, timeout);
}

void ESP8266_ClearBuffer(void) {
    memset(esp8266.rxBuffer, 0, ESP8266_RX_BUF_SIZE);
    esp8266.rxLength = 0;
    esp8266.rxComplete = 0;
}

uint8_t ESP8266_WaitForResponse(const char *response, uint32_t timeout) {
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < timeout) {
        if (ESP8266_ContainsString(response)) return 1;
        ESP8266_Delay(10);
    }
    return 0;
}

uint8_t ESP8266_ContainsString(const char *str) {
    return strstr((char *)esp8266.rxBuffer, str) != NULL;
}

char* ESP8266_GetResponseBuffer(void) { return (char *)esp8266.rxBuffer; }

void ESP8266_SetOnDataReceived(void (*cb)(ESP8266_RxData_t *)) { esp8266.onDataReceived = cb; }
void ESP8266_SetOnWifiConnected(void (*cb)(void)) { esp8266.onWifiConnected = cb; }
void ESP8266_SetOnWifiDisconnected(void (*cb)(void)) { esp8266.onWifiDisconnected = cb; }
void ESP8266_SetOnClientConnected(void (*cb)(uint8_t)) { esp8266.onClientConnected = cb; }
void ESP8266_SetOnClientDisconnected(void (*cb)(uint8_t)) { esp8266.onClientDisconnected = cb; }

static uint8_t ESP8266_ParseIPD(ESP8266_RxData_t *rxData) {
    char *ptr = strstr((char *)esp8266.rxBuffer, "+IPD,");
    if (!ptr) return 0;
    ptr += 5;
    if (esp8266.multiConnMode) { rxData->linkId = atoi(ptr); ptr = strchr(ptr, ','); if (!ptr) return 0; ptr++; }
    else rxData->linkId = 0;
    rxData->length = atoi(ptr);
    ptr = strchr(ptr, ':');
    if (!ptr) return 0;
    ptr++;
    uint16_t copyLen = rxData->length < ESP8266_RX_BUF_SIZE - 1 ? rxData->length : ESP8266_RX_BUF_SIZE - 1;
    memcpy(rxData->data, ptr, copyLen);
    rxData->data[copyLen] = '\0';
    return 1;
}

void ESP8266_ProcessData(void) {
    if (!esp8266.rxComplete) return;
    
    if (ESP8266_ContainsString("WIFI DISCONNECT")) {
        esp8266.wifiConnected = 0;
        if (esp8266.onWifiDisconnected) esp8266.onWifiDisconnected();
    } else if (ESP8266_ContainsString("WIFI CONNECTED")) {
        esp8266.wifiConnected = 1;
        if (esp8266.onWifiConnected) esp8266.onWifiConnected();
    }
    
    if (ESP8266_ContainsString("+IPD,")) {
        ESP8266_RxData_t rxData;
        if (ESP8266_ParseIPD(&rxData) && esp8266.onDataReceived) {
            esp8266.onDataReceived(&rxData);
        }
    }
}

uint8_t ESP8266_IsInitialized(void) { return esp8266.initialized; }
uint8_t ESP8266_IsWifiConnected(void) { return esp8266.wifiConnected; }
uint8_t ESP8266_IsTxBusy(void) { return esp8266.txBusy; }
