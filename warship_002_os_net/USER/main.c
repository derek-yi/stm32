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

 
 
/************************************************
 ALIENTEK战舰STM32开发板实验53
 UCOSII实验3-消息队列、信号量集和软件定时器  实验 
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com 
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/


/////////////////////////UCOSII任务设置///////////////////////////////////
//任务堆栈	
OS_STK START_TASK_STK[START_STK_SIZE];
OS_STK UART_TASK_STK[UART_STK_SIZE];
OS_STK LED_TASK_STK[LED_STK_SIZE];
OS_STK TOUCH_TASK_STK[TOUCH_STK_SIZE];
OS_STK QMSGSHOW_TASK_STK[QMSGSHOW_STK_SIZE];
OS_STK MAIN_TASK_STK[MAIN_STK_SIZE];
OS_STK FLAGS_TASK_STK[FLAGS_STK_SIZE];
OS_STK KEY_TASK_STK[KEY_STK_SIZE];

void lcd_print_log(u8 *log_str)
{
    static u8 line = 0;

    if (log_str == NULL) {
        LCD_Fill(131, 221, lcddev.width-1, lcddev.height-1, WHITE);
        line = 0;
        return ;
    }

    LCD_ShowString(5+130, 225 + 20*line, 240, 16, 16, log_str);
    if (line++ > 20) {
        line = 0;
        LCD_Fill(131, 221, lcddev.width-1, lcddev.height-1, WHITE);
    }
}

//////////////////////////////////////////////////////////////////////////////
#include "lwip_comm.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"

//mode:
//bit0:0,不加载;1,加载前半部分UI
//bit1:0,不加载;1,加载后半部分UI
void lwip_test_ui(u8 mode)
{
	u8 speed;
	u8 buf[30]; 
    
	POINT_COLOR=RED;
	if(mode&1<<0)
	{
		lcd_print_log(NULL);	//清除显示
		lcd_print_log("WarShip STM32F1");
		lcd_print_log("Ethernet lwIP Test");
		lcd_print_log("ATOM@ALIENTEK");
		lcd_print_log("2015/3/21"); 	
	}
    
	if(mode&1<<1)
	{
		lcd_print_log("lwIP Init Successed");
		if(lwipdev.dhcpstatus==2)sprintf((char*)buf,"DHCP IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);//打印动态IP地址
		else sprintf((char*)buf,"Static IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);//打印静态IP地址
		lcd_print_log(buf); 
		speed=DM9000_Get_SpeedAndDuplex();//得到网速
		if(speed&1<<1)lcd_print_log("Ethernet Speed:10M");
		else lcd_print_log("Ethernet Speed:100M");
	}
}

void net_func_init()
{
    lwip_test_ui(1);        //加载前半部分UI      
    
    while(lwip_comm_init()) //lwip初始化
    {
        lcd_print_log("LWIP Init Falied!");
        delay_ms(1200);
        lcd_print_log("Retrying...");  
    }
    
    lcd_print_log("LWIP Init Success!");
    lcd_print_log("DHCP IP configing...");
    
#if LWIP_DHCP   //使用DHCP
    while((lwipdev.dhcpstatus!=2)&&(lwipdev.dhcpstatus!=0XFF))//等待DHCP获取成功/超时溢出
    {
        lwip_periodic_handle(); //LWIP内核需要定时处理的函数
        lwip_pkt_handle();
    }
#endif

    lwip_test_ui(2);        //加载后半部分UI 
    httpd_init();           //Web Server模式

}


//////////////////////////////////////////////////////////////////////////////
    
OS_EVENT * msg_key;			//按键邮箱事件块	  
OS_EVENT * q_msg;			//消息队列
OS_TMR   * tmr1;			//软件定时器1
OS_TMR   * tmr2;			//软件定时器2
OS_TMR   * tmr3;			//软件定时器3
OS_FLAG_GRP * flags_key;	//按键信号量集
void * MsgGrp[256];			//消息队列存储地址,最大支持256个消息

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
	cpuusage+=OSCPUUsage;
	tcnt++;				    
 	LCD_ShowxNum(182,30,my_mem_perused(SRAMIN),3,16,0);	//显示内存使用率	 	  		 					    
	LCD_ShowxNum(182,50,((OS_Q*)(q_msg->OSEventPtr))->OSQEntries,3,16,0X80);//显示队列当前的大小		   
}

void tmr2_callback(OS_TMR *ptmr,void *p_arg) 
{	
    //lcd_print_log("tmr2_callback");
}

//软件定时器3的回调函数，发送消息给q_msg				  	   
void tmr3_callback(OS_TMR *ptmr,void *p_arg) 
{	
	u8* p;	 
	u8 err; 
	static u8 msg_cnt=0;	//msg编号	  
	
	p = mymalloc(SRAMIN,13);	//申请13个字节的内存
	if(p)
	{
	 	sprintf((char*)p,"ALIENTEK %03d",msg_cnt);
		msg_cnt++;
		err = OSQPost(q_msg,p);	//发送队列
		if(err!=OS_ERR_NONE) 	//发送失败
		{
			myfree(SRAMIN,p);	//释放内存
			OSTmrStop(tmr3,OS_TMR_OPT_NONE,0,&err);	//关闭软件定时器3
 		}
	}
} 

//加载主界面   
void ucos_load_main_ui(void)
{
	LCD_Clear(WHITE);	//清屏
 	POINT_COLOR=RED;	//设置字体为红色 
	LCD_ShowString(10,10,200,16,16,"WarShip STM32");	
	LCD_ShowString(10,30,200,16,16,"UCOSII TEST3");	
	LCD_ShowString(10,50,200,16,16,"ATOM@ALIENTEK");
   	LCD_ShowString(10,75,240,16,16,"TPAD:TMR2 SW   KEY_UP:ADJUST");	
   	LCD_ShowString(10,95,240,16,16,"KEY0:DS0 KEY1:Q SW KEY2:CLR");	
 	LCD_DrawLine(0,70,lcddev.width,70);
	LCD_DrawLine(130,0,130,70);

 	LCD_DrawLine(0,120,lcddev.width,120);
 	LCD_DrawLine(0,220,lcddev.width,220);
	LCD_DrawLine(130,120,130,lcddev.height);
		    
 	LCD_ShowString(5,125,240,16,16,"QUEUE MSG");//队列消息
	LCD_ShowString(5,150,240,16,16,"Message:");	 
	LCD_ShowString(5+130,125,240,16,16,"FLAGS");//信号量集
	LCD_ShowString(5,225,240,16,16,"TOUCH");	//触摸屏
	
	POINT_COLOR=BLUE;//设置字体为蓝色 
  	LCD_ShowString(150,10,200,16,16,"CPU:   %");	
   	LCD_ShowString(150,30,200,16,16,"MEM:   %");	
   	LCD_ShowString(150,50,200,16,16," Q :000");	

	delay_ms(300);
}	  

int main(void)
{	 		    
	delay_init();	    	 //延时函数初始化	  
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	uart_init(115200);	 	//串口初始化为115200
	 
  	LED_Init();					//初始化LED 
	LCD_Init();			   		//初始化LCD 
	BEEP_Init();				//蜂鸣器初始化	
	KEY_Init();					//按键初始化 
	
	RTC_Init();				    //RTC初始化
	T_Adc_Init();			//ADC初始化
	TIM3_Int_Init(999, 719);//定时器3频率为100hz
	FSMC_SRAM_Init();		//初始化外部SRAM
	
	TPAD_Init(6);				//初始化TPAD 
	my_mem_init(SRAMIN);		//初始化内部内存池
	my_mem_init(SRAMEX);		//初始化外部内存池
   	tp_dev.init();				//初始化触摸屏
	ucos_load_main_ui(); 		//加载主界面
	OSInit();  	 				//初始化UCOSII
  	OSTaskCreate(start_task,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );//创建起始任务
	OSStart();	  
}
 

///////////////////////////////////////////////////////////////////////////////////////////////////
//开始任务
void start_task(void *pdata)
{
    OS_CPU_SR cpu_sr=0;
	u8 err;	    	    
	pdata = pdata; 	
    
	msg_key = OSMboxCreate((void*)0);		//创建消息邮箱
	q_msg = OSQCreate(&MsgGrp[0],256);	//创建消息队列
 	flags_key = OSFlagCreate(0,&err); 	//创建信号量集		  
	  
	OSStatInit();						//初始化统计任务.这里会延时1秒钟左右	
 	OS_ENTER_CRITICAL();				//进入临界区(无法被中断打断)    
 	OSTaskCreate(led_task,(void *)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);						   
 	OSTaskCreate(touch_task,(void *)0,(OS_STK*)&TOUCH_TASK_STK[TOUCH_STK_SIZE-1],TOUCH_TASK_PRIO);	 				   
 	OSTaskCreate(qmsgshow_task,(void *)0,(OS_STK*)&QMSGSHOW_TASK_STK[QMSGSHOW_STK_SIZE-1],QMSGSHOW_TASK_PRIO);	 				   
 	OSTaskCreate(main_task,(void *)0,(OS_STK*)&MAIN_TASK_STK[MAIN_STK_SIZE-1],MAIN_TASK_PRIO);	 				   
 	OSTaskCreate(flags_task,(void *)0,(OS_STK*)&FLAGS_TASK_STK[FLAGS_STK_SIZE-1],FLAGS_TASK_PRIO);	 				   
 	OSTaskCreate(key_task,(void *)0,(OS_STK*)&KEY_TASK_STK[KEY_STK_SIZE-1],KEY_TASK_PRIO);	 				   
 	OSTaskCreate(uart_task,(void *)0,(OS_STK*)&UART_TASK_STK[UART_STK_SIZE-1],UART_TASK_PRIO);	 				   
 	OSTaskSuspend(START_TASK_PRIO);		//挂起起始任务.
	OS_EXIT_CRITICAL();					//退出临界区(可以被中断打断)
}

//LED任务
void led_task(void *pdata)
{
	u8 t;

    net_func_init();
	while(1)
	{
		t++;
		//delay_ms(10);
		if(t==20)LED0=1;	//LED0灭
		if(t==200)		//LED0亮
		{
			t=0;
			LED0=0;
		}

        lwip_periodic_handle();
        lwip_pkt_handle();
        delay_ms(5);
	}									 
}

//触摸屏任务
void touch_task(void *pdata)
{	  	
	u32 cpu_sr;
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
				OS_ENTER_CRITICAL();//进入临界段,防止其他任务,打断LCD操作,导致液晶乱序.
				lcd_draw_bline(lastpos[0],lastpos[1],tp_dev.x[0],tp_dev.y[0],2,RED);//画线
				OS_EXIT_CRITICAL();
				lastpos[0]=tp_dev.x[0];
				lastpos[1]=tp_dev.y[0];     
			}
		}
        else lastpos[0]=0XFFFF;//没有触摸 
        
		delay_ms(5);	 
	}
}     

//队列消息显示任务
void qmsgshow_task(void *pdata)
{
	u8 *p;
	u8 err;
    
	while(1)
	{
		p = OSQPend(q_msg, 0, &err);//请求消息队列
		LCD_ShowString(5,170,240,16,16,p);//显示消息
 		myfree(SRAMIN, p);	  
		delay_ms(500);	 
	}									 
}

//主任务
void main_task(void *pdata)
{							 
	u32 key=0;	
	u8 err;	
 	u8 tmr3sta=0;	//软件定时器3开关状态
	
 	tmr1 = OSTmrCreate(10,10,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr1_callback,0,"tmr1",&err);//100ms执行一次
	tmr2 = OSTmrCreate(10,900,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr2_callback,0,"tmr2",&err);//9000ms执行一次
	tmr3 = OSTmrCreate(10,20,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr3_callback,0,"tmr3",&err);//200ms执行一次
	OSTmrStart(tmr1,&err);//启动软件定时器1				 
	OSTmrStart(tmr2,&err);//启动软件定时器2		
	
 	while(1)
	{
		key=(u32)OSMboxPend(msg_key, 10, &err); 
		if(key)
		{
			OSFlagPost(flags_key,1<<(key-1),OS_FLAG_SET,&err);//设置对应的信号量为1
		}
        
		switch(key)
		{
			case 1://控制DS1
				LED1=!LED1;
				break;
                
			case 2://控制软件定时器3	 
				tmr3sta=!tmr3sta;
				if(tmr3sta) OSTmrStart(tmr3,&err);  
				else OSTmrStop(tmr3,OS_TMR_OPT_NONE,0,&err);		//关闭软件定时器3
 				break;
                
			case 3://清除
 				LCD_Fill(0,221,129,lcddev.height,WHITE);
				break;
                
			case 4://校准
				OSTaskSuspend(TOUCH_TASK_PRIO);						//挂起触摸屏任务		 
				OSTaskSuspend(QMSGSHOW_TASK_PRIO);	 				//挂起队列信息显示任务		 
 				OSTmrStop(tmr1,OS_TMR_OPT_NONE,0,&err);				//关闭软件定时器1
 				if((tp_dev.touchtype&0X80)==0)TP_Adjust();   
				OSTmrStart(tmr1,&err);				//重新开启软件定时器1
 				OSTaskResume(TOUCH_TASK_PRIO);		//解挂
 				OSTaskResume(QMSGSHOW_TASK_PRIO); 	//解挂
				ucos_load_main_ui();				//重新加载主界面		 
				break;
                
			case 5://clear log info
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
	u8 err;	    
    
	while(1)
	{
		flags = OSFlagPend(flags_key, 0X001F, OS_FLAG_WAIT_SET_ANY, 0, &err);//等待信号量

        printf("flags_task> flags = 0x%x \r\n", flags);
 		if(flags&0X0001) LCD_ShowString(140,162,240,16,16,"KEY0 DOWN  "); 
		if(flags&0X0002) LCD_ShowString(140,162,240,16,16,"KEY1 DOWN  "); 
		if(flags&0X0004) LCD_ShowString(140,162,240,16,16,"KEY2 DOWN  "); 
		if(flags&0X0008) LCD_ShowString(140,162,240,16,16,"KEY_UP DOWN"); 
		if(flags&0X0010) LCD_ShowString(140,162,240,16,16,"TPAD DOWN  "); 

		BEEP=1;
		delay_ms(50);
		BEEP=0;
		OSFlagPost(flags_key, 0X001F, OS_FLAG_CLR, &err);//全部信号量清零
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
        
		if(key)OSMboxPost(msg_key,(void*)key);//发送消息
 		delay_ms(10);
	}
}


