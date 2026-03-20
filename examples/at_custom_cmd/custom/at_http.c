/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "at_http.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_at.h"
#include "esp_http_client.h"

static const char *TAG = "AT_HTTP";

// HTTP事件处理器
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    static char *buffer = NULL;
    static int buffer_len = 0;

    switch (evt->event_id) 
    {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            // ESP_LOGI(TAG, "返回数据, len=%d,%s", evt->data_len,evt->data);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT: 
        {
            // 注意重定向事件
            break;
        }
    }
    return ESP_OK;
}

// HTTP GET请求函数
char *http_get_request(const char *url,uint32_t timeout)
{
    esp_http_client_config_t config = 
    {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = timeout,
        .buffer_size = 2048,
        .buffer_size_tx = 1024,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) 
    {
        ESP_LOGE(TAG, "http客户端初始化失败");
        return NULL;
    }
    esp_http_client_open(client,0);
    int content_len = esp_http_client_fetch_headers(client);
    // 获取响应数据
    // ESP_LOGI(TAG, "响应数据长度:%d",content_len);
    if (content_len > 0) 
    {
        char *response = malloc(content_len + 1);
        if (response) 
        {
            esp_http_client_read(client, response, content_len);
            response[content_len] = '\0';
            // ESP_LOGI(TAG, "响应数据:%s",response);
        }
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return response;
    }
    
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return NULL;
}

char *http_post_request(char *url,char *post_data,uint32_t post_len,uint32_t timeout)
{
    esp_http_client_config_t config = 
    {
        .url = url,   // 目标 URL
        .method = HTTP_METHOD_POST,
        .timeout_ms = timeout,
        // 如果是 https，可以用 https:// 开头或 .transport_type = HTTP_TRANSPORT_OVER_SSL
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) 
    {
        ESP_LOGE(TAG, "http客户端初始化失败");
        return NULL;
    }

    // ESP_LOGI(TAG, "POST长度:%d,内容:%s",post_len,post_data);

    //设置 Content-Type 头
    esp_http_client_set_header(client, "Content-Type", "application/json");

    esp_err_t err = esp_http_client_open(client, post_len);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return NULL;
    }

    int wlen = esp_http_client_write(client, post_data, post_len);
    if (wlen < 0 || (uint32_t)wlen != post_len) 
    {
        ESP_LOGE(TAG, "HTTP write failed, wlen=%d", wlen);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return NULL;
    }

    //读取并处理响应头
    int content_length = esp_http_client_fetch_headers(client);
    int state = esp_http_client_get_status_code(client);
    if (content_length < 0) 
    {
        ESP_LOGW(TAG, "提取head失败,错误值= %d", content_length);
    } 
    else 
    {
        // ESP_LOGI(TAG,"内容长度=%d,状态=%d",content_length,state);
        char *buffer = malloc(content_length + 1);
        if(buffer)
        {
            esp_http_client_read(client, buffer, content_length);
            buffer[content_length] = '\0';
            // ESP_LOGI(TAG, "返回数据: %s", buffer);
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            return buffer;
        }
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return NULL;
}
