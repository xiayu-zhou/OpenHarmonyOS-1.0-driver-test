/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: ft6236 touch driver implement.
 * Author: zhaihaipeng
 * Create: 2020-07-25
 */

#include <stdlib.h>
#include <asm/io.h>
#include <fs/fs.h>
#include <fs_poll_pri.h>
#include <los_queue.h>
#include <poll.h>
#include <user_copy.h>
#include <securec.h>
#include "gpio_if.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "osal_irq.h"
#include "osal_mem.h"
#include "osal_time.h"
#include "touch_gt9xx.h"


#define LED_DEVICE "/dev/myTest"
#define TOUCH_DEVICE_MODE 0666
/* task config */
#define TASK_PRIO_LEVEL_TWO 2
#define TASK_SIZE 0x6000
#define TOUCH_EVENT_DOWN       0
#define TOUCH_EVENT_UP         1
#define TOUCH_EVENT_CONTACT    2

/* the macro defines of GT911 */
#define ONE_BYTE_MASK         0xFF
#define ONE_BYTE_OFFSET       8
#define GT_EVENT_UP           0x80
#define GT_EVENT_INVALID      0
#define GT_EVENT_SIZE         6
#define GT_X_LOW              0
#define GT_X_HIGH             1
#define GT_Y_LOW              2
#define GT_Y_HIGH             3
#define GT_PRESSURE_LOW       4
#define GT_PRESSURE_HIGH      5
#define GT_ADDR_LEN           2
#define GT_BUF_STATE_ADDR     0x814E
#define GT_X_LOW_BYTE_BASE    0x8150
#define GT_FINGER_NUM_MASK    0x03
#define GT_CLEAN_DATA_LEN     3
#define GT_REG_HIGH_POS       0
#define GT_REG_LOW_POS        1
#define GT_CLEAN_POS          2
#define GT_CLEAN_FLAG         0x0
/* Config info macro of GT911 */
#define GT_CFG_INFO_ADDR      0x8140
#define GT_CFG_INFO_LEN       10
#define GT_PROD_ID_1ST        0
#define GT_PROD_ID_2ND        1
#define GT_PROD_ID_3RD        2
#define GT_PROD_ID_4TH        3
#define GT_FW_VER_LOW         4
#define GT_FW_VER_HIGH        5
#define GT_SOLU_X_LOW         6
#define GT_SOLU_X_HIGH        7
#define GT_SOLU_Y_LOW         8
#define GT_SOLU_Y_HIGH        9

/* the sleep time for task */
#define TASK_SLEEP_MS 100
#define EVENT_SYNC 0x1

static TouchCoreData *g_coreData;


uint32_t IrqHandle(uint32_t irqId, void *dev);




static volatile unsigned int *CCM_CCGR1                              ;
static volatile unsigned int *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3;
static volatile unsigned int *GPIO5_GDIR                             ;
static volatile unsigned int *GPIO5_DR                               ;

//绿
static volatile unsigned int *CCM_CCGR3                              ;
static volatile unsigned int *IOMUXC_SW_MUX_CTL_PAD_CSI_HSYNC        ;
static volatile unsigned int *GPIO4_GDIR              ;
static volatile unsigned int *GPIO4_DR              ;

//红
static volatile unsigned int *IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO04;
static volatile unsigned int *GPIO1_4_GDIR;
static volatile unsigned int *GPIO1_4_DR;

//蓝 CCM_CCGR3 和绿灯差不多
// static volatile unsigned int *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3;
// static volatile unsigned int *GPIO5_GDIR                             ;
// static volatile unsigned int *GPIO5_DR                               ;

//按键
static volatile unsigned int *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1;
static volatile unsigned int *GPIO5_1GDIR                             ;
static volatile unsigned int *GPIO5_1DR                               ;

//蜂鸣器 CCM_CCGR1
static volatile unsigned int *IOMUXC_SW_MUX_CTL_PAD_UART1_RTS_B;
static volatile unsigned int *GPIO1_19_GDIR;
static volatile unsigned int *GPIO1_19_DR;

void beeInit()
{
    unsigned int val;
    IOMUXC_SW_MUX_CTL_PAD_UART1_RTS_B        = (volatile unsigned int *)IO_DEVICE_ADDR(0x20E0000 + 0x90);
    GPIO1_19_GDIR                            = (volatile unsigned int *)IO_DEVICE_ADDR(0x209C000 + 0x4);
    GPIO1_19_DR                              = (volatile unsigned int *)IO_DEVICE_ADDR(0x209C000);

    *CCM_CCGR1 |= (3<<26);
    val = *IOMUXC_SW_MUX_CTL_PAD_UART1_RTS_B;
	val &= ~(0xf);
	val |= (5);
    *IOMUXC_SW_MUX_CTL_PAD_UART1_RTS_B = val;
    *GPIO1_19_GDIR |= (1<<19);

    *GPIO1_19_DR &= ~(1<<19);
}

static void user_light()
{
    unsigned int val;
    CCM_CCGR1								= (volatile unsigned int *)IO_DEVICE_ADDR(0x20C406C);
    IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 = (volatile unsigned int *)IO_DEVICE_ADDR(0x2290014);
    GPIO5_GDIR								= (volatile unsigned int *)IO_DEVICE_ADDR(0x020AC000 + 0x4);
    GPIO5_DR								= (volatile unsigned int *)IO_DEVICE_ADDR(0x020AC000);
    *CCM_CCGR1 |= (3<<30);
    val = *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3;
    val &= ~(0xf);
    val |= (5);
    *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 = val;
    *GPIO5_GDIR |= (1<<3);
    *GPIO5_DR &= ~(1<<3);
}

static void RGB_init()
{
    unsigned int val;
    //绿灯
    CCM_CCGR3                               = (volatile unsigned int *)IO_DEVICE_ADDR(0x20C4074);
    IOMUXC_SW_MUX_CTL_PAD_CSI_HSYNC         = (volatile unsigned int *)IO_DEVICE_ADDR(0x20E01E0);
    GPIO4_GDIR                              = (volatile unsigned int *)IO_DEVICE_ADDR(0x20A8000 + 0x4);
    GPIO4_DR                                = (volatile unsigned int *)IO_DEVICE_ADDR(0x20A8000);

    *CCM_CCGR1 |= (3<<12);
    val = *IOMUXC_SW_MUX_CTL_PAD_CSI_HSYNC;
	val &= ~(0xf);
	val |= (5);
	*IOMUXC_SW_MUX_CTL_PAD_CSI_HSYNC = val;

    *GPIO4_GDIR |= (1<<20);
    *GPIO4_DR |= (1<<20);

    //红灯 CCM_CCGR1
    IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO04        = (volatile unsigned int *)IO_DEVICE_ADDR(0x20E0000 + 0x6C);
    GPIO1_4_GDIR                            = (volatile unsigned int *)IO_DEVICE_ADDR(0x209C000 + 0x4);
    GPIO1_4_DR                              = (volatile unsigned int *)IO_DEVICE_ADDR(0x209C000);

    *CCM_CCGR1 |= (3<<26);
    val = *IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO04;
	val &= ~(0xf);
	val |= (5);
    *IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO04 = val;
    *GPIO1_4_GDIR |= (1<<4);

    *GPIO1_4_DR |= (1<<4);

    //蓝灯
    *GPIO4_GDIR |= (1<<19);
    *GPIO4_DR |= (1<<19);
}

void buttonInit()
{
    unsigned int val;
    IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1 = (volatile unsigned int *)IO_DEVICE_ADDR(0x2290000 + 0xC);
    GPIO5_1GDIR								= (volatile unsigned int *)IO_DEVICE_ADDR(0x020AC000 + 0x4);
    GPIO5_1DR								= (volatile unsigned int *)IO_DEVICE_ADDR(0x020AC000);
    val = *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1;
    val &= ~(0xf);
    val |= (5);
    *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1 = val;
    *GPIO5_1GDIR &= ~(1<<1);
    //*GPIO5_1DR &= ~(1<<1);
    if((*GPIO5_1DR & (1<<1))  == (1 << 1)){
        PRINT_RELEASE("This is button experiment\n");
    }else{
        PRINT_RELEASE("release\n");
    }
}

static void Led_io_init(void)
{

    user_light();
    RGB_init();
    buttonInit();
    beeInit();
	//unsigned int val;
}

static void led_init(void)
{
    /*初始化gt911_io*/
	Led_io_init();
	/*初始化两个I0都为低电平*/
	//gpio1->DR &= ~(1<<4);
	/*10ms*/
	OsalMSleep(RESET_LOW_DELAY);
	/*reset 输出高，INT转为悬浮输出态*/
	//gpio1->DR |= (1<<4);
	/*100ms*/
	OsalMSleep(RESET_LOW_DELAY*10);
}

/* end for imx6ull */

static int LedSetupGpio(const TouchCoreData *cd)
{
	led_init();
    return HDF_SUCCESS;
}


static int TouchOpen(FAR struct file *filep)
{
    HDF_LOGI("%s: called", __func__);
    if (filep == NULL) {
        HDF_LOGE("%s: fliep is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    PRINT_RELEASE("open succeed\n");	
    return HDF_SUCCESS;
}

static int led_write(struct file *filep, const char  * buf, size_t count)
{
    uint8_t *kbuf = NULL;
    kbuf = (uint8_t *)OsalMemCalloc(count);
    if (kbuf == NULL) {
        HDF_LOGE("%s: malloc kbuf fail!", __func__);
        return 0;
    }

    if (LOS_CopyToKernel(kbuf, count, buf, count) != LOS_OK) {
        HDF_LOGE("%s: copy to kernel fail!", __func__);
        OsalMemFree(kbuf);
        return 0;
    }

    //PRINT_RELEASE("kubf:%s\n",kbuf);
	
    switch (kbuf[0])
    {
    case 'R':
        *GPIO1_4_DR &= ~(1<<4);
        break;
    case 'r':
        *GPIO1_4_DR |= (1<<4);
        break;
    case 'G':
        *GPIO4_DR &= ~(1<<20);
        break;
    case 'g':
        *GPIO4_DR |= (1<<20);
        break;
    case 'B':
        *GPIO4_DR &= ~(1<<19);
        break;
    case 'b':
        *GPIO4_DR |= (1<<19);
        break;
    case 'F':
        *GPIO1_19_DR |= (1<<19);
        break;
    case 'f':
        *GPIO1_19_DR &= ~(1<<19);
        break;   
    }

    //PRINT_RELEASE("write succeed\n");	
	
    return count;
}

static int LedClose(struct file *filep)
{
    PRINT_RELEASE("close succeed\n");
    return 0;
}

static int my_Test_read(struct file *filep,  char  * buf, size_t count)
{
    // PRINT_RELEASE("my_Test_read succeed\n");	
    // PRINT_RELEASE("read count : %d\n",count);
    uint8_t *kbuf = NULL;
    kbuf = (uint8_t *)OsalMemCalloc(count);
    if (kbuf == NULL) {
        HDF_LOGE("%s: malloc kbuf fail!", __func__);
        return 0;
    }

    if((*GPIO5_1DR & (1<<1))  == (1 << 1)){
        kbuf[0] = '1';
    }else{
        kbuf[0] = '0';
    }
    

    if (LOS_CopyFromKernel(buf, count, kbuf, count) != LOS_OK) {
        HDF_LOGE("%s: copy to kernel fail!", __func__);
        OsalMemFree(kbuf);
        return 0;
    }
    //PRINT_RELEASE("read succeed : %s\n",kbuf);
    return count;
}

static const struct file_operations_vfs g_touchDevOps = {
    .open = TouchOpen,
    .close = LedClose,
    .read = my_Test_read,
    .write = led_write,
    .seek = NULL,
    .ioctl = NULL,
    .mmap = NULL,
};


int32_t LedDispatch(struct HdfDeviceIoClient *client, int cmdId, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    (void)client;
    (void)cmdId;
    if (data == NULL || reply == NULL) {
        HDF_LOGE("%s: param is null", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t LedDriverOpen(struct HdfDeviceObject *object)
{
    if (object == NULL) {
        HDF_LOGE("%s: param is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    static struct IDeviceIoService service = {
        .object = {0},
        .Dispatch = LedDispatch,
    };
    object->service = &service;
    return HDF_SUCCESS;
}

int LedDriverInit(struct HdfDeviceObject *object)
{
    (void)object;
    HDF_LOGI("%s: enter", __func__);
    g_coreData = (TouchCoreData *)OsalMemAlloc(sizeof(TouchCoreData));
    if (g_coreData == NULL) {
        HDF_LOGE("%s: malloc failed", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }
    (void)memset_s(g_coreData, sizeof(TouchCoreData), 0, sizeof(TouchCoreData));	
    if (LedSetupGpio(g_coreData)) {
        goto ERR_EXIT;
    } 
    PRINT_RELEASE("test_led\n");	
    (void)mkdir("/dev/input", DEFAULT_DIR_MODE);
    int ret = register_driver(LED_DEVICE, &g_touchDevOps, TOUCH_DEVICE_MODE, NULL);
    if (ret != 0) {
        HDF_LOGE("%s: register touch dev failed, ret %d", __func__, ret);
        goto ERR_EXIT;
    }
    if(g_touchDevOps.read == NULL){
        PRINT_RELEASE("read is null\n");
    }else{
        PRINT_RELEASE("read is not null\n");
    }
    HDF_LOGI("%s: exit succ", __func__);
    return HDF_SUCCESS;

ERR_EXIT:
    if (g_coreData->i2cClient.i2cHandle != NULL) {
        I2cClose(g_coreData->i2cClient.i2cHandle);
        g_coreData->i2cClient.i2cHandle = NULL;
    }
    OsalMemFree(g_coreData);
    g_coreData = NULL;
    return HDF_FAILURE;
}

struct HdfDriverEntry LedDevEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_LED",
    .Bind = LedDriverOpen,
    .Init = LedDriverInit,
};

HDF_INIT(LedDevEntry);
