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
                                            .callback = &

                                        }


static xQueueHandle gpio_evt_queue = NULL;//设置消息队列，用于传递中断信息
/**********
 * 2022/9/27
 * GPIO状态
 * GPIO4:上拉输入，上升沿和下降沿中断
 * GPIO5:上拉输入，下降沿中断
 * GPIO15:推挽输出
 * GPIO16:推挽输出
 * 实现功能:GPIO15连接到GPIO4  GPIO16连接到GPIO5,利用输出信号，触发GPIO4,GPIO5上的中断
 * 学习结论和疑问：1、(已经解决，原因是:配置结构体初值出错)配置输入接口结构体时，无论配置成上升沿还是下降沿触发都不能读到状态变化，
 * 设置成任一边沿模式能读到状态变化，不知道原因。
 * 反复测试得到结果是：结构体方式配置的上升下降触发不能使用，但是通过gpio_set_intr_type设置能有效果
 * 可能的原因https://blog.csdn.net/m0_50064262/article/details/115288638 该文章第三部分有可能解释
 * 2、配置GPIO时，位掩码设置不能理解，有道云相应GPIO文章一有讲解用法，原理不懂
 * *******************************************************************/
//真正的中断服务函数，这里只做一件事，通过队列把中断消息打包发送出去
static void IRAM_ATTR gpio_isr_handle(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue,&gpio_num,NULL);
}



//实际起作用的gpio中断处理函数，一直等待中断发送信息然后到这里处理信息
static void gpio_task_example(void *arg)
{
   uint32_t io_num;
   for(;;)
   {
    if(xQueueReceive(gpio_evt_queue,&io_num,portMAX_DELAY)) //堵塞等待中断给信息
    {
        //打印信息，中断实际上的处理在这里进行
        ESP_LOGI(TAG,"GPIO[%d]intr,val:%d\n",io_num,gpio_get_level(io_num));
    }
   }

}


/********************************************************
 * 
 **初始化GPIO****
 *******************************************************/
static void configure_gpio()
{
    ESP_LOGI(TAG, "Example configured to  GPIO INR!");
    /*****
     * 配置了两个输出GPIO
     * 
    */
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,//不启用中断
        .mode = GPIO_MODE_OUTPUT,//输出模式
        .pin_bit_mask = GPIO_OUTPUT_PIN_SEL,//设置gpio,可以同时设置多个端口
        .pull_down_en = 0, //不上拉
        .pull_up_en = 0 //不下拉
    };
    gpio_config(&io_conf);//使能端口
    /*****
     * 配置了两个输入GPIO
     * 
    */
    gpio_config_t io_conf2 = {
        .intr_type = GPIO_INTR_POSEDGE, //启用上升沿沿中断
        .mode = GPIO_MODE_INPUT,//输入模式
        .pin_bit_mask = GPIO_INPUT_PIN_SEL,//设置gpio,可以同时设置多个端口
        .pull_down_en = 1, //下拉
        .pull_up_en = 0 //不上拉
    };
    gpio_config(&io_conf2);//使能端口
 //   gpio_set_intr_type(GPIO_INPUT_IO_0,GPIO_INTR_ANYEDGE);//输入0设置为任意边沿中断
 //   gpio_set_intr_type(GPIO_INPUT_IO_1,GPIO_INTR_ANYEDGE);//输入1设置为任意边沿中断
    
}




void app_main(void)
{

    /* Configure the peripheral according to the LED type */
    configure_gpio();
//创建用于传递中断信息的消息队列
    gpio_evt_queue = xQueueCreate(10,sizeof(uint32_t));//创建消息队列，第一个参数队列长度，队列中消息单元的大小
    
    //创建中断处理线程，即GPIO处理线程
  //  xTaskCreatePinnedToCore(*** 这个在这里没有使用 xTaskCreatePinnedToCore(task,           //任务函数
    		             //  "task1",         //这个参数没有什么作用，仅作为任务的描述
			             //   2048,            //任务栈的大小
			            //   "task1",         //传给任务函数的参数
			            //   2,              //优先级，数字越大优先级越高
			             //  NULL,            //传回的句柄，不需要的话可以为NULL
			             //  tskNO_AFFINITY); //指定运行任务的ＣＰＵ，使用这个宏表示不会固定到任何核上
             // ************************************************************/
    xTaskCreate(
        gpio_task_example,//任务函数，这里是实际执行中断的函数
        "this is a example for gpio int",//对中断函数或者任务的描述
        2048,//任务栈
        NULL,//传递参数，一般是指针
        10,//中断优先级，数值越大优先级越高
        NULL//传回句柄，不需要为NULL
    );
    //安装GPIO中断驱动（参数为中断的优先级）
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    
    //给指定的GPIO绑定中断服务函数
    gpio_isr_handler_add(GPIO_INPUT_IO_0,gpio_isr_handle,(void*)GPIO_INPUT_IO_0);
    gpio_isr_handler_add(GPIO_INPUT_IO_1,gpio_isr_handle,(void*)GPIO_INPUT_IO_1);
// 打印内存使用情况
    ESP_LOGI(TAG,"Minimum free heap size:%d bytes\n",esp_get_minimum_free_heap_size());
    int cnt = 0;

    while (1) {              
        //ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
    //    ESP_LOGI(TAG, "Turning the LED %d!", s_led_state );
    ESP_LOGI(TAG,"cnt:%d\n",cnt++);
    vTaskDelay(1000/portTICK_RATE_MS);
    gpio_set_level(GPIO_OUTPUT_IO_0,cnt%2);//设置GPIO电平
    gpio_set_level(GPIO_OUTPUT_IO_1,cnt%2);
       
    }
}
