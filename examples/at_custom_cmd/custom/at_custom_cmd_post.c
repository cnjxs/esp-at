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
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "AT_POST";

static SemaphoreHandle_t at_sync_sema = NULL;

void wait_data_callback(void)
{
    xSemaphoreGive(at_sync_sema);
}

//接收数据
uint8_t *at_custom_rev_data(uint32_t len)
{
    uint8_t *buf = (uint8_t *)malloc(len + 20);
    if (buf == NULL) 
    {
        return NULL;
    }

    // 示例代码
    // 不必在此处创建 semaphores
    if (!at_sync_sema) 
    {
        at_sync_sema = xSemaphoreCreateBinary();
        assert(at_sync_sema != NULL);
    }

    // 返回输入数据提示符 ">"
    esp_at_port_write_data((uint8_t *)">", strlen(">"));

    // 设置回调函数，在接收到输入数据后由 AT 端口调用
    esp_at_port_enter_specific(wait_data_callback);

    int32_t received_len = 0;
    int32_t remain_len = 0;
    // 接收输入的数据
    while(xSemaphoreTake(at_sync_sema, portMAX_DELAY)) 
    {
        received_len += esp_at_port_read_data(buf + received_len, len - received_len);

        if (received_len >= len) 
        {
            esp_at_port_exit_specific();

            // 获取剩余输入数据的长度
            remain_len = esp_at_port_get_data_length();
            if (remain_len > 0) 
            {
                esp_at_port_recv_data_notify(remain_len, portMAX_DELAY);
            }
            
            // 输出接收到的数据
            // uint8_t buffer[64] = {0};
            // memset(buffer, 0, 64);
            // snprintf((char *)buffer, 64, "\r\n接收数据: ");
            // esp_at_port_write_data(buffer, strlen((char *)buffer));

            // esp_at_port_write_data(buf, len);
            buf[len] = 0;
            // ESP_LOGI(TAG,"收到的数据:%s",buf);
            break;
        }
    }
    return buf;
}

//设置指令="url",post_len
static uint8_t at_setup_cmd_post(uint8_t para_num)
{
    // const char *send =  "{\"data\" : {\"receiveData\" : [{\"deviceMac\" : \"A4C138FEFE56\",\"dataTime\" : \"2025-12-04 13:25:31\",\"dataType\" : \"101\",\"dataNum\" : \"0\",\"content\" : \"0\"}],\"dataType\" : \"COLLECTOR-DATA-DEVICE-WARNING-RECORD\"},\"seq\" : \"\",\"timestamp\" : \"1763519647870\",\"userId\" : \"1\",\"systemType\" : \"REFRESH_TEMPERATURE\",\"userType\" : \"1\"}";
    // char *back = http_post_request("http://app.refresh.cc/collector_receive/v1/data",send,strlen(send),8000);
    uint8_t index = 0;

    char *url_str = NULL;
    if (esp_at_get_para_as_str(index++,(uint8_t **)&url_str) != ESP_AT_PARA_PARSE_RESULT_OK) 
    {
        ESP_LOGI(TAG,"获取url失败");
        return ESP_AT_RESULT_CODE_ERROR;
    }

    int32_t specified_len = 0;
    if (esp_at_get_para_as_digit(index++, &specified_len) != ESP_AT_PARA_PARSE_RESULT_OK) 
    {
        return ESP_AT_RESULT_CODE_ERROR;
    }

    int32_t return_data_flag = 0;
    if (esp_at_get_para_as_digit(index++, &return_data_flag) != ESP_AT_PARA_PARSE_RESULT_OK) 
    {
        return ESP_AT_RESULT_CODE_ERROR;
    }

    //阻塞接收
    uint8_t *buf = at_custom_rev_data(specified_len);
    if(buf == NULL)
    {
        return ESP_AT_RESULT_CODE_ERROR;
    }

    char *back = http_post_request(url_str,(char *)buf,specified_len,8000);
    free(buf);

    if(return_data_flag != 0)
    {
        esp_at_port_write_data((uint8_t *)back, strlen(back));
    }

    if(back)
    {
        char *rev_str = strstr(back,"\"code\":") + 7;
        if(rev_str)
        {
            if(strstr(rev_str,"1000"))
            {
                free(back);
                return ESP_AT_RESULT_CODE_SEND_OK;
            }
            else if(strstr(rev_str,"1001"))
            {
                free(back);
                return ESP_AT_RESULT_CODE_SEND_FAIL;
            }
            else
            {
                free(back);
                return ESP_AT_RESULT_CODE_SEND_FAIL;
            }
        }
        else
        {
            free(back);
            return ESP_AT_RESULT_CODE_SEND_FAIL;//发送失败
        }
    }
    else
    {
        return ESP_AT_RESULT_CODE_SEND_FAIL;//发送失败
    }

    return ESP_AT_RESULT_CODE_OK;
}

static const esp_at_cmd_struct at_custom_post_cmd[] = 
{
    {"+POST", NULL, NULL, at_setup_cmd_post, NULL},
};

bool esp_at_custom_cmd_post_register(void)
{
    return esp_at_custom_cmd_array_regist(at_custom_post_cmd, sizeof(at_custom_post_cmd) / sizeof(esp_at_cmd_struct));
}

ESP_AT_CMD_SET_INIT_FN(esp_at_custom_cmd_post_register, 1);



#if 0
//设置指令="url",num
static uint8_t at_setup_cmd_post_state_code(uint8_t para_num)
{
    // const char *send =  "{\"data\" : {\"receiveData\" : [{\"deviceMac\" : \"A4C138FEFE56\",\"dataTime\" : \"2025-12-04 13:25:31\",\"dataType\" : \"101\",\"dataNum\" : \"0\",\"content\" : \"0\"}],\"dataType\" : \"COLLECTOR-DATA-DEVICE-WARNING-RECORD\"},\"seq\" : \"\",\"timestamp\" : \"1763519647870\",\"userId\" : \"1\",\"systemType\" : \"REFRESH_TEMPERATURE\",\"userType\" : \"1\"}";
    // char *back = http_post_request("http://app.refresh.cc/collector_receive/v1/data",send,strlen(send),8000);
    uint8_t index = 0;
    uint8_t buffer[64] = {0};

    char *url_str = NULL;
    if (esp_at_get_para_as_str(index++,(uint8_t **)&url_str) != ESP_AT_PARA_PARSE_RESULT_OK) 
    {
        ESP_LOGI(TAG,"获取url失败");
        return ESP_AT_RESULT_CODE_ERROR;
    }

    int32_t specified_len = 0;
    if (esp_at_get_para_as_digit(index++, &specified_len) != ESP_AT_PARA_PARSE_RESULT_OK) 
    {
        return ESP_AT_RESULT_CODE_ERROR;
    }

    //阻塞接收
    uint8_t *buf = at_custom_rev_data(specified_len);
    if(buf == NULL)
    {
        return ESP_AT_RESULT_CODE_ERROR;
    }
    //解析组装


    char *back = http_post_request(url_str,(char *)buf,specified_len,8000);
    free(buf);
    if(back)
    {
        free(back);
    }
    else
    {
        return ESP_AT_RESULT_CODE_SEND_FAIL;//发送失败
    }

    return ESP_AT_RESULT_CODE_OK;
}

static const esp_at_cmd_struct at_custom_post_state_code_cmd[] = 
{
    {"+POSTSTATECODE", NULL, NULL, at_setup_cmd_post_state_code, NULL},
};

bool esp_at_custom_cmd_post_state_code_register(void)
{
    return esp_at_custom_cmd_array_regist(at_custom_post_state_code_cmd, sizeof(at_custom_post_state_code_cmd) / sizeof(esp_at_cmd_struct));
}

ESP_AT_CMD_SET_INIT_FN(esp_at_custom_cmd_post_state_code_register, 1);
#endif

