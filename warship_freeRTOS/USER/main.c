
#include "sys.h"
#include "delay.h"
#include "usart.h" 
#include "led.h" 		 	 
#include "lcd.h"  
#include "key.h"     
#include "malloc.h"
#include "string.h"
#include "tpad.h"    
#include "beep.h"   
#include "touch.h"    

#include "FreeRTOS.h"
#include "task.h"
#include "limits.h"
#include "timers.h"
#include "queue.h"



/////////////////////////UCOSII任务设置///////////////////////////////////
//START 任务
//设置任务优先级
#define START_TASK_PRIO      			30 //开始任务的优先级设置为最低
//设置任务堆栈大小
#define START_STK_SIZE  				256
//任务句柄
TaskHandle_t start_Handler;
//任务函数
void start_task(void *pdata);	


//LED任务
//设置任务优先级
#define LED_TASK_PRIO       			20
//设置任务堆栈大小
#define LED_STK_SIZE  		    		128
//任务句柄
TaskHandle_t led_Handler;
//任务函数
void led_task(void *pdata);


//主任务
//设置任务优先级
#define MAIN_TASK_PRIO       			21 
//设置任务堆栈大小
#define MAIN_STK_SIZE  					128
//任务句柄
TaskHandle_t main_Handler;
//任务函数
void main_task(void *pdata);


//触摸屏任务
//设置任务优先级
#define TOUCH_TASK_PRIO       		 	22
//设置任务堆栈大小
#define TOUCH_STK_SIZE  				128
//任务句柄
TaskHandle_t touch_Handler;
//任务函数
void touch_task(void *pdata);


//队列消息显示任务
//设置任务优先级
#define QMSGSHOW_TASK_PRIO    			23
//设置任务堆栈大小
#define QMSGSHOW_STK_SIZE  		 		128
//任务句柄
TaskHandle_t qshow_Handler;
//任务函数
void qmsgshow_task(void *pdata);


//信号量集任务
//设置任务优先级
#define FLAGS_TASK_PRIO       			24
//设置任务堆栈大小
#define FLAGS_STK_SIZE  		 		128
//任务句柄
TaskHandle_t flags_Handler;
//任务函数
void flags_task(void *pdata);


//按键扫描任务
//设置任务优先级
#define KEY_TASK_PRIO       			25
//设置任务堆栈大小
#define KEY_STK_SIZE  					128
//任务句柄
TaskHandle_t key_Handler;
//任务函数
void key_task(void *pdata);


//设置任务优先级
#define UART_TASK_PRIO       			26 
//设置任务堆栈大小
#define UART_STK_SIZE  					128
//任务句柄
TaskHandle_t uart_Handler;
//任务函数
void uart_task(void *pdata);

TimerHandle_t   tmr1;			//软件定时器1
TimerHandle_t   tmr2;			//软件定时器2
TimerHandle_t   tmr3;			//软件定时器3

QueueHandle_t Key_Queue;	//信息队列句柄
#define KEY_Q_NUM   64   	//发送数据的消息队列的数量 

QueueHandle_t Message_Queue;	//信息队列句柄
#define MESSAGE_Q_NUM   64   	//发送数据的消息队列的数量 

u8 tick_cnt = 0;
u8 led_task_cnt = 0;
u8 key_task_cnt = 0;
u8 uart_task_cnt = 0;
u8 qmsg_task_cnt = 0;
u8 main_task_cnt = 0;


void lcd_print_log(u8 *log_str)
{
    static u8 line = 0;

    if (log_str == NULL) {
        LCD_Fill(131, 221, lcddev.width-1, lcddev.height-1, WHITE);
        return ;
    }

    LCD_ShowString(5+130, 225 + 20*line, 240, 16, 16, log_str);
    if (line++ > 20) {
        line = 0;
        LCD_Fill(131, 221, lcddev.width-1, lcddev.height-1, WHITE);
    }
}



//////////////////////////////////////////////////////////////////////////////

void tmr1_callback(TimerHandle_t xTimer)
{
 	static u16 cpuusage=0;
	static u8 tcnt=0;	    
    
	POINT_COLOR=BLUE;
	if(tcnt==5)
	{
 		LCD_ShowxNum(182,10,cpuusage/5,3,16,0);			//显示CPU使用率  
		cpuusage=0;
		tcnt=0; 
	}
	cpuusage += 0; //todo
	tcnt++;				    
 	LCD_ShowxNum(182,30, my_mem_perused(SRAMIN),3,16,0);	//显示内存使用率	 	  		 					    
	LCD_ShowxNum(182,50, uxQueueMessagesWaiting(Message_Queue),3,16,0X80);//显示队列当前的大小		   
}

void tmr2_callback(TimerHandle_t xTimer)
{	
    u8 print_buf[32];
    
    sprintf((char*)print_buf, "tmr2: tick_cnt %d, led %d\r\n", tick_cnt, led_task_cnt);
    lcd_print_log(print_buf);
    //printf(print_buf);
    
    sprintf((char*)print_buf, "tmr2: uart %d, main %d, key %d\r\n", uart_task_cnt, qmsg_task_cnt, key_task_cnt);
    lcd_print_log(print_buf);
    //printf(print_buf);
}

void tmr3_callback(TimerHandle_t xTimer)
{	
	u8* p;	 
    BaseType_t err;
	static u8 msg_cnt=0;	//msg编号	  
	
	p = mymalloc(SRAMIN, 16);	//申请16个字节的内存
	if(p)
	{
	 	sprintf((char*)p, "ALIENTEK %03d", msg_cnt);
		msg_cnt++;
        
		//err = OSQPost(q_msg,p);	//发送队列
        err = xQueueSend(Message_Queue, &p, 0);
		if(err!=pdPASS) 	//发送失败
		{
			myfree(SRAMIN, p);	//释放内存
			//OSTmrStop(&tmr3, OS_OPT_TMR_NONE, 0, &err);	//关闭软件定时器3
 		}
	}
} 

//加载主界面   
void ucos_load_main_ui(void)
{
	LCD_Clear(WHITE);	//清屏
 	POINT_COLOR=RED;	//设置字体为红色 
	LCD_ShowString(10,10,200,16,16,"WarShip STM32");	
	LCD_ShowString(10,30,200,16,16,"FreeRTOS DEMO");	
	LCD_ShowString(10,50,200,16,16,"ATOM@ALIENTEK");
   	LCD_ShowString(10,75,240,16,16,"TPAD:CLR_LOG   KEY_UP:ADJUST");	
   	LCD_ShowString(10,95,240,16,16,"KEY0:LED KEY1:Q_SW KEY2:CLR");	
 	LCD_DrawLine(0,70,lcddev.width,70);
	LCD_DrawLine(130,0,130,70);

 	LCD_DrawLine(0,120,lcddev.width,120);
 	LCD_DrawLine(0,220,lcddev.width,220);
	LCD_DrawLine(130,120,130,lcddev.height);
		    
 	LCD_ShowString(5,125,240,16,16,"QUEUE MSG");//队列消息
	LCD_ShowString(5,150,240,16,16,"Message:");	 
	LCD_ShowString(5+130,125,240,16,16,"FLAGS");//信号量集
	LCD_ShowString(5,225,240,16,16,"TOUCH");	//触摸屏
	//LCD_ShowString(5+130,225,240,16,16,"TMR2");	//队列消息
	
	POINT_COLOR=BLUE;//设置字体为蓝色 
  	LCD_ShowString(150,10,200,16,16,"CPU:   %");	
   	LCD_ShowString(150,30,200,16,16,"MEM:   %");	
   	LCD_ShowString(150,50,200,16,16," Q :000");	

	delay_ms(300);
}	  


int main(void)
{	 	
    delay_init();	    	 //延时函数初始化	  
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
    uart_init(115200);	 	//串口初始化为115200

    LED_Init();					//初始化LED 
    LCD_Init();			   		//初始化LCD 
	EXTIX_Init();						//初始化外部中断
    BEEP_Init();				//蜂鸣器初始化	
    KEY_Init();					//按键初始化 
    TPAD_Init(6);				//初始化TPAD 
    my_mem_init(SRAMIN);		//初始化内部内存池
    tp_dev.init();				//初始化触摸屏
    ucos_load_main_ui(); 		//加载主界面

    //创建开始任务
    xTaskCreate((TaskFunction_t )start_task,            //任务函数
                (const char*    )"start_task",          //任务名称
                (uint16_t       )START_STK_SIZE,        //任务堆栈大小
                (void*          )NULL,                  //传递给任务函数的参数
                (UBaseType_t    )START_TASK_PRIO,       //任务优先级
                (TaskHandle_t*  )&start_Handler);   //任务句柄              
    vTaskStartScheduler();          //开启任务调度
}
 
///////////////////////////////////////////////////////////////////////////////////////////////////
void start_task(void *pdata)
{
	pdata = pdata; 	

    taskENTER_CRITICAL();           //进入临界区

    Message_Queue = xQueueCreate(MESSAGE_Q_NUM, sizeof( struct u8 * ));        
    Key_Queue = xQueueCreate(KEY_Q_NUM, sizeof(uint32_t));        
    
 	//OSTaskCreate(led_task,(void *)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);		
    xTaskCreate((TaskFunction_t )led_task,             
                (const char*    )"led_task",           
                (uint16_t       )LED_STK_SIZE,        
                (void*          )NULL,                  
                (UBaseType_t    )LED_TASK_PRIO,        
                (TaskHandle_t*  )&led_Handler);   	

 	//OSTaskCreate(main_task,(void *)0,(OS_STK*)&MAIN_TASK_STK[MAIN_STK_SIZE-1],MAIN_TASK_PRIO);	 	
    xTaskCreate((TaskFunction_t )main_task,             
                (const char*    )"main_task",           
                (uint16_t       )MAIN_STK_SIZE,        
                (void*          )NULL,                  
                (UBaseType_t    )MAIN_TASK_PRIO,        
                (TaskHandle_t*  )&main_Handler);   	
    
 	//OSTaskCreate(touch_task,(void *)0,(OS_STK*)&TOUCH_TASK_STK[TOUCH_STK_SIZE-1],TOUCH_TASK_PRIO);
    xTaskCreate((TaskFunction_t )touch_task,             
                (const char*    )"touch_task",           
                (uint16_t       )TOUCH_STK_SIZE,        
                (void*          )NULL,                  
                (UBaseType_t    )TOUCH_TASK_PRIO,        
                (TaskHandle_t*  )&touch_Handler);   	
    
    //OSTaskCreate(qmsgshow_task,(void *)0,(OS_STK*)&QMSGSHOW_TASK_STK[QMSGSHOW_STK_SIZE-1],QMSGSHOW_TASK_PRIO);	 	
    xTaskCreate((TaskFunction_t )qmsgshow_task,             
                (const char*    )"qmsgshow_task",           
                (uint16_t       )QMSGSHOW_STK_SIZE,        
                (void*          )NULL,                  
                (UBaseType_t    )QMSGSHOW_TASK_PRIO,        
                (TaskHandle_t*  )&qshow_Handler);   	

    xTaskCreate((TaskFunction_t )flags_task,             
                (const char*    )"flags_task",           
                (uint16_t       )FLAGS_STK_SIZE,        
                (void*          )NULL,                  
                (UBaseType_t    )FLAGS_TASK_PRIO,        
                (TaskHandle_t*  )&flags_Handler);   	
 	
 	//OSTaskCreate(key_task,(void *)0,(OS_STK*)&KEY_TASK_STK[KEY_STK_SIZE-1],KEY_TASK_PRIO);	 	
    xTaskCreate((TaskFunction_t )key_task,             
                (const char*    )"key_task",           
                (uint16_t       )KEY_STK_SIZE,        
                (void*          )NULL,                  
                (UBaseType_t    )KEY_TASK_PRIO,        
                (TaskHandle_t*  )&key_Handler);   	
 	
    //OSTaskCreate(uart_task,(void *)0,(OS_STK*)&UART_TASK_STK[UART_STK_SIZE-1],UART_TASK_PRIO);    
    xTaskCreate((TaskFunction_t )uart_task,             
                (const char*    )"uart_task",           
                (uint16_t       )UART_STK_SIZE,        
                (void*          )NULL,                  
                (UBaseType_t    )UART_TASK_PRIO,        
                (TaskHandle_t*  )&uart_Handler);   	

    taskEXIT_CRITICAL();            //退出临界区
    vTaskDelete(start_Handler); //删除开始任务
}

#if XX
void uart_task(void *pdata)
{   
    while(1)
    {
        //printf("\r\nNIX>");
        delay_ms(100);
        uart_task_cnt++;
    }
}
#endif

//LED任务
void led_task(void *pdata)
{
	u8 t;
    
	while(1)
	{
		t++;
		delay_ms(10);
		if(t==8)LED0=1;	//LED0灭
		if(t==100)		//LED0亮
		{
			t=0;
			LED0=0;
		}
        led_task_cnt++;
	}									 
}

//触摸屏任务
void touch_task(void *pdata)
{	  	
 	u16 lastpos[2];		//最后一次的数据 
 	
	while(1)
	{
		tp_dev.scan(0);
        
		if(tp_dev.sta&TP_PRES_DOWN)		//触摸屏被按下
		{	
		 	if( tp_dev.x[0]<(130-1 )&& tp_dev.y[0]<lcddev.height && tp_dev.y[0]>(220+1) )
			{			
				if(lastpos[0]==0XFFFF)
				{
					lastpos[0]=tp_dev.x[0];
					lastpos[1]=tp_dev.y[0]; 
				}
				//OS_ENTER_CRITICAL();//进入临界段,防止其他任务,打断LCD操作,导致液晶乱序.
				lcd_draw_bline(lastpos[0],lastpos[1],tp_dev.x[0],tp_dev.y[0],2,RED);//画线
				//OS_EXIT_CRITICAL();
				lastpos[0]=tp_dev.x[0];
				lastpos[1]=tp_dev.y[0];     
			}
		}
        else lastpos[0]=0XFFFF;//没有触摸 

		vTaskDelay(10);	 
	}
}   

//队列消息显示任务
void qmsgshow_task(void *pdata)
{
	u8 *p;
    BaseType_t err;
    
	while(1)
	{
		//p = OSQPend(q_msg, 0, &err);//请求消息队列
		err = xQueueReceive(Message_Queue, &p, portMAX_DELAY);
		LCD_ShowString(5,170,240,16,16,p);//显示消息
		
 		if(err == pdPASS) myfree(SRAMIN, p);	  
		vTaskDelay(50);	 
        qmsg_task_cnt++;
	}									 
}

//主任务
void main_task(void *pdata)
{							 
	BaseType_t err;
 	u8 tmr3sta=0;	//软件定时器3开关状态
	
 	//tmr1 = OSTmrCreate(10,10,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr1_callback,0,"tmr1",&err);//100ms执行一次
    tmr1 = xTimerCreate((const char*		)"tmr1",
    				    (TickType_t			)100,
    		            (UBaseType_t		)pdTRUE,
    		            (void*				)1,
    		            (TimerCallbackFunction_t)tmr1_callback); //周期定时器，周期1s(1000个时钟节拍)，周期模式
    
	//tmr2 = OSTmrCreate(10,900,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr2_callback,0,"tmr2",&err);//3000ms执行一次
    tmr2 = xTimerCreate((const char*		)"tmr1",
    				    (TickType_t			)3000,
    		            (UBaseType_t		)pdTRUE,
    		            (void*				)2,
    		            (TimerCallbackFunction_t)tmr2_callback); //周期定时器，周期1s(1000个时钟节拍)，周期模式
    
	//tmr3 = OSTmrCreate(10,20,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr3_callback,0,"tmr3",&err);//500ms执行一次
    tmr3 = xTimerCreate((const char*		)"tmr1",
    				    (TickType_t			)500,
    		            (UBaseType_t		)pdTRUE,
    		            (void*				)3,
    		            (TimerCallbackFunction_t)tmr3_callback); //周期定时器，周期1s(1000个时钟节拍)，周期模式

    xTimerStart(tmr1, 0);
    xTimerStart(tmr2, 0);
    xTimerStart(tmr3, 0);
    
 	while(1)
	{
        uint32_t NotifyValue;
        uint32_t key = 0;

        main_task_cnt++;            
		err = xQueueReceive(Key_Queue, &NotifyValue, portMAX_DELAY);
		if(err==pdPASS) {
		   if(NotifyValue&(1<<1)) key = 1;	
           if(NotifyValue&(1<<2)) key = 2;	
           if(NotifyValue&(1<<3)) key = 3;	
           if(NotifyValue&(1<<4)) key = 4;	
           if(NotifyValue&(1<<5)) key = 5;	
		} else {
		    key = 0;
        }
        
		switch(key)
		{
			case 1://控制DS1
				LED1=!LED1;
				break;
                
			case 2://控制软件定时器3	 
				tmr3sta=!tmr3sta;
				if(tmr3sta) vTaskSuspend(qshow_Handler);
				else vTaskResume(qshow_Handler);
 				break;
                
			case 3://清除
 				LCD_Fill(0,221,129,lcddev.height,WHITE);
				break;
                
			case 4://校准
				//OSTaskSuspend(touch_task_tcb, &err);						//挂起触摸屏任务		 
				//OSTaskSuspend(qmsg_task_tcb, &err);	 				//挂起队列信息显示任务		 
 				//OSTmrStop(&tmr1, OS_OPT_TMR_NONE, 0, &err);				//关闭软件定时器1
 				
 				if((tp_dev.touchtype&0X80)==0) TP_Adjust();   
				//OSTmrStart(&tmr1, &err);				//重新开启软件定时器1
 				//OSTaskResume(touch_task_tcb, &err);		//解挂
 				//OSTaskResume(qmsg_task_tcb, &err); 	//解挂
				ucos_load_main_ui();				//重新加载主界面		 
				break;
                
			case 5://clear log info, TPAD
				LCD_Fill(131, 221, lcddev.width-1, lcddev.height-1, WHITE);
				break;				 
				
		}  
        
		vTaskDelay(10);
	}
}		

//信号量集处理任务
void flags_task(void *pdata)
{	
    uint32_t flags;
	BaseType_t err;
    
	while(1)
	{
		err = xTaskNotifyWait((uint32_t	)0x00,				//进入函数的时候不清除任务bit
							(uint32_t	)ULONG_MAX,			//退出函数的时候清除所有的bit
							(uint32_t*	)&flags,		//保存任务通知值
							(TickType_t	)portMAX_DELAY);	//阻塞时间

        //printf("flags_task> flags = 0x%x \r\n", flags);
 		if(flags&(1<<1)) LCD_ShowString(140,162,240,16,16,"KEY0 DOWN  "); 
		if(flags&(1<<2)) LCD_ShowString(140,162,240,16,16,"KEY1 DOWN  "); 
		if(flags&(1<<3)) LCD_ShowString(140,162,240,16,16,"KEY2 DOWN  "); 
		if(flags&(1<<4)) LCD_ShowString(140,162,240,16,16,"KEY_UP DOWN"); 
		if(flags&(1<<5)) LCD_ShowString(140,162,240,16,16,"TPAD DOWN  "); 

		BEEP=1;
		vTaskDelay(50);
		BEEP=0;

        err = xQueueSend(Key_Queue, &flags, 0);
 	}
}

//按键扫描任务
void key_task(void *pdata)
{	
	u8 key;	
    
	while(1)
	{
		key = KEY_Scan(0);   
		if(key==0)
		{
			if(TPAD_Scan(0))key=5;
		}
        
		if(key) {
            //OSMboxPost(msg_key,(void*)key);//发送消息
            xTaskNotify((TaskHandle_t   )flags_Handler,//接收任务通知的任务句柄
                        (uint32_t       )(1<<key),            //要更新的bit
                        (eNotifyAction  )eSetBits);             //更新指定的bit
            key_task_cnt++;
        }
 		vTaskDelay(100);
	}
}

