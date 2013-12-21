/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_sharp.h"
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#ifdef CONFIG_GLOBALFIFO_ENABLE
#include <cdev/globalfifo.h>
static struct levy_fifo_dev *levy_fifo_temp;
#endif
static int dcdc_set_lcd_power_init(struct i2c_client *client);
static int dcdc_set_lcd_power_disable(struct i2c_client *client);

static struct i2c_client *dcdc_i2c_client;

static struct msm_panel_common_pdata *mipi_sharp_pdata;
static struct dsi_buf sharp_tx_buf;
static struct dsi_buf sharp_rx_buf;

static int mipi_sharp_bl_ctrl;

static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};

static char display_on[2] = {0x29, 0x00};
static char sleep_out[2] = {0x11, 0x00};
//static char mipi_shutdown[2] = {0x00,0x00};
static char power_off01[5] = {
	0xDF, 0x55, 0xAA, 0x52,
	0x08, 
};
static char power_off02[9] = {
	0xB7, 0x03, 0x05, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00,
};

static struct dsi_cmd_desc sharp_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(power_off01), power_off01},
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(power_off02), power_off02},

	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(enter_sleep), enter_sleep}
};

static char disp_on01[5] = {
	0xDF, 0x55, 0xAA, 0x52,
	0x08, 
};

static char disp_on02[9] = {
	0xB5, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x04, 0x00,
	0x00,
};

static char disp_on03[9] = {
	0xB7, 0x00, 0x00, 0x02,
	0x32, 0x32, 0x54, 0x54,
	0x00,
};

static char disp_on04[10] = {
	0xC8, 0x11, 0x19, 0x04,
	0x0D, 0x28, 0x08, 0x00,
	0x04, 0x00,
};

static char disp_on05[2] = {
	0x36, 0xC0,	
};

static struct dsi_cmd_desc sharp_cmd_display_on_cmds[] = {

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_out), sleep_out},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(disp_on01), disp_on01},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(disp_on02), disp_on02},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(disp_on03), disp_on03},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(disp_on04), disp_on04},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(disp_on05), disp_on05},
	{DTYPE_DCS_WRITE, 1, 0, 0, 200, sizeof(display_on), display_on},
	
};
static struct dsi_cmd_desc sharp_video_display_on_cmds[] = {

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_out), sleep_out},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(disp_on01), disp_on01},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(disp_on02), disp_on02},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(disp_on03), disp_on03},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(disp_on04), disp_on04},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(disp_on05), disp_on05},
	{DTYPE_DCS_WRITE, 1, 0, 0, 200, sizeof(display_on), display_on},
};

#if 0
static char manufacture_id[2] = {0x0A, 0x00}; /* DTYPE_DCS_READ */
static struct dsi_cmd_desc sharp_manufacture_id_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id), manufacture_id
};
static uint32_t mipi_sharp_dsi_manufacture_id(struct msm_fb_data_type *mfd)
{
	struct dsi_buf *rp, *tp;
	struct dsi_cmd_desc *cmd;
	uint32 *lp;

	tp = &sharp_tx_buf;
	rp = &sharp_rx_buf;
	cmd = &sharp_manufacture_id_cmd;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 3);
	lp = (uint32 *)rp->data;
	printk(KERN_INFO "[clw]%s: manufacture_id=0x%x\n", __func__, *lp);
	return *lp;
}
#endif
#ifdef CONFIG_GLOBALFIFO_ENABLE
static struct param_dev fifo_param[] ={
	{sizeof(sleep_out), sleep_out},
	{sizeof(disp_on01), disp_on01},
	{sizeof(disp_on02), disp_on02},
	{sizeof(disp_on03), disp_on03},
	{sizeof(disp_on04), disp_on04},
	{sizeof(disp_on05), disp_on05},
	{sizeof(display_on), display_on},
};
#endif

static int mipi_sharp_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;

#ifdef CONFIG_GLOBALFIFO_ENABLE
	param_get(fifo_param, levy_fifo_temp);
#endif
	mfd = platform_get_drvdata(pdev);

	printk("[clw] sharp video lcd on E\n");
	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi  = &mfd->panel_info.mipi;

	if (!mfd->cont_splash_done) {
		mfd->cont_splash_done = 1;
		return 0;
	}

#if 0
	/* clean up ack_err_status */
	mipi_dsi_cmd_bta_sw_trigger();
	mipi_sharp_dsi_manufacture_id(mfd);
#endif

	/* Set dcdc power */
	dcdc_set_lcd_power_init(dcdc_i2c_client);

    mipi_set_tx_power_mode(1);
	if (mipi->mode == DSI_VIDEO_MODE) {
		printk("sharp video mode is on E \n");
		mipi_dsi_cmds_tx(&sharp_tx_buf,
			sharp_video_display_on_cmds,
			ARRAY_SIZE(sharp_video_display_on_cmds));
	} else if (mipi->mode == DSI_CMD_MODE) {
		mipi_dsi_cmds_tx(&sharp_tx_buf,
			sharp_cmd_display_on_cmds,
			ARRAY_SIZE(sharp_cmd_display_on_cmds));
	}
    mipi_set_tx_power_mode(0);
	
	printk("[clw] sharp video lcd on X\n");

	return 0;
}

static int mipi_sharp_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	printk("[clw] mipi_sharp_lcd_off E\n");

#ifdef CONFIG_GLOBALFIFO_ENABLE
	levy_fifo_temp = param_put(fifo_param, ARRAY_SIZE(fifo_param), levy_fifo_devp);
#endif
	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_sharp_pdata->pmic_backlight(0);

	mipi_set_tx_power_mode(1);
	mipi_dsi_cmds_tx(&sharp_tx_buf, sharp_display_off_cmds,
			ARRAY_SIZE(sharp_display_off_cmds));
	mipi_set_tx_power_mode(0);

	msleep(120);

	dcdc_set_lcd_power_disable(dcdc_i2c_client);

	printk("[clw] mipi_sharp_lcd_off X\n");
	return 0;
}

static ssize_t mipi_sharp_wta_bl_ctrl(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret = strnlen(buf, PAGE_SIZE);
	int err;

	err =  kstrtoint(buf, 0, &mipi_sharp_bl_ctrl);
	if (err)
		return ret;

	pr_info("%s: bl ctrl set to %d\n", __func__, mipi_sharp_bl_ctrl);
	mipi_sharp_pdata->pmic_backlight(mipi_sharp_bl_ctrl);

	return ret;
}

static DEVICE_ATTR(bl_ctrl, S_IWUSR, NULL, mipi_sharp_wta_bl_ctrl);

static struct attribute *mipi_sharp_fs_attrs[] = {
	&dev_attr_bl_ctrl.attr,
	NULL,
};

static struct attribute_group mipi_sharp_fs_attr_group = {
	.attrs = mipi_sharp_fs_attrs,
};

static int mipi_sharp_create_sysfs(struct platform_device *pdev)
{
	int rc;
	struct msm_fb_data_type *mfd = platform_get_drvdata(pdev);

	if (!mfd) {
		pr_err("%s: mfd not found\n", __func__);
		return -ENODEV;
	}
	if (!mfd->fbi) {
		pr_err("%s: mfd->fbi not found\n", __func__);
		return -ENODEV;
	}
	if (!mfd->fbi->dev) {
		pr_err("%s: mfd->fbi->dev not found\n", __func__);
		return -ENODEV;
	}
	rc = sysfs_create_group(&mfd->fbi->dev->kobj,
		&mipi_sharp_fs_attr_group);
	if (rc) {
		pr_err("%s: sysfs group creation failed, rc=%d\n",
			__func__, rc);
		return rc;
	}
	return 0;
}

static int __devinit mipi_sharp_lcd_probe(struct platform_device *pdev)
{
	struct platform_device *pthisdev = NULL;
	struct msm_fb_panel_data *pdata;
	pr_debug("%s\n", __func__);

	if (pdev->id == 0) {
		mipi_sharp_pdata = pdev->dev.platform_data;
		if (mipi_sharp_pdata->bl_lock)
			spin_lock_init(&mipi_sharp_pdata->bl_spinlock);
		mipi_sharp_bl_ctrl = 0;
		return 0;
	}

	pdata = pdev->dev.platform_data;

	pthisdev = msm_fb_add_device(pdev);
	mipi_sharp_create_sysfs(pthisdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_sharp_lcd_probe,
	.driver = {
		.name   = "mipi_sharp",
	},
};

/*===================== clw: add DCDC_ISL98706 func start  =========================== */

static uint8_t dcdc_i2c_read(struct i2c_client *client, unsigned char *rxdata, int data_length)
{
	uint8_t rc = 0;
	uint16_t saddr = client->addr;

	struct i2c_msg msgs[] = {
		{
			.addr  = saddr,
			.flags = 0,
			.len   = 1, 
			.buf   = rxdata,
		},
		{
			.addr  = saddr,
			.flags = I2C_M_RD,
			.len   = data_length,
			.buf   = rxdata,
		},
	};
	rc = i2c_transfer(client->adapter, msgs, 2);
	if (rc < 0)
		printk("msm_camera_i2c_rxdata failed 0x%x\n", saddr);
	return rc;
}

static int32_t dcdc_i2c_write(struct i2c_client *client, unsigned char *txdata, int length)
{
	int32_t rc = 0;
	uint16_t saddr = client->addr;
	struct i2c_msg msg[] = {
		{
			.addr = saddr,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};
	printk("I2C write, saddr:0x%x, addr:0x%x, data:0x%x\r\n",saddr, *txdata, *(txdata+1));
	rc = i2c_transfer(client->adapter, msg, 1);
	if (rc < 0)
		printk("msm_dcdc_i2c_txdata faild 0x%x\n", saddr);
	return 0;
}

static unsigned char vbst_vout[]={	0x06, 0x0B,};  /* VBST_VOUT = 5.7V */
static unsigned char vn_vout[]={	0x08, 0x08,};  /* VN_VOUT = -5.4V */
static unsigned char vp_vout[]={	0x09, 0x0C,};  /* VP_VOUT = 5.6V */
static unsigned char vout_disable[]={0x05, 0x00,}; 
static unsigned char vout_enable[]={0x05, 0x07,};  

static int dcdc_set_lcd_power_disable(struct i2c_client *client)
{
	return dcdc_i2c_write(client, vout_disable, 2);
	
}
static int dcdc_set_lcd_power_init(struct i2c_client *client)
{
	int32_t rc = 0;
	unsigned char rxdata[2]={0};

	rxdata[0] = 0x05;

	dcdc_i2c_write(client, vout_enable, 2);
	
	dcdc_i2c_read(client, rxdata, 1);

	printk("[clw] %x,%x\r\n", rxdata[0], rxdata[1]);

	dcdc_i2c_write(client, vbst_vout, 2);

	dcdc_i2c_write(client, vp_vout, 2);

	mdelay(1);

	dcdc_i2c_write(client, vn_vout, 2);

	mdelay(1);

	return rc;
}

int32_t dcdc_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int32_t rc = 0;
	unsigned char rxdata[2]={0};

	rxdata[0] = 0x05;
	printk("[clw] dcdc_i2c_probe entry\n");

	dcdc_i2c_client = client;
	
	dcdc_i2c_write(client, vout_enable, 2);

	dcdc_i2c_read(dcdc_i2c_client, rxdata, 1);

	printk("[clw] %x,%x\r\n", rxdata[0], rxdata[1]);

	dcdc_i2c_write(dcdc_i2c_client, vbst_vout, 2);

	dcdc_i2c_write(dcdc_i2c_client, vn_vout, 2);

	dcdc_i2c_write(dcdc_i2c_client, vp_vout, 2);

	return rc;
}
static struct i2c_client dcdc_i2c_ctrl = {
	.name = "dcdc_isl98607",
    .addr = 0x29, /* NOT used */
};

static const struct i2c_device_id dcdc_i2c_id[]={
	{"dcdc_isl98607", (kernel_ulong_t)&dcdc_i2c_ctrl},
	{}
};

static struct i2c_driver dcdc_i2c_driver = {
	.id_table = dcdc_i2c_id,
	.probe  = dcdc_i2c_probe,
	.driver = {
		.name = "dcdc_isl98607",
	},
};

/*===================== clw: add DCDC_ISL98706 func end  =========================== */

static void mipi_sharp_set_backlight(struct msm_fb_data_type *mfd)
{
	int bl_level,ret;
	bl_level = mfd->bl_level;

	if (mipi_sharp_pdata && mipi_sharp_pdata->pmic_backlight)
	{
		if(bl_level < 5)
			bl_level = 5;

		ret = mipi_sharp_pdata->pmic_backlight(bl_level);
	}
	else
		pr_err("%s(): Backlight level set failed", __func__);
	return;
}

static struct msm_fb_panel_data sharp_panel_data = {
	.on	= mipi_sharp_lcd_on,
	.off = mipi_sharp_lcd_off,
	.set_backlight = mipi_sharp_set_backlight,
};

static int ch_used[3];

static int mipi_sharp_lcd_init(void)
{
	mipi_dsi_buf_alloc(&sharp_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&sharp_rx_buf, DSI_BUF_SIZE);
	return platform_driver_register(&this_driver);
}

int mipi_sharp_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;
	
	printk("%s E \n", __func__);
	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

    ret = i2c_add_driver(&dcdc_i2c_driver);
	printk("clw: %x\n", ret);

	ret = mipi_sharp_lcd_init();
	if (ret) {
		pr_err("mipi_sharp_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_sharp", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	sharp_panel_data.panel_info = *pinfo;
	ret = platform_device_add_data(pdev, &sharp_panel_data,
				sizeof(sharp_panel_data));
	if (ret) {
		pr_debug("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		pr_debug("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}
	
	printk("%s X \n", __func__);
	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}
