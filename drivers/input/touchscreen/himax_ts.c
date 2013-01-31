/*
self driver for himax

*/

#include <linux/module.h>
#include <linux/delay.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/himax_ts.h>
#include <linux/gpio.h>
#include <linux/kernel.h>

#define ChangeIref1u 1




//#define __HX_DEBUG__
#define __USB_CHARGER_STATUS__
#ifdef __HX_DEBUG__

#define dbg_printk(fmt,args...)   \
 printk(KERN_DEBUG"himax->%s_%d:"fmt,__FUNCTION__,__LINE__,##args)
#else
#define dbg_printk(fmt,args...)
#endif

#define err_printk(fmt,args...)   \
printk(KERN_ERR"himax->%s_%d:"fmt,__FUNCTION__,__LINE__,##args)

#define info_printk(fmt,args...)   \
printk(KERN_INFO"himax->%s_%d:"fmt,__FUNCTION__,__LINE__,##args)


#if 0
#define HIMAX_RO_ATTR S_IRUGO
#define HIMAX_RW_ATTR (S_IRUGO | S_IWUSR | S_IWGRP)
#define HIMAX_WO_ATTR (S_IWUSR | S_IWGRP)
#else
#define HIMAX_RO_ATTR (S_IRUGO | S_IWUSR)
#define HIMAX_RW_ATTR HIMAX_RO_ATTR
#define HIMAX_WO_ATTR HIMAX_RO_ATTR
#endif



#define GPIO_OUTPUT_HI    1
#define GPIO_OUTPUT_LO    0
#define TOUCH_UP      0xAA
#define TOUCH_DOWN    0xBB
#define TOUCH_INVALID 0xCC

#define TOUCH_DATA_INVALID 0xFFFFFFFF
#define TIMER_DEFAULT_VALUE 50
#define TIMER_BASE 1000000
#define TIMER_MAX_VALUE 80
#define HIMAX_X_MAX 540
#define HIMAX_Y_MAX 1030
#define HIMAX_MAJOR_MAX 1
#define HIMAX_WIDTH_MAX 10



static struct workqueue_struct *himax_wq;

struct touch_points_info {
 uint8_t  pre_status ;  /*up or down*/
 uint8_t  cur_status ;  /*up or down*/
};

enum {
    HIMAX_STATE_NONE,
    HIMAX_STATE_ACTIVE,
    HIMAX_STATE_DEEPSLEEP,
    HIMAX_STATE_IDLE
};
#define PRO_NAME_FLASH_ADDR        (0x2a8/4)
#define PRO_NAME_FLASH_LEN         (3)
#define POINT_NUMBER_ADDR          176 //0x02C0
#define POINT_NUMBER_LEN            3

#define FINGER_FOUR_PACKET_SIZE     24
#define FINGER_FIVE_PACKET_SIZE     28

static char * driver_ver [] = {
 "add 0x90 command to notifier TP IC usb charger status",
 "update firmware to support 5 finger number",
 "remove tp support 5 point function ",
 "compatible with 5 point firmware,and add temperature feature",
 "add himax tp support 5 point function"
};


//#define  CONFIG_HX8526A_FW_UPDATE
#define FW_VER_C8680    "Himax_FW_Truly_CT1291_Cellon_C8680_V16-0_2012-12-13_1053.i"
#define FW_VER_C8680QM  "Himax_FW_Truly_CT4F0064_Cellon_C8680QM_V70_2012-12-05_2002.i"
#define FW_VER_C8680GP  "Himax_FW_Truly_Cellon_C8680-4-Point_V12-3_2012-12-04_1514.i"


struct himax_ts_data {
    uint16_t addr;
    struct i2c_client *client;
    struct input_dev *input_dev;
    int charger_status ;  /* 1,charger on;0:charger off*/
    int use_irq;
    unsigned int irq_count ; /* irq counts */
    char himax_state ;/*active,deep sleep */
    int  bit_map ;
    int  point_num ;
    char * finger_buf ;//24 or 28 Byte
    bool cmd_92    ; //set five finger
    struct device dev ;
    struct hrtimer timer;
    unsigned long interval;//ms
    const char * fw_name ;
    char fw_ver[POINT_NUMBER_LEN*4+1] ;
    int err_count ;
#ifdef CONFIG_HX8526A_FW_UPDATE
    struct regulator * regulator ;
#endif
    bool upgrade ;  //1:success ;
    struct work_struct  work;
#ifdef __SELF_REPORT__
    struct touch_points_info points[TOUCH_MAX_NUMBER];
#endif
    int (*power)(int on);
#ifdef CONFIG_HAS_EARLYSUSPEND
    struct early_suspend early_suspend;
#endif
};

static void himax_ts_reset(struct himax_platform_data *pdata);
static int  himax_ts_poweron(struct himax_ts_data * ts);

#define PROJECT_C8680                 "CT1291"
#define PROJECT_C8680_FOUR_VER_MIN     "V00"
#define PROJECT_C8680_FF_VER_MIN       "V16"
#define PROJECT_C8680_FOUR_VER_MAX     "V20"
#define PROJECT_C8680_FIVE_VER_MIN     "V21"
#define PROJECT_C8680_FIVE_VER_MAX     "V49"

#define PROJECT_C8680QM                 "C8680QM"
#define PROJECT_C8680QM_FOUR_VER_MIN     "V70"
#define PROJECT_C8680QM_FF_VER_MIN       "V75"
#define PROJECT_C8680QM_FOUR_VER_MAX     "V79"
#define PROJECT_C8680QM_FIVE_VER_MIN     "V50"
#define PROJECT_C8680QM_FIVE_VER_MAX     "V69"

#define PROJECT_C8680GP                  "C8680GP"
#define PROJECT_C8680GP_FOUR_VER_MIN      "V80"
#define PROJECT_C8680GP_FF_VER_MIN        "V85"
#define PROJECT_C8680GP_FOUR_VER_MAX      "V89"
#define PROJECT_C8680GP_FIVE_VER_MIN      "V90"
#define PROJECT_C8680GP_FIVE_VER_MAX      "V99"
#define C8680GP_X_MIN  265
#define C8680GP_X_MAX  280
#define C8680GP_Y_MIN  980
#define C8680GP_X_POINT 200
#define C8680GP_Y_POINT 1008


#define POINT_NONE_BIT             BIT(0)
#define PROJECT_C8680_BIT          BIT(1)
#define PROJECT_C8680QM_BIT        BIT(2)
#define PROJECT_C8680GP_BIT        BIT(4)
#define PROJECT_NAME_NONE          BIT(3)

#define FINGER_FOUR        4
#define FINGER_FIVE        5
#define FINGER_NONE        0

#define PROJECT_NONE       0
#define C8680_VAL      1
#define C8680QM_VAL    2
#define C8680GP_VAL    3


int himax_i2c_read(struct himax_ts_data * ts,unsigned char addr, int datalen, unsigned char data[], int *readlen)
 {
     int ret = 0;
     struct i2c_msg msg[2];
     uint8_t start_reg;
     //uint8_t buf[128] = {0};


     start_reg = addr;
     msg[0].addr = ts->client->addr;
     msg[0].flags = 0;
     msg[0].len = 1;
     msg[0].buf = &start_reg;

     msg[1].addr = ts->client->addr;
     msg[1].flags = I2C_M_RD;
     msg[1].len =  datalen;
     msg[1].buf = data;

     ret = i2c_transfer(ts->client->adapter, msg, 2);
     if (ret < 0) {
         memset(data, 0xff , 24);

         return 0;
     }
     return 0;
 }


static int hx8526_read_flash(struct himax_ts_data * ts,unsigned char *buf, unsigned int addr, unsigned int length)
 {
	unsigned char index_byte,index_page,index_sector;
    int readLen;
	unsigned char buf0[7];
	unsigned int i;
	unsigned int j = 0;

	index_byte = (addr & 0x001F);
	index_page = ((addr & 0x03E0) >> 5);
	index_sector = ((addr & 0x1C00) >> 10);

	for (i = addr; i < addr + length; i++)
	{
		buf0[0] = 0x44;
		buf0[1] = i&0x1F;
		buf0[2] = (i>>5)&0x1F;
		buf0[3] = (i>>10)&0x1F;

		i2c_master_send(ts->client, buf0,4 );//himax_i2c_master_write(slave_address, 4, buf0);
		udelay(10);

		buf0[0] = 0x46;
		i2c_master_send(ts->client, buf0,1 );//himax_i2c_master_write(slave_address, 1, buf0);
		udelay(10);

		//Himax: Read FW Version to buf
		himax_i2c_read(ts,0x59, 4, &buf[0+j], &readLen);
		udelay(10);
		j += 4;
	}

	return 1;
}
/*return value: 0:error 1: c8680  2 : c8680QM
C8680   : CT1291
0-15    : 4 point
16-20   : 4 or 5 point
21-49   : 5 point

C8680QM   :C8680QM
50-69     :5 point
70-74     :4 point
75-79     :4 or 5 point

//one icon
C8680GP   :C8680GP
80-84     :4 point
85-89     :4 or 5 point
90-99     :5 point

*/
static inline int get_point(struct himax_ts_data *ts)
{
        if(ts->bit_map&PROJECT_NAME_NONE)
            return FINGER_NONE ;
        else if(ts->bit_map&PROJECT_C8680_BIT){
            if(strncmp(ts->fw_ver,PROJECT_C8680_FOUR_VER_MAX,3)<=0){
                if(strncmp(ts->fw_ver,PROJECT_C8680_FF_VER_MIN,3)>=0){
                 ts->cmd_92 = true ; //send cmd 92 to switch 5 finger number
                 return FINGER_FIVE ;
                 }else return FINGER_FOUR ;

           }else if(strncmp(ts->fw_ver,PROJECT_C8680_FIVE_VER_MIN,3)>=0 &&
                strncmp(ts->fw_ver,PROJECT_C8680_FIVE_VER_MAX,3)<=0){
                   return FINGER_FIVE ;
                }

        }else if(ts->bit_map&PROJECT_C8680QM_BIT){
            if(strncmp(ts->fw_ver,PROJECT_C8680QM_FOUR_VER_MIN,3)>=0 &&
               strncmp(ts->fw_ver,PROJECT_C8680QM_FOUR_VER_MAX,3)<=0){
                if(strncmp(ts->fw_ver,PROJECT_C8680QM_FF_VER_MIN,3)>=0){
                    ts->cmd_92 = true ;
                    return FINGER_FIVE ;
                }else return FINGER_FOUR ;

            }else if(strncmp(ts->fw_ver,PROJECT_C8680QM_FIVE_VER_MIN,3)>=0 &&
                strncmp(ts->fw_ver,PROJECT_C8680QM_FIVE_VER_MAX,3)<=0)
                return FINGER_FIVE ;
        }else if(ts->bit_map&PROJECT_C8680GP_BIT){
            if(strncmp(ts->fw_ver,PROJECT_C8680GP_FOUR_VER_MIN,3)>=0 &&
               strncmp(ts->fw_ver,PROJECT_C8680GP_FOUR_VER_MAX,3)<=0){
                if(strncmp(ts->fw_ver,PROJECT_C8680GP_FF_VER_MIN,3)>=0){
                    ts->cmd_92 = true ;
                    return FINGER_FIVE ;
                }else return FINGER_FOUR ;

            }else if(strncmp(ts->fw_ver,PROJECT_C8680GP_FIVE_VER_MIN,3)>=0 &&
                strncmp(ts->fw_ver,PROJECT_C8680GP_FIVE_VER_MAX,3)<=0)
                return FINGER_FIVE ;
        }

    err_printk("not detect finger number \n");
    return FINGER_NONE ;
}
static int detect_point_number(struct himax_ts_data *ts)
{
    unsigned char buf[POINT_NUMBER_LEN*4 +1];

    unsigned char cmd[5];

    //Himax: Power On Flow
    cmd[0] = 0x42;
    cmd[1] = 0x02;
    i2c_master_send(ts->client, cmd, 2);//himax_i2c_master_write(slave_address, 2, cmd);
    udelay(10);

    cmd[0] = 0x36;
    cmd[1] = 0x0F;
    cmd[2] = 0x53;
    i2c_master_send(ts->client, cmd, 3);//himax_i2c_master_write(slave_address, 3, cmd);
    udelay(10);

    //Himax: Check Flash Pre-Patch
    cmd[0] = 0xDD;
    cmd[1] = 0x06; //Himax: Modify by case
    cmd[2] = 0x02;
    i2c_master_send(ts->client, cmd, 3);//himax_i2c_master_write(slave_address, 3, cmd);
    udelay(10);

    cmd[0] = 0x81;
    i2c_master_send(ts->client, cmd, 1);//himax_i2c_master_write(slave_address, 1, cmd);
    udelay(10);

    //Himax: Flash Read Enable
    cmd[0] = 0x43;
    cmd[1] = 0x01;
    cmd[2] = 0x00;
    cmd[3] = 0x02;
    i2c_master_send(ts->client, cmd, 4);//himax_i2c_master_write(slave_address, 4, cmd);
    udelay(10);

    //Himax: Read FW Major Version
    if (hx8526_read_flash(ts,buf, POINT_NUMBER_ADDR, POINT_NUMBER_LEN) < 0){
        err_printk("read project name error \n");
        return 0 ;
    }

    buf[POINT_NUMBER_LEN*4] = '\0' ;

    //Himax: Flash Read Disable
    cmd[0] = 0x43;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x02;
    i2c_master_send(ts->client, cmd, 4);//himax_i2c_master_write(slave_address, 4, cmd);
    udelay(10);

    info_printk("point number [%s]\n",buf);
    strncpy(ts->fw_ver,buf,POINT_NUMBER_LEN*4+1) ;
    ts->point_num = get_point(ts) ;
    return 0 ;
}

/*
  1:C8680
  2:C8680QM
  3:C8680GP
  -1:error
*/
static inline int get_project(struct himax_ts_data *ts)
{
    if(ts->bit_map & PROJECT_C8680_BIT)
        return C8680_VAL ;
    else if(ts->bit_map &PROJECT_C8680QM_BIT)
        return C8680QM_VAL ;
    else if(ts->bit_map &PROJECT_C8680GP_BIT)
        return C8680GP_VAL ;
    return PROJECT_NONE ;

}

static int detect_project_name(struct himax_ts_data * ts)
{
        unsigned char buf[PRO_NAME_FLASH_LEN*4 +1];
        int ret ;
//        unsigned int i;
        unsigned char cmd[5];

        //Himax: Power On Flow
        cmd[0] = 0x42;
        cmd[1] = 0x02;
        ret = i2c_master_send(ts->client, cmd, 2);//himax_i2c_master_write(slave_address, 2, cmd);
        if(ret <=0){
        err_printk(" detect project name error [%d]\n",ret);
        return -ENODEV ;
        }
        udelay(10);

        cmd[0] = 0x36;
        cmd[1] = 0x0F;
        cmd[2] = 0x53;
        i2c_master_send(ts->client, cmd, 3);//himax_i2c_master_write(slave_address, 3, cmd);
        udelay(10);

        //Himax: Check Flash Pre-Patch
        cmd[0] = 0xDD;
        cmd[1] = 0x06; //Himax: Modify by case
        cmd[2] = 0x02;
        i2c_master_send(ts->client, cmd, 3);//himax_i2c_master_write(slave_address, 3, cmd);
        udelay(10);

        cmd[0] = 0x81;
        i2c_master_send(ts->client, cmd, 1);//himax_i2c_master_write(slave_address, 1, cmd);
        udelay(10);

        //Himax: Flash Read Enable
        cmd[0] = 0x43;
        cmd[1] = 0x01;
        cmd[2] = 0x00;
        cmd[3] = 0x02;
        i2c_master_send(ts->client, cmd, 4);//himax_i2c_master_write(slave_address, 4, cmd);
        udelay(10);

        //Himax: Read FW Major Version
        if (hx8526_read_flash(ts,buf, PRO_NAME_FLASH_ADDR, PRO_NAME_FLASH_LEN) < 0){
            err_printk("read project name error \n");
            return 0 ;
        }
        buf[PRO_NAME_FLASH_LEN*4] = '\0' ;

        //Himax: Flash Read Disable
        cmd[0] = 0x43;
        cmd[1] = 0x00;
        cmd[2] = 0x00;
        cmd[3] = 0x02;
        i2c_master_send(ts->client, cmd, 4);//himax_i2c_master_write(slave_address, 4, cmd);
        udelay(10);

        info_printk(" raw buf  [%s]\n",buf);


        if(!strncasecmp(buf,PROJECT_C8680,strlen(PROJECT_C8680))){
          info_printk(" project name  [%s]\n",PROJECT_C8680);
          ts->bit_map |= PROJECT_C8680_BIT ;
          ts->fw_name = FW_VER_C8680 ;
        }else if(!strncasecmp(buf,PROJECT_C8680QM,strlen(PROJECT_C8680QM))){
          info_printk(" project name  [%s]\n",PROJECT_C8680QM);
          ts->bit_map |= PROJECT_C8680QM_BIT ;
          ts->fw_name = FW_VER_C8680QM ;
        }else if(!strncasecmp(buf,PROJECT_C8680GP,strlen(PROJECT_C8680GP))){
          info_printk(" project name  [%s]\n",buf);
          ts->bit_map |= PROJECT_C8680GP_BIT ;
          ts->fw_name = FW_VER_C8680GP ;
          }else {
          info_printk("project name none\n");
          ts->bit_map |= PROJECT_NAME_NONE ;
          return -ENODEV ;
        }


       return  detect_point_number(ts);

}
#ifdef CONFIG_HX8526A_FW_UPDATE
#define FW_VER_MAJ_FLASH_ADDR       33 //0x0085
#define FW_VER_MAJ_FLASH_LENG       1
#define FW_VER_MIN_FLASH_ADDR       182 //0x02D8
#define FW_VER_MIN_FLASH_LENG       1
#define CFG_VER_MAJ_FLASH_ADDR      173 //0x02B4
#define CFG_VER_MAJ_FLASH_LENG      3
#define CFG_VER_MIN_FLASH_ADDR      176 //0x02C0
#define CFG_VER_MIN_FLASH_LENG      3
#define REGULATOR_MNI_VAL           3000000
#define REGULATOR_MAX_VAL           3000000
static unsigned char HIMAX_FW_C8680[]=
{
	#include FW_VER_C8680 //Paul Check
};
static unsigned char HIMAX_FW_C8680QM[]=
{
	#include FW_VER_C8680QM //Paul Check
};
static unsigned char HIMAX_FW_C8680GP[]=
{
	#include FW_VER_C8680GP //Paul Check
};

unsigned char SFR_3u_1[16][2] = {{0x18,0x06},{0x18,0x16},{0x18,0x26},{0x18,0x36},{0x18,0x46},
                           	{0x18,0x56},{0x18,0x66},{0x18,0x76},{0x18,0x86},{0x18,0x96},
				{0x18,0xA6},{0x18,0xB6},{0x18,0xC6},{0x18,0xD6},{0x18,0xE6},{0x18,0xF6}};

unsigned char SFR_6u_1[16][2] = {{0x98,0x04},{0x98,0x14},{0x98,0x24},{0x98,0x34},{0x98,0x44},
                                {0x98,0x54},{0x98,0x64},{0x98,0x74},{0x98,0x84},{0x98,0x94},
			        {0x98,0xA4},{0x98,0xB4},{0x98,0xC4},{0x98,0xD4},{0x98,0xE4},
				{0x98,0xF4}};

#endif

#if ChangeIref1u
#define TarIref 1
		unsigned char SFR_1u_1[16][2] = {{0x18,0x07},{0x18,0x17},{0x18,0x27},{0x18,0x37},{0x18,0x47},
			{0x18,0x57},{0x18,0x67},{0x18,0x77},{0x18,0x87},{0x18,0x97},
			{0x18,0xA7},{0x18,0xB7},{0x18,0xC7},{0x18,0xD7},{0x18,0xE7},
			{0x18,0xF7}};

		unsigned char SFR_2u_1[16][2] = {{0x98,0x06},{0x98,0x16},{0x98,0x26},{0x98,0x36},{0x98,0x46},
			{0x98,0x56},{0x98,0x66},{0x98,0x76},{0x98,0x86},{0x98,0x96},
			{0x98,0xA6},{0x98,0xB6},{0x98,0xC6},{0x98,0xD6},{0x98,0xE6},
			{0x98,0xF6}};

		unsigned char SFR_3u_11[16][2] = {{0x18,0x06},{0x18,0x16},{0x18,0x26},{0x18,0x36},{0x18,0x46},
			{0x18,0x56},{0x18,0x66},{0x18,0x76},{0x18,0x86},{0x18,0x96},
			{0x18,0xA6},{0x18,0xB6},{0x18,0xC6},{0x18,0xD6},{0x18,0xE6},
			{0x18,0xF6}};

		unsigned char SFR_4u_1[16][2] = {{0x98,0x05},{0x98,0x15},{0x98,0x25},{0x98,0x35},{0x98,0x45},
			{0x98,0x55},{0x98,0x65},{0x98,0x75},{0x98,0x85},{0x98,0x95},
			{0x98,0xA5},{0x98,0xB5},{0x98,0xC5},{0x98,0xD5},{0x98,0xE5},
			{0x98,0xF5}};

		unsigned char SFR_5u_1[16][2] = {{0x18,0x05},{0x18,0x15},{0x18,0x25},{0x18,0x35},{0x18,0x45},
			{0x18,0x55},{0x18,0x65},{0x18,0x75},{0x18,0x85},{0x18,0x95},
			{0x18,0xA5},{0x18,0xB5},{0x18,0xC5},{0x18,0xD5},{0x18,0xE5},
			{0x18,0xF5}};

		unsigned char SFR_6u_11[16][2] = {{0x98,0x04},{0x98,0x14},{0x98,0x24},{0x98,0x34},{0x98,0x44},
			{0x98,0x54},{0x98,0x64},{0x98,0x74},{0x98,0x84},{0x98,0x94},
			{0x98,0xA4},{0x98,0xB4},{0x98,0xC4},{0x98,0xD4},{0x98,0xE4},
			{0x98,0xF4}};

		unsigned char SFR_7u_1[16][2] = {{0x18,0x04},{0x18,0x14},{0x18,0x24},{0x18,0x34},{0x18,0x44},
			{0x18,0x54},{0x18,0x64},{0x18,0x74},{0x18,0x84},{0x18,0x94},
			{0x18,0xA4},{0x18,0xB4},{0x18,0xC4},{0x18,0xD4},{0x18,0xE4},
			{0x18,0xF4}};
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
static void himax_ts_early_suspend(struct early_suspend *h);
static void himax_ts_late_resume(struct early_suspend *h);
#endif

#if ChangeIref1u
void himax_unlock_flash_Iref(struct himax_ts_data * ts)
{
    struct i2c_client * i2c_client = ts->client ;
    unsigned char cmd[5];

	/* unlock sequence start */
	cmd[0] = 0x01;cmd[1] = 0x00;cmd[2] = 0x06;
	i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]);

	cmd[0] = 0x03;cmd[1] = 0x00;cmd[2] = 0x00;
	i2c_smbus_write_i2c_block_data(i2c_client, 0x44, 3, &cmd[0]);

	cmd[0] = 0x00;cmd[1] = 0x00;cmd[2] = 0x3D;cmd[3] = 0x03;
	i2c_smbus_write_i2c_block_data(i2c_client, 0x45, 4, &cmd[0]);

	i2c_smbus_write_i2c_block_data(i2c_client, 0x4A, 0, &cmd[0]);
	mdelay(50);
	/* unlock sequence stop */
}

int ChangeIrefSPP(struct himax_ts_data * ts)
{
	//struct i2c_client * i2c_client = ts->client ;

	unsigned char i;
	//int readLen;
	unsigned char cmd[5];
	//unsigned char Iref[2] = {0x00,0x00};

	unsigned char spp_source[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,		//SPP
						  	        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};		//SPP

	unsigned char spp_target[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,		//SPP
 						  	        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};		//SPP
	unsigned char retry;
	unsigned char spp_ok;

	info_printk("entern Himax ChangeIrefSPP by zhuxinglong\n");

	//--------------------------------------------------------------------------
	//Inital
	//--------------------------------------------------------------------------
	cmd[0] = 0x42;
	cmd[1] = 0x02;
	if(i2c_master_send(ts->client, cmd, 2) < 0)
	{
		err_printk("entern Himax ChangeIrefSPP by zhuxinglong1\n");
		return -ENODEV;
	}
	udelay(10);

	cmd[0] = 0x81;
	if(i2c_master_send(ts->client, cmd, 1) < 0)
	{
		err_printk("entern Himax ChangeIrefSPP by zhuxinglong2\n");
		return -ENODEV;
	}
	mdelay(160);

	//--------------------------------------------------------------------------
	//read 16-byte SPP to spp_source
	//--------------------------------------------------------------------------
	cmd[0] = 0x43;
	cmd[1] = 0x01;
	cmd[2] = 0x00;
	cmd[3] = 0x1A;
	if(i2c_master_send(ts->client, cmd, 4) < 0)
	{
		err_printk("entern Himax ChangeIrefSPP by zhuxinglong3\n");
		return 0;
	}
	udelay(10);

	//4 words
	for (i = 0; i < 4; i++)
	{
		cmd[0] = 0x44;
		cmd[1] = i;			//word
		cmd[2] = 0x00;		//page
		cmd[3] = 0x00;		//sector
		if(i2c_master_send(ts->client, cmd, 4) < 0)
		{
			printk("entern Himax ChangeIrefSPP by zhuxinglong4\n");
			return 0;
		}
		udelay(10);

		cmd[0] = 0x46;
		if(i2c_master_send(ts->client, cmd, 1) < 0)
		{
			printk("entern Himax ChangeIrefSPP by zhuxinglong5\n");
			return 0;
		}
		udelay(10);

		cmd[0] = 0x59;
		if(i2c_master_send(ts->client, cmd, 1) < 0)
		{
			printk("entern Himax ChangeIrefSPP by zhuxinglong6\n");
			return 0;
		}
		mdelay(5);				//check this

		if(i2c_master_recv(ts->client, cmd, 4) < 0)
			return 0;
		udelay(10);

		//save data
		spp_source[4*i + 0] = cmd[0];
		spp_source[4*i + 1] = cmd[1];
		spp_source[4*i + 2] = cmd[2];
		spp_source[4*i + 3] = cmd[3];

		printk("entern Himax ChangeIrefSPP by zhuxinglong7 cmd0 = 0x%x\n",cmd[0]);
		printk("entern Himax ChangeIrefSPP by zhuxinglong7 cmd1 = 0x%x\n",cmd[1]);
		printk("entern Himax ChangeIrefSPP by zhuxinglong7 cmd2 = 0x%x\n",cmd[2]);
		printk("entern Himax ChangeIrefSPP by zhuxinglong7 cmd3 = 0x%x\n",cmd[3]);
		//printk("entern Himax ChangeIrefSPP by zhuxinglong7 cmd4 = 0x%x\n",cmd[4]);
		printk("entern Himax ChangeIrefSPP by zhuxinglong7\n");
	}

	//--------------------------------------------------------------------------
	//Search 3u Iref
	//--------------------------------------------------------------------------
	for (i = 0; i < 16; i++)
	{
		if(spp_source[0]==SFR_1u_1[i][0] && spp_source[1]==SFR_1u_1[i][1])
		{
			//found in 1uA
			return (1);				//OK
		}
	}

	spp_ok = 0;
	for (i = 0; i < 16; i++)
	{
		if(spp_source[0]==SFR_3u_11[i][0] && spp_source[1]==SFR_3u_11[i][1])
		{
			//found in 3uA
			spp_ok = 1;

			spp_source[0]= SFR_1u_1[i][0];
			spp_source[1]= SFR_1u_1[i][1];
			break;
		}
	}

	if (spp_ok == 0)
	{
		//no matched pair in SFR_1u_1 or SFR_3u_1
		return 0;
	}

	//--------------------------------------------------------------------------
	//write SPP (retry for 3 times if errors occur)
	//--------------------------------------------------------------------------
	for (retry = 0; retry < 3; retry++)
	{
		himax_unlock_flash_Iref(ts);

		//write 16-byte SPP
		cmd[0] = 0x43;
		cmd[1] = 0x01;
		cmd[2] = 0x00;
		cmd[3] = 0x1A;
		if(i2c_master_send(ts->client, cmd, 4) < 0)
			return 0;
		udelay(10);

		cmd[0] = 0x4A;
		if(i2c_master_send(ts->client, cmd, 1) < 0)
			return 0;
		udelay(10);

		for (i = 0; i < 4; i++)
		{
			cmd[0] = 0x44;
			cmd[1] = i;			//word
			cmd[2] = 0x00;		//page
			cmd[3] = 0x00;		//sector
			if(i2c_master_send(ts->client, cmd, 4) < 0)
				return 0;
			udelay(10);

			cmd[0] = 0x45;
			cmd[1] = spp_source[4 * i + 0];
			cmd[2] = spp_source[4 * i + 1];
			cmd[3] = spp_source[4 * i + 2];
			cmd[4] = spp_source[4 * i + 3];
			if(i2c_master_send(ts->client, cmd, 5) < 0)
				return 0;
			udelay(10);

			cmd[0] = 0x4B;		//write SPP
			if(i2c_master_send(ts->client, cmd, 1) < 0)
				return 0;
			udelay(10);
		}

		cmd[0] = 0x4C;
		if(i2c_master_send(ts->client, cmd, 1) < 0)
			return 0;
		mdelay(50);

		//read 16-byte SPP to spp_target
		cmd[0] = 0x43;
		cmd[1] = 0x01;
		cmd[2] = 0x00;
		cmd[3] = 0x1A;
		if(i2c_master_send(ts->client, cmd, 4) < 0)
		{
			printk("entern Himax ChangeIrefSPP by zhuxinglong3\n");
			return 0;
		}
		udelay(10);

		for (i = 0; i < 4; i++)
		{
			cmd[0] = 0x44;
			cmd[1] = i;			//word
			cmd[2] = 0x00;		//page
			cmd[3] = 0x00;		//sector
			if(i2c_master_send(ts->client, cmd, 4) < 0)
			{
				printk("entern Himax ChangeIrefSPP by zhuxinglong4\n");
				return 0;
			}
			udelay(10);

			cmd[0] = 0x46;
			if(i2c_master_send(ts->client, cmd, 1) < 0)
			{
				printk("entern Himax ChangeIrefSPP by zhuxinglong5\n");
				return 0;
			}
			udelay(10);

			cmd[0] = 0x59;
			if(i2c_master_send(ts->client, cmd, 1) < 0)
			{
				printk("entern Himax ChangeIrefSPP by zhuxinglong6\n");
				return 0;
			}
			mdelay(5);		//check this

			if(i2c_master_recv(ts->client, cmd, 4) < 0)
				return 0;
			udelay(10);

			spp_target[4*i + 0] = cmd[0];
			spp_target[4*i + 1] = cmd[1];
			spp_target[4*i + 2] = cmd[2];
			spp_target[4*i + 3] = cmd[3];

			printk("entern Himax ChangeIrefSPP by zhuxinglong9 cmd0 = 0x%x\n",cmd[0]);
			printk("entern Himax ChangeIrefSPP by zhuxinglong9 cmd1 = 0x%x\n",cmd[1]);
			printk("entern Himax ChangeIrefSPP by zhuxinglong9 cmd2 = 0x%x\n",cmd[2]);
			printk("entern Himax ChangeIrefSPP by zhuxinglong9 cmd3 = 0x%x\n",cmd[3]);
			//printk("entern Himax ChangeIrefSPP by zhuxinglong9 cmd4 = 0x%x\n",cmd[4]);
			printk("entern Himax ChangeIrefSPP by zhuxinglong9\n");
		}

		//compare source and target
		spp_ok = 1;
		for (i = 0; i < 16; i++)
		{
			if (spp_target[i] != spp_source[i])
			{
				spp_ok = 0;
			}
		}

		if (spp_ok == 1)
		{
			printk("entern Himax ChangeIrefSPP by zhuxinglong10\n");
			return 1;	//Modify Success
		}

		//error --> reset SFR
		cmd[0] = 0x43;
		cmd[1] = 0x01;
		cmd[2] = 0x00;
		cmd[3] = 0x06;
		if(i2c_master_send(ts->client, cmd, 4) < 0)
			return 0;
		udelay(10);

		//write 16-byte SFR
		for (i = 0; i < 4; i++)
		{
			cmd[0] = 0x44;
			cmd[1] = i;			//word
			cmd[2] = 0x00;		//page
			cmd[3] = 0x00;		//sector
			if(i2c_master_send(ts->client, cmd, 4) < 0)
				return 0;
			udelay(10);

			cmd[0] = 0x45;
			cmd[1] = spp_source[4 * i + 0];
			cmd[2] = spp_source[4 * i + 1];
			cmd[3] = spp_source[4 * i + 2];
			cmd[4] = spp_source[4 * i + 3];
			if(i2c_master_send(ts->client, cmd, 5) < 0)
				return 0;
			udelay(10);

			cmd[0] = 0x4A;		//write SFR
			if(i2c_master_send(ts->client, cmd, 1) < 0)
				return 0;
			udelay(10);
		}

    }	//retry

	info_printk("entern Himax ChangeIrefSPP by zhuxinglong8\n");
	return 1;			//No 3u Iref setting


}
#endif
static int himax_init_panel(struct himax_ts_data *ts)
{
    return 0;
}

static inline int disable_device_irq(struct himax_ts_data *ts,int sync)
{

    if(sync){
     disable_irq(ts->client->irq);
    }else {
     disable_irq_nosync(ts->client->irq);
    }
    ts->irq_count-- ;

    return 0;
}

static inline int enable_device_irq(struct himax_ts_data *ts)
{
    enable_irq(ts->client->irq);
    ts->irq_count++;
    return 0;
}
#ifdef __USB_CHARGER_STATUS__
extern int  get_usb_charger_status(void)	;
static int charger_notifier_call (struct himax_ts_data *ts,int status)
{
    char buf[2] ;
    int ret ;
    buf[0] = 0x90 ;
    buf[1] = !!status ;

    if(ts->charger_status != status){
        info_printk("charger status change charger[%d],current[%d]\n",
            ts->charger_status,status);
        ret = i2c_master_send(ts->client, buf,2) ;
        if(ret<0){
            err_printk("i2c send error[%d]\n",ret);
            return -EINVAL ;
        }
        ts->charger_status = !!status ;
    }

    return 0 ;
}
#endif
//#define __SELF_REPORT__
#ifdef __SELF_REPORT__
static void self_report_function(struct work_struct * work)
{
    int i;
    int ret;
    int finger_number = 0 ;
    unsigned long x=0 ,y=0;
    unsigned long major = 0;
    unsigned long width = 0 ;
    int checksum = 0;
    uint32_t * buf_32 ;
    static int report_key = 0 ;
    struct i2c_msg msg[2];
    uint8_t start_reg;
    uint32_t buf[6];
    uint8_t * buf_8 ;
    struct himax_ts_data *ts = container_of(work, struct himax_ts_data, work);
    major = width ;
    finger_number = finger_number ;
    //read all events
    start_reg = 0x86;

    msg[0].addr = ts->client->addr;
    msg[0].flags = 0;
    msg[0].len = 1;
    msg[0].buf = &start_reg;

    msg[1].addr = ts->client->addr;
    msg[1].flags = I2C_M_RD;
    //msg[1].len = 4 * TOUCH_MAX_NUMBER;
	msg[1].len =  24;
    msg[1].buf = (__u8 *)&buf[0];

    ret = i2c_transfer(ts->client->adapter, msg, 2);
    if (ret < 0)
	{
        err_printk("i2c transfer error ret[%d]\n",ret);
        ts->err_count++ ;
        if(ts->err_count%2==0){
            himax_ts_reset(dev_get_platdata(&ts->client->dev));
            himax_ts_poweron(ts);
            info_printk(0,"himax tp reset success,err_count[%d]\n",ts->err_count);
        }
        goto OUT ;
	}
    buf_8 = (uint8_t *)buf ;
    for (i = 0 ; i < 22; i++)
    {
        checksum += buf_8[i];
    }

    i = buf_8[22];
    i <<= 8;
    i |= buf_8[23];
    if (checksum != i)
    {
        err_printk("i2c checksum error ret[%d]\n",ret);
        buf_32 = (uint32_t *)buf ;
        for(i=0;i < 6;i++){
           info_printk(0,"layer[%d]:data[%x]\n",i,buf_32[i]);
        }
        goto OUT ;
    }
    for(i=0; i<TOUCH_MAX_NUMBER; i++)
    {
        if(buf[i]!=TOUCH_DATA_INVALID) {
            uint8_t * data  = (uint8_t *)&buf[i] ;
            x = (data[0]<<8) | data[1] ;
            y = (data[2]<<8) | data[3] ;
            ret = x ;
            x = y ;
            y = ret ;
            dbg_printk("x=%ld,y=%ld\n",x,y);
            if(x > HIMAX_X_MAX || y > HIMAX_Y_MAX){
            ts->points[i].cur_status = TOUCH_UP ;
            }else {
            major = HIMAX_MAJOR_MAX;
            width = HIMAX_WIDTH_MAX/2;
            ts->points[i].cur_status = TOUCH_DOWN ;
            finger_number++ ;
            report_key = 1 ;
            }
        }else{
            ts->points[i].cur_status = TOUCH_UP ;
        }

        //report key
        if(ts->points[i].cur_status==TOUCH_DOWN){
         input_report_abs(ts->input_dev,ABS_MT_TOUCH_MAJOR,major) ;
         input_report_abs(ts->input_dev,ABS_MT_WIDTH_MAJOR,width) ;
         input_report_abs(ts->input_dev,ABS_MT_POSITION_X,x) ;
         input_report_abs(ts->input_dev,ABS_MT_POSITION_Y,y) ;
         input_report_abs(ts->input_dev,ABS_MT_TRACKING_ID,i);
         input_mt_sync(ts->input_dev);
        }else if(ts->points[i].cur_status==TOUCH_UP&&ts->points[i].pre_status==TOUCH_UP){
            //nothing doing....
			  continue;
        }
        ts->points[i].pre_status = ts->points[i].cur_status ;
    }

  if(finger_number==0&&(!report_key)){
    info_printk(0,"no key to report finger [%d],key[%d]\n",finger_number,report_key);
    goto OUT ;
  }
    input_report_key(ts->input_dev,BTN_TOUCH,!!finger_number);
    input_sync(ts->input_dev);
 if(!finger_number){
      report_key = 0 ;
 }
OUT:
    if (ts->use_irq)
    	enable_device_irq(ts);

    return ;

}
#else

static inline void is_project_C8680GP(struct himax_ts_data * ts,unsigned long * x,unsigned long * y)
{

    if(*x >=C8680GP_X_MIN&&*x<=C8680GP_X_MAX&&
        *y>=C8680GP_Y_MIN&&(get_project(ts)==C8680GP_VAL)){
        *x = C8680GP_X_POINT ;
        *y = C8680GP_Y_POINT ;
    }

}

 static void report_function(struct work_struct *work)
{

    int i;
	int ret;
    int checksum = 0;

	unsigned long x, y;
    static int report_key = 0 ;
    static int protocol = 0 ;
    static int packet_size = 0 ;
    uint8_t * buf ;
	struct i2c_msg msg[2];
	uint8_t start_reg;

	uint32_t finger_num;
    uint32_t * buf_32 ;


	struct himax_ts_data *ts = container_of(work, struct himax_ts_data, work);
    if(!ts->point_num){
       err_printk(" finger number is not detect %d\n",ts->point_num);
       goto OUT ;
    }

    if(!protocol){
        protocol++ ;

        if(ts->point_num==4)
         packet_size = FINGER_FOUR_PACKET_SIZE ;
        else if(ts->point_num==5)
         packet_size = FINGER_FIVE_PACKET_SIZE ;

        ts->finger_buf = kmalloc(packet_size,GFP_KERNEL) ;
        info_printk("finger number [%d],packet_size[%d]\n",ts->point_num,
            packet_size);
        if(!ts->finger_buf){
            protocol-- ;
            goto OUT ;
        }

    }
#ifdef __USB_CHARGER_STATUS__
        charger_notifier_call(ts,!!get_usb_charger_status());
#endif
	start_reg = 0x86;
    buf = (uint8_t *)ts->finger_buf ;

	msg[0].addr = ts->client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &start_reg;

	msg[1].addr = ts->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len =  packet_size;
	msg[1].buf = &buf[0];

	ret = i2c_transfer(ts->client->adapter, msg, 2);
    if (ret < 0)
	{
        err_printk("i2c transfer error ret[%d]\n",ret);
        ts->err_count++ ;
        if(ts->err_count%2==0){
            himax_ts_reset(dev_get_platdata(&ts->client->dev));
            himax_ts_poweron(ts);
        }
        goto OUT ;
	}

    //MID:20121122_01
    for (i = 0 ; i < packet_size-2; i++)
    {
        checksum += buf[i];
    }

    i = buf[packet_size-2];
    i <<= 8;
    i |= buf[packet_size-1];
    if (checksum != i)
    {
        err_printk("i2c checksum error ret[%d]\n",ret);
        buf_32 = (uint32_t *)buf ;
        for(i=0 ;i < 6;i++){
           info_printk("layer[%d]:data[%x]\n",i,buf_32[i]);
        }
        goto OUT ;
    }

	finger_num=buf[packet_size - 4]&0x0F;
    buf_32 = (uint32_t *)buf ;
   // info_printk("finger number [%x]\n",finger_num);

    #if 0
    for(i=0;i<6;i++)
        dbg_printk("layer[%d]:data[%x]\n",i,buf_32[i]);
    #endif

    //MID:20121122_01
    if((finger_num>0)&&(finger_num<=5))
    {
	    for(i=0; i<ts->point_num; i++)
	    {

           if((buf[i*4] == 0xff && buf[i*4+1] == 0xff && buf[i*4+2] == 0xff  &&buf[i*4+3] == 0xff)){
                  continue ;
	        }else{
                x = buf[i*4]<< 8   |( buf[i*4+1] ) ;
                y = buf[i*4+2]<< 8 | (buf[i*4+3]);
                ret = x;
                x   = y ;
                y   = ret ;

             is_project_C8680GP(ts,&x,&y) ;

             //info_printk("x=%ld,y=%ld\n",x,y);
			 if(x > HIMAX_X_MAX  ||  y > HIMAX_Y_MAX){
                    continue ;
			  }else{
				 input_report_key(ts->input_dev, BTN_TOUCH, 1);
				 input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 1); //Finger Size
				 input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
				 input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
				 input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 5); //Touch Size
				 input_mt_sync(ts->input_dev);
				 report_key = 1 ;
			  }
		}
	 }
        if(report_key){
          input_sync(ts->input_dev);
        }
        goto OUT ;

    }else if(finger_num==0x0f&&report_key){
        dbg_printk("all finger leave\n");
        //input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0); //Finger Size
       // input_report_abs(ts->input_dev, ABS_MT_POSITION_X,0);
       // input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, 0);
       // input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0); //Touch Size
        input_report_key(ts->input_dev, BTN_TOUCH, 0);
        input_mt_sync(ts->input_dev);
        input_sync(ts->input_dev);
        report_key = 0 ;
	}else {
        err_printk("himax tp data is error finger [%d],report_key[%d] \n",finger_num,report_key);
         #if 0
         for(i=0;i<6;i++)
         info_printk("layer[%d]:data[%x]\n",i,buf_32[i]);
         #endif
	}

OUT:
    if (ts->use_irq)
	  enable_device_irq(ts);

}
#endif
static void himax_ts_work_func(struct work_struct *work)
{
    #ifdef __SELF_REPORT__
        self_report_function(work);
    #else
        report_function(work);
    #endif
}

static enum hrtimer_restart himax_ts_timer_func(struct hrtimer *timer)
{
    struct himax_ts_data *ts = container_of(timer, struct himax_ts_data, timer);
    //printk("himax_ts_timer_func\n");

    queue_work(himax_wq, &ts->work);
    if(!ts->interval)
        ts->interval = TIMER_DEFAULT_VALUE;
    hrtimer_start(&ts->timer, ktime_set(0, ts->interval*TIMER_BASE), HRTIMER_MODE_REL);
    return HRTIMER_NORESTART;
}

static irqreturn_t himax_ts_irq_handler(int irq, void *dev_id)
{
    struct himax_ts_data *ts = dev_id;
    int ret ;

    ret = queue_work(himax_wq, &ts->work);
    if(ret==0){
        info_printk("himax work is already in workqueue\n");
        return IRQ_HANDLED;
    }else if(ret==1) {
        disable_device_irq(ts,0);
    }else {
        info_printk("irq queue error %d\n",ret);
    }

    return IRQ_HANDLED;
}

static inline void himax_ts_reset(struct himax_platform_data *pdata)
{
    gpio_set_value(pdata->gpio_rst, GPIO_OUTPUT_HI);
    msleep(5); //5ms
    gpio_set_value(pdata->gpio_rst, GPIO_OUTPUT_LO);
    msleep(10);//100us
    gpio_set_value(pdata->gpio_rst, GPIO_OUTPUT_HI);
    msleep(10);
}

static int himax_ts_poweron(struct himax_ts_data *ts)
{
    uint8_t buf0[6];
    int ret = 0;

	buf0[0] = 0x42;
    buf0[1] = 0x02;
    ret = i2c_master_send(ts->client, buf0, 2);//Reload Disable
    if(ret < 0) {
        err_printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
        return ret ;
    }
    udelay(100);
    buf0[0] = 0x35;
    buf0[1] = 0x02;
    ret = i2c_master_send(ts->client, buf0, 2);//enable MCU
    if(ret < 0) {
        err_printk("i2c_master_send failed addr = 0x%x\n",ts->client->addr);
        return ret ;
    }

	udelay(100);
    buf0[0] = 0x36;
    buf0[1] = 0x0F;
    buf0[2] = 0x53;
    ret = i2c_master_send(ts->client, buf0, 3);//enable flash
    if(ret < 0) {
        err_printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
        return ret ;
    }
    udelay(100);

    buf0[0] = 0xDD;
    buf0[1] = 0x06;
    buf0[2] = 0x02;
    ret = i2c_master_send(ts->client, buf0, 3);//prefetch
    if(ret < 0) {
        err_printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
        return ret ;
    }

    buf0[0] = 0x76;
    buf0[1] = 0x01 ;
    buf0[2] = 0x1B ;
    ret = i2c_master_send(ts->client, buf0, 3);//set output
    if(ret < 0) {
        err_printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
        return ret ;
    }


    buf0[0] = 0xE9;
    buf0[1] = 0x00;
    ret = i2c_master_send(ts->client, buf0, 2);//set output
    if(ret < 0) {
        err_printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
        return ret ;
    }


     //add for 4/5 point switch
   if(ts->cmd_92==true){
      buf0[0] = 0x92;
      buf0[1] = 0x01;        //noneZero :five finger ;Zero :four finger
      ret = i2c_master_send(ts->client, buf0, 2);//set output
      if(ret < 0) {
       err_printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
       return ret ;
      }
   }

    buf0[0] = 0x83;
    ret = i2c_master_send(ts->client, buf0, 1);//sense on
    if(ret < 0) {
        err_printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
        return ret ;
    }

    msleep(50);
   buf0[0] = 0x81;
    ret = i2c_master_send(ts->client, buf0, 1);//sleep out
    if(ret < 0) {
        err_printk("i2c_master_send failed addr = 0x%x\n",ts->client->addr);
        return ret ;
    }
	else
	{
		info_printk("OK addr = 0x%x\n",ts->client->addr);
	}
    ts->charger_status = 0 ;
    msleep(120); //120ms


    return 0;
}

#if 0
static int himax_ts_read_flash(struct himax_ts_data *ts)
{
    uint8_t buf0[6];
    uint16_t word_address, Index_word, Index_page,Index_sector;
    int ret = 0;
    struct i2c_msg msg[2];
    uint8_t start_reg;
    uint32_t buf[6];
    int i;

    start_reg = 0x59;

   buf0[0] = 0x81;
    ret = i2c_master_send(ts->client, buf0, 1);//sleep out
    if(ret < 0) {
        printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
    }
	else
	{
		printk(KERN_ERR "OK addr = 0x%x\n",ts->client->addr);
	}
    msleep(120); //120ms

   buf0[0] = 0x43;
   buf0[1] = 0x01;
   buf0[2] = 0x00;
   buf0[3] = 0x02;
    ret = i2c_master_send(ts->client, buf0, 4);//sleep out
    if(ret < 0) {
        printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
    }
    udelay(10);

    word_address = 0x400;
    Index_word = word_address & 0x001F;
    Index_page = (word_address & 0x03E0) >> 5;
    Index_sector = (word_address & 0x1C00) >> 10;

   buf0[0] = 0x44;
   buf0[1] = Index_word;
   buf0[2] = Index_page;
   buf0[3] = Index_sector;
    ret = i2c_master_send(ts->client, buf0, 4);//sleep out
    if(ret < 0) {
        printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
    }
    udelay(5);
    buf0[0] = 0x46;
    ret = i2c_master_send(ts->client, buf0, 1);//sleep out
    if(ret < 0) {
        printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
    }
    udelay(5);

    msg[0].addr = ts->client->addr;
    msg[0].flags = 0;
    msg[0].len = 1;
    msg[0].buf = &start_reg;

    msg[1].addr = ts->client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].len = 32;
    msg[1].buf =(__u8*)&buf[0];
      buf[0] = 0;
    ret = i2c_transfer(ts->client->adapter, msg, 2);
	   	    if(ret < 0) {
	        printk(KERN_ERR "i2c_master_receive failed\n");
	    }
	for(i=0;i<4;i++)
	printk("     buf[%d] = 0x%x",i,buf[i]);

    udelay(5);
//I2C_start(slave_addr);
//I2C_CMD(0x43);
//I2C_PAs(0x00);
//I2C_PAs(0x00);
//I2C_PAs(0x02);
//I2C_stop();
//delay(10);
   buf0[0] = 0x43;
   buf0[1] = 0x00;
   buf0[2] = 0x00;
   buf0[3] = 0x02;
    ret = i2c_master_recv(ts->client, buf0, 4);//sleep out
    if(ret < 0) {
        printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
    }
    udelay(10);

    return 0;
}
#endif
static int himax_ts_sleep(struct himax_ts_data *ts)
{
    uint8_t buf0[6];
    int ret = 0;

   buf0[0] = 0x82;
   info_printk("\n");
    ret = i2c_master_send(ts->client, buf0, 1);//sense off
    if(ret < 0) {
        err_printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
        return ret ;
    }
    msleep(120); //120ms

   buf0[0] = 0x80;
    ret = i2c_master_send(ts->client, buf0, 1); //sleep in
    if(ret < 0) {
        err_printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
        return ret ;
    }
    msleep(120); //120ms


   buf0[0] = 0xD7;
   buf0[0] = 0x01;//DP_STB = 1
    ret = i2c_master_send(ts->client, buf0, 2); //command SETDEEPSTB
    if(ret < 0) {
        err_printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
        return ret ;
    }
    ts->himax_state = HIMAX_STATE_DEEPSLEEP ;
    info_printk("himax TP entry deepsleep mode\n");
    //now in deep sleep mode
    return 0;
}



static int himax_ts_active(struct himax_ts_data *ts)
{

    int ret = 0;

#if 0
    uint8_t buf0[6];
   buf0[0] = 0xD7;
   buf0[1] = 0x00;//DP_STB = 1
    info_printk(0,"\n");
    ret = i2c_master_send(ts->client, buf0, 2);//command SETDEEPSTB
    if(ret < 0) {
        //printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
        err_printk("i2c_master_send failed\n");
        return ret ;
    }
    //msleep(100); //120ms
    udelay(120);

   buf0[0] = 0x42;
   buf0[0] = 0x02;//DP_STB = 1
    ret = i2c_master_send(ts->client, buf0, 2); //turn on reload disable
    if(ret < 0) {
        //printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts->client->addr);
        err_printk("i2c master send failed \n");
        return ret ;
    }

    udelay(100); //120ms
#endif
    himax_ts_reset(dev_get_platdata(&ts->client->dev));
    info_printk("reset device success\n");
    ret = himax_ts_poweron(ts);
    if(ret < 0 ){
        ts->himax_state = HIMAX_STATE_DEEPSLEEP ;
        return ret ;
    }
    ts->himax_state = HIMAX_STATE_ACTIVE ;
    info_printk("himax entry active mode \n");
    return 0;
}

static ssize_t himax_mode_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
    struct himax_ts_data * ts  ;
    char * mode ;
    ts = dev_get_drvdata(dev) ;

    if(ts->use_irq){
        mode = "interrupt" ;
    }else {
        mode = "timer" ;
    }

	return snprintf(buf, PAGE_SIZE,"%s\n",mode);
}

static ssize_t himax_mode_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf,size_t count)
{
    struct himax_ts_data * ts  ;
    int ret ;
    ts = dev_get_drvdata(dev) ;

    if(!strncmp(buf,"timer",5)){
        if(!ts->use_irq)
            return count ;
        if(ts->himax_state!= HIMAX_STATE_DEEPSLEEP)
            disable_device_irq(ts,1);

        ret = cancel_work_sync(&ts->work);
        if(ret && ts->use_irq){
          info_printk("work is already on workqueue\n");
          enable_device_irq(ts);
        }

        hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        ts->timer.function = himax_ts_timer_func;
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
        ts->use_irq = 0 ;

    }else if(!strncmp(buf,"interrupt",strlen("interrupt"))){
        if(ts->use_irq)
            return count ;

        hrtimer_cancel(&ts->timer);
        ts->interval = 0 ;
        if(ts->himax_state!=HIMAX_STATE_DEEPSLEEP)
           enable_device_irq(ts);
        ts->use_irq = 1 ;

    }else {
        info_printk("mode %s not suports\n",buf);
    }

	return count;
}


static ssize_t himax_irq_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
    struct himax_ts_data * ts = dev_get_drvdata(dev);
    return snprintf(buf,PAGE_SIZE,"irq:%d err:%d\n",ts->irq_count,ts->err_count);
}

static ssize_t himax_irq_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf,size_t count)
{
    int enable = 0 ;
    struct himax_ts_data * ts = dev_get_drvdata(dev);
    enable = simple_strtoul(buf,NULL,10);
    if(enable==1){
        enable_device_irq(ts);
    }else{
        disable_device_irq(ts,1);
    }
    return count ;
}

static ssize_t himax_state_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
    char * state ;
    struct himax_ts_data * ts = dev_get_drvdata(dev) ;
    if(ts->himax_state==HIMAX_STATE_ACTIVE){
        state = "active" ;
    }else if(ts->himax_state==HIMAX_STATE_DEEPSLEEP){
        state = "deepsleep" ;
    }else {
        state = "idle" ;
    }
    return snprintf(buf,PAGE_SIZE,"%s\n",state);
}

static ssize_t himax_state_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf,size_t count)
{

    int ret ;
    struct himax_ts_data * ts = dev_get_drvdata(dev) ;
    if(!strncmp(buf,"active",6)){
        if(ts->himax_state == HIMAX_STATE_ACTIVE)
            return count ;
        himax_ts_active(ts);
        if (ts->use_irq) {
           enable_device_irq(ts);
        }else
           hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
        info_printk("in active mode\n");
    }else if(!strncmp(buf,"deepsleep",strlen("deepsleep"))){
        if(ts->himax_state==HIMAX_STATE_DEEPSLEEP)
            return count ;
        if (ts->use_irq)
            disable_device_irq(ts,1);
        else
            hrtimer_cancel(&ts->timer);

        ret = cancel_work_sync(&ts->work);
        if(ret && ts->use_irq)
             enable_device_irq(ts);
        himax_ts_sleep(ts);
        info_printk("in deepsleep mode\n");
    }else {
        info_printk("state is not support\n");
    }

    return count ;
}

static ssize_t himax_value_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
    int gpio_int,gpio_rst;
    struct himax_ts_data * ts = dev_get_drvdata(dev) ;
    struct himax_platform_data * pdata = dev_get_platdata(&ts->client->dev);

    gpio_int = gpio_get_value(pdata->gpio_int);
    gpio_rst = gpio_get_value(pdata->gpio_rst);

    return snprintf(buf,PAGE_SIZE,"int:%d rst:%d\n",gpio_int,gpio_rst);
}

static ssize_t himax_timer_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{

    struct himax_ts_data * ts = dev_get_drvdata(dev) ;

    return snprintf(buf,PAGE_SIZE,"%ld\n",ts->interval);
}
static ssize_t himax_fw_ver_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{

    struct himax_ts_data * ts = dev_get_drvdata(dev) ;
    int count = 0 ;
    count += snprintf(buf,PAGE_SIZE,"fw_name:%s:%d\n",ts->fw_name,ts->upgrade);
    count += snprintf(buf+count,PAGE_SIZE,"fw_ver:%s:%d\n",ts->fw_ver,ts->point_num);
    return count ;
}
static ssize_t himax_driver_show(struct device *dev,
                       struct device_attribute *attr,
                       char *buf)
 {
        int i ;
        int ret = 0;
        for(i=0;i< ARRAY_SIZE(driver_ver);i++){
          ret += snprintf(buf+ret,PAGE_SIZE,"ver%2d:%s\n",i,driver_ver[i]);
        }
    return ret ;
 }

static ssize_t himax_timer_store(struct device *dev,
                struct device_attribute *atrr,
                const char * buf,size_t count)
{

    struct himax_ts_data *ts = dev_get_drvdata(dev);

    ts->interval= simple_strtoul(buf,NULL,10);
    if(ts->interval > TIMER_MAX_VALUE )
        ts->interval = TIMER_DEFAULT_VALUE;

    return count ;


}

static ssize_t himax_reset_store(struct device *dev,
                struct device_attribute *atrr,
                const char * buf,size_t count)
{

    struct himax_ts_data *ts = dev_get_drvdata(dev);
    struct himax_platform_data * pdata = dev_get_platdata(&ts->client->dev);
    int reset = 0 ;
    reset = simple_strtoul(buf,NULL,10);
    if(reset==1){
        himax_ts_reset(pdata);
        himax_ts_poweron(ts) ;
        info_printk("reset himax tp success\n");
    }
    return count ;
}


struct device_attribute himax_attr[] = {
    __ATTR(mode, HIMAX_RW_ATTR, himax_mode_show,himax_mode_store),
    __ATTR(state,HIMAX_RW_ATTR, himax_state_show,himax_state_store),
    __ATTR(irq,  HIMAX_RW_ATTR, himax_irq_show,himax_irq_store),
    __ATTR(gpio, HIMAX_RO_ATTR, himax_value_show,NULL),
    __ATTR(fw, HIMAX_RO_ATTR, himax_fw_ver_show,NULL),
    __ATTR(version, HIMAX_RO_ATTR, himax_driver_show,NULL),
    __ATTR(reset,  HIMAX_WO_ATTR, NULL,himax_reset_store),
    __ATTR(timer,  HIMAX_RW_ATTR, himax_timer_show,himax_timer_store)
};



static void create_sysfs_file(struct himax_ts_data * ts)
{
    int i ,ret ;
    ts->dev.init_name ="himax_ts" ;
    ret = dev_set_drvdata(&ts->dev, ts) ;
    ret = device_register(&ts->dev);
    for(i=0;i<ARRAY_SIZE(himax_attr);i++)
    {
        ret = sysfs_create_file(&ts->dev.kobj,&himax_attr[i].attr);
        if(ret < 0 ){
            err_printk("create %s error\n",himax_attr[i].attr.name);
            sysfs_remove_file(&ts->dev.kobj,&himax_attr[i].attr);
        }
    }
    ret=sysfs_create_link(&ts->dev.kobj,&ts->input_dev->dev.kobj,dev_name(&ts->input_dev->dev));
    return  ;
}

#ifdef CONFIG_HX8526A_FW_UPDATE
void himax_ManualMode(struct i2c_client * i2c_client,int enter)
{
	unsigned char cmd[2];
	cmd[0] = enter;
	i2c_smbus_write_i2c_block_data(i2c_client, 0x42, 1, &cmd[0]);
}

void himax_FlashMode(struct i2c_client * i2c_client,int enter)
{
	unsigned char cmd[2];
	cmd[0] = enter;
	i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 1, &cmd[0]);
}



//1:need upgrade
u8 himax_read_FW_ver(struct himax_ts_data * ts) //OK
{
	unsigned char FW_VER_MAJ_FLASH_buff[FW_VER_MAJ_FLASH_LENG*4 ];
	unsigned char FW_VER_MIN_FLASH_buff[FW_VER_MIN_FLASH_LENG*4 ];
	unsigned char CFG_VER_MAJ_FLASH_buff[CFG_VER_MAJ_FLASH_LENG*4];
	unsigned char CFG_VER_MIN_FLASH_buff[CFG_VER_MIN_FLASH_LENG*4];
    unsigned char * FW ;
    int ret = 0 ;
	unsigned int i;
	unsigned char cmd[5];


    if(get_project(ts)==1)
        FW = HIMAX_FW_C8680 ;
    else if (get_project(ts)==2)
        FW = HIMAX_FW_C8680QM ;
    else if (get_project(ts)==3)
        FW = HIMAX_FW_C8680GP ;
    else{
        err_printk("project name not detect!!!!\n");
        return 0 ;
    }



	//Himax: Power On Flow
	cmd[0] = 0x42;
	cmd[1] = 0x02;
	ret = i2c_master_send(ts->client, cmd, 2);//himax_i2c_master_write(slave_address, 2, cmd);
    if( ret < 0 ){
        err_printk(" read firmware version error\n");
        return 0 ;
    }
	udelay(10);

	cmd[0] = 0x36;
	cmd[1] = 0x0F;
	cmd[2] = 0x53;
	i2c_master_send(ts->client, cmd, 3);//himax_i2c_master_write(slave_address, 3, cmd);
	udelay(10);

	//Himax: Check Flash Pre-Patch
	cmd[0] = 0xDD;
	cmd[1] = 0x06; //Himax: Modify by case
	cmd[2] = 0x02;
	i2c_master_send(ts->client, cmd, 3);//himax_i2c_master_write(slave_address, 3, cmd);
	udelay(10);

	cmd[0] = 0x81;
	i2c_master_send(ts->client, cmd, 1);//himax_i2c_master_write(slave_address, 1, cmd);
	udelay(10);

	//Himax: Flash Read Enable
	cmd[0] = 0x43;
	cmd[1] = 0x01;
	cmd[2] = 0x00;
	cmd[3] = 0x02;
	i2c_master_send(ts->client, cmd, 4);//himax_i2c_master_write(slave_address, 4, cmd);
	udelay(10);

	//Himax: Read FW Major Version
	if (hx8526_read_flash(ts,FW_VER_MAJ_FLASH_buff, FW_VER_MAJ_FLASH_ADDR, FW_VER_MAJ_FLASH_LENG) < 0)	goto err;
	//Himax: Read FW Minor Version
	if (hx8526_read_flash(ts,FW_VER_MIN_FLASH_buff, FW_VER_MIN_FLASH_ADDR, FW_VER_MIN_FLASH_LENG) < 0)	goto err;
	//Himax: Read CFG Major Version
	if (hx8526_read_flash(ts,CFG_VER_MAJ_FLASH_buff, CFG_VER_MAJ_FLASH_ADDR, CFG_VER_MAJ_FLASH_LENG) < 0)	goto err;
	//Himax: Read CFG Minor Version
	if (hx8526_read_flash(ts,CFG_VER_MIN_FLASH_buff, CFG_VER_MIN_FLASH_ADDR, CFG_VER_MIN_FLASH_LENG) < 0)	goto err;

	//if (hx8526_read_flash(ts,CFG_VER_MIN_FLASH_buff, CFG_VER_MIN_FLASH_ADDR, CFG_VER_MIN_FLASH_LENG) < 0)	goto err;



	//Himax: Check FW Major Version
	for (i = 0; i < FW_VER_MAJ_FLASH_LENG * 4; i++)
	{
          info_printk("%d:flash major [%x],fw major[%x]\n",i,FW_VER_MAJ_FLASH_buff[i],
            *(FW + (FW_VER_MAJ_FLASH_ADDR * 4) + i));
		 if (FW_VER_MAJ_FLASH_buff[i] != *(FW + (FW_VER_MAJ_FLASH_ADDR * 4) + i))
			return 1;
	}

	//Himax: Read FW Minor Version
	for (i = 0; i < FW_VER_MIN_FLASH_LENG * 4; i++)
	{

        info_printk("%d:flash minor [%x],fw minor[%x]\n",i,FW_VER_MIN_FLASH_buff[i],
            *(FW + (FW_VER_MIN_FLASH_ADDR * 4) + i));

		if (FW_VER_MIN_FLASH_buff[i] != *(FW + (FW_VER_MIN_FLASH_ADDR * 4) + i))
			return 1;
	}

	//Himax: Read CFG Major Version
	for (i = 0; i < CFG_VER_MAJ_FLASH_LENG * 4; i++)
	{

    	info_printk("%d:cfg major [%x],fw major[%x]\n",i,CFG_VER_MAJ_FLASH_buff[i],
            *(FW + (CFG_VER_MAJ_FLASH_ADDR * 4) + i));

		if (CFG_VER_MAJ_FLASH_buff[i] != *(FW + (CFG_VER_MAJ_FLASH_ADDR * 4) + i))
			return 1;
	}

	//Himax: Read CFG Minor Version
	for (i = 0; i < CFG_VER_MIN_FLASH_LENG * 4; i++)
	{

        info_printk("%d:cfg min [%x],fw major[%x]\n",i,CFG_VER_MIN_FLASH_buff[i],
                    *(FW + (CFG_VER_MIN_FLASH_ADDR * 4) + i));

		if (CFG_VER_MIN_FLASH_buff[i] != *(FW + (CFG_VER_MIN_FLASH_ADDR * 4) + i))
			return 1;
	}

	//Himax: Flash Read Disable
	cmd[0] = 0x43;
	cmd[1] = 0x00;
	cmd[2] = 0x00;
	cmd[3] = 0x02;
	i2c_master_send(ts->client, cmd, 4);//himax_i2c_master_write(slave_address, 4, cmd);
	udelay(10);

	return 0;

err:
	printk("Himax TP: FW update error exit\n");
	return 0;
}


void himax_unlock_flash(struct himax_ts_data * ts)
{
    struct i2c_client * i2c_client = ts->client ;
    unsigned char cmd[5];

	/* unlock sequence start */
	cmd[0] = 0x01;cmd[1] = 0x00;cmd[2] = 0x06;
	i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]);

	cmd[0] = 0x03;cmd[1] = 0x00;cmd[2] = 0x00;
	i2c_smbus_write_i2c_block_data(i2c_client, 0x44, 3, &cmd[0]);

	cmd[0] = 0x00;cmd[1] = 0x00;cmd[2] = 0x3D;cmd[3] = 0x03;
	i2c_smbus_write_i2c_block_data(i2c_client, 0x45, 4, &cmd[0]);

	i2c_smbus_write_i2c_block_data(i2c_client, 0x4A, 0, &cmd[0]);
	mdelay(50);
	/* unlock sequence stop */
}

static int himax_modifyIref(struct himax_ts_data * ts)
{
    struct i2c_client * i2c_client = ts->client ;

	unsigned char i;
	unsigned char cmd[5];
	unsigned char Iref[2] = {0x00,0x00};

	cmd[0] = 0x01;cmd[1] = 0x00;cmd[2] = 0x08;
	if((i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]))< 0)
		return 0;

	cmd[0] = 0x00;cmd[1] = 0x00;cmd[2] = 0x00;
	if((i2c_smbus_write_i2c_block_data(i2c_client, 0x44, 3, &cmd[0]))< 0)
		return 0;

	if((i2c_smbus_write_i2c_block_data(i2c_client, 0x46, 0, &cmd[0]))< 0)
		return 0;

	if((i2c_smbus_read_i2c_block_data(i2c_client, 0x59, 4, &cmd[0]))< 0)
			return 0;

	mdelay(5);
	for(i=0;i<16;i++)
	{
		if(cmd[1]==SFR_3u_1[i][0]&&cmd[2]==SFR_3u_1[i][1])
		{
			Iref[0]= SFR_6u_1[i][0];
			Iref[1]= SFR_6u_1[i][1];
		}
	}

	cmd[0] = 0x01;cmd[1] = 0x00;cmd[2] = 0x06;
	if((i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]))< 0)
		return 0;

	cmd[0] = 0x00;cmd[1] = 0x00;cmd[2] = 0x00;
	if((i2c_smbus_write_i2c_block_data(i2c_client, 0x44, 3, &cmd[0]))< 0)
		return 0;

	cmd[0] = Iref[0];cmd[1] = Iref[1];cmd[2] = 0x27;cmd[3] = 0x27;
	if((i2c_smbus_write_i2c_block_data(i2c_client, 0x45, 4, &cmd[0]))< 0)
		return 0;

	if((i2c_smbus_write_i2c_block_data(i2c_client, 0x4A, 0, &cmd[0]))< 0)
		return 0;

	return 1;
}

//1:success,0,fail
static uint8_t himax_calculateChecksum(struct himax_ts_data * ts,char *ImageBuffer, int fullLength)//, int address, int RST)
{
    struct i2c_client * i2c_client = ts->client ;
	u16 checksum = 0;
	unsigned char cmd[5], last_byte;
	int FileLength, i, readLen, k, lastLength;

	FileLength = fullLength - 2;
	memset(cmd, 0x00, sizeof(cmd));

	//himax_HW_reset(RST);

  //if((i2c_smbus_write_i2c_block_data(i2c_client, 0x81, 0, &cmd[0]))< 0)
		//return 0;

	//mdelay(120);
	//printk("Ghong_zguoqing_marked, Sleep out: %d\n", __LINE__);
	//himax_unlock_flash();

	//Bizzy added for Iref
	if(himax_modifyIref(ts) == 0){
        err_printk("modiy I ref error \n");
        return 0;
	}

	himax_FlashMode(ts->client,1);

	FileLength = (FileLength + 3) / 4;
	for (i = 0; i < FileLength; i++)
	{
		last_byte = 0;
		readLen = 0;

		cmd[0] = i & 0x1F;
		if (cmd[0] == 0x1F || i == FileLength - 1)
			last_byte = 1;
		cmd[1] = (i >> 5) & 0x1F;cmd[2] = (i >> 10) & 0x1F;
		if((i2c_smbus_write_i2c_block_data(i2c_client, 0x44, 3, &cmd[0]))< 0)
			return 0;

		if((i2c_smbus_write_i2c_block_data(i2c_client, 0x46, 0, &cmd[0]))< 0)
			return 0;

		if((i2c_smbus_read_i2c_block_data(i2c_client, 0x59, 4, &cmd[0]))< 0)
			return -1;

		if (i < (FileLength - 1))
		{
			checksum += cmd[0] + cmd[1] + cmd[2] + cmd[3];
			if (i == 0)
				info_printk("cmd 0 to 3 (first): %d, %d, %d, %d\n", cmd[0], cmd[1], cmd[2], cmd[3]);
		}
		else
		{
			info_printk("cmd 0 to 3 (last): %d, %d, %d, %d\n", cmd[0], cmd[1], cmd[2], cmd[3]);
			info_printk("checksum (not last): %d\n", checksum);
			lastLength = (((fullLength - 2) % 4) > 0)?((fullLength - 2) % 4):4;

			for (k = 0; k < lastLength; k++)
				checksum += cmd[k];
			info_printk("checksum (final): %d\n", checksum);

			//Check Success
			//if (ImageBuffer[fullLength - 2] == *((uint8_t *)(&checksum)) && ImageBuffer[fullLength - 1] == *((uint8_t *)(&checksum) + 1))
            info_printk("imagebuffer[0x%x] checksum[0x%x]\n",ImageBuffer[fullLength - 1],(0xFF & (checksum >> 8)));
            info_printk("imagebuffer[0x%x] checksum[0x%x]\n",ImageBuffer[fullLength - 2],(0xFF & (checksum)));
            if (ImageBuffer[fullLength - 1] == (u8)(0xFF & (checksum >> 8)) && ImageBuffer[fullLength - 2] == (u8)(0xFF & checksum))
            //if (ImageBuffer[fullLength - 2] == (u8)(0xFF & (checksum >> 8)) && ImageBuffer[fullLength - 1] == (u8)(0xFF & checksum))
			{

				himax_FlashMode(ts->client,0);
				return 1;
			}
			else //Check Fail
			{
                err_printk("check falil \n");
				himax_FlashMode(ts->client,0);
				return 0;
			}
		}
	}
    return 1 ;
}

void himax_lock_flash(struct himax_ts_data * ts)
{
    struct i2c_client * i2c_client = ts->client ;
	unsigned char cmd[5];

	/* lock sequence start */
	cmd[0] = 0x01;cmd[1] = 0x00;cmd[2] = 0x06;
	i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]);

	cmd[0] = 0x03;cmd[1] = 0x00;cmd[2] = 0x00;
	i2c_smbus_write_i2c_block_data(i2c_client, 0x44, 3, &cmd[0]);

	cmd[0] = 0x00;cmd[1] = 0x00;cmd[2] = 0x7D;cmd[3] = 0x03;
	i2c_smbus_write_i2c_block_data(i2c_client, 0x45, 4, &cmd[0]);

	i2c_smbus_write_i2c_block_data(i2c_client, 0x4A, 0, &cmd[0]);
	mdelay(50);
	/* lock sequence stop */
}


//return 1:Success, 0:Fail
static int fts_ctpm_fw_upgrade_with_i_file(struct himax_ts_data * ts)
{
    struct i2c_client * i2c_client = ts->client ;
    unsigned char* ImageBuffer ;
    int fullFileLength ; //Paul Check
    int i, j;
    unsigned char cmd[5], last_byte, prePage;
    int FileLength;
    uint8_t checksumResult = 0;


    if(get_project(ts)==1){
        ImageBuffer = HIMAX_FW_C8680 ;
        fullFileLength = sizeof(HIMAX_FW_C8680) ;
    }else if(get_project(ts)==2){
        ImageBuffer = HIMAX_FW_C8680QM ;
        fullFileLength = sizeof(HIMAX_FW_C8680QM) ;
    }else if(get_project(ts)==3){
        ImageBuffer = HIMAX_FW_C8680GP ;
        fullFileLength = sizeof(HIMAX_FW_C8680GP) ;
    }


	//Try 1 Times
	for (j = 0; j < 1; j++)
	{
		FileLength = fullFileLength - 2;

		//himax_HW_reset(RST);

        //if((i2c_smbus_write_i2c_block_data(i2c_client, 0x81, 0, &cmd[0]))< 0)
		//return 0;
		//mdelay(120);

		himax_unlock_flash(ts);  //unlock flash

		cmd[0] = 0x05;cmd[1] = 0x00;cmd[2] = 0x02;
    if((i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]))< 0)	 //enable erase
		   return 0;

    if((i2c_smbus_write_i2c_block_data(i2c_client, 0x4F, 0, &cmd[0]))< 0)	 //begin erase
		   return 0;

        mdelay(50);

		himax_ManualMode(ts->client,1);												 //ok
		himax_FlashMode(ts->client,1);													 //ok

		FileLength = (FileLength + 3) / 4;
		for (i = 0, prePage = 0; i < FileLength; i++)
		{
			last_byte = 0;

			cmd[0] = i & 0x1F;  //colume
			if (cmd[0] == 0x1F || i == FileLength - 1)
				last_byte = 1;
			cmd[1] = (i >> 5) & 0x1F; //page
			cmd[2] = (i >> 10) & 0x1F; //sector
			if((i2c_smbus_write_i2c_block_data(i2c_client, 0x44, 3, &cmd[0]))< 0) //set flash address
		      return 0;

			if (prePage != cmd[1] || i == 0) //write next page
			{
				prePage = cmd[1];

				cmd[0] = 0x01;cmd[1] = 0x09;cmd[2] = 0x02;
				if((i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]))< 0)
		   		return 0;

				cmd[0] = 0x01;cmd[1] = 0x0D;cmd[2] = 0x02;
				if((i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]))< 0)
		   		return 0;

				cmd[0] = 0x01;cmd[1] = 0x09;cmd[2] = 0x02;
				if((i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]))< 0)
		   		return 0;
			}

			memcpy(&cmd[0], &ImageBuffer[4*i], 4);//Paul
			if((i2c_smbus_write_i2c_block_data(i2c_client, 0x45, 4, &cmd[0]))< 0) //write 4 byte
		   		return 0;

			cmd[0] = 0x01;cmd[1] = 0x0D;cmd[2] = 0x02;
			if((i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]))< 0)
		   		return 0;

			cmd[0] = 0x01;cmd[1] = 0x09;cmd[2] = 0x02;
			if((i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]))< 0)
		   		return 0;

			if (last_byte == 1)  //write data form flash buffer to this page
			{
				cmd[0] = 0x01;cmd[1] = 0x01;cmd[2] = 0x02;
				if((i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]))< 0)
		   		return 0;

				cmd[0] = 0x01;cmd[1] = 0x05;cmd[2] = 0x02;
				if((i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]))< 0)
		   		return 0;

				cmd[0] = 0x01;cmd[1] = 0x01;cmd[2] = 0x02;
				if((i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]))< 0)
		   		return 0;

				cmd[0] = 0x01;cmd[1] = 0x00;cmd[2] = 0x02;
				if((i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]))< 0)
		   			return 0;

				mdelay(10);
				if (i == (FileLength - 1))
				{
					himax_FlashMode(ts->client,0);
					himax_ManualMode(ts->client,0);
					checksumResult = himax_calculateChecksum(ts,ImageBuffer, fullFileLength);//, address, RST);
					//himax_ManualMode(0);
					himax_lock_flash(ts);

					if (checksumResult) //Success
					{
						info_printk("checksum success.....\n");
						return 1;
					}
					else if (!checksumResult) //Fail
					{
						info_printk("checksum fail.......\n");
						break ;
					}
					else //Retry
					{
						himax_FlashMode(ts->client,0);
						himax_ManualMode(ts->client,0);
					}
				}
			}     //last byte
		}    //for file length
	}
    return 0;
}

static int himax_fw_upgrade(struct himax_ts_data * ts)
{

    int upgrade = 0 ;
    int ret ;
    ret = detect_project_name(ts) ;
    if(ret < 0){
      err_printk("detect project name error ret[%d]\n",ret);
      return -ENODEV ;
    }


    upgrade = himax_read_FW_ver(ts) ;
    if(upgrade==1){
      struct himax_platform_data * pdata = dev_get_platdata(&ts->client->dev);
      info_printk("TP need upgrade firmware\n");
     if(fts_ctpm_fw_upgrade_with_i_file(ts) == 0){
        err_printk(" TP upgrade error\n");
        ts->upgrade = 0 ;
     }else {
        info_printk("TP upgrade success\n");
        ts->upgrade = 1 ;
     }
     himax_ts_reset(pdata);
    }else {
        info_printk("TP not need to upgrade firmware \n");
  }
 return 0;
}

#endif
static int himax_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct himax_ts_data *ts ;
    struct himax_platform_data *pdata;
    int ret = -1;


    info_printk("himax TS probe\n"); //[Step 1]
    pdata = dev_get_platdata(&client->dev);
    if(!pdata){
        err_printk("platform data error\n");
        return -ENODEV ;
    }
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        printk(KERN_ERR "himax_ts_probe: need I2C_FUNC_I2C\n");
        ret = -ENODEV;
        goto err_check_functionality_failed;
    }

    ts = kzalloc(sizeof(*ts), GFP_KERNEL);
    if (ts == NULL) {
        ret = -ENOMEM;
        goto err_alloc_data_failed;
    }
#ifdef __SELF_REPORT__
    for(i=0;i<TOUCH_MAX_NUMBER;i++) {
        ts->points[i].pre_status = TOUCH_UP ;
        ts->points[i].cur_status = TOUCH_UP ;
    }
#endif

    INIT_WORK(&ts->work, himax_ts_work_func);
    client->irq = gpio_to_irq(pdata->gpio_int);
    ts->client = client;
    i2c_set_clientdata(client, ts);

    ret = gpio_request(pdata->gpio_int,"himax_attn");
    if(ret < 0 ){
        err_printk("request himax interrupt error %d\n",ret);
        return ret ;
    }

    gpio_direction_input(pdata->gpio_int);

    ret = gpio_request(pdata->gpio_rst,"himax_rst");
    if(ret < 0 ){
        err_printk("request himax reset pin error\n");
        return ret ;
    }
    gpio_direction_output(pdata->gpio_rst,GPIO_OUTPUT_HI) ;

    ts->upgrade   = 1 ;
    ts->err_count = 0 ;
    ts->cmd_92    = false ;

#if ChangeIref1u
	ret = ChangeIrefSPP(ts );
	msleep(20);		//Wake_huang added @ 2012-11-29
	if(ret <= 0){
		//himax_ts_reset(dev_get_platdata(&ts->client->dev));	//Hardware Reset
        err_printk("ChangeIrefSPP error\n");
    }
#endif


   himax_ts_reset(pdata) ;

#ifdef CONFIG_HX8526A_FW_UPDATE
   ret = himax_fw_upgrade(ts);
#endif

    ret = detect_project_name(ts) ;
    if(ret<0)goto FREE ;

    ret = himax_ts_poweron(ts) ;
    if(ret < 0 ){
 FREE:
        gpio_free(pdata->gpio_rst);
        gpio_free(pdata->gpio_int);
        kfree(ts);
        return ret ;
    }


    if(!client->irq) {
        msleep(300);
    }

    /* allocate input device */
    ts->input_dev = input_allocate_device();
    if (ts->input_dev == NULL) {
        ret = -ENOMEM;
        printk(KERN_ERR "himax_ts_probe: Failed to allocate input device\n");
        goto err_input_dev_alloc_failed;
    }
    ts->input_dev->name = "himax-touchscreen";
    set_bit(EV_SYN,ts->input_dev->evbit) ;
    set_bit(EV_ABS,ts->input_dev->evbit) ;
    set_bit(EV_KEY,ts->input_dev->evbit);

    set_bit(BTN_TOUCH,ts->input_dev->keybit);
    #if 0
    ts->input_dev->absbit[0] =  \
                    BITS_TO_LONGS(ABS_MT_POSITION_X) | BITS_TO_LONGS(ABS_MT_POSITION_Y) |
                    BITS_TO_LONGS(ABS_MT_TOUCH_MAJOR) | BITS_TO_LONGS(ABS_MT_WIDTH_MAJOR);
    #endif
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, 540, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, 960, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, HIMAX_MAJOR_MAX, 0, 0); //Finger Size
    input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, HIMAX_WIDTH_MAX, 0, 0); //Touch Size
    //input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID,0,TOUCH_MAX_NUMBER-1,0,0);

    ret = input_register_device(ts->input_dev);
    if (ret) {
        printk(KERN_ERR "himax_ts_probe: Unable to register %s input device\n", ts->input_dev->name);
        goto err_input_register_device_failed;
    }

    ts->himax_state = HIMAX_STATE_ACTIVE ;
    ts->irq_count   = 1 ;
    ts->charger_status = 0 ;
    if (client->irq) {
        ret = request_irq(client->irq, himax_ts_irq_handler,IRQF_DISABLED|IRQF_TRIGGER_LOW, client->name, ts);
        // |IRQF_SHARED | IRQF_SAMPLE_RANDOM
        //IRQF_TRIGGER_HIGH | IRQF_TRIGGER_LOW |IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING
        if (ret == 0) {
            ts->use_irq = 1;
        }
        else { dev_err(&client->dev, "request_irq failed\n");}
    }

    if (!ts->use_irq) {
        hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        ts->timer.function = himax_ts_timer_func;
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
    }
#ifdef CONFIG_HAS_EARLYSUSPEND
    ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    ts->early_suspend.suspend = himax_ts_early_suspend;
    ts->early_suspend.resume = himax_ts_late_resume;
    register_early_suspend(&ts->early_suspend);
#endif

    create_sysfs_file(ts);

    info_printk("Start touchscreen %s in %s mode\n", ts->input_dev->name, ts->use_irq ? "interrupt" : "polling");

    return 0;

err_input_register_device_failed:
    input_free_device(ts->input_dev);

err_input_dev_alloc_failed:
    kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
    return ret;
}

static int himax_ts_remove(struct i2c_client *client)
{
    struct himax_ts_data *ts = i2c_get_clientdata(client);
#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&ts->early_suspend);
#endif
    if (ts->use_irq)
        free_irq(client->irq, ts);
    else
        hrtimer_cancel(&ts->timer);
    input_unregister_device(ts->input_dev);
    kfree(ts->finger_buf);
    kfree(ts);
    return 0;
}

static int himax_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
    int ret;

    struct himax_ts_data *ts;
    info_printk("Himax TS Suspend\n");
    ts =  i2c_get_clientdata(client);
    if (ts->use_irq){
        disable_device_irq(ts,1);
        info_printk("disable IRQ %d,count[%d]\n", client->irq,ts->irq_count);
    }
    else
        hrtimer_cancel(&ts->timer);

    ret = cancel_work_sync(&ts->work);
    if(ret && ts->use_irq)
        enable_device_irq(ts);
    ret = himax_ts_sleep(ts);
    if(ret < 0 ){
        err_printk("himax entry sleep mode error \n");
    }
    return 0;
}

static int himax_ts_resume(struct i2c_client *client)
{

    struct himax_ts_data *ts = i2c_get_clientdata(client);

    himax_init_panel(ts);

    //TODO do resume

    info_printk("himax TS Resume\n");
    himax_ts_active(ts);
    if (ts->use_irq) {
        enable_device_irq(ts);
        info_printk("enabling IRQ %d,count[%d]\n", client->irq,ts->irq_count);
    }
    else
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);

    return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void himax_ts_early_suspend(struct early_suspend *h)
{
    struct himax_ts_data *ts;
    ts = container_of(h, struct himax_ts_data, early_suspend);
    himax_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void himax_ts_late_resume(struct early_suspend *h)
{
    struct himax_ts_data *ts;
    ts = container_of(h, struct himax_ts_data, early_suspend);
    himax_ts_resume(ts->client);
}
#endif

#define HIMAX_TS_NAME "himax_ts"

static const struct i2c_device_id himax_ts_id[] = {
    { HIMAX_TS_NAME, 0 },
    { }
};

static struct i2c_driver himax_ts_driver = {
    .probe      = himax_ts_probe,
    .remove     = himax_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
    .suspend    = himax_ts_suspend,
    .resume     = himax_ts_resume,
#endif
    .id_table   = himax_ts_id,
    .driver = {
        .name   = HIMAX_TS_NAME,
    },
};

static int __devinit himax_ts_init(void)
{
    printk("Himax TS init\n"); //[Step 0]
    himax_wq = create_singlethread_workqueue("himax_wq");
    if (!himax_wq)
        return -ENOMEM;
    return i2c_add_driver(&himax_ts_driver);
}

static void __exit himax_ts_exit(void)
{
    printk("Himax TS exit\n");
    i2c_del_driver(&himax_ts_driver);
    if (himax_wq)
        destroy_workqueue(himax_wq);
}

module_init(himax_ts_init);
module_exit(himax_ts_exit);

MODULE_DESCRIPTION("Himax Touchscreen Driver");
MODULE_LICENSE("GPL");
