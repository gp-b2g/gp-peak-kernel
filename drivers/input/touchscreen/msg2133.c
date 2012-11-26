#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/timer.h>
#include <linux/gpio.h>

#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <mach/gpio.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <asm/unistd.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/earlysuspend.h>
//#define TP_DIFF

#define DEBUG
#ifdef DEBUG
#define TP_DEBUG(format, args...)   printk(KERN_INFO "TP_:%s( )_%d_: " format, \
	__FUNCTION__ , __LINE__, ## args);
#define DBG() printk("[%s]:%d => \n",__FUNCTION__,__LINE__)
#else
#define TP_DEBUG(format, args...);
#define DBG()
#endif
#define __FIRMWARE_UPDATE__
#define u8 unsigned char
#define U8 unsigned char
#define u32 unsigned int
#define U32 unsigned int
#define s32 signed int
#define U16 ushort


#define REPORT_PACKET_LENGTH  8
#define MSG21XX_INT_GPIO 82
#define MSG21XX_RESET_GPIO 30   //hight work
#define FLAG_GPIO    122
#define ST_KEY_SEARCH 	217
#define MSG21XX_RETRY_COUNT 3
#define MS_TS_MSG20XX_X_MIN   0
#define MS_TS_MSG20XX_Y_MIN   0
#define MS_TS_MSG20XX_X_MAX   240
#define MS_TS_MSG20XX_Y_MAX   320
static int msg21xx_irq = 0;
static struct i2c_client *msg21xx_i2c_client;
static struct work_struct msg21xx_wq;
static struct early_suspend early_suspend;
static struct mutex msg21xx_mutex;
//static struct input_dev *msg21xx_input = NULL;
static struct input_dev *simulate_key_input = NULL;
static 	struct input_dev *input=NULL;
/*
static u8 temp_key_value;
static u8 flag_key_down= 0;
static u8 flag_two_finger = 0;
static   	int copy_x1;
static    	int copy_y1;
static   	int copy_x2;
static    	int copy_y2;
*/


u8 Checksum = 0;
u8 val[REPORT_PACKET_LENGTH] = {0};
static	u8 key_value;
static	u32 uFinalX, uFinalY;
static	u32 uFinalX2, uFinalY2;
static  u8 flag_key_in = 0;

static      int key_position;
static    	int my_tempx; 
static    	int my_tempy; 
static      int tp_type;
enum {
        SOURCE,
        SOURCE_2ND
};
//static      struct timer_list my_timer;

#ifdef __FIRMWARE_UPDATE__
#define FW_ADDR_MSG20XX   0x26 //device address of msg20xx
#define FW_UPDATE_ADDR_MSG20XX   0x92>>1
static  char *fw_version;
#define DWIIC_MODE_ISP    0
#define DWIIC_MODE_DBBUS  1
static u8 temp[94][1024];   //
static int FwDataCnt;
//static int FwVersion;
struct class *firmware_class;
struct device *firmware_cmd_dev;
#endif

struct msg21xx_ts_data {
	uint16_t addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	int use_irq;
	struct hrtimer timer;
	struct work_struct  work;
	int (*power)(int on);
	int (*get_int_status)(void);
	void (*reset_ic)(void);
};
//static struct msg21xx_ts_data *msg21xx_ts_gpts = NULL;

typedef struct
{
    u32 u32X;       /* X coordinate, 0~2047 */
    u32 u32Y;       /* Y coordinate, 0~2047 */
    u8  u8Finger;   /* Number of fingers in the touch event */
    u32 u32Dis_X;   /* X distance between two fingers */
    u32 u32Dis_Y;   /* Y distance between two fingers. This value could be negative */
} touch_data_st;




#ifdef __FIRMWARE_UPDATE__





void HalTscrCReadI2CSeq(u8 addr, u8* read_data, u8 size)
{
   //according to your platform.
   	int rc;
 
	struct i2c_msg msgs[] = {
		{
			.addr = addr,
			.flags = I2C_M_RD,
			.len = size,
			.buf = read_data,
		},
	};
	
	rc = i2c_transfer(msg21xx_i2c_client->adapter, msgs, 1);
	if (rc < 0) {
		printk("HalTscrCReadI2CSeq error %d\n", rc);
	}
	
   
}

void HalTscrCDevWriteI2CSeq(u8 addr, u8* data, u16 size)
{
    //according to your platform.
   	int rc;
 
	struct i2c_msg msgs[] = {
		{
			.addr = addr,
			.flags = 0,
			.len = size,
			.buf = data,
		},
	};
	rc = i2c_transfer(msg21xx_i2c_client->adapter, msgs, 1);
	if(rc < 0){
		printk("HalTscrCDevWriteI2CSeq error %d\n", rc);
	}
}

void Get_Chip_Version(void)
{

    unsigned char dbbus_tx_data[3];
    unsigned char dbbus_rx_data[2];

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0xCE;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, &dbbus_tx_data[0], 3);
    HalTscrCReadI2CSeq(FW_ADDR_MSG20XX, &dbbus_rx_data[0], 2);
    if (dbbus_rx_data[1] == 0)
    {
        // it is Catch2
        TP_DEBUG("*** Catch2 ***\n");
        //FwVersion  = 2;// 2 means Catch2
    }
    else
    {
        // it is catch1
        TP_DEBUG("*** Catch1 ***\n");
        //FwVersion  = 1;// 1 means Catch1
    }

}

void dbbusDWIICEnterSerialDebugMode(void)
{
    U8 data[5];

    // Enter the Serial Debug Mode
    data[0] = 0x53;
    data[1] = 0x45;
    data[2] = 0x52;
    data[3] = 0x44;
    data[4] = 0x42;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, data, 5);
}

void dbbusDWIICStopMCU(void)
{
    U8 data[1];

    // Stop the MCU
    data[0] = 0x37;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, data, 1);
}

void dbbusDWIICIICUseBus(void)
{
    U8 data[1];

    // IIC Use Bus
    data[0] = 0x35;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, data, 1);
}

void dbbusDWIICIICReshape(void)
{
    U8 data[1];

    // IIC Re-shape
    data[0] = 0x71;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, data, 1);
}

void dbbusDWIICIICNotUseBus(void)
{
    U8 data[1];

    // IIC Not Use Bus
    data[0] = 0x34;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, data, 1);
}

void dbbusDWIICNotStopMCU(void)
{
    U8 data[1];

    // Not Stop the MCU
    data[0] = 0x36;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, data, 1);
}

void dbbusDWIICExitSerialDebugMode(void)
{
    U8 data[1];

    // Exit the Serial Debug Mode
    data[0] = 0x45;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, data, 1);

    // Delay some interval to guard the next transaction
    //udelay ( 200 );        // delay about 0.2ms
}

void drvISP_EntryIspMode(void)
{
    U8 bWriteData[5] =
    {
        0x4D, 0x53, 0x54, 0x41, 0x52
    };

    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, bWriteData, 5);

    //udelay ( 1000 );        // delay about 1ms
}

U8 drvISP_Read(U8 n, U8* pDataToRead)    //First it needs send 0x11 to notify we want to get flash data back.
{
    U8 Read_cmd = 0x11;
    unsigned char dbbus_rx_data[2] = {0};
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, &Read_cmd, 1);
    if (n == 1)
    {
        HalTscrCReadI2CSeq(FW_UPDATE_ADDR_MSG20XX, &dbbus_rx_data[0], 2);
        *pDataToRead = dbbus_rx_data[0];
    }
    else
        HalTscrCReadI2CSeq(FW_UPDATE_ADDR_MSG20XX, pDataToRead, n);

    return 0;
}

void drvISP_WriteEnable(void)
{
    U8 bWriteData[2] =
    {
        0x10, 0x06
    };
    U8 bWriteData1 = 0x12;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, bWriteData, 2);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, &bWriteData1, 1);
}


void drvISP_ExitIspMode(void)
{
    U8 bWriteData = 0x24;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, &bWriteData, 1);
}

U8 drvISP_ReadStatus(void)
{
    U8 bReadData = 0;
    U8 bWriteData[2] =
    {
        0x10, 0x05
    };
    U8 bWriteData1 = 0x12;

    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, bWriteData, 2);
    drvISP_Read(1, &bReadData);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, &bWriteData1, 1);
    return bReadData;
	 
}

void drvISP_BlockErase(U32 addr)
{
    U8 bWriteData[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
    U8 bWriteData1 = 0x12;

    drvISP_WriteEnable();

    //Enable write status register
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x50;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, bWriteData, 2);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, &bWriteData1, 1);

    //Write Status
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x01;
    bWriteData[2] = 0x00;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, bWriteData, 3);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, &bWriteData1, 1);

    //Write disable
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x04;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, bWriteData, 2);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, &bWriteData1, 1);

    while ((drvISP_ReadStatus() & 0x01) == 0x01);

    drvISP_WriteEnable();

    bWriteData[0] = 0x10;
    bWriteData[1] = 0xC7;//0xD8;        //Block Erase
    //bWriteData[2] = ((addr >> 16) & 0xFF) ;
    //bWriteData[3] = ((addr >> 8) & 0xFF) ;
    //bWriteData[4] = (addr & 0xFF) ;
	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, bWriteData, 2);
    //HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, &bWriteData, 5);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, &bWriteData1, 1);

    while ((drvISP_ReadStatus() & 0x01) == 0x01);
	 
}

void drvISP_Program(U16 k, U8* pDataToWrite)
{
    U16 i = 0;
    U16 j = 0;
    //U16 n = 0;
    U8 TX_data[133];
    U8 bWriteData1 = 0x12;
    U32 addr = k * 1024;

    for (j = 0; j < 8; j++)   //128*8 cycle
    {
        TX_data[0] = 0x10;
        TX_data[1] = 0x02;// Page Program CMD
        TX_data[2] = (addr + 128 * j) >> 16;
        TX_data[3] = (addr + 128 * j) >> 8;
        TX_data[4] = (addr + 128 * j);
        for (i = 0; i < 128; i++)
        {
            TX_data[5 + i] = pDataToWrite[j * 128 + i];
        }
        while ((drvISP_ReadStatus() & 0x01) == 0x01);     //wait until not in write operation
        drvISP_WriteEnable();
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, TX_data, 133);   //write 133 byte per cycle
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, &bWriteData1, 1);
    }
	    
}


void drvISP_Verify(U16 k, U8* pDataToVerify)
{
    U16 i = 0, j = 0;
    U8 bWriteData[5] =
    {
        0x10, 0x03, 0, 0, 0
    };
    U8 RX_data[256];
    U8 bWriteData1 = 0x12;
    U32 addr = k * 1024;
    U8 index=0;
    for (j = 0; j < 8; j++)   //128*8 cycle
    {
        bWriteData[2] = (U8)((addr + j * 128) >> 16);
        bWriteData[3] = (U8)((addr + j * 128) >> 8);
        bWriteData[4] = (U8)(addr + j * 128);
        while ((drvISP_ReadStatus() & 0x01) == 0x01);     //wait until not in write operation
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, bWriteData, 5);    //write read flash addr
        drvISP_Read(128, RX_data);
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, &bWriteData1, 1);    //cmd end
        for (i = 0; i < 128; i++)   //log out if verify error
        {
        if((RX_data[i]!=0)&&index<10)
		{
        TP_DEBUG("j=%d,RX_data[%d]=0x%x\n",j,i,RX_data[i]);
        index++;
		}
            if (RX_data[i] != pDataToVerify[128 * j + i])
            {
                TP_DEBUG("k=%d,j=%d,i=%d===============Update Firmware Error================",k,j,i);
            }
        }
    }
}

static ssize_t firmware_update_show(struct device *dev,
                                    struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%s\n", fw_version);
}
void _HalTscrHWReset(void)
{
	gpio_direction_output(MSG21XX_RESET_GPIO, 1);
	gpio_set_value(MSG21XX_RESET_GPIO, 1);	
	gpio_set_value(MSG21XX_RESET_GPIO, 0);
    mdelay(10);  /* Note that the RST must be in LOW 10ms at least */
	gpio_set_value(MSG21XX_RESET_GPIO, 1);
    /* Enable the interrupt service thread/routine for INT after 50ms */
    mdelay(50);
}
static ssize_t firmware_update_store(struct device *dev,
                                     struct device_attribute *attr, const char *buf, size_t size)
{
    U8 i;
    U8 dbbus_tx_data[4];
    unsigned char dbbus_rx_data[2] = {0};


    drvISP_EntryIspMode();
    drvISP_BlockErase(0x00000);
    _HalTscrHWReset();
    mdelay(300);
    //msctpc_LoopDelay ( 100 );        // delay about 100ms*****

    // Enable slave's ISP ECO mode
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x08;
    dbbus_tx_data[2] = 0x0c;
    dbbus_tx_data[3] = 0x08;

    // Disable the Watchdog
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 4);



    //Get_Chip_Version();
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x11;
    dbbus_tx_data[2] = 0xE2;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 4);
    
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x60;
    dbbus_tx_data[3] = 0x55;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 4);

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x61;
    dbbus_tx_data[3] = 0xAA;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 4);

//Stop MCU
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x0F;
    dbbus_tx_data[2] = 0xE6;
    dbbus_tx_data[3] = 0x01;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 4);

//Enable SPI Pad
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x02;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 3);
    HalTscrCReadI2CSeq(FW_ADDR_MSG20XX, &dbbus_rx_data[0], 2);
    TP_DEBUG("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
    dbbus_tx_data[3] = (dbbus_rx_data[0] | 0x20);  //Set Bit 5
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 4);

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x25;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 3);
    dbbus_rx_data[0] = 0;
    dbbus_rx_data[1] = 0;
    HalTscrCReadI2CSeq(FW_ADDR_MSG20XX, &dbbus_rx_data[0], 2);
    TP_DEBUG("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
    dbbus_tx_data[3] = dbbus_rx_data[0] & 0xFC;  //Clear Bit 1,0
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 4);

//WP overwrite
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x0E;
    dbbus_tx_data[3] = 0x02;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 4);


//set pin high
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x10;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 4);


    dbbusDWIICIICNotUseBus();

    dbbusDWIICNotStopMCU();

    dbbusDWIICExitSerialDebugMode();


    ///////////////////////////////////////
    // Start to load firmware
    ///////////////////////////////////////
    drvISP_EntryIspMode();
    //drvISP_BlockErase(0x00000);
        //TP_DEBUG("FwVersion=2");
        for (i = 0; i < 94; i++)   // total  94 KB : 1 byte per R/W
        {
            drvISP_Program(i, temp[i]);    // program to slave's flash
            drvISP_Verify ( i, temp[i] ); //verify data
        }
		TP_DEBUG("update OK\n");
    drvISP_ExitIspMode();
    FwDataCnt = 0;
    return size;
}

static DEVICE_ATTR(update, 0777, firmware_update_show, firmware_update_store);

/*test=================*/
static ssize_t firmware_clear_show(struct device *dev,
                                    struct device_attribute *attr, char *buf)
{
    U16 k=0,i = 0, j = 0;
    U8 bWriteData[5] =
    {
        0x10, 0x03, 0, 0, 0
    };
    U8 RX_data[256];
    U8 bWriteData1 = 0x12;
	U32 addr = 0;

	for (k = 0; k < 94; i++)   // total  94 KB : 1 byte per R/W
   {
      addr = k * 1024;
    for (j = 0; j < 8; j++)   //128*8 cycle
    {
        bWriteData[2] = (U8)((addr + j * 128) >> 16);
        bWriteData[3] = (U8)((addr + j * 128) >> 8);
        bWriteData[4] = (U8)(addr + j * 128);
        while ((drvISP_ReadStatus() & 0x01) == 0x01);     //wait until not in write operation
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, bWriteData, 5);    //write read flash addr
        drvISP_Read(128, RX_data);
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG20XX, &bWriteData1, 1);    //cmd end
        for (i = 0; i < 128; i++)   //log out if verify error
        {
            if (RX_data[i] != 0xFF)
            {
                TP_DEBUG("k=%d,j=%d,i=%d===erase not clean===",k,j,i);
            }
        }
    }
	}
   TP_DEBUG("read finish\n");
    return sprintf(buf, "%s\n", fw_version);
}

static ssize_t firmware_clear_store(struct device *dev,
                                     struct device_attribute *attr, const char *buf, size_t size)
{

		U8 dbbus_tx_data[4];
		unsigned char dbbus_rx_data[2] = {0};


		//msctpc_LoopDelay ( 100 ); 	   // delay about 100ms*****

		// Enable slave's ISP ECO mode
		/*
		dbbusDWIICEnterSerialDebugMode();
		dbbusDWIICStopMCU();
		dbbusDWIICIICUseBus();
		dbbusDWIICIICReshape();*/

		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x08;
		dbbus_tx_data[2] = 0x0c;
		dbbus_tx_data[3] = 0x08;

		// Disable the Watchdog
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 4);



		//Get_Chip_Version();
		//FwVersion  = 2;

		//if (FwVersion  == 2)
		{
			dbbus_tx_data[0] = 0x10;
			dbbus_tx_data[1] = 0x11;
			dbbus_tx_data[2] = 0xE2;
			dbbus_tx_data[3] = 0x00;

			HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 4);
		}

		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x3C;
		dbbus_tx_data[2] = 0x60;
		dbbus_tx_data[3] = 0x55;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 4);
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x3C;
		dbbus_tx_data[2] = 0x61;
		dbbus_tx_data[3] = 0xAA;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 4);

	//Stop MCU
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x0F;
		dbbus_tx_data[2] = 0xE6;
		dbbus_tx_data[3] = 0x01;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 4);

	//Enable SPI Pad
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x02;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 3);
		HalTscrCReadI2CSeq(FW_ADDR_MSG20XX, &dbbus_rx_data[0], 2);
		TP_DEBUG("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
		dbbus_tx_data[3] = (dbbus_rx_data[0] | 0x20);  //Set Bit 5
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 4);

		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x25;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 3);
		dbbus_rx_data[0] = 0;
		dbbus_rx_data[1] = 0;
		HalTscrCReadI2CSeq(FW_ADDR_MSG20XX, &dbbus_rx_data[0], 2);
		TP_DEBUG("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
		dbbus_tx_data[3] = dbbus_rx_data[0] & 0xFC;  //Clear Bit 1,0
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 4);

	//WP overwrite
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x0E;
		dbbus_tx_data[3] = 0x02;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 4);


	//set pin high
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x10;
		dbbus_tx_data[3] = 0x08;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG20XX, dbbus_tx_data, 4);


		dbbusDWIICIICNotUseBus();

		dbbusDWIICNotStopMCU();

		dbbusDWIICExitSerialDebugMode();


    ///////////////////////////////////////
    // Start to load firmware
    ///////////////////////////////////////
    drvISP_EntryIspMode();
	TP_DEBUG("chip erase+\n");
    drvISP_BlockErase(0x00000);
	TP_DEBUG("chip erase-\n");
    drvISP_ExitIspMode();
	
    return size;
}

static DEVICE_ATTR(clear, 0777, firmware_clear_show, firmware_clear_store);

/*test=================*/
/*Add by Tracy.Lin for update touch panel firmware and get fw version*/

static ssize_t firmware_version_show(struct device *dev,
                                     struct device_attribute *attr, char *buf)
{
    TP_DEBUG("*** firmware_version_show fw_version = %s***\n", fw_version);
    return sprintf(buf, "%s\n", fw_version);
}

static ssize_t firmware_version_store(struct device *dev,
                                      struct device_attribute *attr, const char *buf, size_t size)
{
    unsigned char dbbus_tx_data[3];
    unsigned char dbbus_rx_data[4] ;
    unsigned short major=0, minor=0;
/*
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();

*/
    fw_version = kzalloc(sizeof(char), GFP_KERNEL);

    //Get_Chip_Version();
    dbbus_tx_data[0] = 0x53;
    dbbus_tx_data[1] = 0x00;
    dbbus_tx_data[2] = 0x74;
    HalTscrCDevWriteI2CSeq(0x26, &dbbus_tx_data[0], 3);
    HalTscrCReadI2CSeq(0x26, &dbbus_rx_data[0], 4);

    major = (dbbus_rx_data[1]<<8)+dbbus_rx_data[0];
    minor = (dbbus_rx_data[3]<<8)+dbbus_rx_data[2];

    TP_DEBUG("***major = %d ***\n", major);
    TP_DEBUG("***minor = %d ***\n", minor);
    sprintf(fw_version,"%03d%03d", major, minor);
    TP_DEBUG("***fw_version = %s ***\n", fw_version);


    return size;
}
static DEVICE_ATTR(version, 0777, firmware_version_show, firmware_version_store);

static ssize_t firmware_data_show(struct device *dev,
                                  struct device_attribute *attr, char *buf)
{
    return FwDataCnt;
}

static ssize_t firmware_data_store(struct device *dev,
                                   struct device_attribute *attr, const char *buf, size_t size)
{

    int i;
	TP_DEBUG("***FwDataCnt = %d ***\n", FwDataCnt);
    for (i = 0; i < 1024; i++)
    {
        memcpy(temp[FwDataCnt], buf, 1024);
    }
    FwDataCnt++;
    return size;
}
static DEVICE_ATTR(data, 0777, firmware_data_show, firmware_data_store);
#endif




touch_data_st touch_data;
static int msg21xx_i2c_rx_data(char *buf, int len)
{
	uint8_t i;
	struct i2c_msg msg[] = {
		{
			.addr	= msg21xx_i2c_client->addr,
			.flags	= I2C_M_RD,
			.len	= len,
			.buf	= buf,
		}
	};
	
	for (i = 0; i < MSG21XX_RETRY_COUNT; i++) {
		if (i2c_transfer(msg21xx_i2c_client->adapter, msg, 1) >= 0) {
			break;
		}
		mdelay(10);
	}

	if (i >= MSG21XX_RETRY_COUNT) {
		pr_err("%s: retry over %d\n", __FUNCTION__, MSG21XX_RETRY_COUNT);
		return -EIO;
	}
	return 0;
	  
}

static void msg21xx_chip_init(void)
{
    /* After the LCD is on, power on the TP controller */
      
	gpio_direction_output(MSG21XX_RESET_GPIO, 1);
	gpio_set_value(MSG21XX_RESET_GPIO, 1);	
	gpio_set_value(MSG21XX_RESET_GPIO, 0);
    mdelay(10);  /* Note that the RST must be in LOW 10ms at least */
	gpio_set_value(MSG21XX_RESET_GPIO, 1);
    /* Enable the interrupt service thread/routine for INT after 50ms */
    mdelay(50);	   

}
static int msg21xx_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	
	gpio_set_value(MSG21XX_RESET_GPIO, 0);
	return 0;
	   
}
static int msg21xx_ts_resume(struct i2c_client *client)
{
	
	gpio_set_value(MSG21XX_RESET_GPIO, 1);
    /* Enable the interrupt service thread/routine for INT after 50ms */
    //mdelay(60);

    return 0;
}

static void msg21xx_ts_early_suspend(struct early_suspend *h)
{
	
	gpio_set_value(MSG21XX_RESET_GPIO, 0);
	disable_irq_nosync(msg21xx_irq);
	    
}

static void msg21xx_ts_late_resume(struct early_suspend *h)
{
	
	gpio_set_value(MSG21XX_RESET_GPIO, 1);
	enable_irq(msg21xx_irq);
	  
}


u8 Calculate_8BitsChecksum( u8 *msg, s32 s32Length )
{
    s32 s32Checksum = 0;
    s32 i;

    for ( i = 0 ; i < s32Length; i++ )
    {
        s32Checksum += msg[i];
    }

    return (u8)( ( -s32Checksum ) & 0xFF );
}


// val[] 0-use the space distinguish between the finger and key
//        1.2.3-use the spaces input  the first finger event
//        4.5.6 -use the spaces input the second finger event
//          7.-check those spaces sum
void msg2133_first(void)
{


		if ( (( Checksum == val[7] && val[0] == 0x52 ) )||(Checksum == val[7] && val[0] == 0x53 ))
	{
		//TP_DEBUG("checksum ok\n");
		if ( ( Checksum == val[7] && val[0] == 0x52 ) )
		{
		
		        touch_data.u32X = ( val[1] & 0xF0 ) << 4 | val[2];
		        touch_data.u32Y = ( val[1] & 0x0F )<< 8 | val[3];
		        touch_data.u32Dis_X = ( val[4] & 0xF0 ) << 4 | val[5];
		        touch_data.u32Dis_Y = ( val[4] & 0x0F ) << 8 | val[6];

				my_tempy = touch_data.u32X;
				my_tempx = touch_data.u32Y;
		        touch_data.u32X = my_tempx;
		        touch_data.u32Y = my_tempy;
		        
				my_tempy = touch_data.u32Dis_X;
				my_tempx = touch_data.u32Dis_Y;  
		        touch_data.u32Dis_X = my_tempx;
		        touch_data.u32Dis_Y = my_tempy;


///1111122222222222

	        uFinalX = 240-((touch_data.u32X*240)/2048);
	        uFinalY = ((touch_data.u32Y*320)/2048);
			
	        	TP_DEBUG("one-finger-touch  x1=%d y1 = %d\n",uFinalX,uFinalY);

#
	if(val[1]==0xff&&val[2]==0xff&&val[3]==0xff&&val[4]==0xff&&val[5]==0xff&&val[6]==0xff)// dian  up
		{

			
            	///TP_DEBUG("key_up\n");
            	#if 0
				input_report_abs(input, ABS_MT_TOUCH_MAJOR, 0);
				input_report_abs(input, ABS_MT_POSITION_X, uFinalX);
				input_report_abs(input, ABS_MT_POSITION_Y, uFinalY);
				#endif
				input_mt_sync(input);
				#if 0
				input_report_abs(input, ABS_MT_TOUCH_MAJOR, 0);
				input_report_abs(input, ABS_MT_POSITION_X, uFinalX2);
				input_report_abs(input, ABS_MT_POSITION_Y, uFinalY2);
				#endif
				input_mt_sync(input);	
				input_sync(input);
		
		}



				
	        /* The coordinate report stands for Touch-On-Panel event */
	        else if ( touch_data.u32Dis_X == 0 && touch_data.u32Dis_Y == 0 )
	        {
				
	            touch_data.u8Finger = 1;
				input_report_abs(input, ABS_MT_TOUCH_MAJOR, 1);
				input_report_abs(input, ABS_MT_POSITION_X, uFinalX);
				input_report_abs(input, ABS_MT_POSITION_Y, uFinalY);
				input_mt_sync(input);	

	        }
	        else 
	        {
				if(touch_data.u32Dis_X > 2048) touch_data.u32Dis_X -= 4096;
				if(touch_data.u32Dis_Y > 2048) touch_data.u32Dis_Y -= 4096;
	        	uFinalX2 = 240-(((u32)(touch_data.u32X + touch_data.u32Dis_X)) * 240) / 2048;
	        	uFinalY2 = (((u32)(touch_data.u32Y + touch_data.u32Dis_Y)) * 320) / 2048;

	            touch_data.u8Finger = 2;
				input_report_abs(input, ABS_MT_TOUCH_MAJOR, 1);
				input_report_abs(input, ABS_MT_POSITION_X, uFinalX);
				input_report_abs(input, ABS_MT_POSITION_Y, uFinalY);
				input_mt_sync(input);	
				input_report_abs(input, ABS_MT_TOUCH_MAJOR, 1);
				input_report_abs(input, ABS_MT_POSITION_X, uFinalX2);
				input_report_abs(input, ABS_MT_POSITION_Y, uFinalY2);
				input_mt_sync(input);	


	        }


		}
		 




		
	if ( Checksum == val[7] && val[0] == 0x53 )//key
        //if((touch_data.u32X == 0xFFF) && (touch_data.u32Y == 0xFFF))
        {
        
                TP_DEBUG("val[2]=%d\n",val[2]);
	       TP_DEBUG("val[1]=%d\n",val[1]);
		
           
                    switch(val[1]) {
                        case 0x00:
                                TP_DEBUG(KERN_DEBUG "Report key SEARCH.\n");
                 	            key_value = KEY_SEARCH;
                                break;
                        case 0x01:
                                TP_DEBUG(KERN_DEBUG "Report key BACK.\n");
                 	            key_value = KEY_BACK;
                                break;
                        case 0x02:
                                TP_DEBUG(KERN_DEBUG "Report key MENU.\n");
                 	            key_value = KEY_MENU;
                                break;
                        case 0x03:
                                TP_DEBUG(KERN_DEBUG "Report key HOME.\n");
                 	            key_value = KEY_HOME;
                                break;
                        default:
                                BUG();
                                break;
                    }

		 input_report_key(simulate_key_input, key_value, 1);
                 input_mt_sync(simulate_key_input);
                 flag_key_in = 1;
	#if 0			
               if ( val[2] == 0x01  )
            {//key in

                  input_report_key(simulate_key_input, key_value, 1);
                 input_mt_sync(simulate_key_input);
                 flag_key_in = 1;
			
               
				 
            }
	#endif		   
	 if ( val[2] == 0x00  ) //key up
	{
		flag_key_in = 0;
		input_report_key(simulate_key_input, key_value, 0);
		//input_report_key(simulate_key_input, 0, 0);
		//input_sync(simulate_key_input);
	}

        }
        input_sync(input);
		
    }
    else
    {
    	TP_DEBUG("checksum bad \n");
        /* Packet error happened. */
        /* Check I2C signal quality and I2C master codes */
    }
}

void msg2133_second(void)
{


	
	if ( ( Checksum == val[7] && val[0] == 0x52 ) )
	{


        touch_data.u32X = ( val[1] & 0xF0 ) << 4 | val[2];
        touch_data.u32Y = ( val[1] & 0x0F )<< 8 | val[3];
        touch_data.u32Dis_X = ( val[4] & 0xF0 ) << 4 | val[5];
        touch_data.u32Dis_Y = ( val[4] & 0x0F ) << 8 | val[6];




        if((touch_data.u32X == 0xFFF) && (touch_data.u32Y == 0xFFF))
        {

            if ((val[key_position] == 0x01)||(val[key_position] == 0x02)||(val[key_position] == 0x04)||(val[key_position] == 0x08))
            {//key in

           
                    switch(val[key_position]) {
                        case 0x01:
                                TP_DEBUG(KERN_DEBUG "Report key SEARCH.\n");
                 	            key_value = KEY_SEARCH;
                                break;
                        case 0x02:
                                TP_DEBUG(KERN_DEBUG "Report key BACK.\n");
                 	            key_value = KEY_BACK;
                                break;
                        case 0x04:
                                TP_DEBUG(KERN_DEBUG "Report key MENU.\n");
                 	            key_value = KEY_MENU;
                                break;
                        case 0x08:
                                TP_DEBUG(KERN_DEBUG "Report key HOME.\n");
                 	            key_value = KEY_HOME;
                                break;
                        default:
                                BUG();
                                break;
                    }
                

                 input_report_key(simulate_key_input, key_value, 1);
                 input_mt_sync(simulate_key_input);
                 flag_key_in = 1;
				 
            }
            else	
            {//key_up
            	printk("key_up\n");
				#if 0
				input_report_abs(input, ABS_MT_TOUCH_MAJOR, 0);
				input_report_abs(input, ABS_MT_POSITION_X, uFinalX);
				input_report_abs(input, ABS_MT_POSITION_Y, uFinalY);
				#endif
				input_mt_sync(input);	
				#if 0
				input_report_abs(input, ABS_MT_TOUCH_MAJOR, 0);
				input_report_abs(input, ABS_MT_POSITION_X, uFinalX2);
				input_report_abs(input, ABS_MT_POSITION_Y, uFinalY2);
				#endif
				input_mt_sync(input);	
				input_report_key(simulate_key_input, key_value, 0);
				input_mt_sync(simulate_key_input);
				input_sync(simulate_key_input);
				input_sync(input);
		//		input_sync(input);

            }

        }
             
        else
        {//tp
        	if(flag_key_in)
        	{
        		flag_key_in = 0;
				input_report_key(simulate_key_input, key_value, 0);
				input_mt_sync(simulate_key_input);
				input_sync(simulate_key_input);
			}
            uFinalX = ((touch_data.u32X * MS_TS_MSG20XX_X_MAX)/2048);
            uFinalY = ((touch_data.u32Y * MS_TS_MSG20XX_Y_MAX)/2048);
            
            /* The coordinate report stands for Touch-On-Panel event */
            if ( touch_data.u32Dis_X == 0 && touch_data.u32Dis_Y == 0 )
            {
				
                touch_data.u8Finger = 1;
				input_report_abs(input, ABS_MT_TOUCH_MAJOR, 1);
				input_report_abs(input, ABS_MT_POSITION_X, uFinalX);
				input_report_abs(input, ABS_MT_POSITION_Y, uFinalY);
				input_mt_sync(input);	

            }
            else 
            {
				if(touch_data.u32Dis_X > 2048) touch_data.u32Dis_X -= 4096;
				if(touch_data.u32Dis_Y > 2048) touch_data.u32Dis_Y -= 4096;
            	uFinalX2 = (((u32)(touch_data.u32X + touch_data.u32Dis_X)) * MS_TS_MSG20XX_X_MAX ) / 2048;
            	uFinalY2 = (((u32)(touch_data.u32Y + touch_data.u32Dis_Y)) * MS_TS_MSG20XX_Y_MAX ) / 2048;

                touch_data.u8Finger = 2;
				input_report_abs(input, ABS_MT_TOUCH_MAJOR, 1);
				input_report_abs(input, ABS_MT_POSITION_X, uFinalX);
				input_report_abs(input, ABS_MT_POSITION_Y, uFinalY);
				input_mt_sync(input);	
				input_report_abs(input, ABS_MT_TOUCH_MAJOR, 1);
				input_report_abs(input, ABS_MT_POSITION_X, uFinalX2);
				input_report_abs(input, ABS_MT_POSITION_Y, uFinalY2);
				input_mt_sync(input);	
	

            }

        }
		
	    TP_DEBUG("one-finger-touch  x1=%d y1 = %d   x_src=%d y_src=%d key=%d\n",uFinalX,uFinalY,touch_data.u32X,touch_data.u32Y, val[5]);
        TP_DEBUG("VAL[0 - 7]: [0]= %d [1]= %d [2]= %d [3]= %d [4]= %d [5]= %d [6]= %d [7]= %d \n\n\n", val[0], val[1], val[2], val[3], val[4], val[5], val[6], val[7]);
        input_sync(input);
		
    }
    else
    {
    	TP_DEBUG("checksum bad \n");
        /* Packet error happened. */
        /* Check I2C signal quality and I2C master codes */
    }   
}

#if 1

static void msg21xx_do_work(struct work_struct *work)
{
	  
	mutex_lock(&msg21xx_mutex);
	msg21xx_i2c_rx_data( &val[0], REPORT_PACKET_LENGTH );	 
	Checksum = Calculate_8BitsChecksum( val, (REPORT_PACKET_LENGTH-1) );
	//	TP_DEBUG("msg21xx_do_work\n");
	if(tp_type ==SOURCE)
	msg2133_first();
	else
	msg2133_second();

	enable_irq(msg21xx_irq);
	mutex_unlock(&msg21xx_mutex);
	
}

#endif
static int msg21xx_ts_open(struct input_dev *dev)
{
	return 0;
}
static void msg21xx_ts_close(struct input_dev *dev)
{
	printk("msg21xx_ts_close\n");

}

static int msg21xx_init_input(void)
{
	//int ret;
	int err;

	input = input_allocate_device();
	input->name = msg21xx_i2c_client->name;
	input->phys = "I2C";
	input->id.bustype = BUS_I2C;
	input->dev.parent = &msg21xx_i2c_client->dev;
	input->open = msg21xx_ts_open;
	input->close = msg21xx_ts_close;

	set_bit(EV_ABS, input->evbit);
	//set_bit(EV_SYN, input->evbit);
	//set_bit(BTN_TOUCH, input->keybit);

	input_set_abs_params(input, ABS_MT_TRACKING_ID, 0, 2, 0, 0);
	input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, 2, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_X, 0, MS_TS_MSG20XX_X_MAX , 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, 0, MS_TS_MSG20XX_Y_MAX , 0, 0);

	err = input_register_device(input);
	if (err)
		goto fail_alloc_input; 

	//create simulate key input
	simulate_key_input = input_allocate_device();
	if (simulate_key_input == NULL) {
		printk("%s: not enough memory for input device\n", __func__);
		err = -ENOMEM;
		goto fail_alloc_input;
	}
	simulate_key_input->name = "7x27a_kp";
	__set_bit(EV_KEY, simulate_key_input->evbit);
	__set_bit(KEY_BACK, simulate_key_input->keybit);
	__set_bit(KEY_MENU, simulate_key_input->keybit);
	__set_bit(KEY_HOME, simulate_key_input->keybit);
	__set_bit(ST_KEY_SEARCH, simulate_key_input->keybit);
	err = input_register_device(simulate_key_input);
	if (err != 0) {
		printk("%s: failed to register input device\n", __func__);
		goto fail_alloc_input;
	}
fail_alloc_input:	

	return 0;
}
static irqreturn_t msg21xx_interrupt(int irq, void *dev_id)
{	
	
	disable_irq_nosync(msg21xx_irq);
	schedule_work(&msg21xx_wq);
    
	return IRQ_HANDLED;
}


static int __devinit msg21xx_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int  err = 0;

#ifdef __FIRMWARE_UPDATE__
 // struct platform_device *pdev;
#endif

	msg21xx_i2c_client = client;
	
	gpio_request(MSG21XX_INT_GPIO, "interrupt");
	gpio_request(MSG21XX_RESET_GPIO, "reset");
	gpio_tlmm_config(GPIO_CFG(30, 0, 
					GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, 
					GPIO_CFG_2MA), GPIO_CFG_ENABLE);

					
	
	gpio_direction_input(MSG21XX_INT_GPIO);
	gpio_set_value(MSG21XX_INT_GPIO, 1);

	msg21xx_chip_init();
	INIT_WORK(&msg21xx_wq, msg21xx_do_work);
	msg21xx_init_input();
	msg21xx_irq = MSM_GPIO_TO_INT(MSG21XX_INT_GPIO);
	err = request_irq(msg21xx_irq, msg21xx_interrupt, 
							IRQF_TRIGGER_RISING, "msg21xx", NULL);
	if (err != 0) {
		printk("%s: cannot register irq\n", __func__);
		goto exit;
	} else {
        printk("%s: request irq[%d] for TP used.\n", __func__, msg21xx_irq);
    }	

#ifdef __FIRMWARE_UPDATE__
	  firmware_class = class_create(THIS_MODULE, "ms-touchscreen-msg20xx");
    if (IS_ERR(firmware_class))
     pr_err("Failed to create class(firmware)!\n");
     
    firmware_cmd_dev = device_create(firmware_class,
                                     NULL, 0, NULL, "device");
    if (IS_ERR(firmware_cmd_dev))
     pr_err("Failed to create device(firmware_cmd_dev)!\n");
     
    // version
    if (device_create_file(firmware_cmd_dev, &dev_attr_version) < 0)
        pr_err("Failed to create device file(%s)!\n", dev_attr_version.attr.name);
    // update
    if (device_create_file(firmware_cmd_dev, &dev_attr_update) < 0)
        pr_err("Failed to create device file(%s)!\n", dev_attr_update.attr.name);
    // data
    if (device_create_file(firmware_cmd_dev, &dev_attr_data) < 0)
        pr_err("Failed to create device file(%s)!\n", dev_attr_data.attr.name);

    if (device_create_file(firmware_cmd_dev, &dev_attr_clear) < 0)
        pr_err("Failed to create device file(%s)!\n", dev_attr_clear.attr.name);
        
    dev_set_drvdata(firmware_cmd_dev, NULL);
#endif
	
	early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	early_suspend.suspend = msg21xx_ts_early_suspend;
	early_suspend.resume = msg21xx_ts_late_resume;
	register_early_suspend(&early_suspend);
	return 0;

	
exit:
	return err;
}

static int __devexit msg21xx_ts_remove(struct i2c_client *client)
{

	return 0;
}



static const struct i2c_device_id msg21xx_ts_id[] = {
	{ "ms-msg20xx", 0x26 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, msg21xx_ts_id);


static struct i2c_driver msg21xx_ts_driver = {
	.driver = {
		.name = "ms-msg20xx",
	},
	.probe = msg21xx_ts_probe,
	.remove = __devexit_p(msg21xx_ts_remove),
	.suspend = msg21xx_ts_suspend,
	.resume = msg21xx_ts_resume,
	.id_table = msg21xx_ts_id,
};
/*
void timer_func(unsigned long data)
{
	printk("time[%d]\n",(int)jiffies);
	mod_timer(&my_timer,jiffies+2*HZ);
}

static int test_init(void)
{
	init_timer(&my_timer);
	my_timer.expires = jiffies + 5*HZ;
	my_timer.function= timer_func;
	my_timer.data = 99;

	printk("time[%d]\n",(int)jiffies);
	mdelay(1000);
	printk("time[%d]\n",(int)jiffies);
	add_timer(&my_timer);
	printk("start\n");
	printk("hello kernel\n");
	return 0;
}
*/


static int __init msg21xx_init(void)
{
	int err;

    gpio_request(FLAG_GPIO, "flag_msg21xx");
	gpio_direction_input(FLAG_GPIO);
	udelay(10);
    /* judge TP type by GPIO level: HIGH for source, LOW for 2nd source */
	if(gpio_get_value(FLAG_GPIO)) {
            printk("\nmsg21xx: TP type is source.\n");
            tp_type = SOURCE;
            key_position = 6;    
    } else {
            printk("\nmsg21xx: TP type is 2nd source.\n");
            tp_type = SOURCE_2ND;
            key_position = 5;
    }
	
    mutex_init(&msg21xx_mutex);
	err = i2c_add_driver(&msg21xx_ts_driver);
	if (err) {
		printk(KERN_WARNING "msg21xx  driver failed "
		       "(errno = %d)\n", err);
	} else {
		printk( "Successfully added driver %s\n",
		          msg21xx_ts_driver.driver.name);
	}
	
	return err;
}

static void __exit msg21xx_cleanup(void)
{
	i2c_del_driver(&msg21xx_ts_driver);
}


module_init(msg21xx_init);
module_exit(msg21xx_cleanup);

MODULE_AUTHOR("msg 2133 touchscreen cellon");
MODULE_DESCRIPTION("Driver for msg21xx Touchscreen Controller");
MODULE_LICENSE("GPL");
