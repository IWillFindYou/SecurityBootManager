/******************************************************************************
 *
 * Copyright(c) 2009-2012  Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Taehee Yoo	<ap420073@gmail.com>
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/

#include "../wifi.h"
#include "../core.h"
#include "../usb.h"
#include "../efuse.h"
#include "../base.h"
#include "reg.h"
#include "def.h"
#include "phy.h"
#include "dm.h"
#include "rf.h"
#include "fw.h"
#include "sw.h"
#include "trx.h"
#include "led.h"
#include "hw.h"
#include <linux/module.h>

MODULE_AUTHOR("Georgia		<georgia@realtek.com>");
MODULE_AUTHOR("Ziv Huang	<ziv_huang@realtek.com>");
MODULE_AUTHOR("Larry Finger	<Larry.Finger@lwfinger.net>");
MODULE_AUTHOR("Taehee Yoo	<ap420073@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Realtek 8188E 802.11n USB wireless");
MODULE_FIRMWARE("rtlwifi/rtl8188eufw.bin");

static int rtl88eu_init_sw_vars(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	int err;
	u8 tid;

	rtlpriv->dm.dm_initialgain_enable = true;
	rtlpriv->dm.dm_flag = 0;
	rtlpriv->dm.disable_framebursting = false;
	rtlpriv->dm.thermalvalue = 0;
	rtlpriv->dbg.global_debuglevel = rtlpriv->cfg->mod_params->debug;
	
	rtlpriv->rtlhal.current_bandtype = BAND_ON_2_4G;
	rtlpriv->rtlhal.bandset = BAND_ON_2_4G;
	rtlpriv->rtlhal.macphymode = SINGLEMAC_SINGLEPHY;

	rtlpriv->psc.reg_fwctrl_lps = 3;
	rtlpriv->psc.reg_max_lps_awakeintvl = 5;

	if (rtlpriv->psc.reg_fwctrl_lps == 1)
		rtlpriv->psc.fwctrl_psmode = FW_PS_MIN_MODE;
	else if (rtlpriv->psc.reg_fwctrl_lps == 2)
		rtlpriv->psc.fwctrl_psmode = FW_PS_MAX_MODE;
	else if (rtlpriv->psc.reg_fwctrl_lps == 3)
		rtlpriv->psc.fwctrl_psmode = FW_PS_DTIM_MODE;


	rtlpriv->psc.low_power_enable = false;
	
	rtlpriv->rtlhal.earlymode_enable = false;
	rtlpriv->rtlhal.max_earlymode_num = 10;
	for (tid = 0; tid < 8; tid++)
		skb_queue_head_init(&rtlpriv->mac80211.skb_waitq[tid]);


	/* for firmware buf */
	rtlpriv->rtlhal.pfirmware = vzalloc(0x4000);
	if (!rtlpriv->rtlhal.pfirmware) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "Can't alloc buffer for fw\n");
		return 1;
	}
	rtlpriv->cfg->fw_name = "rtlwifi/rtl8188eufw.bin";
	pr_info("Loading firmware %s\n", rtlpriv->cfg->fw_name);
	rtlpriv->max_fw_size = 0x4000;
	err = request_firmware_nowait(THIS_MODULE, 1,
				      rtlpriv->cfg->fw_name, rtlpriv->io.dev,
				      GFP_KERNEL, hw, rtl_fw_cb);
	return err;
}

static void rtl88eu_deinit_sw_vars(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	if (rtlpriv->rtlhal.pfirmware) {
		vfree(rtlpriv->rtlhal.pfirmware);
		rtlpriv->rtlhal.pfirmware = NULL;
	}
}

/* get bt coexist status */
static bool rtl88eu_get_btc_status(void)
{
	return false;
}

static struct rtl_hal_ops rtl8188eu_hal_ops = {
	.init_sw_vars = rtl88eu_init_sw_vars,
	.deinit_sw_vars = rtl88eu_deinit_sw_vars,
	.read_chip_version = rtl88eu_read_chip_version,
	.read_eeprom_info = rtl88eu_read_eeprom_info,
	.enable_interrupt = rtl88eu_enable_interrupt,
	.disable_interrupt = rtl88eu_disable_interrupt,
	.hw_init = rtl88eu_hw_init,
	.hw_disable = rtl88eu_card_disable,
	.set_network_type = rtl88eu_set_network_type,
	.set_chk_bssid = rtl88eu_set_check_bssid,
	.set_qos = rtl88eu_set_qos,
	.set_bcn_reg = rtl88eu_set_beacon_related_registers,
	.set_bcn_intv = rtl88eu_set_beacon_interval,
	.update_interrupt_mask = rtl88eu_update_interrupt_mask,
	.get_hw_reg = rtl88eu_get_hw_reg,
	.set_hw_reg = rtl88eu_set_hw_reg,
	.update_rate_tbl = rtl88eu_update_hal_rate_tbl,
	.fill_tx_desc = rtl88eu_tx_fill_desc,
	.fill_tx_cmddesc = rtl88eu_tx_fill_cmddesc,
	.query_rx_desc = rtl88eu_rx_query_desc,
	.set_channel_access = rtl88eu_update_channel_access_setting,
	.radio_onoff_checking = rtl88eu_gpio_radio_on_off_checking,
	.set_bw_mode = rtl88e_phy_set_bw_mode,
	.switch_channel = rtl88e_phy_sw_chnl,
	.dm_watchdog = rtl88e_dm_watchdog,
	.scan_operation_backup = rtl_phy_scan_operation_backup,
	.set_rf_power_state = rtl88e_phy_set_rf_power_state,
	.led_control = rtl88eu_led_control,
	.enable_hw_sec = rtl88eu_enable_hw_security_config,
	.set_key = rtl88eu_set_key,
	.init_sw_leds = rtl88eu_init_sw_leds,
	.deinit_sw_leds = rtl88eu_deinit_sw_leds,
	.get_bbreg = rtl88e_phy_query_bb_reg,
	.set_bbreg = rtl88e_phy_set_bb_reg,
	.get_rfreg = rtl88e_phy_query_rf_reg,
	.set_rfreg = rtl88e_phy_set_rf_reg,
	.phy_lc_calibrate = rtl88e_phy_lc_calibrate,
	.fill_h2c_cmd = rtl88eu_fill_h2c_cmd,
	.get_btc_status = rtl88eu_get_btc_status,
};

static struct rtl_mod_params rtl88eu_mod_params = {
	.sw_crypto = false,
	.inactiveps = false,
	.fwctrl_lps = false,
	.swctrl_lps = false,
	.debug = DBG_EMERG,
};

module_param_named(swenc, rtl88eu_mod_params.sw_crypto, bool, 0444);
module_param_named(debug, rtl88eu_mod_params.debug, int, 0444);
MODULE_PARM_DESC(swenc, "Set to 1 for software crypto (default 0)\n");
MODULE_PARM_DESC(debug, "Set debug level (0-5) (default 0)");

#define RTL88E_USB_BULK_IN_NUM			1
#define RTL88E_NUM_RX_URBS			32

static struct rtl_hal_usbint_cfg rtl88eu_interface_cfg = {
	/* rx */
	.in_ep_num = RTL88E_USB_BULK_IN_NUM,
	.rx_urb_num = RTL88E_NUM_RX_URBS,
	.rx_max_size = 15360,
	.usb_rx_hdl = rtl8188eu_rx_hdl,
	.usb_rx_segregate_hdl = NULL, /* rtl8188eu_rx_segregate_hdl; */
	/* tx */
	.usb_tx_cleanup = rtl8188eu_tx_cleanup,
	.usb_tx_post_hdl = rtl8188eu_tx_post_hdl,
	.usb_tx_aggregate_hdl = rtl8188eu_tx_aggregate_hdl,
	/* endpoint mapping */
	.usb_endpoint_mapping = rtl88eu_endpoint_mapping,
	.usb_mq_to_hwq = rtl8188eu_mq_to_hwq,
};

static struct rtl_hal_cfg rtl88eu_hal_cfg = {
	.name = "rtl88e_usb",
	.fw_name = "rtlwifi/rtl8188eufw.bin",
	.ops = &rtl8188eu_hal_ops,
	.mod_params = &rtl88eu_mod_params,
	.usb_interface_cfg = &rtl88eu_interface_cfg,

	.maps[SYS_ISO_CTRL] = REG_SYS_ISO_CTRL,
	.maps[SYS_FUNC_EN] = REG_SYS_FUNC_EN,
	.maps[SYS_CLK] = REG_SYS_CLKR,
	.maps[MAC_RCR_AM] = AM,
	.maps[MAC_RCR_AB] = AB,
	.maps[MAC_RCR_ACRC32] = ACRC32,
	.maps[MAC_RCR_ACF] = ACF,
	.maps[MAC_RCR_AAP] = AAP,
	.maps[MAC_HIMR] = REG_HIMR,
	.maps[MAC_HIMRE] = REG_HIMRE,
	.maps[MAC_HSISR] = REG_HSISR,

	.maps[EFUSE_ACCESS] = REG_EFUSE_ACCESS,
	.maps[EFUSE_TEST] = REG_EFUSE_TEST,
	.maps[EFUSE_CTRL] = REG_EFUSE_CTRL,
	.maps[EFUSE_CLK] = 0,
	.maps[EFUSE_CLK_CTRL] = REG_EFUSE_CTRL,
	.maps[EFUSE_PWC_EV12V] = PWC_EV12V,
	.maps[EFUSE_FEN_ELDR] = FEN_ELDR,
	.maps[EFUSE_LOADER_CLK_EN] = LOADER_CLK_EN,
	.maps[EFUSE_ANA8M] = ANA8M,
	.maps[EFUSE_HWSET_MAX_SIZE] = HWSET_MAX_SIZE,
	.maps[EFUSE_MAX_SECTION_MAP] = EFUSE_MAX_SECTION,
	.maps[EFUSE_REAL_CONTENT_SIZE] = EFUSE_REAL_CONTENT_LEN,
	.maps[EFUSE_OOB_PROTECT_BYTES_LEN] = EFUSE_OOB_PROTECT_BYTES,

	.maps[RWCAM] = REG_CAMCMD,
	.maps[WCAMI] = REG_CAMWRITE,
	.maps[RCAMO] = REG_CAMREAD,
	.maps[CAMDBG] = REG_CAMDBG,
	.maps[SECR] = REG_SECCFG,
	.maps[SEC_CAM_NONE] = CAM_NONE,
	.maps[SEC_CAM_WEP40] = CAM_WEP40,
	.maps[SEC_CAM_TKIP] = CAM_TKIP,
	.maps[SEC_CAM_AES] = CAM_AES,
	.maps[SEC_CAM_WEP104] = CAM_WEP104,

	.maps[RTL_IMR_BCNDMAINT6] = IMR_BCNDMAINT6,
	.maps[RTL_IMR_BCNDMAINT5] = IMR_BCNDMAINT5,
	.maps[RTL_IMR_BCNDMAINT4] = IMR_BCNDMAINT4,
	.maps[RTL_IMR_BCNDMAINT3] = IMR_BCNDMAINT3,
	.maps[RTL_IMR_BCNDMAINT2] = IMR_BCNDMAINT2,
	.maps[RTL_IMR_BCNDMAINT1] = IMR_BCNDMAINT1,
/*	.maps[RTL_IMR_BCNDOK8] = IMR_BCNDOK8,     */   /*need check*/
	.maps[RTL_IMR_BCNDOK7] = IMR_BCNDOK7,
	.maps[RTL_IMR_BCNDOK6] = IMR_BCNDOK6,
	.maps[RTL_IMR_BCNDOK5] = IMR_BCNDOK5,
	.maps[RTL_IMR_BCNDOK4] = IMR_BCNDOK4,
	.maps[RTL_IMR_BCNDOK3] = IMR_BCNDOK3,
	.maps[RTL_IMR_BCNDOK2] = IMR_BCNDOK2,
	.maps[RTL_IMR_BCNDOK1] = IMR_BCNDOK1,
/*	.maps[RTL_IMR_TIMEOUT2] = IMR_TIMEOUT2,*/
/*	.maps[RTL_IMR_TIMEOUT1] = IMR_TIMEOUT1,*/

	.maps[RTL_IMR_TXFOVW] = IMR_TXFOVW,
	.maps[RTL_IMR_PSTIMEOUT] = IMR_PSTIMEOUT,
	.maps[RTL_IMR_BCNINT] = IMR_BCNDMAINT0,
	.maps[RTL_IMR_RXFOVW] = IMR_RXFOVW,
	.maps[RTL_IMR_RDU] = IMR_RDU,
	.maps[RTL_IMR_ATIMEND] = IMR_ATIMEND,
	.maps[RTL_IMR_BDOK] = IMR_BCNDOK0,
	.maps[RTL_IMR_MGNTDOK] = IMR_MGNTDOK,
	.maps[RTL_IMR_TBDER] = IMR_TBDER,
	.maps[RTL_IMR_HIGHDOK] = IMR_HIGHDOK,
	.maps[RTL_IMR_TBDOK] = IMR_TBDOK,
	.maps[RTL_IMR_BKDOK] = IMR_BKDOK,
	.maps[RTL_IMR_BEDOK] = IMR_BEDOK,
	.maps[RTL_IMR_VIDOK] = IMR_VIDOK,
	.maps[RTL_IMR_VODOK] = IMR_VODOK,
	.maps[RTL_IMR_ROK] = IMR_ROK,
	.maps[RTL_IMR_HSISR_IND] = IMR_HSISR_IND_ON_INT,
	.maps[RTL_IBSS_INT_MASKS] = (IMR_BCNDMAINT0 | IMR_TBDOK | IMR_TBDER),
	
	.maps[RTL_RC_CCK_RATE1M] = DESC92C_RATE1M,
	.maps[RTL_RC_CCK_RATE2M] = DESC92C_RATE2M,
	.maps[RTL_RC_CCK_RATE5_5M] = DESC92C_RATE5_5M,
	.maps[RTL_RC_CCK_RATE11M] = DESC92C_RATE11M,
	.maps[RTL_RC_OFDM_RATE6M] = DESC92C_RATE6M,
	.maps[RTL_RC_OFDM_RATE9M] = DESC92C_RATE9M,
	.maps[RTL_RC_OFDM_RATE12M] = DESC92C_RATE12M,
	.maps[RTL_RC_OFDM_RATE18M] = DESC92C_RATE18M,
	.maps[RTL_RC_OFDM_RATE24M] = DESC92C_RATE24M,
	.maps[RTL_RC_OFDM_RATE36M] = DESC92C_RATE36M,
	.maps[RTL_RC_OFDM_RATE48M] = DESC92C_RATE48M,
	.maps[RTL_RC_OFDM_RATE54M] = DESC92C_RATE54M,

	.maps[RTL_RC_HT_RATEMCS7] = DESC92C_RATEMCS7,
	.maps[RTL_RC_HT_RATEMCS15] = DESC92C_RATEMCS15,
};

#define USB_VENDER_ID_REALTEK		0x0bda

/* 2010-10-19 DID_USB_V3.4 */
static struct usb_device_id rtl8188eu_usb_ids[] = {
	{RTL_USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8179, rtl88eu_hal_cfg)},
	/* 8188EUS */
	{RTL_USB_DEVICE(USB_VENDER_ID_REALTEK, 0x0179, rtl88eu_hal_cfg)},
	/* 8188ETV */
	/*=== Customer ID ===*/
	/****** 8188EUS ********/
	{RTL_USB_DEVICE(0x056e, 0x4008, rtl88eu_hal_cfg)},
	/* Elecom WDC-150SU2M */
	{RTL_USB_DEVICE(0x07b8, 0x8179, rtl88eu_hal_cfg)},
	/* Abocom - Abocom */
	{RTL_USB_DEVICE(0x2001, 0x330F, rtl88eu_hal_cfg)},
	/* DLink DWA-125 REV D1 */
	{RTL_USB_DEVICE(0x2001, 0x3310, rtl88eu_hal_cfg)},
	/* Dlink DWA-123 REV D1 */
	{RTL_USB_DEVICE(0x2001, 0x3311, rtl88eu_hal_cfg)},
	/* DLink GO-USB-N150 REV B1 */
	{RTL_USB_DEVICE(0x0df6, 0x0076, rtl88eu_hal_cfg)},
	/* Sitecom N150 v2 */
	{}/* Terminating entry */

};

MODULE_DEVICE_TABLE(usb, rtl8188eu_usb_ids);

static int rtl8188eu_probe(struct usb_interface *intf,
			   const struct usb_device_id *id)
{
	return rtl_usb_probe(intf, id, &rtl88eu_hal_cfg);
}

static struct usb_driver rtl8188eu_driver = {
	.name = "rtl8188eu",
	.probe = rtl8188eu_probe,
	.disconnect = rtl_usb_disconnect,
	.id_table = rtl8188eu_usb_ids,

#ifdef CONFIG_PM
	/* .suspend = rtl_usb_suspend, */
	/* .resume = rtl_usb_resume, */
	/* .reset_resume = rtl8188eu_resume, */
#endif /* CONFIG_PM */
	/* .disable_hub_initiated_lpm = 1, */
};

module_usb_driver(rtl8188eu_driver);
