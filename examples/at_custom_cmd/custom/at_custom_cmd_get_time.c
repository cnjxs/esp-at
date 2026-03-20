/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_at.h"
#include "at_http.h"

static const char *TAG = "TIME_AT";

// 简化版实现，不依赖cJSON
static esp_err_t get_timezone_info_simple(char *timestamp, int timestamp_len, int32_t *offset_value)
{
    char *rev = http_get_request("http://ipwho.is",8000);
    if (!rev) 
    {
        return ESP_FAIL;
    }
    
    // ESP_LOGI(TAG,"===========:%s",rev);

//     {
//     "About_Us": "https://ipwhois.io",
//     "ip": "113.88.243.198",
//     "success": true,
//     "type": "IPv4",
//     "continent": "Asia",
//     "continent_code": "AS",
//     "country": "China",
//     "country_code": "CN",
//     "region": "Guangdong Province",
//     "region_code": "44",
//     "city": "Guangzhou",
//     "latitude": 23.12911,
//     "longitude": 113.264385,
//     "is_eu": false,
//     "postal": "510030",
//     "calling_code": "86",
//     "capital": "Beijing",
//     "borders": "AF,BT,HK,IN,KG,KP,KZ,LA,MM,MN,MO,NP,PK,RU,TJ,VN",
//     "flag": {
//         "img": "https://cdn.ipwhois.io/flags/cn.svg",
//         "emoji": "🇨🇳",
//         "emoji_unicode": "U+1F1E8 U+1F1F3"
//     },
//     "connection": {
//         "asn": 4134,
//         "org": "Chinanet Guangdong Province Network",
//         "isp": "CHINANET BACKBONE",
//         "domain": "chinatelecom.cn"
//     },
//     "timezone": {
//         "id": "Asia/Shanghai",
//         "abbr": "CST",
//         "is_dst": false,
//         "offset": 28800,
//         "utc": "+08:00",
//         "current_time": "2025-12-03T17:31:46+08:00"
//     }
// }
    // 1. 解析 offset (数值)
    const char *offset_key = "\"offset\":";
    char *offset_start = strstr(rev, offset_key);

    if (offset_start != NULL) 
    {
        offset_start += strlen(offset_key); // 移动到值开始的位置
        // atoi 将字符串转换为整数，遇到非数字字符停止
        *offset_value = atoi(offset_start);
        // ESP_LOGI(TAG,"解析出的时区偏移量 (offset): %d 秒\n", *offset_value);
    } 
    else 
    {
        // ESP_LOGI(TAG,"未找到 offset 字段\n");
        free(rev);
        return ESP_FAIL;
    }

    // 2. 解析 current_time (字符串)
    const char *time_key = "\"current_time\":\"";
    char *time_start = strstr(rev, time_key);

    if (time_start != NULL) 
    {
        time_start += strlen(time_key); // 移动到值开始的位置（第一个引号之后）
        char *time_end = strchr(time_start, '\"'); // 找到下一个引号，即值的结束位置
        
        if (time_end != NULL) 
        {
            // 计算字符串长度并复制
            size_t time_len = time_end - time_start;
            strncpy(timestamp, time_start, time_len);
            timestamp[10] = ' ';
            timestamp[19] = '\0';
            // timestamp[time_len] = '\0'; // 确保字符串正确终止
            
            // ESP_LOGI(TAG,"解析出的当前时间 (current_time): %s\n", timestamp);
            free(rev);
            return ESP_OK;
        } 
    } 

    free(rev);
    return ESP_FAIL;
}


//测试指令=?
static uint8_t at_test_cmd_get_time(uint8_t *cmd_name)
{
    return ESP_AT_RESULT_CODE_OK;
}

//查询指令?
static uint8_t at_query_cmd_get_time(uint8_t *cmd_name)
{
    return ESP_AT_RESULT_CODE_OK;
}

//设置指令=
static uint8_t at_setup_cmd_get_time(uint8_t para_num)
{
    return ESP_AT_RESULT_CODE_OK;
}

//执行指令
static uint8_t at_exe_cmd_get_time(uint8_t *cmd_name)
{
    char timestamp[32] = {0};
    int32_t offset = 0;
    uint8_t buffer[64] = {0};

    // 获取时区信息
    esp_err_t ret = get_timezone_info_simple(timestamp, sizeof(timestamp), &offset);
    
    if (ret == ESP_OK) 
    {
        snprintf((char *)buffer, 64, "%s%s,offset:%d\r\n", "+GETTIME:",timestamp,offset);
        esp_at_port_write_data(buffer, strlen((char *)buffer));
        return ESP_AT_RESULT_CODE_OK;
    } 

    return ESP_AT_RESULT_CODE_FAIL;
}

static const esp_at_cmd_struct at_custom_get_time_cmd[] = {
    {"+GETTIME", at_test_cmd_get_time, at_query_cmd_get_time, at_setup_cmd_get_time, at_exe_cmd_get_time},
};

bool esp_at_custom_cmd_get_time_register(void)
{
    return esp_at_custom_cmd_array_regist(at_custom_get_time_cmd, sizeof(at_custom_get_time_cmd) / sizeof(esp_at_cmd_struct));
}

ESP_AT_CMD_SET_INIT_FN(esp_at_custom_cmd_get_time_register, 1);
