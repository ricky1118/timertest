/********* timertest *******
 * Example This example code is软件定时器 .
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "sdkconfig.h"
#include "driver/timer.h"
#include "esp_timer.h"
static const char *TAG = "tmiertest";

/**
 * 摘要
 * 使用软件定时，2个：1个单次运行的定时器，1个周期运行的定时器
 * 
 * 
 ************/
//声明2个定时器的回调函数
void test_timer_periodic_cb(void *arg);//周期定时回调函数
void test_timer_once_cb(void *arg);//一次定时回调函数

//定义两个定时器句柄
esp_timer_handle_t test_p_handle = 0 ;
esp_timer_handle_t test_o_handle = 0 ;

//定义一个单次运行的定时器结构体
esp_timer_create_args_t test_once_arg = {
                                            .callback = &test_timer_once_cb,//设置回调函数
                                            .arg = NULL,//不带参数
                                            .name ="TestOnceTimer"//
                                            };
//定义一个周期重复运行的定时器结构体
esp_timer_create_args_t test_period_arg ={
                                            .callback = &test_timer_periodic_cb,
                                            .arg = NULL,
                                            .name = "TestPeriodicTimer"
                                        };

void test_timer_periodic_cb(void *arg){
        int64_t tick = esp_timer_get_time();
        ESP_LOGI(TAG,"方法回调名字：%s,距离定时器开启时间间隔 =%lld \r\n",test_period_arg.name,tick);

        if(tick >100000000){
            //停止定时器工作，并获取是否停止成功
            esp_err_t err = esp_timer_stop(test_p_handle);
            ESP_LOGI(TAG,"要停止的定时器名称：%s,是否停止成功：%s",test_period_arg.name,err == ESP_OK ? "OK!\r\n" : "failed!\r\n");
            err = esp_timer_delete(test_p_handle);
            ESP_LOGI(TAG,"要删除定时器名字：%s,是否删除成功：%s",test_period_arg.name,err ==ESP_OK ? "OK!\r\n" : "failed!\r\n");

        }
         // 低电平
         gpio_set_level(GPIO_NUM_16,0);
         vTaskDelay(1000 / portTICK_PERIOD_MS);
         //高电平
         gpio_set_level(GPIO_NUM_16,1);
         vTaskDelay(1000/portTICK_PERIOD_MS);
}
void test_timer_once_cb(void *arg) {

	int64_t tick = esp_timer_get_time();

	ESP_LOGI(TAG,"方法回调名字: %s , 距离定时器开启时间间隔 = %lld \r\n", test_once_arg.name, tick);

	esp_err_t err = esp_timer_delete(test_o_handle);
	ESP_LOGI(TAG,"要删除的定时器名字：%s , 是否删除成功：%s", test_once_arg.name,err == ESP_OK ? "ok!\r\n" : "failed!\r\n");
}

/***********************************************/
void app_main(void)
{
    esp_timer_init();// 使用定时器API函数，先调用接口初始化,(未调用此函数时，使用不受影响，不知为何)：原因：只需要再启动代码中调用此函数，使用其它esp_timer API不需要调用此函数

    gpio_pad_select_gpio(GPIO_NUM_16);
    gpio_set_direction(GPIO_NUM_16,GPIO_MODE_OUTPUT);

    //开始创建一个周期定时器并且执行
    esp_err_t err = esp_timer_create(&test_period_arg,&test_p_handle);
    err = esp_timer_start_periodic(test_p_handle,1000*1000);
    ESP_LOGI(TAG,"重复周期运行定时器创建状态码：%s",err == ESP_OK ? "OK!\r\n" : "failed!\r\n");
    //开创一个单词周期定时器并且执行

    err = esp_timer_create(&test_once_arg,&test_o_handle);
    err = esp_timer_start_once(test_o_handle,10*1000*1000);
    ESP_LOGI(TAG,"单次运行定时器创建状态码：%s",err == ESP_OK ? "OK!\r\n" : "failed!\r\n");
   
}
