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
 ALIENTEKս��STM32������ʵ��53
 UCOSIIʵ��3-��Ϣ���С��ź������������ʱ��  ʵ�� 
 ����֧�֣�www.openedv.com
 �Ա����̣�http://eboard.taobao.com 
 ��ע΢�Ź���ƽ̨΢�źţ�"����ԭ��"����ѻ�ȡSTM32���ϡ�
 ������������ӿƼ����޹�˾  
 ���ߣ�����ԭ�� @ALIENTEK
************************************************/


/////////////////////////UCOSII��������///////////////////////////////////
//�����ջ	
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
//bit0:0,������;1,����ǰ�벿��UI
//bit1:0,������;1,���غ�벿��UI
void lwip_test_ui(u8 mode)
{
	u8 speed;
	u8 buf[30]; 
    
	POINT_COLOR=RED;
	if(mode&1<<0)
	{
		lcd_print_log(NULL);	//�����ʾ
		lcd_print_log("WarShip STM32F1");
		lcd_print_log("Ethernet lwIP Test");
		lcd_print_log("ATOM@ALIENTEK");
		lcd_print_log("2015/3/21"); 	
	}
    
	if(mode&1<<1)
	{
		lcd_print_log("lwIP Init Successed");
		if(lwipdev.dhcpstatus==2)sprintf((char*)buf,"DHCP IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);//��ӡ��̬IP��ַ
		else sprintf((char*)buf,"Static IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);//��ӡ��̬IP��ַ
		lcd_print_log(buf); 
		speed=DM9000_Get_SpeedAndDuplex();//�õ�����
		if(speed&1<<1)lcd_print_log("Ethernet Speed:10M");
		else lcd_print_log("Ethernet Speed:100M");
	}
}

void net_func_init()
{
    lwip_test_ui(1);        //����ǰ�벿��UI      
    
    while(lwip_comm_init()) //lwip��ʼ��
    {
        lcd_print_log("LWIP Init Falied!");
        delay_ms(1200);
        lcd_print_log("Retrying...");  
    }
    
    lcd_print_log("LWIP Init Success!");
    lcd_print_log("DHCP IP configing...");
    
#if LWIP_DHCP   //ʹ��DHCP
    while((lwipdev.dhcpstatus!=2)&&(lwipdev.dhcpstatus!=0XFF))//�ȴ�DHCP��ȡ�ɹ�/��ʱ���
    {
        lwip_periodic_handle(); //LWIP�ں���Ҫ��ʱ����ĺ���
        lwip_pkt_handle();
    }
#endif

    lwip_test_ui(2);        //���غ�벿��UI 
    httpd_init();           //Web Serverģʽ

}


//////////////////////////////////////////////////////////////////////////////
    
OS_EVENT * msg_key;			//���������¼���	  
OS_EVENT * q_msg;			//��Ϣ����
OS_TMR   * tmr1;			//�����ʱ��1
OS_TMR   * tmr2;			//�����ʱ��2
OS_TMR   * tmr3;			//�����ʱ��3
OS_FLAG_GRP * flags_key;	//�����ź�����
void * MsgGrp[256];			//��Ϣ���д洢��ַ,���֧��256����Ϣ

//ÿ100msִ��һ��,������ʾCPUʹ���ʺ��ڴ�ʹ����		   
void tmr1_callback(OS_TMR *ptmr,void *p_arg) 
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
	cpuusage+=OSCPUUsage;
	tcnt++;				    
 	LCD_ShowxNum(182,30,my_mem_perused(SRAMIN),3,16,0);	//��ʾ�ڴ�ʹ����	 	  		 					    
	LCD_ShowxNum(182,50,((OS_Q*)(q_msg->OSEventPtr))->OSQEntries,3,16,0X80);//��ʾ���е�ǰ�Ĵ�С		   
}

void tmr2_callback(OS_TMR *ptmr,void *p_arg) 
{	
    //lcd_print_log("tmr2_callback");
}

//�����ʱ��3�Ļص�������������Ϣ��q_msg				  	   
void tmr3_callback(OS_TMR *ptmr,void *p_arg) 
{	
	u8* p;	 
	u8 err; 
	static u8 msg_cnt=0;	//msg���	  
	
	p = mymalloc(SRAMIN,13);	//����13���ֽڵ��ڴ�
	if(p)
	{
	 	sprintf((char*)p,"ALIENTEK %03d",msg_cnt);
		msg_cnt++;
		err = OSQPost(q_msg,p);	//���Ͷ���
		if(err!=OS_ERR_NONE) 	//����ʧ��
		{
			myfree(SRAMIN,p);	//�ͷ��ڴ�
			OSTmrStop(tmr3,OS_TMR_OPT_NONE,0,&err);	//�ر������ʱ��3
 		}
	}
} 

//����������   
void ucos_load_main_ui(void)
{
	LCD_Clear(WHITE);	//����
 	POINT_COLOR=RED;	//��������Ϊ��ɫ 
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
		    
 	LCD_ShowString(5,125,240,16,16,"QUEUE MSG");//������Ϣ
	LCD_ShowString(5,150,240,16,16,"Message:");	 
	LCD_ShowString(5+130,125,240,16,16,"FLAGS");//�ź�����
	LCD_ShowString(5,225,240,16,16,"TOUCH");	//������
	
	POINT_COLOR=BLUE;//��������Ϊ��ɫ 
  	LCD_ShowString(150,10,200,16,16,"CPU:   %");	
   	LCD_ShowString(150,30,200,16,16,"MEM:   %");	
   	LCD_ShowString(150,50,200,16,16," Q :000");	

	delay_ms(300);
}	  

int main(void)
{	 		    
	delay_init();	    	 //��ʱ������ʼ��	  
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�����ж����ȼ�����Ϊ��2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	uart_init(115200);	 	//���ڳ�ʼ��Ϊ115200
	 
  	LED_Init();					//��ʼ��LED 
	LCD_Init();			   		//��ʼ��LCD 
	BEEP_Init();				//��������ʼ��	
	KEY_Init();					//������ʼ�� 
	
	RTC_Init();				    //RTC��ʼ��
	T_Adc_Init();			//ADC��ʼ��
	TIM3_Int_Init(999, 719);//��ʱ��3Ƶ��Ϊ100hz
	FSMC_SRAM_Init();		//��ʼ���ⲿSRAM
	
	TPAD_Init(6);				//��ʼ��TPAD 
	my_mem_init(SRAMIN);		//��ʼ���ڲ��ڴ��
	my_mem_init(SRAMEX);		//��ʼ���ⲿ�ڴ��
   	tp_dev.init();				//��ʼ��������
	ucos_load_main_ui(); 		//����������
	OSInit();  	 				//��ʼ��UCOSII
  	OSTaskCreate(start_task,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );//������ʼ����
	OSStart();	  
}
 

///////////////////////////////////////////////////////////////////////////////////////////////////
//��ʼ����
void start_task(void *pdata)
{
    OS_CPU_SR cpu_sr=0;
	u8 err;	    	    
	pdata = pdata; 	
    
	msg_key = OSMboxCreate((void*)0);		//������Ϣ����
	q_msg = OSQCreate(&MsgGrp[0],256);	//������Ϣ����
 	flags_key = OSFlagCreate(0,&err); 	//�����ź�����		  
	  
	OSStatInit();						//��ʼ��ͳ������.�������ʱ1��������	
 	OS_ENTER_CRITICAL();				//�����ٽ���(�޷����жϴ��)    
 	OSTaskCreate(led_task,(void *)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);						   
 	OSTaskCreate(touch_task,(void *)0,(OS_STK*)&TOUCH_TASK_STK[TOUCH_STK_SIZE-1],TOUCH_TASK_PRIO);	 				   
 	OSTaskCreate(qmsgshow_task,(void *)0,(OS_STK*)&QMSGSHOW_TASK_STK[QMSGSHOW_STK_SIZE-1],QMSGSHOW_TASK_PRIO);	 				   
 	OSTaskCreate(main_task,(void *)0,(OS_STK*)&MAIN_TASK_STK[MAIN_STK_SIZE-1],MAIN_TASK_PRIO);	 				   
 	OSTaskCreate(flags_task,(void *)0,(OS_STK*)&FLAGS_TASK_STK[FLAGS_STK_SIZE-1],FLAGS_TASK_PRIO);	 				   
 	OSTaskCreate(key_task,(void *)0,(OS_STK*)&KEY_TASK_STK[KEY_STK_SIZE-1],KEY_TASK_PRIO);	 				   
 	OSTaskCreate(uart_task,(void *)0,(OS_STK*)&UART_TASK_STK[UART_STK_SIZE-1],UART_TASK_PRIO);	 				   
 	OSTaskSuspend(START_TASK_PRIO);		//������ʼ����.
	OS_EXIT_CRITICAL();					//�˳��ٽ���(���Ա��жϴ��)
}

//LED����
void led_task(void *pdata)
{
	u8 t;

    net_func_init();
	while(1)
	{
		t++;
		//delay_ms(10);
		if(t==20)LED0=1;	//LED0��
		if(t==200)		//LED0��
		{
			t=0;
			LED0=0;
		}

        lwip_periodic_handle();
        lwip_pkt_handle();
        delay_ms(5);
	}									 
}

//����������
void touch_task(void *pdata)
{	  	
	u32 cpu_sr;
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
				OS_ENTER_CRITICAL();//�����ٽ��,��ֹ��������,���LCD����,����Һ������.
				lcd_draw_bline(lastpos[0],lastpos[1],tp_dev.x[0],tp_dev.y[0],2,RED);//����
				OS_EXIT_CRITICAL();
				lastpos[0]=tp_dev.x[0];
				lastpos[1]=tp_dev.y[0];     
			}
		}
        else lastpos[0]=0XFFFF;//û�д��� 
        
		delay_ms(5);	 
	}
}     

//������Ϣ��ʾ����
void qmsgshow_task(void *pdata)
{
	u8 *p;
	u8 err;
    
	while(1)
	{
		p = OSQPend(q_msg, 0, &err);//������Ϣ����
		LCD_ShowString(5,170,240,16,16,p);//��ʾ��Ϣ
 		myfree(SRAMIN, p);	  
		delay_ms(500);	 
	}									 
}

//������
void main_task(void *pdata)
{							 
	u32 key=0;	
	u8 err;	
 	u8 tmr3sta=0;	//�����ʱ��3����״̬
	
 	tmr1 = OSTmrCreate(10,10,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr1_callback,0,"tmr1",&err);//100msִ��һ��
	tmr2 = OSTmrCreate(10,900,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr2_callback,0,"tmr2",&err);//9000msִ��һ��
	tmr3 = OSTmrCreate(10,20,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr3_callback,0,"tmr3",&err);//200msִ��һ��
	OSTmrStart(tmr1,&err);//���������ʱ��1				 
	OSTmrStart(tmr2,&err);//���������ʱ��2		
	
 	while(1)
	{
		key=(u32)OSMboxPend(msg_key, 10, &err); 
		if(key)
		{
			OSFlagPost(flags_key,1<<(key-1),OS_FLAG_SET,&err);//���ö�Ӧ���ź���Ϊ1
		}
        
		switch(key)
		{
			case 1://����DS1
				LED1=!LED1;
				break;
                
			case 2://���������ʱ��3	 
				tmr3sta=!tmr3sta;
				if(tmr3sta) OSTmrStart(tmr3,&err);  
				else OSTmrStop(tmr3,OS_TMR_OPT_NONE,0,&err);		//�ر������ʱ��3
 				break;
                
			case 3://���
 				LCD_Fill(0,221,129,lcddev.height,WHITE);
				break;
                
			case 4://У׼
				OSTaskSuspend(TOUCH_TASK_PRIO);						//������������		 
				OSTaskSuspend(QMSGSHOW_TASK_PRIO);	 				//���������Ϣ��ʾ����		 
 				OSTmrStop(tmr1,OS_TMR_OPT_NONE,0,&err);				//�ر������ʱ��1
 				if((tp_dev.touchtype&0X80)==0)TP_Adjust();   
				OSTmrStart(tmr1,&err);				//���¿��������ʱ��1
 				OSTaskResume(TOUCH_TASK_PRIO);		//���
 				OSTaskResume(QMSGSHOW_TASK_PRIO); 	//���
				ucos_load_main_ui();				//���¼���������		 
				break;
                
			case 5://clear log info
				LCD_Fill(131, 221, lcddev.width-1, lcddev.height-1, WHITE);
				break;				 
				
		}  
        
		delay_ms(10);
	}
}		

//�ź�������������
void flags_task(void *pdata)
{	
	u16 flags;	
	u8 err;	    
    
	while(1)
	{
		flags = OSFlagPend(flags_key, 0X001F, OS_FLAG_WAIT_SET_ANY, 0, &err);//�ȴ��ź���

        printf("flags_task> flags = 0x%x \r\n", flags);
 		if(flags&0X0001) LCD_ShowString(140,162,240,16,16,"KEY0 DOWN  "); 
		if(flags&0X0002) LCD_ShowString(140,162,240,16,16,"KEY1 DOWN  "); 
		if(flags&0X0004) LCD_ShowString(140,162,240,16,16,"KEY2 DOWN  "); 
		if(flags&0X0008) LCD_ShowString(140,162,240,16,16,"KEY_UP DOWN"); 
		if(flags&0X0010) LCD_ShowString(140,162,240,16,16,"TPAD DOWN  "); 

		BEEP=1;
		delay_ms(50);
		BEEP=0;
		OSFlagPost(flags_key, 0X001F, OS_FLAG_CLR, &err);//ȫ���ź�������
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
        
		if(key)OSMboxPost(msg_key,(void*)key);//������Ϣ
 		delay_ms(10);
	}
}


