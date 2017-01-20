/***********************************************************
 * ��Ȩ��Ϣ: ����Ƽ���Ȩ���У�����һ��Ȩ��
 * �ļ�����: drv_tda.c
 * �ļ�����: linsl
 * �������: 2015-09-22
 * ��ǰ�汾: 1.0.0
 * ��Ҫ����: SPAM��DS8007��доƬ����
 * �汾��ʷ: 
 ***********************************************************/
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux/swab.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <plat/gpmc.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <asm/delay.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/sched.h>


#define G_VER           "v1.00"
#define DEV_NAME        "gv_tda"
#define DEV_FILE_NAME   "TDAmodule"

#define DEV_MAJOR       242

//����8007ʱ��Ƶ��
#define CRYSTAL_FREQUENCY_8007 14745600L  

#define CARD_SLOTS  2

/** 8007 Register definitions */
#define CSR     0x00	 /** Offset address of CSR */
#define CCR     0x01	 /** Offset address of CCR */
#define PDR     0x02	 /** Offset address of PDR */
#define UCR2    0x03	 /** Offset address of UCR2*/
#define GTR     0x05	 /** Offset address of GTR */
#define UCR1    0x06	 /** Offset address of UCR1*/
#define PCR     0x07	 /** Offset address of PCR */
#define TOC     0x08	 /** Offset address of TOC */
#define TOR1    0x09	 /** Offset address of TOR1*/
#define TOR2    0x0A	 /** Offset address of TOR2*/
#define TOR3    0x0B	 /** Offset address of TOR3*/
#define MSR     0x0C	 /** Offset address of MSR */
#define FCR     0x0C	 /** Offset address of FCR */
#define UTR     0x0D	 /** Offset address of UTR */
#define URR     0x0D	 /** Offset address of URR */
#define USR     0x0E	 /** Offset address of USR */
#define HSR     0x0F	 /** Offset address of HSR */

/** Bit definitions of CSR register */
#define CSR_ID_MASK         0xF0  /** Device Identification Register Bits */
#define CSR_nRIU_MASK       0x08  /** Reset ISO UART bit */
#define CSR_SC3_MASK        0x04  /** Select Card Configuration Bits */
#define CSR_SC2_MASK        0x02	  
#define CSR_SC1_MASK        0x01      

/** Bit definitions of HSR register */
#define HSR_PRTL2_MASK      0x40  /** Protection Card Interface B Status Bit */
#define HSR_PRTL1_MASK      0x20  /** Protection Card Interface A Status Bit */
#define HSR_SUPL_MASK       0x10  /** Supervisor Latch bit */
#define HSR_PRL2_MASK       0x08  /** Presence Latch B bit */
#define HSR_PRL1_MASK       0x04  /** Presence Latch A bit */
#define HSR_INTAUXL_MASK    0x02  /** INTAUX Latch bit */
#define HSR_PTL_MASK        0x01  /** Protection Thermal Latch bit */

/** Bit definitions of MSR register */
#define MSR_CLKSW_MASK      0x80  /** Clock Switch */
#define MSR_FE_MASK         0x40  /** FIFO Empty Status Bit */
#define MSR_BGT_MASK        0x20  /** Block Guard Time Status Bit */
#define MSR_CRED_MASK       0x10  /** Control Ready */
#define MSR_PR2_MASK        0x08  /** Presence Card B */
#define MSR_PR1_MASK        0x04  /** Presence Card A */
#define MSR_INTAUX_MASK     0x02  /** INTAUX bit */
#define MSR_TBE_RBF_MASK    0x01  /** Transmit Buffer Empty/Receive Buffer Full bit */

/** Bit definitions of FCR register */
#define FCR_PEC_MASK        0x70  /** Parity Error Count bits */
#define FCR_FL_MASK         0x07  /** FIFO Length bits */

/** Bit definitions of USR register */
#define USR_TOL3_MASK       0x80  /** Time-Out Counter 3 Status bit */
#define USR_TOL2_MASK       0x40  /** Time-Out Counter 2 Status bit */
#define USR_TOL1_MASK       0x20  /** Time-Out Counter 1 Status bit */
#define USR_EA_MASK         0x10  /** Early Answer Detected bit */
#define USR_PE_MASK         0x08  /** Parity Error bit */
#define USR_OVR_MASK        0x04  /** Overrun FIFO bit */
#define USR_FER_MASK        0x02  /** Framing Error bit */
#define USR_TBE_RBF_MASK    0x01  /** Transmit Buffer Empty/Receive Buffer Full bit */

/** Bit definitions of UCR1 register */
#define UCR1_FTE0_MASK      0x80  /** */
#define UCR1_FIP_MASK       0x40  /** Force Inverse Parity bit */
#define UCR1_PROT_MASK      0x10  /** Protocol Select bit */
#define UCR1_T_R_MASK       0x08  /** Transmit/Receive bit */
#define UCR1_LCT_MASK       0x04  /** Last Character to Transmit bit */
#define UCR1_SS_MASK        0x02  /** Software Convention Setting bit */
#define UCR1_CONV_MASK      0x01  /** Convention bit */

/** Bit definitions of UCR2 register */
#define UCR2_DISTBE_RBF_MASK 0x40 /** Disable TBE/RBF Interrupt bit */
#define UCR2_DISAUX_MASK     0x20 /** Disable Auxiliary Interrupt bit */
#define UCR2_PDWN_MASK       0x10 /** Power Down Mode Enable bit */
#define UCR2_SAN_MASK        0x08 /** Synchronous/asynchronous Card Select bit */
#define UCR2_nAUTOCONV_MASK  0x04 /** Auto Convention Disable bit */
#define UCR2_CKU_MASK        0x02 /** Clock UART Doubler Enable bit */
#define UCR2_PSC_MASK        0x01 /** Prescaler Select bit */

/** Bit definitions of CCR register */
#define CCR_SHL_MASK        0x20  /** Stop High Low Select bit */
#define CCR_CST_MASK        0x10  /** Clock Stop Enable bit */
#define CCR_SC_MASK         0x08  /** Synchronous Clock bit */
#define CCR_AC_MASK         0x07  /** Alternating Clock Select bit */

/** Bit definitions of PCR register */
#define PCR_C8_MASK         0x20  /** Contact 8 bit */
#define PCR_C4_MASK         0x10  /** Contact 4 bit */
#define PCR_1V8_MASK        0x08  /** 1.8V Card Select bit */
#define PCR_RSTIN_MASK      0x04  /** Reset Bit */
#define PCR_3V_5V_MASK      0x02  /** 3V/5V Card Select bit */
#define PCR_START_MASK      0x01  /** Start bit, It initiates activation sequence */


#define POWERUP_5V      	0
#define POWERUP_3V      	1
#define POWERUP_1V8     	2

#define PROTOCOL_T0   		0
#define PROTOCOL_T1   		1    

#define LOW    				0
#define HIGH   				1 

//����ioctl�ӿں���
#define IOCTL_PSAM_APDU_DISPOSE 		0			//����APDU��������Ӧ����
#define IOCTL_ONE_PSAM_RESET_DISPOSE 	1			//��λ����PSAM������
#define IOCTL_ALL_PSAM_RESET_DISPOSE 	2			//��λ����PSAM������
#define IOCTL_TDA_POWERUP_DISPOSE		3			//TDA�ϵ����
#define IOCTL_TDA_POWERDOWN_DISPOSE		4			//TDA�µ����
#define IOCTL_ReadPsamInfo_DISPOSE		5			//��ȡPSAM�������Ϣ���� 

#define GET_PSAM_TERMID					0			//ȡPSAM���ն˻����
#define Enter_PSAM_DF01					1			//��PSAM��DF01Ŀ¼
#define Enter_PSAM_3F01					2			//��PSAM��3F01Ŀ¼
#define Enter_PSAM_1001					3			//��PSAM��1001Ŀ¼
#define CAL_PSAM_DESINIT				4			//DES��ʼ��
#define CAL_PSAM_DES					5			//DES����
#define CAL_PSAM_DESINIT_GB				6			//DES����
#define GET_PSAM_RAND					7			//ȡ�����
#define Enter_PSAM_3F00					8			//��PSAM��3F00Ŀ¼
 

//======= ������붨�� ========================================
#define NO_ERROR					0 			//�ɹ�

#define RESET_PSAM_NOT_3B3F_ERROR		-1 			//��λPSAM��ʱ,��λ��Ϣ�ĵ�һ���ֽڲ���3B��3F
#define RESET_PSAM_TIMEOUT				-2			//��λPSAM����ʱ
#define SELECT_CARDVOLTAGE_ERROR		-3			//����ѹѡ�����
#define TDA_POWERUP_ERROR				-4			//TDA����PSAM��ʧ��
#define SELECT_PSAM_NUMBER_ERROR		-5			//ѡ��PSAM������
#define TIMECOS_CMD_LEN_ERROR			-6			//Timecosָ��ȳ���
#define DISPOSE_PSAM_T1_TIMEOUT_ERROR	-7			//����T=1��PSAM����ʱ
#define PSAM_T1_RETURN_LEN_ERROR		-8			//T=1��PSAM���������ݳ��ȳ���
#define PSAM_RESPONSE_BCC_ERROR			-9			//T=1��PSAM���������ݵ�bccУ�����
#define DISPOSE_PSAM_PPS_TIMEOUT_ERROR	-10			//PPSʧ��
#define DISPOSE_PSAM_T0_TIMEOUT_ERROR	-11			//����T=0��PSAM����ʱ
#define PSAM_T0_RESPONSE_NOT_C0			-12			//T=0�������صĵ�3��ʱû��0xc0��ͷ
#define PSAM_T0_RESPONSE_NOT_CMD_HEADER	-13			//T=0����������С��6�ֽ��ǣ����ص�����û������ͷ
#define KERNEL_TIMEOUT					-14			//�ں˶�ʱ����ʱ��ʱ




u8 PSAMInfoBuf[500];				//����PSAM����λ��Ϣ	

u8 g_ATR_buf[500];				//ATR���ݻ�����,����λ����������Ϊȫ�ֱ�������PPSʱ���õ���λ����2012-2-20
u8 g_ATRlen;						//ATR���ݳ���					
 
u8 g_cardtype;				//��Ź���������
u8 g_cardtype_buf[500];  		//��Э������  (T=0 or T=1)     ע : g_cardtype_buf[0]  	δ��
u8 g_cmdtype;				//��Ź�����������
u8 g_cmdtype_buf[500]; 		//����������  (����T=1������)  ע : g_cmdtype_buf[0]	δ��

u8 g_resetspeed_buf[500];		//��λ�ٶȱ�־������ ע��g_resetspeed_buf[0]δ�� 0--���� 1--����



//����IOCTL��������Ϊ�����뷢������ 
typedef struct 
{
	u8   psamnum;				//PSAM��
	u8	datalen;	     		//���ݳ��� (�������Ȼ���Ӧ���ݳ���)
	u8	databuf[130];   		//���ݻ�����
	u8   resetbuf[6];			//��Ÿ�λ��Ϣ (1--���� 0--������) 
}IoctlInfo;

static IoctlInfo g_SendInfo; 	//��������ṹ��
static IoctlInfo g_RecvInfo; 	//������Ӧ�ṹ��


struct gv_tda_io
{
	int io_wr_out;
	int io_rd_out;

	int io_psam_select1;
	int io_psam_select2;
	int io_psam_select3;

	int io_psam_led1;
	int io_psam_led2;
	int io_psam_led3;
	int io_psam_led4;
	int io_psam_led5;
	int io_psam_led6;
};

//�豸��
static struct class *tda_class;

//�豸˽������
struct gv_dev 
{
    char *name;
    int id;

    void __iomem *ioaddr;

    struct platform_device *pdev;
    struct cdev cdev;
    dev_t devno; //device node num
	int irq;

    wait_queue_head_t recv_wait;
    u32 recv_ready;

    spinlock_t recv_lock;

    struct mutex open_lock;

	struct gv_tda_io *dio;

	u8 recv_buf[1024];
	int rlen;
	
	u8 atr_buf[1024]; //reset buf
};

static  void __iomem *tda_addr = NULL;
static struct gv_tda_io *tda_io = NULL;

/*****************************************************************
 �������ƣ�cal_bcc
 �����������������ݵ�BCCУ��
 ���������buf������
		   len�����ݴ�С
 �����������
 ����˵����BCCֵ
 ����˵����
 *****************************************************************/
static u8 cal_bcc(u8 *buf, int len)
{
	u8 bcc = 0;	
	int i = 0;

	for (i=0; i<len; i++)
	{
		bcc ^= buf[i];
	}
	
	return bcc;
}

/*****************************************************************
 �������ƣ�gv_io_out
 ����������GPIO�����ƽ
 ���������ionum���ܽű��
		   val��ƽ0,1
 �����������
 ����˵������
 ����˵����
 *****************************************************************/
static inline void gv_io_out(int ionum, int val)
{   
    gpio_set_value(ionum, val);
}

/*****************************************************************
 �������ƣ�gv_io_in
 ������������GPIO�ĵ�ƽ
 �����������
 �����������
 ����˵������ƽ�����״̬
 ����˵����
 *****************************************************************/
static inline int gv_io_in(int ionum)
{   
    return gpio_get_value(ionum);
}

/*****************************************************************
 �������ƣ�psam_led_control
 ��������������PSAM����ָʾ��
 ���������psam_idx�����۱��
		   onoff, ����
 �����������
 ����˵������
 ����˵����
 *****************************************************************/
static void psam_led_control(u8 psam_idx , u8 onoff )
{
	switch(psam_idx)
	{
		case 1:						//PSAM1ָʾ��ģʽ
			if(onoff)
			{
				gv_io_out(tda_io->io_psam_led1, 1);
			}	
			else
			{
				gv_io_out(tda_io->io_psam_led1, 0);
			}	
			break;	
		
		case 2:						//PSAM2ָʾ��ģʽ
			if(onoff)
			{
				gv_io_out(tda_io->io_psam_led2, 1);
			}	
			else
			{
				gv_io_out(tda_io->io_psam_led2, 0);
			}	
			break;	
		
		case 3:						//PSAM3ָʾ��ģʽ
			if(onoff)
			{
				gv_io_out(tda_io->io_psam_led3, 1);
			}	
			else
			{
				gv_io_out(tda_io->io_psam_led3, 0);
			}	
			break;	
		
		case 4:						//PSAM4ָʾ��ģʽ
			if(onoff)
			{
				gv_io_out(tda_io->io_psam_led4, 1);
			}	
			else
			{
				gv_io_out(tda_io->io_psam_led4, 0);
			}	
			break;				
		
		case 5:						//PSAM5ָʾ��ģʽ
			if(onoff)
			{
				gv_io_out(tda_io->io_psam_led5, 1);
			}	
			else
			{
				gv_io_out(tda_io->io_psam_led5, 0);
			}	
			break;	
		
		case 6:						//PSAM6ָʾ��ģʽ
			if(onoff)
			{
				gv_io_out(tda_io->io_psam_led6, 1);
			}	
			else
			{
				gv_io_out(tda_io->io_psam_led6, 0);
			}	
			break;	
		
		default:
			break;			
	}
}

/*****************************************************************
 �������ƣ�tda_write
 ������������оƬ�Ĵ���д������
 ���������reg���Ĵ������
		   val������
 �����������
 ����˵������
 ����˵����
 *****************************************************************/
static void tda_write(u8 reg, u8 val)
{ 
   volatile unsigned char *offset  = ((volatile unsigned char *)tda_addr) + reg;
	
	gv_io_out(tda_io->io_rd_out, 0);
	offset[0] = val;	
	
	udelay(5);
}

/*****************************************************************
 �������ƣ�tda_read
 ������������ȡоƬ�Ĵ�������
 ���������reg���Ĵ������
 �����������
 ����˵�����Ĵ�������
 ����˵����
 *****************************************************************/
static u8 tda_read(u8 reg) 
{
	volatile unsigned char *offset  = ((volatile unsigned char *)tda_addr) + reg;
	
	gv_io_out(tda_io->io_rd_out, 1);
	return offset[0];
}


volatile int g_count1;
volatile int g_count2;
/*****************************************************************
 �������ƣ�iso7816_sendbyte
 ������������SPAM��д��һ�ż�����
 ���������content����������
		   lct���Ƿ����һ���ֽ�����
 �����������
 ����˵����0���ɹ��������豸
 ����˵����
 *****************************************************************/
static int iso7816_sendbyte(u8 content, u8 lct)
{
		int timeout_us = 100 * 1000;
	  u8 val = tda_read(UCR1);
	  
	  if(lct)
	  		tda_write(UCR1, val | (UCR1_T_R_MASK  |UCR1_LCT_MASK));	//T/R=1 and LCT=1
	  else
	  		tda_write(UCR1, (val | UCR1_T_R_MASK) & ~UCR1_LCT_MASK);	//T/R=1 and LCT=0

	  tda_write(UTR,content); 

      //������͵������һ���ַ��Ͳ����жϷ��ͻ������Ƿ�Ϊ�գ�ֱ���˳�����ΪLCT=1ʱ
      //���������һ���ַ���USR_TBE_RBF_MASK�Զ�����0����ȥ�����ܵ���һֱ��ⲻ�������������ѭ����
      //����PSAM�����ֹ������� 2012-2-20 
      if(lct)
      {
	       	 return 0;   
      }  

	  while (!(tda_read(USR) & USR_TBE_RBF_MASK))					//��Ϊ����LCT,�������һ���ֽ�ʱ�������ж�	
	  {																//�����ڴ���ѭUSR
	  	   //֮ǰ��10000,�����ں��ݲ���ʱ�������׳�ʱ(��ʱ�ı�����У��MAC2ʧ��)
	  	   //�޸ĺ������� ������ 2009-5-19 �ں��ݲ���	  	    
	  	    udelay(1);
	  	    if (timeout_us-- < 0)
	  	    {
	  	   		return KERNEL_TIMEOUT;		  	    	
	  	    }	  	    
	  }

	  return 0;	  
}

/*****************************************************************
 �������ƣ�tda_timer_start
 ��������������оƬ�ڶ�ʱ��������PSAM ETU
 �����������
 �����������
 ����˵������
 ����˵����
 *****************************************************************/
static void tda_timer_start(u8 mode, u8 tor3, u8 tor2, u8 tor1)
{
	  u8 i,val;
	  
	  tda_write(TOC ,0x00);
	  for(i=0;i<200;i++);
	  tda_write(TOR3,tor3);
	  for(i=0;i<200;i++);
	  tda_write(TOR2,tor2);
	  for(i=0;i<200;i++);
	  tda_write(TOR1,tor1); 
	  for(i=0;i<200;i++);
	  tda_write(TOC ,mode);      		//����timer 
	  
//	  ds8007_int=0;
//	  atomic_set(&ds8007_int,0);
	  
  
 	  g_count1=0; 
	  g_count2=0;
	  
	  val=tda_read(USR);					//���־λ
	  val=tda_read(URR);
}

/*****************************************************************
 �������ƣ�tda_timer_stop
 ����������ֹͣ��ʱ��
 �����������
 �����������
 ����˵������
 ����˵����
 *****************************************************************/
void tda_timer_stop(void)
{
	  tda_write(TOC ,0x00);
}

/*****************************************************************
 �������ƣ�iso7816_sendcmd
 ������������SPAM����������APDU
 ���������buff������
		   len�������
 �����������
 ����˵�����ɹ���ʶ
 ����˵����
 *****************************************************************/
static int iso7816_sendcmd( u8 *buff, int len )      				                 
{                                                           
	int i;
	int ret;
	
	for(i=0;i<len;i++) 
	{
		if(i==(len-1))
			ret=iso7816_sendbyte(buff[i],1);		// LCT=1	
		else
			ret=iso7816_sendbyte(buff[i],0);	
			
		if(ret!=0)
		{
			return ret; 				
		}			
	}

	return 0;	
} 

/*****************************************************************
 �������ƣ�send_pps
 ������������PSAM��������������
 ���������protocoltype, T0, ����T1
 �����������
 ����˵�����ɹ���ʶ
 ����˵����
 *****************************************************************/
static int send_pps(u8 protocoltype)
{
	int i,ret,cnt;
    u8 val,USRval,MSRval;
    u8 CMDBuf[500];
	u8 PPS_RecvBuf[500];
	u8 PPS_Recvlen=0;
	
	if( protocoltype==PROTOCOL_T0 )		//T=0;
	{
		CMDBuf[0]=0XFF;
		CMDBuf[1]=0X10;
		CMDBuf[2]=0X13;					//��4����
		CMDBuf[3]=0XFC;
	}
	else 								//T=1
	{
		CMDBuf[0]=0XFF; 
		CMDBuf[1]=0X11;
		CMDBuf[2]=0X13;					//��4����
		CMDBuf[3]=0XFD;
	}
	
	tda_timer_start(0x61,0x0a,0xc3,0x00);		//���ó�ʱʱ��
	
	local_irq_disable();					//���ж�
	ret=iso7816_sendcmd( CMDBuf, 4 );				//��������
	if(ret!=0)
    {
        local_irq_enable();					//���ж�
		return ret; 
	}
	
	cnt=0;
	while(1)       
	{			
		udelay(50);						//������ʱ2009-12-17
		USRval = tda_read(USR);
		if (USRval & USR_TBE_RBF_MASK)		//�жϴ����Ƿ�������		
		{	
			for(i=0;i<4001;i++)  
			{
			     udelay(50);				//������ʱ2009-12-17
			     USRval=tda_read(USR);
			     MSRval=tda_read(MSR);
			     if( (USRval&USR_TBE_RBF_MASK) || (!(MSRval&MSR_FE_MASK)) )
			     {
					 val = tda_read(URR);					
					 PPS_RecvBuf[PPS_Recvlen++] = val; 
				     if( PPS_Recvlen>2 ) 				//�ж�PPS�Ƿ񷵻�����(���ص�������PPS������ͬ)
				     {
				    	tda_write(PDR,3);				//���÷�Ƶ  ����
						break;
					 }	 
				 } 
				 else if ( (USRval&USR_TOL3_MASK) || (i==4000) )
				 {
					  local_irq_enable();			//���ж�
					  return DISPOSE_PSAM_PPS_TIMEOUT_ERROR;   	      
				 }     
			}    
			break;     
	    } 
	//  else  if (USRval & USR_TOL3_MASK)   
		else
		{	
			cnt++;   
			if(cnt==2000)  				//����ʱ�޸ĳɴ������Ƶķ�ʽ,��Ϊ�����ʱ�Ĵ������ײ������õ��¿��������� 2009-12-17
			{
				//�ж��Ƿ�������MF1����PSAM��������Ǿ�ǿ������2012-2-20
				if(g_ATR_buf[0]==0x3b && g_ATR_buf[1]==0x7e && g_ATR_buf[2]==0x13 && g_ATR_buf[3]==0x00 && g_ATR_buf[4]==0x02 && g_ATR_buf[5]==0x15)
				{
				    printk ("PSAM֧�����ٵ�PPS����Ӧ,ǿ������!\n");   
				    tda_write(PDR,3);				//���÷�Ƶ  ���� 
				    local_irq_enable();				//���ж� 
				    return 0;  
				}    
				
				local_irq_enable();						//���ж�
				return DISPOSE_PSAM_PPS_TIMEOUT_ERROR;   	      
			}
		} 	 
	}	
	
	local_irq_enable();				//���ж� 
	
//	printk ("PPS_Recvlen = 0x%02x\n", PPS_Recvlen);
//	 for(i=0;i<PPS_Recvlen;i++) 
//	  		printk ("PPS_RecvBuf = 0x%02x\n", PPS_RecvBuf[i]);

	return 0;  

}

/*****************************************************************
 �������ƣ�tda_init
 ������������ʼ��оƬ
 �����������
 �����������
 ����˵������
 ����˵����
 *****************************************************************/
static void tda_init(void)
{
	  u8 val = 0;
	  
	  //---------  ��ѡ���κο�  ---------
	  val = tda_read(CSR);
	  tda_write(CSR,val & ~(CSR_SC1_MASK | CSR_SC2_MASK | CSR_SC3_MASK));	//����ѡ��

	  udelay(5);  

	  //------  ��A�Ĵ�����ʼ�� ---------
	  val = tda_read(CSR);
	  tda_write(CSR,(val | CSR_SC1_MASK) & ~(CSR_SC2_MASK | CSR_SC3_MASK));	//ѡ��A
	  udelay(5);  
	  val = tda_read(CSR);
	  tda_write(CSR,val & ~CSR_nRIU_MASK);	
	  udelay(5);  
	  tda_write(CSR,val | CSR_nRIU_MASK);									//RIUλ���ͼ�΢��,��λ�Ĵ���
	  udelay(5);  
	  //------  ��B�Ĵ�����ʼ�� ---------
	  val = tda_read(CSR);
	  tda_write(CSR,(val | CSR_SC2_MASK) & ~(CSR_SC1_MASK | CSR_SC3_MASK));	//ѡ��B
	  udelay(5);  
	  val = tda_read(CSR);
	  tda_write(CSR,val & ~CSR_nRIU_MASK);	
	  udelay(5);  
	  tda_write(CSR,val | CSR_nRIU_MASK);									//RIUλ���ͼ�΢��,��λ�Ĵ���
	  udelay(5);  
	  //-----------  �ر�A��  -----------
	  val = tda_read(CSR);
	  tda_write(CSR,(val | CSR_SC1_MASK) & ~(CSR_SC2_MASK | CSR_SC3_MASK));	//ѡ��A
	  udelay(5);  
	  val = tda_read(PCR);
	  tda_write(PCR,val & ~PCR_START_MASK);									//�رտ�A
	  udelay(5);  
	  //-----------  �ر�B��  -----------
	  val = tda_read(CSR);
	  tda_write(CSR,(val | CSR_SC2_MASK) & ~(CSR_SC1_MASK | CSR_SC3_MASK));	//ѡ��B
	  udelay(5);  
	  val = tda_read(PCR); 
	  tda_write(PCR,val & ~PCR_START_MASK);									//�رտ�B
	udelay(5);  
	  //---------  ��ѡ���κο�  ---------
	  val = tda_read(CSR);
	  tda_write(CSR,0);														//����ѡ��
	  udelay(5);  
	  
	  printk("tda init ok\n");
}

/*****************************************************************
 �������ƣ�tda_select_channel
 ����������
 �����������
 �����������
 ����˵�����ɹ���ʶ
 ����˵����
 *****************************************************************/
static int tda_select_channel(u8 chan)
{
	  u8 val = tda_read(CSR);
	  
	  switch (chan) 
	  {
	    case 1:
	      	tda_write(CSR, (val & ~(CSR_SC3_MASK | CSR_SC2_MASK))|CSR_SC1_MASK);		//ѡ��A	      
	      	break;
	    case 2:
	      	tda_write(CSR, (val & ~(CSR_SC3_MASK|CSR_SC1_MASK))|CSR_SC2_MASK);		//ѡ��B
	     		break;
	     		
	    default:
	    		tda_write(CSR,val & ~(CSR_SC1_MASK | CSR_SC2_MASK | CSR_SC3_MASK));		//����ѡ��
	      	return 1;
	  }	
	  
	  return 0;
}

/*****************************************************************
 �������ƣ�psam_select_card
 ��������������SPAM����
 ���������psam_num�����ۺ�
 �����������
 ����˵������
 ����˵����
 *****************************************************************/
//only connect on the tda channel 1
static void psam_select_card(int psam_num)
{
	switch(psam_num)
	{
		case 1:					//ѡ��1
			gv_io_out(tda_io->io_psam_select1, 0);
			gv_io_out(tda_io->io_psam_select2, 0);
			gv_io_out(tda_io->io_psam_select3, 0);						
			break;
			
		case 2:					//ѡ��2
			gv_io_out(tda_io->io_psam_select1, 1);
			gv_io_out(tda_io->io_psam_select2, 0);
			gv_io_out(tda_io->io_psam_select3, 0);						
			break;
		case 3:					//ѡ��3
			gv_io_out(tda_io->io_psam_select1, 0);
			gv_io_out(tda_io->io_psam_select2, 1);
			gv_io_out(tda_io->io_psam_select3, 0);						
			break;
			
		case 4:					//ѡ��4
			gv_io_out(tda_io->io_psam_select1, 1);
			gv_io_out(tda_io->io_psam_select2, 1);
			gv_io_out(tda_io->io_psam_select3, 0);						
			break;
			
		case 5:					//ѡ��5
			gv_io_out(tda_io->io_psam_select1, 0);
			gv_io_out(tda_io->io_psam_select2, 0);
			gv_io_out(tda_io->io_psam_select3, 1);						
			break;
			
		case 6:					//ѡ��6
			gv_io_out(tda_io->io_psam_select1, 1);
			gv_io_out(tda_io->io_psam_select2, 0);
			gv_io_out(tda_io->io_psam_select3, 1);						
			break;
			
		default:				//	
			gv_io_out(tda_io->io_psam_select1, 0);
			gv_io_out(tda_io->io_psam_select2, 0);
			gv_io_out(tda_io->io_psam_select3, 0);								
			break;
	}
}

/*****************************************************************
 �������ƣ�tda_powerdown
 ����������оƬ���磬����͹���ģʽ
 �����������
 �����������
 ����˵������ƽ�����״̬
 ����˵����
 *****************************************************************/
static void tda_powerdown(void)
{
  	u8 val = tda_read(PCR);		
  	tda_write(PCR,val & ~PCR_START_MASK);		//�رտ�
}

/*****************************************************************
 �������ƣ�tda_powerup
 ����������оƬ�˳��͹���ģʽ
 ���������voltage
 �����������
 ����˵�����ɹ���ʶ
 ����˵����
 *****************************************************************/
static int tda_powerup(u8 voltage)
{
	u8 val;	
		
	tda_write(CCR,0x01);			//�ⲿ����2��Ƶ  

	//----- ���ÿ������ѹ ------
	val = tda_read(PCR);	  
	tda_write(PCR,val & ~(PCR_3V_5V_MASK|PCR_1V8_MASK));		//��1.8V��3V-5Vλ
	
	switch(voltage)
	{
		case POWERUP_5V:					//ʲô�����������Ѿ���5V
			break;
			
		case POWERUP_3V:					//���ó�3V
		    val = tda_read(PCR);
		    tda_write(PCR,val | PCR_3V_5V_MASK);
		    break;
		    
		case POWERUP_1V8:					//���ó�1.8V
		  	val = tda_read(PCR);
		   	tda_write(PCR,val | PCR_1V8_MASK);
		   	break;
		   
		default:
		  	return SELECT_CARDVOLTAGE_ERROR;

	}
		
	//--------- TDA����PSAM�� ---------
	val = tda_read(PCR);
	tda_write(PCR,val & ~PCR_RSTIN_MASK);	//��λ���õ� 

	val = tda_read(HSR);						//��HSR�Ĵ�����Ϊ����HSR�Ĵ���	  
	do
	{ 
		val = tda_read(PCR);
		tda_write(PCR,val | (PCR_C8_MASK|PCR_C4_MASK|PCR_START_MASK));	//C4,C8�ø� STRATλ��1����������
		  
		val = tda_read(HSR);
		if (val & (HSR_PRTL2_MASK|HSR_PRTL1_MASK|HSR_PRL2_MASK|HSR_PRL1_MASK|HSR_PTL_MASK))
		{	 	
			 	printk("Power problem detected (HSR=)0x%2x\n",val);
		    tda_powerdown();
		    return TDA_POWERUP_ERROR;
		}
		val = tda_read(PCR);
	}while (!(val & PCR_START_MASK));	

	val = tda_read(PCR);							//�ͷŸ�λ���� 2011-5-6 �޸ģ������¼���PSAM��ʱ���ִ�����
	tda_write(PCR,val | PCR_RSTIN_MASK);

	return 0;
}

/*****************************************************************
 �������ƣ�iso7816_readbyte
 �����������ȴ�SPAM������һ���ֽ�����
 ���������timeout_ms���ȴ�ʱ��
 �����������
 ����˵����>=0 ���ݣ�������ʱʧ��
 ����˵����
 *****************************************************************/
static int iso7816_readbyte(int timeout_ms)
{
	u8 val = 0;
	u8 usr = 0;
	u8 msr = 0;
	int timeout = timeout_ms * 1000 / 2;
	
	do
	{
   	  usr = tda_read(USR); 
   	  msr = tda_read(MSR);
   	  if( (usr & USR_TBE_RBF_MASK) || (!(msr & MSR_FE_MASK)) )
   	  {
	       val = tda_read(URR);		
	       return val;
	    }
	    
	    udelay(1);
	    timeout--;
	}	while (timeout > 0);
	
	return -1;
}

/*****************************************************************
 �������ƣ�TDA_ATRsequence
 ����������
 �����������
 �����������
 ����˵����
 ����˵����
 *****************************************************************/
static int TDA_ATRsequence(int PSAMnum, u8 speed)
{
	  u8 val;
	  int i;
	  int ret;
	  	  
	  //��������ǰ��ʼ����ؼĴ���
	  val = tda_read(CSR);
	  tda_write(CSR,val & ~CSR_nRIU_MASK);			//��λ����
	  
	  val = tda_read(CSR);
	  tda_write(CSR,val | CSR_nRIU_MASK);			//ʹ�ܴ���
	  
	  tda_write(FCR,0x3a);  
	  tda_write(FCR,FCR_PEC_MASK);					//����1�ֽ�FIFO���,7��У���������
	  
	  if(speed)
	  {
	  	  tda_write(PDR,3);							//���÷�Ƶ  ����
	  	  g_resetspeed_buf[PSAMnum]=1;				//���ø��ٸ�λ��־	
	  }	  
	  else
	  {
	  	  tda_write(PDR,12);							//���÷�Ƶ 	����	
	  	  g_resetspeed_buf[PSAMnum]=0;				//���õ��ٸ�λ��־
	  }	  
	 
 	  tda_write(UCR2,0x20);							//��ֹ�����жϣ�ʹ���Զ�Լ��,��ֹINTAUX�����ж�

	  val = tda_read(UCR1);	
	  tda_write(UCR1,val|UCR1_FTE0_MASK|UCR1_SS_MASK);	//SS=1 FTE0=1
		
	  //============  ��λPSAM�� ================
	  val = tda_read(PCR);
	  tda_write(PCR,val & ~PCR_RSTIN_MASK);			//��λ���õ� 
	  mdelay(10);
	  
	  tda_timer_start(0x61,0x1a,0xc3,0x00); 			//���ó�ʱʱ��
			
	  val = tda_read(PCR);							//�ͷŸ�λ���� 
	  tda_write(PCR,val | PCR_RSTIN_MASK);
	  
	  g_ATRlen = 0;
	  local_irq_disable();					//���ж�
	  
	  for (;;)
	  {
	  	int val = iso7816_readbyte(200);
	  	if (val >= 0)
	  	{
	  			g_ATR_buf[g_ATRlen++] = val; 
	   	  //   if( g_ATRlen > 3 )
	   		   if( g_ATRlen > 6 )       //�˴�����սӸ��ֽڣ�PPS��Ҫ�ж��Ƿ�������PSAM��2012-2-20 
		           break;    	  			
	  	}
	  	else
	  	{
						local_irq_enable();		//���ж�
				 		return DISPOSE_PSAM_T0_TIMEOUT_ERROR;     	  			
	  	}
	  }

	 local_irq_enable();			//���ж� 
	 
	 mdelay(20);
	 printk ("g_ATRlen=0x%02x\n", g_ATRlen); 
	 for(i=0;i<g_ATRlen;i++)  
	  	printk ("g_ATR_buf = 0x%02x\n", g_ATR_buf[i]);

	  //�ж��Ƿ����3B��3F
	  if( (g_ATR_buf[0] != 0x3f) && (g_ATR_buf[0] != 0x3b) ) 	
	  {
	  		return RESET_PSAM_NOT_3B3F_ERROR; 
	  }	
	  
	  //�ж���T=0������T=1�� 
	  if( g_ATR_buf[1] & 0x80 ) 					
	  		g_cardtype_buf[PSAMnum]=1;	
	  else
		    g_cardtype_buf[PSAMnum]=0;	
	  		
	  mdelay(10); 									//�ȴ�10ms�ȴ���λ��Ϣ����
	  val=tda_read(USR); 	
      val=tda_read(URR); 							//����ر�־	
      
      tda_write(GTR, 00);			//���ñ���ʱ��
//    tda_write(FCR,0x40);  			//��T=0ʱ��żУ����������Դ���
      
     //����ǵ��ٿ����ж��Ƿ�֧��PPS (2010-9-26�޸�)
	 //֮ǰ���ֹ����ܼ��ݹ���PSAM�������������Ϊ��PSAM����Ȼ�ǵ��ٿ�������֧��PPS���ڴ˴������ж�
	 if( (g_ATR_buf[1]&0x10) && (speed==LOW) ) 			
	 {
      	  ret=send_pps( g_cardtype_buf[PSAMnum] ); 	  
      	  if(ret==0)
      	  {
      	  		printk ("���ٳɹ�!\n"); 
      	  		g_resetspeed_buf[PSAMnum]=1;		//���ø��ٸ�λ��־	
      	  }	  	
  		  return ret;				
     } 
     
     return 0;     
} 


/*****************************************************************
 �������ƣ�tda_psam_reset
 ������������λSPAM��
 ���������psam_idx��SPAM�����
 �����������
 ����˵�����ɹ���ʶ
 ����˵����
 *****************************************************************/
static int tda_psam_reset( int psam_idx )			
{	
	int ret; 

	switch(psam_idx)
	{
		case 1:
			psam_select_card(1);
			break;						//ѡ��1
			
		case 2:
			psam_select_card(2);
			break;						//ѡ��2	
			
		case 3:
			psam_select_card(3);
			break;						//ѡ��3	
			
		case 4:
			psam_select_card(4);
			break;						//ѡ��4	
			
		case 5:
			psam_select_card(5);
			break;						//ѡ��5
			
		case 6:
			psam_select_card(6);
			break;						//ѡ��6	
			
		default:
			return SELECT_PSAM_NUMBER_ERROR;		//ѡ�����	
	}
	
	//ȡ��λ��Ӧ����
	ret = TDA_ATRsequence(psam_idx , LOW );	 
	if(ret==0)
	{
		mdelay(10);
		return 0;  
	}	
		
	ret = TDA_ATRsequence(psam_idx , HIGH );	
	mdelay(10);
	return ret;  
} 

/*****************************************************************
 �������ƣ�Check_SW1SW2
 ����������ö��SPAM�����ص�״̬�Ƿ�����
 �����������
 �����������
 ����˵����������ʶ
 ����˵����
 *****************************************************************/
static int Check_SW1SW2( u8 *data_buf, u8 len, u8 *SW_buf )
{
	if(len<2)
		return 1;				//��������״̬	
	
	if( ((data_buf[len-2]==0x62) && (data_buf[len-1]==0x81)) || 		//���͵����ݿ�������
	    ((data_buf[len-2]==0x6a) && (data_buf[len-1]==0x80)) ||			//
	    ((data_buf[len-2]==0x6a) && (data_buf[len-1]==0x81)) ||			//
	    ((data_buf[len-2]==0x6a) && (data_buf[len-1]==0x82)) ||			//
	    ((data_buf[len-2]==0x6a) && (data_buf[len-1]==0x83)) ||			//
	    ((data_buf[len-2]==0x6a) && (data_buf[len-1]==0x84)) ||			//
	    ((data_buf[len-2]==0x6a) && (data_buf[len-1]==0x86)) ||			//
	    ((data_buf[len-2]==0x93) && (data_buf[len-1]==0x02)) ||			//
	    ((data_buf[len-2]==0x93) && (data_buf[len-1]==0x03)) ||			//
	    ((data_buf[len-2]==0x94) && (data_buf[len-1]==0x01)) ||			//
	    ((data_buf[len-2]==0x94) && (data_buf[len-1]==0x03)) ||			//
	    ((data_buf[len-2]==0x62) && (data_buf[len-1]==0x83)) ||			//ѡ���ļ���Ч���ļ�����ԿУ�����
	    ((data_buf[len-2]==0x63) && ((data_buf[len-1]&0xf0)==0xc0)) ||	//�������ٳ���X��
	    ((data_buf[len-2]==0x64) && (data_buf[len-1]==0x00)) ||			//״̬��־δ��д
	    ((data_buf[len-2]==0x65) && (data_buf[len-1]==0x81)) ||			//дEEROM���ɹ�
	    ((data_buf[len-2]==0x67) && (data_buf[len-1]==0x00)) ||			//
	    ((data_buf[len-2]==0x69) && (data_buf[len-1]==0x00)) ||			//
	    ((data_buf[len-2]==0x69) && (data_buf[len-1]==0x00)) ||			//
	    ((data_buf[len-2]==0x69) && (data_buf[len-1]==0x01)) ||			//
	    ((data_buf[len-2]==0x69) && (data_buf[len-1]==0x81)) ||			//
	    ((data_buf[len-2]==0x69) && (data_buf[len-1]==0x82)) ||			//
	    ((data_buf[len-2]==0x69) && (data_buf[len-1]==0x83)) ||			//
	    ((data_buf[len-2]==0x69) && (data_buf[len-1]==0x85)) ||			//
	    ((data_buf[len-2]==0x69) && (data_buf[len-1]==0x87)) ||			//
	    ((data_buf[len-2]==0x69) && (data_buf[len-1]==0x88)) ||			//
	    
	    ((data_buf[len-2]==0x6b) && (data_buf[len-1]==0x00)) ||			//
	    ((data_buf[len-2]==0x6e) && (data_buf[len-1]==0x00)) ||			//
	    ((data_buf[len-2]==0x6f) && (data_buf[len-1]==0x00)) ||			//
	    
	    ((data_buf[len-2]==0x94) && (data_buf[len-1]==0x06))			//
	  )
	{
		SW_buf[0]=data_buf[len-2];
		SW_buf[1]=data_buf[len-1];		//����SW1,SW2
		return 0;						//����״̬��Ϣ
	}
	
	return 1;
}

/*****************************************************************
 �������ƣ�Delay_us
 ����������ԭ�صȴ���ʱUS
 ���������usnum��US
 �����������
 ����˵������
 ����˵����
 *****************************************************************/
static void Delay_us(uint usnum)
{
   volatile int i,j;
   for(j=0;j<usnum;j++)
       for(i = 0; i < 20; i++);
}

//===========================================================================
//�������ƣ�TDA_Send_Recv_APDU
//����˵����TDA���������PSAM����������PSAM�����ص����ݣ�����T=0��T=1Э��
//��    ����PSAMnum--PSAM��,*req_buf--�����������, req_len���������  
//			*resp_buf--��Ӧ���ݻ����� *resp_len--��Ӧ���ݳ���ָ��
//����ֵ  ��0--�ɹ� ��0--ʧ��
//===========================================================================
static int TDA_Send_Recv_APDU(u8 PSAMnum, u8 *req_buf, u8 req_len, u8 *resp_buf, u8 *resp_len)
{
  	int i,j,ret,cnt;
    u8 val,USRval,MSRval,temp; 
    u8 CMDBuf[500];
	u8 TmpBuf[500],tmplen;
	u8 SWbuf[500];
	u8 dlength = 0; 

//	printk ("g_resetspeed_buf[PSAMnum]=0x%02x\n",g_resetspeed_buf[PSAMnum]);

	//�ж��Ƿ��������PSAM��2011-4-19���Ӵ�����
	if( (PSAMnum==0) || (PSAMnum>6) )
	{
		printk ("PSAM���ۺų�����Χ#\n"); 
		return 1;
	}	
	if( g_RecvInfo.resetbuf[PSAMnum-1]==0 )
	{
		printk ("PSAM����λʧ�ܽ�ִֹ�и�TimeCosָ��#\n"); 
		return 1; 
	}
	
	tmplen=0;
	*resp_len=0;
	
	//----------  �ж�TimeCos�����Ƿ񳬳�  -------------
	if(req_len>100 || req_len<5)
		return TIMECOS_CMD_LEN_ERROR;	 
				 
	//-------------  ѡ��Ƭ  -------------------------
	switch(PSAMnum)
	{
		case 1:psam_select_card(1);break;						//ѡ��1		
		case 2:psam_select_card(2);break;						//ѡ��2	
		case 3:psam_select_card(3);break;						//ѡ��3	
		case 4:psam_select_card(4);break;						//ѡ��4	
		case 5:psam_select_card(5);break;						//ѡ��5
		case 6:psam_select_card(6);break;						//ѡ��6	
		default:return SELECT_PSAM_NUMBER_ERROR;break;		//ѡ�����	
	}
	
	for(i=0;i<100;i++); 
	
	if(g_resetspeed_buf[PSAMnum])
	  	 tda_write(PDR,3);							//���÷�Ƶ  ���� 
	else
	  	 tda_write(PDR,12);							//���÷�Ƶ 	����	
	
	//-----------  ����ͨѶ��ؼĴ���  ------------------
	g_cardtype = g_cardtype_buf[PSAMnum];					//���ÿ�Э������
	g_cmdtype  = g_cmdtype_buf[PSAMnum];					//���ÿ���������(��T=1������)
	
	val = tda_read(UCR1);
	if( g_cardtype == 0 )
	    	tda_write(UCR1,val & ~UCR1_PROT_MASK);			//����T=0Э��
	  else
	    	tda_write(UCR1,val | UCR1_PROT_MASK);			//����T=1Э��

	//===========================================================
	//===============  T=1����������  ===========================
	j=0;
	if( g_cardtype == PROTOCOL_T1 )
	{		
		CMDBuf[j++] = 0x00; 
		CMDBuf[j++] = g_cmdtype;				//����00/40
		CMDBuf[j++] = req_len; 					//�����          
		for(i=0;i<req_len;i++)
			CMDBuf[j++] = req_buf[i];			//TimeCos��������
		temp=cal_bcc(CMDBuf,j);
		CMDBuf[j++]=temp;						//����BCC  
		
		tda_timer_start(0x61,0x0a,0xc3,0x00);		//���ó�ʱʱ��
 		
 	 	local_irq_disable();					//���ж�
 	 	ret=iso7816_sendcmd( CMDBuf, j );				//��������
        if(ret!=0)
        {
        	local_irq_enable();					//���ж�
			return ret; 
		}
	    
	    cnt=0;
	    while(1)       
	    {			
			Delay_us(50);						//������ʱ2009-12-17
			USRval = tda_read(USR);
			if (USRval & USR_TBE_RBF_MASK)		//�жϴ����Ƿ�������		
			{	
			     for(i=0;i<58001;i++) 
			     {
			     	  Delay_us(50);			//������ʱ2009-12-17
			     	  USRval=tda_read(USR);
			     	  MSRval=tda_read(MSR);
			     	  if( (USRval&USR_TBE_RBF_MASK) || (!(MSRval&MSR_FE_MASK)) )
			     	  {
					       val = tda_read(URR);					
					   	   TmpBuf[tmplen++] = val; 
					   	   if( (tmplen==1) && (TmpBuf[0]!=0) )
					   	   {
					   	   		tmplen=0;
					   	   }
					   	   else if( (tmplen==2) && (TmpBuf[1]!=g_cmdtype) )
					   	   {
					   	   		if(TmpBuf[1]==0)
					   	   			tmplen=1; 
					   	   		else 
					  	   			tmplen=0;	
					   	   	} 
						   if( (tmplen>4) && (tmplen==TmpBuf[2]+4) )	//�������(�ڴ��жϳ��ȣ����ж�9000)
						   		break;	
					   }
					   else if ( (USRval&USR_TOL3_MASK) || (i==58000) ) 
					   {
					   //		printk ("USRval=0x%02x,i=%d\n",USRval,i);	
					   //		for(i=0;i<tmplen;i++) 
					   //			printk ("TmpBuf=0x%02x\n",TmpBuf[i]);
					   		
					   		local_irq_enable();						//���ж�
					   		//�л�����
							if( g_cmdtype==0 )
								g_cmdtype=0x40;
							else	
								g_cmdtype=0x00;
							g_cmdtype_buf[PSAMnum]=g_cmdtype;		//������������
					   		
					   		return DISPOSE_PSAM_T1_TIMEOUT_ERROR;   	      
					   	}  
				 } 
				 break;    
			}     
		//	else  if (USRval & USR_TOL3_MASK) 
			else  
			{	
				cnt++;  
				if(cnt==42000)  				//����ʱ�޸ĳɴ������Ƶķ�ʽ,��Ϊ�����ʱ�Ĵ������ײ������õ��¿��������� 2009-12-17
				{
					local_irq_enable();	//���ж�
					return DISPOSE_PSAM_T1_TIMEOUT_ERROR;   	      
				}	
			} 
	    } 	      
		
		local_irq_enable();				//���ж�
		 
//		printk ("i=%d\n",i); 		 
//   	for(i=0;i<tmplen;i++) 
//			printk ("22TmpBuf=0x%02x\n",TmpBuf[i]); 

	    //У��bcc�Ƿ���ȷ
		temp=0;
		for(i=0;i<tmplen;i++)		
			temp ^= TmpBuf[i]; 
		if( temp != 0)	
		{
			return PSAM_RESPONSE_BCC_ERROR;	
		}
		
		//�л�����
		if( g_cmdtype==0 )
			g_cmdtype=0x40;
		else	
			g_cmdtype=0x00;
		g_cmdtype_buf[PSAMnum]=g_cmdtype;		//������������
			
		//�ж�PSAM�����ص������Ƿ񳬳�
		if( TmpBuf[2]>100 )
			return PSAM_T1_RETURN_LEN_ERROR;	 
		
		//��������
		*resp_len=TmpBuf[2];
		for(i=0;i<TmpBuf[2];i++)	 			
			resp_buf[i]=TmpBuf[3+i]; 
		
		return NO_ERROR; 
	}	
	  
	  
	//========================================================================
	//===============  T=0����������  ========================================
	//========================================================================

	//===========  �����С��6ʱ  ================= ������������ֽ�1�η�������
	if(req_len < 6)
	{		
	//	tda_timer_start(0x61,0x0a,0xc3,0x00); 
		tda_timer_start(0x61,0x1a,0xc3,0x00);			//�ڱ������Է���ȡ�����ָ�ʱ���ӳ���ʱ�������2009-7-7 �������ڱ���			
		
		local_irq_disable();						//���ж�
		ret=iso7816_sendcmd( req_buf, 5 );
		if(ret!=0)
		{  
			local_irq_enable();						//���ж�
			return ret;
		}
		
		cnt=0;					
	    while(1)    
	    {			
			Delay_us(50);							//������ʱ2009-12-17
			USRval = tda_read(USR);
			if (USRval & USR_TBE_RBF_MASK)			//�жϴ����Ƿ�������		
			{	
			     for(i=0;i<58001;i++)
			     {
			     	  Delay_us(50);					//������ʱ2009-12-17
			     	  USRval=tda_read(USR);
			     	  MSRval=tda_read(MSR);
			     	  if( (USRval&USR_TBE_RBF_MASK) || (!(MSRval&MSR_FE_MASK)) )
			     	  {
					      val = tda_read(URR);					
					   	  TmpBuf[tmplen++] = val; 
					   	  
					   	  if(tmplen>2)    
					   	  {
						 	  if( (tmplen==req_buf[4]+3) && (TmpBuf[tmplen-2]==0x90) && (TmpBuf[tmplen-1]==0x00) )		//�ڴ����ӳ����ж� ��ֹ��������9000 ������ 2009-8-5 �ڹ�˾ 
							   		break; 	
						  }
						  else   
						  {
						 	  if( (tmplen==req_buf[4]+2) && (TmpBuf[tmplen-2]==0x90) && (TmpBuf[tmplen-1]==0x00) )		//��������DES��ʼ��ʱֻ��5���ֽڣ����´˴������жϳ���ֻ����2���ֽڵ�9000�������Ѹ���2010-10-20
						 	  {
							   		//��������
									*resp_len=2;		 				 
									resp_buf[0]=0x90;
									resp_buf[1]=0x00; 	
									return NO_ERROR;  	 		 
							   }  		 
						  }	 	   	 	     
					   }
					   else if ( (USRval&USR_TOL3_MASK) || (i==58000) ) 
					   {
					   	//	printk ("USRval=0x%02x,i=%d\n",USRval,i);	
					   		local_irq_enable();			//���ж�
					   		//����Ƿ����״̬��Ϣ 
							ret=Check_SW1SW2( TmpBuf, tmplen, SWbuf );		
							if(ret==0)
							{
								*resp_len=2;		 			
								for(j=0;j<2;j++)			 
									resp_buf[j]=SWbuf[j];
								return NO_ERROR; 	
							} 
							else
							{
					   			return DISPOSE_PSAM_T0_TIMEOUT_ERROR;   	      
					   		}	
					   	}  
				 } 
				 break; 
			}  
		//	else if (USRval & USR_TOL3_MASK)
			else
			{	
				cnt++;  
				if(cnt==42000)  				//����ʱ�޸ĳɴ������Ƶķ�ʽ,��Ϊ�����ʱ�Ĵ������ײ������õ��¿��������� 2009-12-17
				{	
					local_irq_enable();		//���ж�
			 		return DISPOSE_PSAM_T0_TIMEOUT_ERROR;   
			 	}	     	
			 } 
	     } 	      
	    
	    local_irq_enable();					//���ж� 
		//�����Ƿ��������ͷ
		for(i=0;i<8;i++)						//����������ж�8���Ƿ��������ͷ				
		{										//���8���ж϶�û������ͷ����Ϊ������󣬲�����
			if(TmpBuf[i]==req_buf[1])		
				break;
			if(i==7)
			{
				return PSAM_T0_RESPONSE_NOT_CMD_HEADER;	
			}	
		}
			
//		for(j=0;j<tmplen;j++)	
//			printk ("TmpBuf=0x%02x\n",TmpBuf[j]);	

		//��������
		temp = tmplen-i-1;				//��������ͷ���ж����ݳ��ȣ����ұ�������
		*resp_len=temp;					//�������Ա�����Ϊ���ڴ�����ն�������Ч����
		for(j=0;j<temp;j++)			
			resp_buf[j]=TmpBuf[i+1+j];	

		return NO_ERROR; 	
	}	 
	
	//==================================================
	//===========  �����>=6ʱ =================>>>>>> ��1�� �ȷ��������ǰ5�ֽ�

		tda_timer_start(0x61,0x1a,0xc3,0x00);			
	 
		local_irq_disable();						//���ж�
		ret=iso7816_sendcmd( req_buf, 5 );						
		if(ret!=0)
		{
			local_irq_enable();						//���ж�
			return ret;
		}
		
		cnt=0;
		while(1)    
	    {			
			Delay_us(50);							//������ʱ2009-12-17
			USRval = tda_read(USR);
			if (USRval & USR_TBE_RBF_MASK)			//�жϴ����Ƿ�������		
			{	
				for(i=0;i<58001;i++)
			     {
			     	  Delay_us(50);					//������ʱ2009-12-17
			     	  USRval=tda_read(USR); 
			     	  MSRval=tda_read(MSR);
			     	  if( (USRval&USR_TBE_RBF_MASK) || (!(MSRval&MSR_FE_MASK)) )
			     	  {
					       val = tda_read(URR);					
					   	   TmpBuf[tmplen++] = val; 
					   	   if( tmplen>0 )
					   //  if( (tmplen>0) && (TmpBuf[tmplen-1]==req_buf[1]) )
						   		break;	
					   }
					   else if ( (USRval&USR_TOL3_MASK) || (i==58000) )
					   {
					   		local_irq_enable();			//���ж� 
					   		//����Ƿ����״̬��Ϣ 
							ret=Check_SW1SW2( TmpBuf, tmplen, SWbuf );		
							if(ret==0)
							{
								*resp_len=2;		 	 		
								for(j=0;j<2;j++)			 
									resp_buf[j]=SWbuf[j];
								return NO_ERROR; 	
							} 
							else
							{
									printk("apdu timeout1...\n");
					   			return DISPOSE_PSAM_T0_TIMEOUT_ERROR;   	      
					   		}	
					   	}  
				 } 
				 break;  
			}  
		//	else  if (USRval & USR_TOL3_MASK) 
			else 
			{
				cnt++;  
				if(cnt==42000)  				//����ʱ�޸ĳɴ������Ƶķ�ʽ,��Ϊ�����ʱ�Ĵ������ײ������õ��¿��������� 2009-12-17
				{	
					printk("apdu timeout2...\n");
					local_irq_enable();		//���ж�  	
			 		return DISPOSE_PSAM_T0_TIMEOUT_ERROR;  
			 	}	  
			 }
	     } 	     

		//===========================================>>>>>> ��2�� �������ʣ���ֽڷ�������
		local_irq_enable();						//���ж�
		tmplen=0;
		
//		while( (tda_read(MSR)&MSR_BGT_MASK)!=MSR_BGT_MASK );		//�ж��Ƿ�����鱣��ʱ��
//		for(i=0;i<400;i++);  			//���ݲ��ԣ�������Ҫ�Ļ���
	 
		for( i=0;i<req_len-5;i++ )
			CMDBuf[i] = req_buf[5+i];
			 
		tda_timer_start(0x61,0x0a,0xc3,0x00);			
	
		local_irq_disable();						//���ж�
		ret=iso7816_sendcmd( CMDBuf, req_len-5 );						
		if(ret!=0)
		{
			local_irq_enable();						//���ж�
			return ret;
		}
		
		cnt=0;
		while(1)    
	    {			
			Delay_us(50);							//������ʱ2009-12-17
			USRval = tda_read(USR);
			if (USRval & USR_TBE_RBF_MASK)			//�жϴ����Ƿ�������		
			{	
				 for(i=0;i<58001;i++)
			     {
			     	  Delay_us(50);					//������ʱ2009-12-17
			     	  USRval=tda_read(USR);
			     	  MSRval=tda_read(MSR);
			     	  if( (USRval&USR_TBE_RBF_MASK) || (!(MSRval&MSR_FE_MASK)) )
			     	  {
					       val = tda_read(URR);					
					   	   TmpBuf[tmplen++] = val; 
					   	   if( (tmplen>1) && (TmpBuf[tmplen-2]==0x61) )
						   {
								 dlength = TmpBuf[tmplen-1];		//�˴���2���ֽڣ�����0x61����һ���ֽ�,���ǽ�Ҫ���ص����ݳ���	
								 break;	
							}

					   	   if( (tmplen>1) && (TmpBuf[tmplen-2] ==0x90) && (TmpBuf[tmplen-1]==0x00) )
						   { 
								*resp_len=tmplen;					//�������ݳ���
								for(i=0;i<tmplen;i++)				//��������	
						  			resp_buf[i]=TmpBuf[i];	
						  		local_irq_enable();					//���ж�	
								return NO_ERROR;
							}	 	
					   }
					   else if ( (USRval&USR_TOL3_MASK) || (i==58000) )
					   {	
						   		local_irq_enable();						//���ж�
						   		//����Ƿ����״̬��Ϣ 
								ret=Check_SW1SW2( TmpBuf, tmplen, SWbuf );		
								if(ret==0)
								{ 
									*resp_len=2;		 			
									for(j=0;j<2;j++)			 
										resp_buf[j]=SWbuf[j];
									return NO_ERROR; 	 
								} 
								else
								{
										printk("apdu timeout...\n");
						   			return DISPOSE_PSAM_T0_TIMEOUT_ERROR;   	      
						   		}	
					   	}  
				 } 
				 break;    
			}  
		//	else if (USRval & USR_TOL3_MASK)
			else
			{
				cnt++;  
				if(cnt==48000)  					//����ʱ�޸ĳɴ������Ƶķ�ʽ,��Ϊ�����ʱ�Ĵ������ײ������õ��¿��������� 2009-12-17
				{	
					printk("apdu timeout4...\n");
					local_irq_enable();			//���ж�
					return DISPOSE_PSAM_T0_TIMEOUT_ERROR; 	 
				}	
			}
	     } 	     

		//===========================================>>>>>> ��3�� ȡ����
		local_irq_enable();				//���ж�
		tmplen=0;

//		while( (tda_read(MSR)&MSR_BGT_MASK)!=MSR_BGT_MASK );		//�ж��Ƿ�����鱣��ʱ��	
//		for(i=0;i<1000;i++);		//���ݲ��ԣ�������Ҫ�Ļ���
		
		tmplen=0;
		CMDBuf[0] = 0x00; 
		CMDBuf[1] = 0xc0;
		CMDBuf[2] = 0x00;
		CMDBuf[3] = 0x00;
		CMDBuf[4] = dlength;			//ȡ��������
		
		tda_timer_start(0x61,0x0a,0xc3,0x00);			
		
		local_irq_disable();						//���ж�	
		ret=iso7816_sendcmd( CMDBuf, 5 );	
		if(ret!=0)
		{
			local_irq_enable();						//���ж�
			return ret;
		}
		
		cnt=0; 
		while(1)    
	    {				
			Delay_us(50);							//������ʱ2009-12-17
			USRval = tda_read(USR);
			if (USRval & USR_TBE_RBF_MASK)			//�жϴ����Ƿ�������		
			{	
				for(i=0;i<58001;i++)					//��Ϊ�ڶ�������Ϣ�ϳ���ʱ��ʱʱ�䲻���������ӳ���2009-5-17
			    {
			     	  Delay_us(50);					//������ʱ2009-12-17
			     	  USRval=tda_read(USR);
			     	  MSRval=tda_read(MSR);
			     	  if( (USRval&USR_TBE_RBF_MASK) || (!(MSRval&MSR_FE_MASK)) )
			     	  {
					       val = tda_read(URR);					
					   	   TmpBuf[tmplen++] = val; 
					  	   if( (tmplen>1) && (TmpBuf[tmplen-2] ==0x90) && (TmpBuf[tmplen-1]==0x00) )
						   {
								//------- �����Ƿ��������ͷ0xc0 -----------
								for(i=0;i<6;i++)						//����������ж�6���Ƿ��������ͷ0xc0,���6���ж϶�û������ͷ����Ϊ������󣬲�����				
								{										
									if( TmpBuf[i]==0xc0 ) 				
										break;
									if(i==5)  
									{	 
										local_irq_enable();				//���ж�
										return PSAM_T0_RESPONSE_NOT_C0;						
									}	
								}
								if( (tmplen-i-1)==(dlength+2) )			//��⵽����ͷ���ټ��鳤���Ƿ���ȷ   (֮ǰ�����������9000���߼���bug�������Ѿ������������� 2009-8-5 �޸�)
								{	
									*resp_len=tmplen-1;					//�������ݳ���
									for(i=0;i<tmplen-1;i++)				//��������
								  		resp_buf[i]=TmpBuf[i+1];	 
									break;	
								}		  
							}		   
					   }
					   else if ( (USRval&USR_TOL3_MASK) || (i==58000) )
					   {
					   		local_irq_enable();			//���ж�
					   		//����Ƿ����״̬��Ϣ 
							ret=Check_SW1SW2( TmpBuf, tmplen, SWbuf );		
							if(ret==0)
							{
								*resp_len=2;		 			
								for(j=0;j<2;j++)			 
									resp_buf[j]=SWbuf[j];
								return NO_ERROR; 	
							} 
							else
							{
					   			return DISPOSE_PSAM_T0_TIMEOUT_ERROR;   	      
					   		}	
					   }  
				 }  
				 break;  
			}  
		//	else   if (USRval & USR_TOL3_MASK)
			else
			{
				cnt++;  
				if(cnt==42000)  					//����ʱ�޸ĳɴ������Ƶķ�ʽ,��Ϊ�����ʱ�Ĵ������ײ������õ��¿��������� 2009-12-17
				{	
					local_irq_enable();			//���ж�
					return DISPOSE_PSAM_T0_TIMEOUT_ERROR;  	
				}	 
			}
	    } 	      
   
//	 for(j=0;j<tmplen;j++)	
//		printk ("TmpBuf=0x%02x\n",TmpBuf[j]);	 
	//-------------- 
	
	local_irq_enable();				//���ж�
	
	return NO_ERROR; 
	
} 

/*****************************************************************
 �������ƣ�gv_ioctl
 ����������������IO�����ӿ�
 ���������filp���ļ���Ϣ�ṹ
		   cmd�������
		   arg��ѡ�����������
 ���������arg��ѡ����������
 ����˵�����ɹ���ʶ
 ����˵����
 *****************************************************************/
static long gv_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    //struct gv_dev *dev = filp->private_data;
	//struct gv_tda_io *dio = dev->dio;
	int ret = 0;
	int psam_num = arg;
	int i, j;

	switch (cmd)
	{
		//========== 0 ����APDU��PSAM����ȡ��Ӧ����  ================
		case IOCTL_PSAM_APDU_DISPOSE:
			
			ret = copy_from_user( &g_SendInfo,(IoctlInfo *)arg ,sizeof(IoctlInfo) );		//���û�̬��������    	    

#if 0		    
		    printk ("apdu send len = %d: ", g_SendInfo.datalen); 	
	 		for(i=0;i<g_SendInfo.datalen;i++) 
	  			printk ("%02x ", g_SendInfo.databuf[i]); 			
		    printk("\n");
#endif
		    g_RecvInfo.psamnum = g_SendInfo.psamnum;
		    ret=TDA_Send_Recv_APDU( g_SendInfo.psamnum, g_SendInfo.databuf, g_SendInfo.datalen, g_RecvInfo.databuf, &g_RecvInfo.datalen );	
			if(ret==0) 
			{
			 	ret = copy_to_user( (IoctlInfo *)arg, &g_RecvInfo ,sizeof(g_RecvInfo) );	//������������û�̬ 	 		 	
#if 0			
				printk("apdu recv, len = %d: ", g_RecvInfo.datalen);
				for(i=0;i<g_RecvInfo.datalen;i++) 
	 				printk ("%02x ", g_RecvInfo.databuf[i]); 	
				printk("\n");
#endif
				
			}  
			
			else
			{
				printk("tda rsp error, ret = %02x\n", ret);							
			}
	
			return ret;		
		
		//============ 1  ��λ����PSAM������  ========================
		case IOCTL_ONE_PSAM_RESET_DISPOSE:			
	
			if( (arg>0) && (arg<7) )
			{
				psam_led_control(psam_num , 0);				//��λ���ȹء�PSAM�����ڡ�ָʾ��
				ret=tda_psam_reset(psam_num);		//��λPSAM��
				if(ret<0)
				{ 
					psam_led_control(psam_num , 0);			//�ء�PSAM�����ڡ�ָʾ��
					g_RecvInfo.resetbuf[psam_num-1] = 0;		//ʧ�� 

					printk ("Reset PSAM%d ERROR!\n", psam_num);
				}	
				else
				{
					psam_led_control( psam_num , 1);			//����PSAM�����ڡ�ָʾ��
					g_RecvInfo.resetbuf[psam_num-1] = 1;		//�ɹ�
				
					if(g_resetspeed_buf[psam_num])
					{
						printk ("HIGH Speed Reset PSAM%d OK , T=%d\n", psam_num, g_cardtype_buf[psam_num]); 	
					}
					else
					{
						printk ("LOW Speed Reset PSAM%d OK , T=%d\n", psam_num, g_cardtype_buf[psam_num]); 	
					}
			
				}	
 
				g_cmdtype_buf[ psam_num ] = 0;		//���������� 
				
				//�����м�⵽��PSAM��ִ��ȡ�ն˻���Ų���2011-4-19
			//	ret=get_all_psam_termid();
	
				return ret;	  
			}
	 
			return 1; 
			

		
		//================ 3  TDA�ϵ����  ==========================
		case IOCTL_TDA_POWERUP_DISPOSE:			
			
			 printk("TDA PowerUP ...! \n");

			
			tda_init();						//TDA8007��ʼ��
    		tda_select_channel(1);			//ѡ��ͨ��A
			ret=tda_powerup( POWERUP_3V );	//8007�ϵ硢��λ 
			if(ret<0)
				printk("TDA PowerUP Error! \n");
    		else
    			printk("TDA PowerUP OK! \n");
			return ret;
		
		//============== 4  TDA�������  ==========================
		case IOCTL_TDA_POWERDOWN_DISPOSE:			
			
			tda_select_channel(1);
			tda_powerdown(); 
			tda_select_channel(2);
			tda_powerdown();				//�رտ�Ƭ

  		printk("Power Down\n");
	
			//P1_C1_L;
			//P1_C2_L; 
			//P1_C3_L;						//PSAM��ѡ�������
			
			return NO_ERROR;


		//============== 2 ��λ����PSAM������  ========================
		case IOCTL_ALL_PSAM_RESET_DISPOSE:			

			for (i=1; i<7; i++)
			{
					 psam_led_control(i, 0);
			}
			
			for(i=1;i<7;i++)
			{
				ret=tda_psam_reset( i );				//��λPSAM��
				if(ret<0)
				{
					psam_led_control( i , 0);					//�ء�PSAM�����ڡ�ָʾ��
					g_RecvInfo.resetbuf[i-1] = 0;		//ʧ��
				}	
				else
				{
					psam_led_control( i , 1);					//����PSAM�����ڡ�ָʾ��
					g_RecvInfo.resetbuf[i-1] = 1;		//�ɹ�	
				}
				g_cmdtype_buf[ i ] = 0;					//����������
			}
			
			ret = copy_to_user( (IoctlInfo *)arg, &g_RecvInfo ,sizeof(g_RecvInfo) );	//������������û�̬ 	 		 	
			
			return 0;


		case IOCTL_ReadPsamInfo_DISPOSE: 						
			j=0;
			for(i=0;i<6;i++)
				PSAMInfoBuf[j++]=g_RecvInfo.resetbuf[i];	//��λ��Ϣ
			for(i=0;i<6;i++) 
				PSAMInfoBuf[j++]=g_resetspeed_buf[1+i];		//������Ϣ 
			for(i=0;i<6;i++)
				PSAMInfoBuf[j++]=g_cardtype_buf[1+i];		//��������Ϣ 
			
			ret = copy_to_user((void *)arg, PSAMInfoBuf ,18 ); 	
			return NO_ERROR; 
				
		default:
			printk("drv_tda ioctl cmd %d not support\n", cmd);
			return -1;
	}


    return 0;
}

/*****************************************************************
 �������ƣ�gv_open
 �����������򿪵ظ��豸�ӿ�
 ���������inode,�豸�ڵ�
		   filp���ļ�����
 �����������
 ����˵�����ɹ���ʶ
 ����˵����
 *****************************************************************/
static int gv_open(struct inode *inode, struct file *filp)
{
    struct gv_dev *dev = container_of(inode->i_cdev, struct gv_dev, cdev);

    unsigned long flags = 0;

    //mutex open    
    if (!mutex_trylock(&dev->open_lock))
    {
        printk("tda dev already open, return.\n");
        return -1;
    }

    spin_lock_irqsave(&dev->recv_lock, flags);
    filp->private_data = dev;   
    dev->recv_ready = 0;
    spin_unlock_irqrestore(&dev->recv_lock, flags); 

    return 0;
}

/*****************************************************************
 �������ƣ�gv_release
 �����������ͷŵظ��豸�ӿ�
 ���������inode,�豸�ڵ�
		   filp���ļ�����
 �����������
 ����˵�����ɹ���ʶ
 ����˵����
 *****************************************************************/
static int gv_release(struct inode *inode, struct file *filp)
{
    struct gv_dev *dev = container_of(inode->i_cdev, struct gv_dev, cdev);
    
    filp->private_data = dev;
    mutex_unlock(&dev->open_lock);

    return 0;
}

/*****************************************************************
 �������ƣ�gv_poll
 �����������ȴ��жӲ�ѯ��Դ�Ƿ����, ϵͳ����select�ڲ���ʹ��
 ���������filp���ļ�����
		   table,�ȴ��ж�
 �����������
 ����˵����> 0,��Դ����
 ����˵����
 *****************************************************************/
static unsigned int gv_poll(struct file *filp, poll_table *wait)
{
    unsigned int mask;
    unsigned long flags;
    struct gv_dev *dev = filp->private_data;
    
    poll_wait(filp, &dev->recv_wait, wait);

    spin_lock_irqsave(&dev->recv_lock, flags);
    if (dev->recv_ready)
    {
        mask =   (POLLIN | POLLRDNORM);
    }
    else           
    {
        mask = 0;
    }
    spin_unlock_irqrestore(&dev->recv_lock, flags);

    return mask;
}

/*****************************************************************
 �������ƣ�gv_write
 �����������豸д�ӿ�
 ���������file���ļ���Ϣ�ṹ
		   buf��д���ݻ���
		   count��д���ݴ�С
		   f_pos��д��λ��
 ���������f_pos������д���λ��
 ����˵����<0��д��ʧ�ܣ�����Ϊд��ɹ����ݴ�С
 ����˵����
 *****************************************************************/
static ssize_t gv_write(struct file *filp, const char __user *buf,
        size_t count, loff_t *f_pos)
{
	int ret;
	u8 sbuf[256];
    struct gv_dev *dev = filp->private_data;
	
	if (count > 126)
	{
		printk("tda send buf too long, len = %d\n", count);
		return -1;
	}

	dev->recv_ready = 0;
	ret = copy_from_user(sbuf, (u8 *)buf, count);
	sbuf[count] = cal_bcc(sbuf, count);

	return count + 1;
	//return tda_write(dev, sbuf, count+1);
}

/*****************************************************************
 �������ƣ�gv_read
 �����������豸���ӿ�
 ���������file���ļ���Ϣ�ṹ
		   count�������ݴ�С
		   f_pos����λ��
 ���������buf�������ݻ���
		   f_pos�����¶����λ��
 ����˵����<0����ʧ�ܣ�����Ϊ��ȡ�ɹ����ݴ�С
 ����˵����
 *****************************************************************/
static ssize_t gv_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    int ret = 0;
    struct gv_dev *dev = filp->private_data;

	while (dev->recv_ready == 0)
	{
		ret = wait_event_interruptible(dev->recv_wait, dev->recv_ready);
		if (ret < 0)
		{
			break; //interrupt by signal
		}
	}
	
	if ((dev->recv_ready > 0) && (dev->rlen > 0))
    {	
		ret = copy_to_user(buf, dev->recv_buf, dev->rlen);
		dev->recv_ready = 0;
		return dev->rlen;
	}

    return -1;	
}


//�豸�ļ�����
static struct file_operations gv_fops = {
    .owner =    THIS_MODULE,
    .write =    gv_write,
    .read =     gv_read,
    .unlocked_ioctl = gv_ioctl,
    .open =     gv_open,
    .release =  gv_release,
    .llseek =   no_llseek,
    .poll   = gv_poll,
};

/*****************************************************************
 �������ƣ�cdev_setup
 ������������ʼ���ַ��豸�������豸�ļ�
 ���������pdata���豸˽������
		major���ַ��豸���豸��
		minor���ַ��豸���豸��
		fops���ַ��豸�ļ�����
 �����������
 ����˵������
 ����˵����
 *****************************************************************/
static void cdev_setup(struct gv_dev *pdata, int major, int minor, struct file_operations *fops)
{
    struct cdev *cdev = &pdata->cdev;   
    struct device *dev;
    int err;
    dev_t devno = MKDEV(major, minor);

    cdev_init(cdev, fops);
    cdev->owner = THIS_MODULE;
    cdev->ops = fops;
    
    err = cdev_add(cdev, devno, 1);
    if (err)
    {
        printk("gpmc add cdev error\n");
    }   
    
    dev = device_create(tda_class, &pdata->pdev->dev, devno, pdata, "%s", pdata->name);
    if (!IS_ERR(dev))
    {
        pdata->devno = devno;       
    }
}

/*****************************************************************
 �������ƣ�cdev_release
 ����������ע���ַ��豸��ɾ���豸�ļ�
 ���������dev���豸˽������
 �����������
 ����˵������
 ����˵����
 *****************************************************************/
static void cdev_release(struct gv_dev *dev)
{
    cdev_del(&dev->cdev);

    if (dev->devno)
    {
        device_destroy(tda_class, dev->devno);
    }
}

/*****************************************************************
 �������ƣ�tda_irq_handler
 ����������оƬ���������жϷ���
 ���������irq���жϱ��
		   dev_id���ж�˽������
 �����������
 ����˵�����жϴ���ɹ���ʶ
 ����˵����
 *****************************************************************/
static irqreturn_t tda_irq_handler(int irq, void *dev_id)
{
    struct gv_dev  *dev = dev_id;

	//dev->rlen = tda_read(dev);

	dev->recv_ready = 1;
	wake_up_interruptible(&dev->recv_wait);

    return IRQ_HANDLED;
}

/*****************************************************************
 �������ƣ�gv_drv_probe
 �����������豸̽�⺯�����豸��������Ժ�̽���ܷ���������
 ���������pdev���豸����
 �����������
 ����˵�����ɹ���ʶ
 ����˵����
 *****************************************************************/
static int __devinit gv_drv_probe(struct platform_device *pdev)
{
    int retval = -1;
    struct gv_dev  *pdata;
    int res_size;
    struct resource *res, *irq_res;


    int irq_flags = IORESOURCE_IRQ_HIGHEDGE; //IORESOURCE_IRQ_LOWEDGE;

    pdata = kzalloc(sizeof(struct gv_dev), GFP_KERNEL);    
    dev_set_drvdata(&pdev->dev, pdata);
		tda_io = pdata->dio = pdev->dev.platform_data;
	

    pdata->id = pdev->id;
    pdata->pdev = pdev;

    printk("tda_drv_probe, pdev-id = %d, rw_io=%d...\n", pdev->id, pdata->dio->io_wr_out);


    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        printk("platform_get_resource idx = %d error\n", 0);
        return -1;
    }
    res_size = resource_size(res);

    pdata->ioaddr = ioremap_nocache(res->start, res_size);
    if (pdata->ioaddr == NULL) {
        printk("ioremap error\n");
        return -2;
    }
		tda_addr = pdata->ioaddr;

    irq_res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if (!irq_res) {
        printk("drv_tda Could not allocate irq resource.\n");
        return -1;
    }

    pdata->irq = irq_res->start;
    irq_flags = irq_res->flags & IRQF_TRIGGER_MASK;


     retval = request_irq(pdata->irq, tda_irq_handler, irq_flags, "tda_irq", pdata);			
    
    if (retval) 
    {
        printk( "Unable to claim requested irq: %d\n", pdata->irq);
    }

    printk("id = %d, start = %08x, size = %d, ioaddr = %08x, irq = %d\n", pdev->id, res->start, res_size, (unsigned int)pdata->ioaddr, pdata->irq);



		gv_io_out(pdata->dio->io_wr_out, 0); 


    spin_lock_init(&pdata->recv_lock);
    init_waitqueue_head(&pdata->recv_wait);
    pdata->recv_ready = 0;

    mutex_init(&pdata->open_lock);

    pdata->name = DEV_FILE_NAME;
    cdev_setup(pdata, DEV_MAJOR, pdata->id, &gv_fops);

#if 0
	{
		int ret = 0;

	tda_init();								//TDA8007��ʼ��
	
    tda_select_channel(1);					//ѡ��ͨ��A
	ret = tda_powerup( POWERUP_3V );		//8007�ϵ硢��λ 	
	if(ret<0)
		printk("TDA PowerUP Error! \n");
    else
    	printk("TDA PowerUP OK! \n"); 

	mdelay(10); 		
		
		ret=tda_psam_reset(1);		
		if(ret<0)
		{
			printk ("Reset PSAM%d ERROR ## \n",1);  		
		}  		
	}
#endif

    return 0;
}

/*****************************************************************
 �������ƣ�gv_drv_remove
 ����������ע���豸����Դ
 ���������pdev���豸����
 �����������
 ����˵������
 ����˵����
 *****************************************************************/
static int gv_drv_remove(struct platform_device *pdev)
{
    struct gv_dev  *pdata = dev_get_drvdata(&pdev->dev);
    printk("g90 fpga remove, id = %d\n", pdata->id);

    free_irq(pdata->irq, pdata);
    iounmap(pdata->ioaddr);

    kfree(pdata);
    cdev_release(pdata);    

    return 0;
}

//GPIOƬ����Դ��ƽ̨�豸����
static struct platform_driver gv_driver = {
    .probe = gv_drv_probe,
    .remove = __devexit_p(gv_drv_remove),
    .driver = {
        .name   = DEV_NAME,
        .owner  = THIS_MODULE,
    },
};

/*****************************************************************
 �������ƣ�gv_init
 ����������ģ�����ʱ��ʼ��
 �����������
 �����������
 ����˵�����ɹ���ʶ
 ����˵����
 *****************************************************************/
static int __init gv_init(void)
{
    printk("G90 gpmc init, driver version = %s.\n", G_VER);

    tda_class = class_create(THIS_MODULE, "tda_class");
    return platform_driver_register(&gv_driver);
}

/*****************************************************************
 �������ƣ�gv_exit
 ����������ģ��ע��ʱ�ͷ���Դ
 �����������
 �����������
 ����˵������
 ����˵����
 *****************************************************************/
static void __exit gv_exit(void)
{
    printk("G90 gpmc deinit.\n");	
    platform_driver_unregister(&gv_driver);
    class_destroy(tda_class);    
}



module_init(gv_init);
module_exit(gv_exit);

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("R.wen");
   
