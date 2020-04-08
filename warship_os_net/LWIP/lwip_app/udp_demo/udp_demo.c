
#include "udp_demo.h" 
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "malloc.h"
#include "stdio.h"
#include "string.h" 

//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK ս�������� V3
//UDP ���Դ���	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2015/3/15
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved									  
//*******************************************************************************
//�޸���Ϣ
//��
////////////////////////////////////////////////////////////////////////////////// 	   
 
//UDP�������ݻ�����
u8 udp_demo_recvbuf[UDP_DEMO_RX_BUFSIZE];	//UDP�������ݻ����� 

//UDP������������
const u8 *tcp_demo_sendbuf = "WarShip STM32F103 UDP demo send data\r\n";

//UDP ����ȫ��״̬��Ǳ���
//bit7:û���õ�
//bit6:0,û���յ�����;1,�յ�������.
//bit5:0,û��������;1,��������.
//bit4~0:����
u8 udp_demo_flag;

//UDP����
void udp_demo_test(void)
{
 	err_t err;
	struct udp_pcb *udppcb;  	//����һ��TCP���������ƿ�
	struct ip4_addr rmtipaddr;  	//Զ��ip��ַ
 	
	u8 *tbuf;
 	u8 key;
	u8 res=0;		
	u8 t=0; 
 	
	lcd_print_log(NULL);	//����
	POINT_COLOR = RED; 	//��ɫ����
	lcd_print_log("STM32F1 UDP Test");
	lcd_print_log("KEY0:Send data");  
	lcd_print_log("KEY_UP:Quit"); 
    
	tbuf = mymalloc(SRAMIN, 200);	//�����ڴ�
	if(tbuf == NULL) return ;		//�ڴ�����ʧ����,ֱ���˳�
	
	sprintf((char*)tbuf,"Local IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);//������IP
	lcd_print_log(tbuf);  
	sprintf((char*)tbuf,"Local Port:%d", UDP_DEMO_PORT);//�ͻ��˶˿ں�
	lcd_print_log(tbuf);  
    
	udppcb = udp_new();
	if(udppcb)//�����ɹ�
	{ 
        err = udp_bind(udppcb, IP_ADDR_ANY, UDP_DEMO_PORT);//�󶨱���IP��ַ��˿ں�
        if (err == ERR_OK) //�����
        {
            lcd_print_log("STATUS:bind ok   ");//�����������(UDP�Ƿǿɿ�����,���������ʾ����UDP�Ѿ�׼����)
            udp_recv(udppcb, udp_demo_recv, NULL);//ע����ջص����� 
            udp_demo_flag |= 1<<5;          //����Ѿ�������
        }else res=1;
	}else res=1;
    
	while(res==0)
	{
		key = KEY_Scan(0);
		if(key==WKUP_PRES) break;
        
		if (key == KEY0_PRES)//KEY0������,��������
		{
			udp_demo_senddata(udppcb);
		}
        
		if(udp_demo_flag&(1<<6)) //�Ƿ��յ�����
		{
            sprintf((char*)tbuf,"Client IP:%d.%d.%d.%d",lwipdev.remoteip[0],lwipdev.remoteip[1],lwipdev.remoteip[2],lwipdev.remoteip[3]);//�ͻ���IP
            lcd_print_log(tbuf);
            lcd_print_log("Receive Data:");//��ʾ��Ϣ       
			lcd_print_log(udp_demo_recvbuf);//��ʾ���յ�������			
			udp_demo_flag &= ~(1<<6);//��������Ѿ���������.
		} 
        
		//lwip_periodic_handle();
		//lwip_pkt_handle();
        
		delay_ms(2);
		t++;
		if(t==200)
		{
			t=0;
			LED0=!LED0;
		}
	}
    
	udp_demo_connection_close(udppcb); 
	myfree(SRAMIN,tbuf);
} 

//UDP�ص�����
void udp_demo_recv(void *arg,struct udp_pcb *upcb,struct pbuf *p, struct ip4_addr *addr,u16_t port)
{
	u32 data_len = 0;
	struct pbuf *q;
    
	if(p!=NULL)	//���յ���Ϊ�յ�����ʱ
	{
		memset(udp_demo_recvbuf, 0, UDP_DEMO_RX_BUFSIZE);  //���ݽ��ջ���������
		for(q=p;q!=NULL;q=q->next)  //����������pbuf����
		{
			//�ж�Ҫ������UDP_DEMO_RX_BUFSIZE�е������Ƿ����UDP_DEMO_RX_BUFSIZE��ʣ��ռ䣬�������
			//�Ļ���ֻ����UDP_DEMO_RX_BUFSIZE��ʣ�೤�ȵ����ݣ�����Ļ��Ϳ������е�����
			if(q->len > (UDP_DEMO_RX_BUFSIZE-data_len)) 
                memcpy(udp_demo_recvbuf+data_len,q->payload,(UDP_DEMO_RX_BUFSIZE-data_len));//��������
			else 
                memcpy(udp_demo_recvbuf+data_len,q->payload,q->len);
			data_len += q->len;  	
			if(data_len > UDP_DEMO_RX_BUFSIZE) break; //����TCP�ͻ��˽�������,����	
		}
        
		upcb->remote_ip =*addr; 				//��¼Զ��������IP��ַ
		upcb->remote_port = port;  			//��¼Զ�������Ķ˿ں�
		lwipdev.remoteip[0]=upcb->remote_ip.addr&0xff; 		//IADDR4
		lwipdev.remoteip[1]=(upcb->remote_ip.addr>>8)&0xff; //IADDR3
		lwipdev.remoteip[2]=(upcb->remote_ip.addr>>16)&0xff;//IADDR2
		lwipdev.remoteip[3]=(upcb->remote_ip.addr>>24)&0xff;//IADDR1 
		udp_demo_flag |= 1<<6;	//��ǽ��յ�������
		pbuf_free(p);//�ͷ��ڴ�
	}
    else
	{
		udp_disconnect(upcb); 
		POINT_COLOR=BLUE;
		lcd_print_log("Connect break��");  
		udp_demo_flag &= ~(1<<5);	//������ӶϿ�
	} 
} 

//UDP��������������
void udp_demo_senddata(struct udp_pcb *upcb)
{
	struct pbuf *ptr;
    
	ptr = pbuf_alloc(PBUF_TRANSPORT,strlen((char*)tcp_demo_sendbuf),PBUF_POOL); //�����ڴ�
	if(ptr)
	{
		ptr->payload=(void*)tcp_demo_sendbuf; 
		udp_send(upcb, ptr);	//udp�������� 
		pbuf_free(ptr);//�ͷ��ڴ�
	} 
} 

//�ر�UDP����
void udp_demo_connection_close(struct udp_pcb *upcb)
{
	udp_disconnect(upcb); 
	udp_remove(upcb);			//�Ͽ�UDP���� 
	udp_demo_flag &= ~(1<<5);	//������ӶϿ�
	
	POINT_COLOR=BLUE;
    lcd_print_log("Connect break��");  
}



