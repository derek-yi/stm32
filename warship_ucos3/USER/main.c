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

 

/////////////////////////UCOSII��������///////////////////////////////////
//START ����
//�����������ȼ�
#define START_TASK_PRIO      			10 //��ʼ��������ȼ�����Ϊ���
//���������ջ��С
#define START_STK_SIZE  				64
//�����ջ	
OS_STK START_TASK_STK[START_STK_SIZE];
//������
void start_task(void *pdata);	

//�����������ȼ�
#define UART_TASK_PRIO       			8 
//���������ջ��С
#define UART_STK_SIZE  					128
//�����ջ	
OS_STK UART_TASK_STK[UART_STK_SIZE];
//������
void uart_task(void *pdata);

//LED����
//�����������ȼ�
#define LED_TASK_PRIO       			7 
//���������ջ��С
#define LED_STK_SIZE  		    		64
//�����ջ
OS_STK LED_TASK_STK[LED_STK_SIZE];
//������
void led_task(void *pdata);

//����������
//�����������ȼ�
#define TOUCH_TASK_PRIO       		 	6
//���������ջ��С
#define TOUCH_STK_SIZE  				128
//�����ջ	
OS_STK TOUCH_TASK_STK[TOUCH_STK_SIZE];
//������
void touch_task(void *pdata);

//������Ϣ��ʾ����
//�����������ȼ�
#define QMSGSHOW_TASK_PRIO    			5 
//���������ջ��С
#define QMSGSHOW_STK_SIZE  		 		128
//�����ջ	
OS_STK QMSGSHOW_TASK_STK[QMSGSHOW_STK_SIZE];
//������
void qmsgshow_task(void *pdata);


//������
//�����������ȼ�
#define MAIN_TASK_PRIO       			4 
//���������ջ��С
#define MAIN_STK_SIZE  					128
//�����ջ	
OS_STK MAIN_TASK_STK[MAIN_STK_SIZE];
//������
void main_task(void *pdata);

//�ź���������
//�����������ȼ�
#define FLAGS_TASK_PRIO       			3 
//���������ջ��С
#define FLAGS_STK_SIZE  		 		128
//�����ջ	
OS_STK FLAGS_TASK_STK[FLAGS_STK_SIZE];
//������
void flags_task(void *pdata);


//����ɨ������
//�����������ȼ�
#define KEY_TASK_PRIO       			2 
//���������ջ��С
#define KEY_STK_SIZE  					128
//�����ջ	
OS_STK KEY_TASK_STK[KEY_STK_SIZE];
//������
void key_task(void *pdata);


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
    
OS_Q msg_key;		//todo  
OS_Q q_msg;			//��Ϣ����
OS_TMR   tmr1;			//������ʱ��1
OS_TMR   tmr2;			//������ʱ��2
OS_TMR   tmr3;			//������ʱ��3
OS_FLAG_GRP flags_key;	//�����ź�����
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
	cpuusage += OSStatTaskCPUUsage;
	tcnt++;				    
 	LCD_ShowxNum(182,30,my_mem_perused(SRAMIN),3,16,0);	//��ʾ�ڴ�ʹ����	 	  		 					    
	LCD_ShowxNum(182,50,q_msg.MsgQ.NbrEntries,3,16,0X80);//��ʾ���е�ǰ�Ĵ�С		   
}

void tmr2_callback(OS_TMR *ptmr,void *p_arg) 
{	
    lcd_print_log("tmr2_callback");
}

//������ʱ��3�Ļص�������������Ϣ��q_msg				  	   
void tmr3_callback(OS_TMR *ptmr,void *p_arg) 
{	
	u8* p;	 
	OS_ERR err; 
	static u8 msg_cnt=0;	//msg���	  
	
	p = mymalloc(SRAMIN, 16);	//����16���ֽڵ��ڴ�
	if(p)
	{
	 	sprintf((char*)p, "ALIENTEK %03d", msg_cnt);
		msg_cnt++;
		//err = OSQPost(q_msg,p);	//���Ͷ���
		OSQPost(&q_msg, p, 16, 0, &err);
		if(err!=OS_ERR_NONE) 	//����ʧ��
		{
			myfree(SRAMIN, p);	//�ͷ��ڴ�
			OSTmrStop(&tmr3, OS_OPT_TMR_NONE, 0, &err);	//�ر�������ʱ��3
 		}
	}
} 

//����������   
void ucos_load_main_ui(void)
{
	LCD_Clear(WHITE);	//����
 	POINT_COLOR=RED;	//��������Ϊ��ɫ 
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

OS_TCB start_task_tcb;

int main(void)
{	 	
    OS_ERR err; 
    
    delay_init();	    	 //��ʱ������ʼ��	  
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�����ж����ȼ�����Ϊ��2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
    uart_init(115200);	 	//���ڳ�ʼ��Ϊ115200

    LED_Init();					//��ʼ��LED 
    LCD_Init();			   		//��ʼ��LCD 
    BEEP_Init();				//��������ʼ��	
    KEY_Init();					//������ʼ�� 
    TPAD_Init(6);				//��ʼ��TPAD 
    my_mem_init(SRAMIN);		//��ʼ���ڲ��ڴ��
    tp_dev.init();				//��ʼ��������
    ucos_load_main_ui(); 		//����������
    OSInit(&err);               //��ʼ��UCOSII
    
    //OSTaskCreate(start_task,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );//������ʼ����
    OSTaskCreate(&start_task_tcb, "start", start_task, (void *)0, START_TASK_PRIO, 
                (OS_STK *)&START_TASK_STK[START_STK_SIZE-1], START_STK_SIZE, START_STK_SIZE, 128, 
                0, NULL, OS_OPT_TASK_NONE, &err);
    OSStart(&err);	  
}
 

///////////////////////////////////////////////////////////////////////////////////////////////////
//��ʼ����
OS_TCB led_task_tcb;
OS_TCB qmsg_task_tcb;
OS_TCB main_task_tcb;
OS_TCB touch_task_tcb;
OS_TCB uart_task_tcb;
OS_TCB flags_task_tcb;
OS_TCB key_task_tcb;

void start_task(void *pdata)
{
    //OS_CPU_SR cpu_sr=0;
	OS_ERR err;	    
    
	pdata = pdata; 	
    
	//msg_key = OSMboxCreate((void*)0);		//������Ϣ����
	OSQCreate(&msg_key, "msg_key", 128, &err);
    
	//q_msg = OSQCreate(&MsgGrp[0],256);	//������Ϣ����
	OSQCreate(&q_msg, "q_msg", 128, &err);
    
 	//flags_key = OSFlagCreate(0,&err); 	//�����ź�����		
    OSFlagCreate(&flags_key, "null", 0, &err);
    
	//OSStatInit();						//��ʼ��ͳ������.�������ʱ1��������	
 	//OS_ENTER_CRITICAL();				//�����ٽ���(�޷����жϴ��) 
 	
 	//OSTaskCreate(led_task,(void *)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);		
    OSTaskCreate(&led_task_tcb, "led", led_task, (void *)0, LED_TASK_PRIO, 
                (OS_STK *)&LED_TASK_STK[START_STK_SIZE-1], LED_STK_SIZE, LED_STK_SIZE, 128, 
                0, NULL, OS_OPT_TASK_NONE, &err);

 	//OSTaskCreate(main_task,(void *)0,(OS_STK*)&MAIN_TASK_STK[MAIN_STK_SIZE-1],MAIN_TASK_PRIO);	 	
    OSTaskCreate(&main_task_tcb, "main_task", main_task, (void *)0, MAIN_TASK_PRIO, 
                (OS_STK *)&MAIN_TASK_STK[MAIN_STK_SIZE-1], MAIN_STK_SIZE, MAIN_STK_SIZE, 128, 
                0, NULL, OS_OPT_TASK_NONE, &err);
    
 	//OSTaskCreate(touch_task,(void *)0,(OS_STK*)&TOUCH_TASK_STK[TOUCH_STK_SIZE-1],TOUCH_TASK_PRIO);
    OSTaskCreate(&touch_task_tcb, "touch_task", touch_task, (void *)0, TOUCH_TASK_PRIO, 
                (OS_STK *)&TOUCH_TASK_STK[TOUCH_STK_SIZE-1], TOUCH_STK_SIZE, TOUCH_STK_SIZE, 128, 
                0, NULL, OS_OPT_TASK_NONE, &err);
    
    //OSTaskCreate(qmsgshow_task,(void *)0,(OS_STK*)&QMSGSHOW_TASK_STK[QMSGSHOW_STK_SIZE-1],QMSGSHOW_TASK_PRIO);	 	
    OSTaskCreate(&qmsg_task_tcb, "qmsgshow", qmsgshow_task, (void *)0, QMSGSHOW_TASK_PRIO, 
                (OS_STK *)&QMSGSHOW_TASK_STK[QMSGSHOW_STK_SIZE-1], QMSGSHOW_STK_SIZE, QMSGSHOW_STK_SIZE, 128, 
                0, NULL, OS_OPT_TASK_NONE, &err);
    
 	//OSTaskCreate(flags_task,(void *)0,(OS_STK*)&FLAGS_TASK_STK[FLAGS_STK_SIZE-1],FLAGS_TASK_PRIO);	 
    OSTaskCreate(&flags_task_tcb, "flags_task", flags_task, (void *)0, FLAGS_TASK_PRIO, 
                (OS_STK *)&FLAGS_TASK_STK[FLAGS_STK_SIZE-1], FLAGS_STK_SIZE, FLAGS_STK_SIZE, 128, 
                0, NULL, OS_OPT_TASK_NONE, &err);
 	
 	//OSTaskCreate(key_task,(void *)0,(OS_STK*)&KEY_TASK_STK[KEY_STK_SIZE-1],KEY_TASK_PRIO);	 	
    OSTaskCreate(&key_task_tcb, "key_task", key_task, (void *)0, KEY_TASK_PRIO, 
                (OS_STK *)&KEY_TASK_STK[KEY_STK_SIZE-1], KEY_STK_SIZE, KEY_STK_SIZE, 128, 
                0, NULL, OS_OPT_TASK_NONE, &err);
 	
#if xx
 	//OSTaskCreate(uart_task,(void *)0,(OS_STK*)&UART_TASK_STK[UART_STK_SIZE-1],UART_TASK_PRIO);	
    OSTaskCreate(&uart_task_tcb, "uart_task", uart_task, (void *)0, UART_TASK_PRIO, 
                (OS_STK *)&UART_TASK_STK[UART_STK_SIZE-1], UART_STK_SIZE, UART_STK_SIZE, 128, 
                0, NULL, OS_OPT_TASK_NONE, &err);
#endif 	

 	OSTaskSuspend(&start_task_tcb, &err);		//������ʼ����.
	//OS_EXIT_CRITICAL();					//�˳��ٽ���(���Ա��жϴ��)
}

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
        
		delay_ms(5);	 
	}
}   

//������Ϣ��ʾ����
void qmsgshow_task(void *pdata)
{
	u8 *p;
	OS_ERR err;
    OS_MSG_SIZE msg_size;
    
	while(1)
	{
		//p = OSQPend(q_msg, 0, &err);//������Ϣ����
		p = OSQPend(&q_msg, 0, OS_OPT_PEND_BLOCKING, &msg_size, NULL, &err);
		LCD_ShowString(5,170,240,16,16,p);//��ʾ��Ϣ
		
 		if(p) myfree(SRAMIN, p);	  
		delay_ms(500);	 
	}									 
}

//������
void main_task(void *pdata)
{							 
	u32 key=10;	
	OS_ERR err;	
 	u8 tmr3sta=0;	//������ʱ��3����״̬
	
 	//tmr1 = OSTmrCreate(10,10,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr1_callback,0,"tmr1",&err);//100msִ��һ��
 	OSTmrCreate(&tmr1, "tmr1", 10,10, OS_OPT_TMR_PERIODIC, (OS_TMR_CALLBACK_PTR)tmr1_callback, 0, &err);
    printf("OSTmrCreate ret %d\r\n", err);
    
	//tmr2 = OSTmrCreate(10,900,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr2_callback,0,"tmr2",&err);//9000msִ��һ��
	OSTmrCreate(&tmr2, "tmr1", 10,900, OS_OPT_TMR_PERIODIC, (OS_TMR_CALLBACK_PTR)tmr2_callback, 0, &err);
    printf("OSTmrCreate ret %d\r\n", err);
    
	//tmr3 = OSTmrCreate(10,20,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr3_callback,0,"tmr3",&err);//200msִ��һ��
	OSTmrCreate(&tmr3, "tmr3", 10,20, OS_OPT_TMR_PERIODIC, (OS_TMR_CALLBACK_PTR)tmr3_callback, 0, &err);
    printf("OSTmrCreate ret %d\r\n", err);
    
	OSTmrStart(&tmr1,&err);//����������ʱ��1				 
    printf("OSTmrStart ret %d\r\n", err);
	OSTmrStart(&tmr2,&err);//����������ʱ��2		
    printf("OSTmrStart ret %d\r\n", err);
	OSTmrStart(&tmr3,&err);//����������ʱ��3		
    printf("OSTmrStart ret %d\r\n", err);
	
 	while(1)
	{
	    OS_MSG_SIZE msg_size;
        OS_STATE key;
        u8* p;
            
		//key=(u32)OSMboxPend(msg_key, 10, &err); 
		p = OSQPend(&msg_key, 0, OS_OPT_PEND_BLOCKING, &msg_size, NULL, &err);
		if(p) {
		    key = (OS_STATE)p[0];
			OSFlagPost(&flags_key, 1<<(key-1), OS_OPT_POST_FLAG_SET, &err);//���ö�Ӧ���ź���Ϊ1
		} else {
		    key = 0;
        }
        
		switch(key)
		{
			case 1://����DS1
				LED1=!LED1;
				break;
                
			case 2://����������ʱ��3	 
				tmr3sta=!tmr3sta;
				if(tmr3sta) OSTaskSuspend(&qmsg_task_tcb, &err);
				else OSTaskResume(&qmsg_task_tcb, &err);
 				break;
                
			case 3://���
 				LCD_Fill(0,221,129,lcddev.height,WHITE);
				break;
                
			case 4://У׼
				OSTaskSuspend(&touch_task_tcb, &err);						//������������		 
				OSTaskSuspend(&qmsg_task_tcb, &err);	 				//���������Ϣ��ʾ����		 
 				OSTmrStop(&tmr1, OS_OPT_TMR_NONE, 0, &err);				//�ر�������ʱ��1
 				if((tp_dev.touchtype&0X80)==0) TP_Adjust();   
				OSTmrStart(&tmr1, &err);				//���¿���������ʱ��1
 				OSTaskResume(&touch_task_tcb, &err);		//���
 				OSTaskResume(&qmsg_task_tcb, &err); 	//���
				ucos_load_main_ui();				//���¼���������		 
				break;
                
			case 5://clear log info, TPAD
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
	OS_ERR err;	    
    
	while(1)
	{
		flags = OSFlagPend(&flags_key, 0X001F, 0, OS_OPT_PEND_FLAG_SET_ANY, 0, &err);//�ȴ��ź���

        //printf("flags_task> flags = 0x%x \r\n", flags);
 		if(flags&0X0001) LCD_ShowString(140,162,240,16,16,"KEY0 DOWN  "); 
		if(flags&0X0002) LCD_ShowString(140,162,240,16,16,"KEY1 DOWN  "); 
		if(flags&0X0004) LCD_ShowString(140,162,240,16,16,"KEY2 DOWN  "); 
		if(flags&0X0008) LCD_ShowString(140,162,240,16,16,"KEY_UP DOWN"); 
		if(flags&0X0010) LCD_ShowString(140,162,240,16,16,"TPAD DOWN  "); 

		BEEP=1;
		delay_ms(50);
		BEEP=0;
		OSFlagPost(&flags_key, 0X001F, OS_OPT_PEND_FLAG_CLR_ALL, &err);//ȫ���ź�������
 	}
}
   		    
//����ɨ������
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
                //OSMboxPost(msg_key,(void*)key);//������Ϣ
                OSQPost(&msg_key, (void *)p, 8, 0, &err);
            }
        }
 		delay_ms(10);
	}
}
