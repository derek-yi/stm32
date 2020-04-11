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
#include "includes.h"  



/////////////////////////UCOSII任务设置///////////////////////////////////
//START 任务
//设置任务优先级
#define START_TASK_PRIO      			30 //开始任务的优先级设置为最低
//设置任务堆栈大小
#define START_STK_SIZE  				128
//任务堆栈	
OS_STK START_TASK_STK[START_STK_SIZE];
//任务函数
void start_task(void *pdata);	


//LED任务
//设置任务优先级
#define LED_TASK_PRIO       			20
//设置任务堆栈大小
#define LED_STK_SIZE  		    		128
//任务堆栈
OS_STK LED_TASK_STK[LED_STK_SIZE];
//任务函数
void led_task(void *pdata);


//主任务
//设置任务优先级
#define MAIN_TASK_PRIO       			21 
//设置任务堆栈大小
#define MAIN_STK_SIZE  					128
//任务堆栈	
OS_STK MAIN_TASK_STK[MAIN_STK_SIZE];
//任务函数
void main_task(void *pdata);


//触摸屏任务
//设置任务优先级
#define TOUCH_TASK_PRIO       		 	22
//设置任务堆栈大小
#define TOUCH_STK_SIZE  				128
//任务堆栈	
OS_STK TOUCH_TASK_STK[TOUCH_STK_SIZE];
//任务函数
void touch_task(void *pdata);


//队列消息显示任务
//设置任务优先级
#define QMSGSHOW_TASK_PRIO    			23
//设置任务堆栈大小
#define QMSGSHOW_STK_SIZE  		 		128
//任务堆栈	
OS_STK QMSGSHOW_TASK_STK[QMSGSHOW_STK_SIZE];
//任务函数
void qmsgshow_task(void *pdata);


//信号量集任务
//设置任务优先级
#define FLAGS_TASK_PRIO       			24
//设置任务堆栈大小
#define FLAGS_STK_SIZE  		 		128
//任务堆栈	
OS_STK FLAGS_TASK_STK[FLAGS_STK_SIZE];
//任务函数
void flags_task(void *pdata);


//按键扫描任务
//设置任务优先级
#define KEY_TASK_PRIO       			25
//设置任务堆栈大小
#define KEY_STK_SIZE  					128
//任务堆栈	
OS_STK KEY_TASK_STK[KEY_STK_SIZE];
//任务函数
void key_task(void *pdata);


//设置任务优先级
#define UART_TASK_PRIO       			26 
//设置任务堆栈大小
#define UART_STK_SIZE  					128
//任务堆栈	
OS_STK UART_TASK_STK[UART_STK_SIZE];
//任务函数
void uart_task(void *pdata);

OS_TMR   tmr1;			//软件定时器1
OS_TMR   tmr2;			//软件定时器2
OS_TMR   tmr3;			//软件定时器3
OS_FLAG_GRP flags_key;	//按键信号量集


OS_TCB *start_task_tcb = NULL;
OS_TCB *qmsg_task_tcb = NULL;
OS_TCB *touch_task_tcb = NULL;
OS_TCB *led_task_tcb = NULL;
OS_TCB *main_task_tcb = NULL;
OS_TCB *uart_task_tcb = NULL;
OS_TCB *flags_task_tcb = NULL;
OS_TCB *key_task_tcb = NULL;

extern int tick_cnt;
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

//每100ms执行一次,用于显示CPU使用率和内存使用率		   
void tmr1_callback(OS_TMR *ptmr,void *p_arg) 
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
	cpuusage += OSStatTaskCPUUsage;
	tcnt++;				    
 	LCD_ShowxNum(182,30,my_mem_perused(SRAMIN),3,16,0);	//显示内存使用率	 	  		 					    
	LCD_ShowxNum(182,50,qmsg_task_tcb->MsgQ.NbrEntries,3,16,0X80);//显示队列当前的大小		   
}

void tmr2_callback(OS_TMR *ptmr,void *p_arg) 
{	
    u8 print_buf[32];
    
    sprintf((char*)print_buf, "tmr2: tick_cnt %d, led %d\r\n", tick_cnt, led_task_cnt);
    lcd_print_log(print_buf);
    //printf(print_buf);
    
    sprintf((char*)print_buf, "tmr2: uart %d, main %d, key %d\r\n", uart_task_cnt, qmsg_task_cnt, key_task_cnt);
    lcd_print_log(print_buf);
    //printf(print_buf);
}

//软件定时器3的回调函数，发送消息给q_msg				  	   
void tmr3_callback(OS_TMR *ptmr,void *p_arg) 
{	
	u8* p;	 
	OS_ERR err; 
	static u8 msg_cnt=0;	//msg编号	  
	
	p = mymalloc(SRAMIN, 16);	//申请16个字节的内存
	if(p)
	{
	 	sprintf((char*)p, "ALIENTEK %03d", msg_cnt);
		msg_cnt++;
		//err = OSQPost(q_msg,p);	//发送队列
        OSTaskQPost(qmsg_task_tcb, p, 16, OS_OPT_POST_FIFO, &err);
		if(err!=OS_ERR_NONE) 	//发送失败
		{
			myfree(SRAMIN, p);	//释放内存
			OSTmrStop(&tmr3, OS_OPT_TMR_NONE, 0, &err);	//关闭软件定时器3
 		}
	}
} 

//加载主界面   
void ucos_load_main_ui(void)
{
	LCD_Clear(WHITE);	//清屏
 	POINT_COLOR=RED;	//设置字体为红色 
	LCD_ShowString(10,10,200,16,16,"WarShip STM32");	
	LCD_ShowString(10,30,200,16,16,"UCOS3 DEMO");	
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
    OS_ERR err; 
    
    delay_init();	    	 //延时函数初始化	  
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
    uart_init(115200);	 	//串口初始化为115200

    LED_Init();					//初始化LED 
    LCD_Init();			   		//初始化LCD 
    BEEP_Init();				//蜂鸣器初始化	
    KEY_Init();					//按键初始化 
    TPAD_Init(6);				//初始化TPAD 
    my_mem_init(SRAMIN);		//初始化内部内存池
    tp_dev.init();				//初始化触摸屏
    ucos_load_main_ui(); 		//加载主界面
    OSInit(&err);               //初始化UCOSII

    start_task_tcb = mymalloc(SRAMIN, sizeof(OS_TCB));
    //OSTaskCreate(start_task,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );//创建起始任务
    OSTaskCreate(start_task_tcb, "start", start_task, (void *)0, START_TASK_PRIO, 
                (OS_STK *)&START_TASK_STK[START_STK_SIZE-1], START_STK_SIZE/8, START_STK_SIZE, 0, 
                0, NULL, OS_OPT_TASK_NONE, &err);
    OSStart(&err);	  
}
 
///////////////////////////////////////////////////////////////////////////////////////////////////
//开始任务


void start_task(void *pdata)
{
    CPU_SR cpu_sr=0;
	OS_ERR err;	    
    
	pdata = pdata; 	

    CPU_Init();
#if OS_CFG_STAT_TASK_EN > 0u
   OSStatTaskCPUUsageInit(&err);    //统计任务                
#endif

#if	OS_CFG_SCHED_ROUND_ROBIN_EN  //当使用时间片轮转的时候
     //使能时间片轮转调度功能,时间片长度为1个系统时钟节拍，既1*5=5ms
    OSSchedRoundRobinCfg(DEF_ENABLED,1,&err);  
#endif	

    led_task_tcb = mymalloc(SRAMIN, sizeof(OS_TCB));
    main_task_tcb = mymalloc(SRAMIN, sizeof(OS_TCB));
    uart_task_tcb = mymalloc(SRAMIN, sizeof(OS_TCB));
    flags_task_tcb = mymalloc(SRAMIN, sizeof(OS_TCB));
    key_task_tcb = mymalloc(SRAMIN, sizeof(OS_TCB));
    qmsg_task_tcb = mymalloc(SRAMIN, sizeof(OS_TCB));
    touch_task_tcb = mymalloc(SRAMIN, sizeof(OS_TCB));

 	//flags_key = OSFlagCreate(0,&err); 	//创建信号量集		
    OSFlagCreate(&flags_key, "null", 0, &err);

    CPU_CRITICAL_ENTER();
 	//OSTaskCreate(led_task,(void *)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);		
    OSTaskCreate(led_task_tcb, "led", led_task, (void *)0, LED_TASK_PRIO, 
                (OS_STK *)&LED_TASK_STK[LED_STK_SIZE-1], LED_STK_SIZE/8, LED_STK_SIZE, 0, 
                0, NULL, OS_OPT_TASK_NONE, &err);

 	//OSTaskCreate(main_task,(void *)0,(OS_STK*)&MAIN_TASK_STK[MAIN_STK_SIZE-1],MAIN_TASK_PRIO);	 	
    OSTaskCreate(main_task_tcb, "main_task", main_task, (void *)0, MAIN_TASK_PRIO, 
                (OS_STK *)&MAIN_TASK_STK[MAIN_STK_SIZE-1], MAIN_STK_SIZE/8, MAIN_STK_SIZE, 128, 
                0, NULL, OS_OPT_TASK_NONE, &err);
    
 	//OSTaskCreate(touch_task,(void *)0,(OS_STK*)&TOUCH_TASK_STK[TOUCH_STK_SIZE-1],TOUCH_TASK_PRIO);
    OSTaskCreate(touch_task_tcb, "touch_task", touch_task, (void *)0, TOUCH_TASK_PRIO, 
                (OS_STK *)&TOUCH_TASK_STK[TOUCH_STK_SIZE-1], TOUCH_STK_SIZE/8, TOUCH_STK_SIZE, 0, 
                0, NULL, OS_OPT_TASK_NONE, &err);
    
    //OSTaskCreate(qmsgshow_task,(void *)0,(OS_STK*)&QMSGSHOW_TASK_STK[QMSGSHOW_STK_SIZE-1],QMSGSHOW_TASK_PRIO);	 	
    OSTaskCreate(qmsg_task_tcb, "qmsgshow", qmsgshow_task, (void *)0, QMSGSHOW_TASK_PRIO, 
                (OS_STK *)&QMSGSHOW_TASK_STK[QMSGSHOW_STK_SIZE-1], QMSGSHOW_STK_SIZE/8, QMSGSHOW_STK_SIZE, 128, 
                0, NULL, OS_OPT_TASK_NONE, &err);
    
 	//OSTaskCreate(flags_task,(void *)0,(OS_STK*)&FLAGS_TASK_STK[FLAGS_STK_SIZE-1],FLAGS_TASK_PRIO);	 
    OSTaskCreate(flags_task_tcb, "flags_task", flags_task, (void *)0, FLAGS_TASK_PRIO, 
                (OS_STK *)&FLAGS_TASK_STK[FLAGS_STK_SIZE-1], FLAGS_STK_SIZE/8, FLAGS_STK_SIZE, 0, 
                0, NULL, OS_OPT_TASK_NONE, &err);
 	
 	//OSTaskCreate(key_task,(void *)0,(OS_STK*)&KEY_TASK_STK[KEY_STK_SIZE-1],KEY_TASK_PRIO);	 	
    OSTaskCreate(key_task_tcb, "key_task", key_task, (void *)0, KEY_TASK_PRIO, 
                (OS_STK *)&KEY_TASK_STK[KEY_STK_SIZE-1], KEY_STK_SIZE/8, KEY_STK_SIZE, 0, 
                0, NULL, OS_OPT_TASK_NONE, &err);
 	
#if 1
    //OSTaskCreate(uart_task,(void *)0,(OS_STK*)&UART_TASK_STK[UART_STK_SIZE-1],UART_TASK_PRIO);    
    OSTaskCreate(uart_task_tcb, "uart_task", uart_task, (void *)0, UART_TASK_PRIO, 
                (OS_STK *)&UART_TASK_STK[UART_STK_SIZE-1], UART_STK_SIZE/8, UART_STK_SIZE, 0, 
                0, NULL, OS_OPT_TASK_NONE, &err);
#endif 	
    CPU_CRITICAL_EXIT();  

    printf("err=%d \r\n", err);
 	//OSTaskSuspend(&start_task_tcb, &err);		//挂起起始任务.
 	delay_ms(100);
 	OS_TaskReturn();
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

		delay_ms(50);	 
	}
}   

//队列消息显示任务
void qmsgshow_task(void *pdata)
{
	u8 *p;
	OS_ERR err;
    OS_MSG_SIZE msg_size;
    
	while(1)
	{
		//p = OSQPend(q_msg, 0, &err);//请求消息队列
		p = OSTaskQPend(0, OS_OPT_PEND_BLOCKING, &msg_size, 0, &err);
		LCD_ShowString(5,170,240,16,16,p);//显示消息
		
 		if(p) myfree(SRAMIN, p);	  
		delay_ms(50);	 
        qmsg_task_cnt++;
	}									 
}

//主任务
void main_task(void *pdata)
{							 
	//u32 key=10;	
	OS_ERR err;	
 	u8 tmr3sta=0;	//软件定时器3开关状态
	
 	//tmr1 = OSTmrCreate(10,10,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr1_callback,0,"tmr1",&err);//100ms执行一次
 	OSTmrCreate(&tmr1, "tmr1", 10,10, OS_OPT_TMR_PERIODIC, (OS_TMR_CALLBACK_PTR)tmr1_callback, 0, &err);
    printf("OSTmrCreate ret %d\r\n", err);
    
	//tmr2 = OSTmrCreate(10,900,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr2_callback,0,"tmr2",&err);//9000ms执行一次
	OSTmrCreate(&tmr2, "tmr1", 10,100, OS_OPT_TMR_PERIODIC, (OS_TMR_CALLBACK_PTR)tmr2_callback, 0, &err);
    printf("OSTmrCreate ret %d\r\n", err);
    
	//tmr3 = OSTmrCreate(10,20,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr3_callback,0,"tmr3",&err);//200ms执行一次
	OSTmrCreate(&tmr3, "tmr3", 10,100, OS_OPT_TMR_PERIODIC, (OS_TMR_CALLBACK_PTR)tmr3_callback, 0, &err);
    printf("OSTmrCreate ret %d\r\n", err);
    
	OSTmrStart(&tmr1,&err);//启动软件定时器1				 
    printf("OSTmrStart ret %d\r\n", err);
	OSTmrStart(&tmr2,&err);//启动软件定时器2		
    printf("OSTmrStart ret %d\r\n", err);
	OSTmrStart(&tmr3,&err);//启动软件定时器3		
    printf("OSTmrStart ret %d\r\n", err);

 	while(1)
	{
	    OS_MSG_SIZE msg_size;
        OS_STATE key;
        u8* p;

        main_task_cnt++;            
		//key=(u32)OSMboxPend(msg_key, 10, &err); 
		p = OSTaskQPend(0, OS_OPT_PEND_BLOCKING, &msg_size, 0, &err);
		if(p) {
		    key = (OS_STATE)p[0];
			OSFlagPost(&flags_key, 1<<(key-1), OS_OPT_POST_FLAG_SET, &err);//设置对应的信号量为1
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
				if(tmr3sta) OSTaskSuspend(qmsg_task_tcb, &err);
				else OSTaskResume(qmsg_task_tcb, &err);
 				break;
                
			case 3://清除
 				LCD_Fill(0,221,129,lcddev.height,WHITE);
				break;
                
			case 4://校准
				OSTaskSuspend(touch_task_tcb, &err);						//挂起触摸屏任务		 
				OSTaskSuspend(qmsg_task_tcb, &err);	 				//挂起队列信息显示任务		 
 				OSTmrStop(&tmr1, OS_OPT_TMR_NONE, 0, &err);				//关闭软件定时器1
 				if((tp_dev.touchtype&0X80)==0) TP_Adjust();   
				OSTmrStart(&tmr1, &err);				//重新开启软件定时器1
 				OSTaskResume(touch_task_tcb, &err);		//解挂
 				OSTaskResume(qmsg_task_tcb, &err); 	//解挂
				ucos_load_main_ui();				//重新加载主界面		 
				break;
                
			case 5://clear log info, TPAD
				LCD_Fill(131, 221, lcddev.width-1, lcddev.height-1, WHITE);
				break;				 
				
		}  
        
		delay_ms(10);
	}
}		

//信号量集处理任务
void flags_task(void *pdata)
{	
	u16 flags;	
	OS_ERR err;	    
    
	while(1)
	{
		flags = OSFlagPend(&flags_key, 0X001F, 0, OS_OPT_PEND_FLAG_SET_ANY, 0, &err);//等待信号量

        //printf("flags_task> flags = 0x%x \r\n", flags);
 		if(flags&0X0001) LCD_ShowString(140,162,240,16,16,"KEY0 DOWN  "); 
		if(flags&0X0002) LCD_ShowString(140,162,240,16,16,"KEY1 DOWN  "); 
		if(flags&0X0004) LCD_ShowString(140,162,240,16,16,"KEY2 DOWN  "); 
		if(flags&0X0008) LCD_ShowString(140,162,240,16,16,"KEY_UP DOWN"); 
		if(flags&0X0010) LCD_ShowString(140,162,240,16,16,"TPAD DOWN  "); 

		BEEP=1;
		delay_ms(50);
		BEEP=0;
		OSFlagPost(&flags_key, 0X001F, OS_OPT_PEND_FLAG_CLR_ALL, &err);//全部信号量清零
 	}
}
   		    
//按键扫描任务
void key_task(void *pdata)
{	
	OS_STATE key;	
    OS_ERR err;
    
	while(1)
	{
		key = (OS_STATE)KEY_Scan(0);   
		if(key==0)
		{
			if(TPAD_Scan(0))key=5;
		}
        
		if(key) {
            u8* p = mymalloc(SRAMIN, 8);
            if(p) {
                p[0] = key;
                //OSMboxPost(msg_key,(void*)key);//发送消息
                OSTaskQPost(main_task_tcb, p, 8, OS_OPT_POST_FIFO, &err);
                key_task_cnt++;
            }
        }
 		delay_ms(100);
	}
}

