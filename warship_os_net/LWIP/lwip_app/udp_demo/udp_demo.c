
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
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK 战舰开发板 V3
//UDP 测试代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2015/3/15
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
//*******************************************************************************
//修改信息
//无
////////////////////////////////////////////////////////////////////////////////// 	   
 
//UDP接收数据缓冲区
u8 udp_demo_recvbuf[UDP_DEMO_RX_BUFSIZE];	//UDP接收数据缓冲区 

//UDP发送数据内容
const u8 *tcp_demo_sendbuf = "WarShip STM32F103 UDP demo send data\r\n";

//UDP 测试全局状态标记变量
//bit7:没有用到
//bit6:0,没有收到数据;1,收到数据了.
//bit5:0,没有连接上;1,连接上了.
//bit4~0:保留
u8 udp_demo_flag;

//UDP测试
void udp_demo_test(void)
{
 	err_t err;
	struct udp_pcb *udppcb;  	//定义一个TCP服务器控制块
	struct ip4_addr rmtipaddr;  	//远端ip地址
 	
	u8 *tbuf;
 	u8 key;
	u8 res=0;		
	u8 t=0; 
 	
	lcd_print_log(NULL);	//清屏
	POINT_COLOR = RED; 	//红色字体
	lcd_print_log("STM32F1 UDP Test");
	lcd_print_log("KEY0:Send data");  
	lcd_print_log("KEY_UP:Quit"); 
    
	tbuf = mymalloc(SRAMIN, 200);	//申请内存
	if(tbuf == NULL) return ;		//内存申请失败了,直接退出
	
	sprintf((char*)tbuf,"Local IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);//服务器IP
	lcd_print_log(tbuf);  
	sprintf((char*)tbuf,"Local Port:%d", UDP_DEMO_PORT);//客户端端口号
	lcd_print_log(tbuf);  
    
	udppcb = udp_new();
	if(udppcb)//创建成功
	{ 
        err = udp_bind(udppcb, IP_ADDR_ANY, UDP_DEMO_PORT);//绑定本地IP地址与端口号
        if (err == ERR_OK) //绑定完成
        {
            lcd_print_log("STATUS:bind ok   ");//标记连接上了(UDP是非可靠连接,这里仅仅表示本地UDP已经准备好)
            udp_recv(udppcb, udp_demo_recv, NULL);//注册接收回调函数 
            udp_demo_flag |= 1<<5;          //标记已经连接上
        }else res=1;
	}else res=1;
    
	while(res==0)
	{
		key = KEY_Scan(0);
		if(key==WKUP_PRES) break;
        
		if (key == KEY0_PRES)//KEY0按下了,发送数据
		{
			udp_demo_senddata(udppcb);
		}
        
		if(udp_demo_flag&(1<<6)) //是否收到数据
		{
            sprintf((char*)tbuf,"Client IP:%d.%d.%d.%d",lwipdev.remoteip[0],lwipdev.remoteip[1],lwipdev.remoteip[2],lwipdev.remoteip[3]);//客户端IP
            lcd_print_log(tbuf);
            lcd_print_log("Receive Data:");//提示消息       
			lcd_print_log(udp_demo_recvbuf);//显示接收到的数据			
			udp_demo_flag &= ~(1<<6);//标记数据已经被处理了.
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

//UDP回调函数
void udp_demo_recv(void *arg,struct udp_pcb *upcb,struct pbuf *p, struct ip4_addr *addr,u16_t port)
{
	u32 data_len = 0;
	struct pbuf *q;
    
	if(p!=NULL)	//接收到不为空的数据时
	{
		memset(udp_demo_recvbuf, 0, UDP_DEMO_RX_BUFSIZE);  //数据接收缓冲区清零
		for(q=p;q!=NULL;q=q->next)  //遍历完整个pbuf链表
		{
			//判断要拷贝到UDP_DEMO_RX_BUFSIZE中的数据是否大于UDP_DEMO_RX_BUFSIZE的剩余空间，如果大于
			//的话就只拷贝UDP_DEMO_RX_BUFSIZE中剩余长度的数据，否则的话就拷贝所有的数据
			if(q->len > (UDP_DEMO_RX_BUFSIZE-data_len)) 
                memcpy(udp_demo_recvbuf+data_len,q->payload,(UDP_DEMO_RX_BUFSIZE-data_len));//拷贝数据
			else 
                memcpy(udp_demo_recvbuf+data_len,q->payload,q->len);
			data_len += q->len;  	
			if(data_len > UDP_DEMO_RX_BUFSIZE) break; //超出TCP客户端接收数组,跳出	
		}
        
		upcb->remote_ip =*addr; 				//记录远程主机的IP地址
		upcb->remote_port = port;  			//记录远程主机的端口号
		lwipdev.remoteip[0]=upcb->remote_ip.addr&0xff; 		//IADDR4
		lwipdev.remoteip[1]=(upcb->remote_ip.addr>>8)&0xff; //IADDR3
		lwipdev.remoteip[2]=(upcb->remote_ip.addr>>16)&0xff;//IADDR2
		lwipdev.remoteip[3]=(upcb->remote_ip.addr>>24)&0xff;//IADDR1 
		udp_demo_flag |= 1<<6;	//标记接收到数据了
		pbuf_free(p);//释放内存
	}
    else
	{
		udp_disconnect(upcb); 
		POINT_COLOR=BLUE;
		lcd_print_log("Connect break！");  
		udp_demo_flag &= ~(1<<5);	//标记连接断开
	} 
} 

//UDP服务器发送数据
void udp_demo_senddata(struct udp_pcb *upcb)
{
	struct pbuf *ptr;
    
	ptr = pbuf_alloc(PBUF_TRANSPORT,strlen((char*)tcp_demo_sendbuf),PBUF_POOL); //申请内存
	if(ptr)
	{
		ptr->payload=(void*)tcp_demo_sendbuf; 
		udp_send(upcb, ptr);	//udp发送数据 
		pbuf_free(ptr);//释放内存
	} 
} 

//关闭UDP连接
void udp_demo_connection_close(struct udp_pcb *upcb)
{
	udp_disconnect(upcb); 
	udp_remove(upcb);			//断开UDP连接 
	udp_demo_flag &= ~(1<<5);	//标记连接断开
	
	POINT_COLOR=BLUE;
    lcd_print_log("Connect break！");  
}



