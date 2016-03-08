/******************************************************************************
 *
 * Copyright(c) 2009-2013  Realtek Corporation.
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

#ifndef __RTL88EU_HW_H__
#define __RTL88EU_HW_H__

#define	HAL_BB_ENABLE			1

#define SW_OFFLOAD_EN			BIT(7)
#define REG_HMEBOX_E0			0x0088
#define CMD_INIT_LLT			BIT(0)
#define CMD_READ_EFUSE_MAP		BIT(1)
#define CMD_EFUSE_PATCH			BIT(2)
#define	RTL_EEPROM_ID			0x8129

#define	EEPROM_DEFAULT_24G_INDEX	0x2D
#define	EEPROM_DEFAULT_24G_HT20_DIFF	0X02
#define	EEPROM_DEFAULT_24G_OFDM_DIFF	0X04
#define	EEPROM_DEFAULT_DIFF		0XFE

#define	EEPROM_DEFAULT_BOARD_OPTION	0x00
#define	EEPROM_VERSION_88E		0xC4
#define	EEPROM_CUSTOMERID_88E		0xC5
#define TX_SELE_HQ			BIT(0)
#define TX_SELE_LQ			BIT(1)
#define TX_SELE_NQ			BIT(2)
#define CALTMR_EN			BIT(10)
#define REG_HISR_88E			0x00B4
#define	IMR_PSTIMEOUT_88E		BIT(29)
#define REG_HIMR_88E			0x00B0
#define	IMR_TXERR_88E			BIT(11)
#define REG_HIMRE_88E			0x00B8
#define INT_BULK_SEL			BIT(4)
#define	IMR_TBDER_88E			BIT(26)
#define	IMR_RXERR_88E			BIT(10)
#define	RCR_APP_PHYSTS			BIT(28)
#define DRIVER_EARLY_INT_TIME		0x05
#define BCN_DMA_ATIME_INT_TIME		0x02
#define TX_TOTAL_PAGE_NUMBER_88E	0xA9

#define TX_PAGE_BOUNDARY_88E (TX_TOTAL_PAGE_NUMBER_88E + 1)

#define WMM_NORMAL_TX_TOTAL_PAGE_NUMBER			\
	TX_TOTAL_PAGE_NUMBER_88E  /* 0xA9 , 0xb0=>176=>22k */
#define WMM_NORMAL_TX_PAGE_BOUNDARY_88E			\
	(WMM_NORMAL_TX_TOTAL_PAGE_NUMBER + 1) /* 0xA9 */
#define DRVINFO_SZ	4 /*  unit is 8bytes */
#define REG_PKT_VO_VI_LIFE_TIME		0x04C0
#define REG_PKT_BE_BK_LIFE_TIME		0x04C2
#define	bRFRegOffsetMask		0xfffff

#define RAM_DL_SEL			BIT(7) /*  1:RAM, 0:ROM */

#define REG_BB_PAD_CTRL			0x0064
#define	IMR_DISABLED_88E		0x0
#define	EEPROM_VID_88EU			0xD0
#define	EEPROM_PID_88EU			0xD2

#define EEPROM_DEFAULT_PID		0x1234
#define EEPROM_DEFAULT_VID		0x5678
#define EEPROM_DEFAULT_CUSTOMERID	0xAB
#define	EEPROM_Default_CustomerID_8188E	0x00
#define EEPROM_Default_SubCustomerID	0xCD
#define EEPROM_Default_Version		0

#define	EEPROM_MAC_ADDR_88EU		0xD7
#define _HW_STATE_NOLINK_		0x00
#define _HW_STATE_ADHOC_		0x01
#define _HW_STATE_STATION_		0x02
#define _HW_STATE_AP_			0x03

#define REG_TSFTR_SYN_OFFSET		0x0518
#define REG_BCN_CTRL_1			0x0551
#define EEPROM_DEFAULT_CRYSTALCAP_88E	0x20
#define	EEPROM_Default_ThermalMeter_88E	0x18

#define	IMR_CPWM_88E			BIT(8)
#define	IMR_CPWM2_88E			BIT(9)

#define	IMR_TXFOVW_88E			BIT(9)
#define	IMR_RXFOVW_88E			BIT(8)


void rtl88eu_iol_mode_enable(struct ieee80211_hw *hw, u8 enable);
s32 rtl88eu_iol_execute(struct ieee80211_hw *hw, u8 control);
bool rtl88eu_iol_applied(struct ieee80211_hw  *hw);
s32 rtl8188e_iol_efuse_patch(struct ieee80211_hw *hw);
void rtl88eu_read_chip_version(struct ieee80211_hw *hw);
s32 rtl88eu_init_llt_table(struct ieee80211_hw *hw, u8 txpktbuf_bndy);
void rtl88eu_set_qos(struct ieee80211_hw *hw, int aci);
void rtl88eu_enable_interrupt(struct ieee80211_hw *hw);
void rtl88eu_disable_interrupt(struct ieee80211_hw *hw);
void rtl88eu_set_check_bssid(struct ieee80211_hw *hw, bool check_bssid);
int rtl88eu_set_network_type(struct ieee80211_hw *hw, enum nl80211_iftype type);
enum rf_pwrstate rtl88eu_rf_on_off_detect(struct ieee80211_hw *hw);
int rtl88eu_hw_init(struct ieee80211_hw *hw);
void rtl88eu_card_disable(struct ieee80211_hw *hw);
void rtl88eu_read_eeprom_info(struct ieee80211_hw *hw);
void rtl88eu_get_hw_reg(struct ieee80211_hw *hw, u8 variable, u8 *val);
void rtl88eu_set_hw_reg(struct ieee80211_hw *hw, u8 variable, u8 *val);
void rtl88eu_update_interrupt_mask(struct ieee80211_hw *hw,
				   u32 add_msr, u32 rm_msr);
void rtl88eu_set_beacon_interval(struct ieee80211_hw *hw);
void rtl88eu_set_beacon_related_registers(struct ieee80211_hw *hw);
void rtl88eu_update_hal_rate_tbl(struct ieee80211_hw *hw,
		struct ieee80211_sta *sta, u8 rssi_level);
void rtl88eu_update_channel_access_setting(struct ieee80211_hw *hw);
bool rtl88eu_gpio_radio_on_off_checking(struct ieee80211_hw *hw, u8 *valid);
void rtl88eu_set_key(struct ieee80211_hw *hw, u32 key_index,
		     u8 *p_macaddr, bool is_group, u8 enc_algo,
		     bool is_wepkey, bool clear_all);
void rtl88eu_enable_hw_security_config(struct ieee80211_hw *hw);
int rtl88eu_endpoint_mapping(struct ieee80211_hw *hw);

#endif
