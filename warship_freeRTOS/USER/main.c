
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



/////////////////////////UCOSII��������///////////////////////////////////
//START ����
//�����������ȼ�
#define START_TASK_PRIO      			30 //��ʼ��������ȼ�����Ϊ���
//���������ջ��С
#define START_STK_SIZE  				256
//������
TaskHandle_t start_Handler;
//������
void start_task(void *pdata);	


//LED����
//�����������ȼ�
#define LED_TASK_PRIO       			20
//���������ջ��С
#define LED_STK_SIZE  		    		128
//������
TaskHandle_t led_Handler;
//������
void led_task(void *pdata);


//������
//�����������ȼ�
#define MAIN_TASK_PRIO       			21 
//���������ջ��С
#define MAIN_STK_SIZE  					128
//������
TaskHandle_t main_Handler;
//������
void main_task(void *pdata);


//����������
//�����������ȼ�
#define TOUCH_TASK_PRIO       		 	22
//���������ջ��С
#define TOUCH_STK_SIZE  				128
//������
TaskHandle_t touch_Handler;
//������
void touch_task(void *pdata);


//������Ϣ��ʾ����
//�����������ȼ�
#define QMSGSHOW_TASK_PRIO    			23
//���������ջ��С
#define QMSGSHOW_STK_SIZE  		 		128
//������
TaskHandle_t qshow_Handler;
//������
void qmsgshow_task(void *pdata);


//�ź���������
//�����������ȼ�
#define FLAGS_TASK_PRIO       			24
//���������ջ��С
#define FLAGS_STK_SIZE  		 		128
//������
TaskHandle_t flags_Handler;
//������
void flags_task(void *pdata);


//����ɨ������
//�����������ȼ�
#define KEY_TASK_PRIO       			25
//���������ջ��С
#define KEY_STK_SIZE  					128
//������
TaskHandle_t key_Handler;
//������
void key_task(void *pdata);


//�����������ȼ�
#define UART_TASK_PRIO       			26 
//���������ջ��С
#define UART_STK_SIZE  					128
//������
TaskHandle_t uart_Handler;
//������
void uart_task(void *pdata);

TimerHandle_t   tmr1;			//�����ʱ��1
TimerHandle_t   tmr2;			//�����ʱ��2
TimerHandle_t   tmr3;			//�����ʱ��3

QueueHandle_t Key_Queue;	//��Ϣ���о��
#define KEY_Q_NUM   64   	//�������ݵ���Ϣ���е����� 

QueueHandle_t Message_Queue;	//��Ϣ���о��
#define MESSAGE_Q_NUM   64   	//�������ݵ���Ϣ���е����� 

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
 		LCD_ShowxNum(182,10,cpuusage/5,3,16,0);			//��ʾCPUʹ����  
		cpuusage=0;
		tcnt=0; 
	}
	cpuusage += 0; //todo
	tcnt++;				    
 	LCD_ShowxNum(182,30, my_mem_perused(SRAMIN),3,16,0);	//��ʾ�ڴ�ʹ����	 	  		 					    
	LCD_ShowxNum(182,50, uxQueueMessagesWaiting(Message_Queue),3,16,0X80);//��ʾ���е�ǰ�Ĵ�С		   
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
	static u8 msg_cnt=0;	//msg���	  
	
	p = mymalloc(SRAMIN, 16);	//����16���ֽڵ��ڴ�
	if(p)
	{
	 	sprintf((char*)p, "ALIENTEK %03d", msg_cnt);
		msg_cnt++;
        
		//err = OSQPost(q_msg,p);	//���Ͷ���
        err = xQueueSend(Message_Queue, &p, 0);
		if(err!=pdPASS) 	//����ʧ��
		{
			myfree(SRAMIN, p);	//�ͷ��ڴ�
			//OSTmrStop(&tmr3, OS_OPT_TMR_NONE, 0, &err);	//�ر������ʱ��3
 		}
	}
} 

//����������   
void ucos_load_main_ui(void)
{
	LCD_Clear(WHITE);	//����
 	POINT_COLOR=RED;	//��������Ϊ��ɫ 
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
		    
 	LCD_ShowString(5,125,240,16,16,"QUEUE MSG");//������Ϣ
	LCD_ShowString(5,150,240,16,16,"Message:");	 
	LCD_ShowString(5+130,125,240,16,16,"FLAGS");//�ź�����
	LCD_ShowString(5,225,240,16,16,"TOUCH");	//������
	//LCD_ShowString(5+130,225,240,16,16,"TMR2");	//������Ϣ
	
	POINT_COLOR=BLUE;//��������Ϊ��ɫ 
  	LCD_ShowString(150,10,200,16,16,"CPU:   %");	
   	LCD_ShowString(150,30,200,16,16,"MEM:   %");	
   	LCD_ShowString(150,50,200,16,16," Q :000");	

	delay_ms(300);
}	  


int main(void)
{	 	
    delay_init();	    	 //��ʱ������ʼ��	  
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//�����ж����ȼ�����Ϊ��2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
    uart_init(115200);	 	//���ڳ�ʼ��Ϊ115200

    LED_Init();					//��ʼ��LED 
    LCD_Init();			   		//��ʼ��LCD 
	EXTIX_Init();						//��ʼ���ⲿ�ж�
    BEEP_Init();				//��������ʼ��	
    KEY_Init();					//������ʼ�� 
    TPAD_Init(6);				//��ʼ��TPAD 
    my_mem_init(SRAMIN);		//��ʼ���ڲ��ڴ��
    tp_dev.init();				//��ʼ��������
    ucos_load_main_ui(); 		//����������

    //������ʼ����
    xTaskCreate((TaskFunction_t )start_task,            //������
                (const char*    )"start_task",          //��������
                (uint16_t       )START_STK_SIZE,        //�����ջ��С
                (void*          )NULL,                  //���ݸ��������Ĳ���
                (UBaseType_t    )START_TASK_PRIO,       //�������ȼ�
                (TaskHandle_t*  )&start_Handler);   //������              
    vTaskStartScheduler();          //�����������
}
 
///////////////////////////////////////////////////////////////////////////////////////////////////
void start_task(void *pdata)
{
	pdata = pdata; 	

    taskENTER_CRITICAL();           //�����ٽ���

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

    taskEXIT_CRITICAL();            //�˳��ٽ���
    vTaskDelete(start_Handler); //ɾ����ʼ����
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

//LED����
void led_task(void *pdata)
{
	u8 t;
    
	while(1)
	{
		t++;
		delay_ms(10);
		if(t==8)LED0=1;	//LED0��
		if(t==100)		//LED0��
		{
			t=0;
			LED0=0;
		}
        led_task_cnt++;
	}									 
}

//����������
void touch_task(void *pdata)
{	  	
 	u16 lastpos[2];		//���һ�ε����� 
 	
	while(1)
	{
		tp_dev.scan(0);
        
		if(tp_dev.sta&TP_PRES_DOWN)		//������������
		{	
		 	if( tp_dev.x[0]<(130-1 )&& tp_dev.y[0]<lcddev.height && tp_dev.y[0]>(220+1) )
			{			
				if(lastpos[0]==0XFFFF)
				{
					lastpos[0]=tp_dev.x[0];
					lastpos[1]=tp_dev.y[0]; 
				}
				//OS_ENTER_CRITICAL();//�����ٽ��,��ֹ��������,���LCD����,����Һ������.
				lcd_draw_bline(lastpos[0],lastpos[1],tp_dev.x[0],tp_dev.y[0],2,RED);//����
				//OS_EXIT_CRITICAL();
				lastpos[0]=tp_dev.x[0];
				lastpos[1]=tp_dev.y[0];     
			}
		}
        else lastpos[0]=0XFFFF;//û�д��� 

		vTaskDelay(10);	 
	}
}   

//������Ϣ��ʾ����
void qmsgshow_task(void *pdata)
{
	u8 *p;
    BaseType_t err;
    
	while(1)
	{
		//p = OSQPend(q_msg, 0, &err);//������Ϣ����
		err = xQueueReceive(Message_Queue, &p, portMAX_DELAY);
		LCD_ShowString(5,170,240,16,16,p);//��ʾ��Ϣ
		
 		if(err == pdPASS) myfree(SRAMIN, p);	  
		vTaskDelay(50);	 
        qmsg_task_cnt++;
	}									 
}

//������
void main_task(void *pdata)
{							 
	BaseType_t err;
 	u8 tmr3sta=0;	//�����ʱ��3����״̬
	
 	//tmr1 = OSTmrCreate(10,10,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr1_callback,0,"tmr1",&err);//100msִ��һ��
    tmr1 = xTimerCreate((const char*		)"tmr1",
    				    (TickType_t			)100,
    		            (UBaseType_t		)pdTRUE,
    		            (void*				)1,
    		            (TimerCallbackFunction_t)tmr1_callback); //���ڶ�ʱ��������1s(1000��ʱ�ӽ���)������ģʽ
    
	//tmr2 = OSTmrCreate(10,900,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr2_callback,0,"tmr2",&err);//3000msִ��һ��
    tmr2 = xTimerCreate((const char*		)"tmr1",
    				    (TickType_t			)3000,
    		            (UBaseType_t		)pdTRUE,
    		            (void*				)2,
    		            (TimerCallbackFunction_t)tmr2_callback); //���ڶ�ʱ��������1s(1000��ʱ�ӽ���)������ģʽ
    
	//tmr3 = OSTmrCreate(10,20,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr3_callback,0,"tmr3",&err);//500msִ��һ��
    tmr3 = xTimerCreate((const char*		)"tmr1",
    				    (TickType_t			)500,
    		            (UBaseType_t		)pdTRUE,
    		            (void*				)3,
    		            (TimerCallbackFunction_t)tmr3_callback); //���ڶ�ʱ��������1s(1000��ʱ�ӽ���)������ģʽ

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
			case 1://����DS1
				LED1=!LED1;
				break;
                
			case 2://���������ʱ��3	 
				tmr3sta=!tmr3sta;
				if(tmr3sta) vTaskSuspend(qshow_Handler);
				else vTaskResume(qshow_Handler);
 				break;
                
			case 3://���
 				LCD_Fill(0,221,129,lcddev.height,WHITE);
				break;
                
			case 4://У׼
				//OSTaskSuspend(touch_task_tcb, &err);						//������������		 
				//OSTaskSuspend(qmsg_task_tcb, &err);	 				//���������Ϣ��ʾ����		 
 				//OSTmrStop(&tmr1, OS_OPT_TMR_NONE, 0, &err);				//�ر������ʱ��1
 				
 				if((tp_dev.touchtype&0X80)==0) TP_Adjust();   
				//OSTmrStart(&tmr1, &err);				//���¿��������ʱ��1
 				//OSTaskResume(touch_task_tcb, &err);		//���
 				//OSTaskResume(qmsg_task_tcb, &err); 	//���
				ucos_load_main_ui();				//���¼���������		 
				break;
                
			case 5://clear log info, TPAD
				LCD_Fill(131, 221, lcddev.width-1, lcddev.height-1, WHITE);
				break;				 
				
		}  
        
		vTaskDelay(10);
	}
}		

//�ź�������������
void flags_task(void *pdata)
{	
    uint32_t flags;
	BaseType_t err;
    
	while(1)
	{
		err = xTaskNotifyWait((uint32_t	)0x00,				//���뺯����ʱ���������bit
							(uint32_t	)ULONG_MAX,			//�˳�������ʱ��������е�bit
							(uint32_t*	)&flags,		//��������ֵ֪ͨ
							(TickType_t	)portMAX_DELAY);	//����ʱ��

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

//����ɨ������
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
            //OSMboxPost(msg_key,(void*)key);//������Ϣ
            xTaskNotify((TaskHandle_t   )flags_Handler,//��������֪ͨ��������
                        (uint32_t       )(1<<key),            //Ҫ���µ�bit
                        (eNotifyAction  )eSetBits);             //����ָ����bit
            key_task_cnt++;
        }
 		vTaskDelay(100);
	}
}

