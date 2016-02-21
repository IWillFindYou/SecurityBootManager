/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 * 
 * Taehee Yoo <ap420073@gmail.com>
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 ******************************************************************************/
#include "../wifi.h"
#include "../efuse.h"
#include "../base.h"
#include "../regd.h"
#include "../cam.h"
#include "../ps.h"
#include "../usb.h"
#include "../pwrseqcmd.h"
#include "reg.h"
#include "def.h"
#include "phy.h"
#include "dm.h"
#include "fw.h"
#include "led.h"
#include "hw.h"
#include "pwrseq.h"


inline u32 rtl_systime_to_ms(u32 systime)
{
	return systime * 1000 / HZ;
}

inline s32 rtl_get_passing_time_ms(u32 start)
{
	return rtl_systime_to_ms(jiffies-start);
}

static void _rtl88eu_reset_8051(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 value8;

	value8 = rtl_read_byte(rtlpriv, REG_SYS_FUNC_EN + 1);
	rtl_write_byte(rtlpriv, REG_SYS_FUNC_EN+1, value8 & (~BIT(2)));
	rtl_write_byte(rtlpriv, REG_SYS_FUNC_EN+1, value8 | (BIT(2)));
}

void rtl88eu_iol_mode_enable(struct ieee80211_hw *hw, u8 enable)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	u8 reg_0xf0 = 0;

	if (enable) {
		/* Enable initial offload */
		reg_0xf0 = rtl_read_byte(rtlpriv, REG_SYS_CFG);
		rtl_write_byte(rtlpriv, REG_SYS_CFG, reg_0xf0 | SW_OFFLOAD_EN);

		if (!rtlhal->fw_ready)
			_rtl88eu_reset_8051(hw);
	} else {
		/* disable initial offload */
		reg_0xf0 = rtl_read_byte(rtlpriv, REG_SYS_CFG);
		rtl_write_byte(rtlpriv, REG_SYS_CFG, reg_0xf0 & ~SW_OFFLOAD_EN);
	}
}

s32 rtl88eu_iol_execute(struct ieee80211_hw *hw, u8 control)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	s32 status = false;
	u8 reg_0x88 = 0;
	u32 start = 0, passing_time = 0;

	control = control & 0x0f;
	reg_0x88 = rtl_read_byte(rtlpriv, REG_HMEBOX_E0);
	rtl_write_byte(rtlpriv, REG_HMEBOX_E0, reg_0x88 | control);

	start = jiffies;
	while ((reg_0x88 = rtl_read_byte(rtlpriv, REG_HMEBOX_E0)) & control &&
	       (passing_time = rtl_get_passing_time_ms(start)) < 1000) {
		;
	}

	reg_0x88 = rtl_read_byte(rtlpriv, REG_HMEBOX_E0);
	status = (reg_0x88 & control) ? false : true;
	if (reg_0x88 & control<<4)
		status = false;
	return status;
}

static s32 _rtl88eu_iol_init_llt_table(struct ieee80211_hw *hw, u8 boundary)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	s32 rst = true;

	rtl88eu_iol_mode_enable(hw, 1);
	rtl_write_byte(rtlpriv, REG_TDECTRL+1, boundary);
	rst = rtl88eu_iol_execute(hw, CMD_INIT_LLT);
	rtl88eu_iol_mode_enable(hw, 0);
	return rst;
}

bool rtl88eu_iol_applied(struct ieee80211_hw *hw)
{
	/* TODO*/
	return true;
}

s32 rtl88eu_iol_efuse_patch(struct ieee80211_hw *hw)
{
	s32 result = true;

	if (rtl88eu_iol_applied(hw)) {
		rtl88eu_iol_mode_enable(hw, 1);
		result = rtl88eu_iol_execute(hw, CMD_READ_EFUSE_MAP);
		if (result == true)
			result = rtl88eu_iol_execute(hw, CMD_EFUSE_PATCH);

		rtl88eu_iol_mode_enable(hw, 0);
	}
	return result;
}

static void _rtl88eu_set_bcn_ctrl_reg(struct ieee80211_hw *hw,
				      u8 set_bits, u8 clear_bits)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_usb_priv *usb_priv = rtl_usbpriv(hw);
	struct rtl_usb *rtlusb = rtl_usbdev(usb_priv);

	rtlusb->reg_bcn_ctrl_val |= set_bits;
	rtlusb->reg_bcn_ctrl_val &= ~clear_bits;

	rtl_write_byte(rtlpriv, REG_BCN_CTRL, (u8) rtlusb->reg_bcn_ctrl_val);
}

void rtl88eu_read_chip_version(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = rtl_phy(rtlpriv);
	u32 value32;
	enum version_8188e version;

	value32 = rtl_read_dword(rtlpriv, REG_SYS_CFG);
	version = VERSION_NORMAL_CHIP_88E;/* TODO */
	rtlphy->rf_type = RF_1T1R;
	RT_TRACE(rtlpriv, COMP_ERR, DBG_LOUD,
		 "Chip RF Type: %s\n", (rtlphy->rf_type == RF_2T2R) ?
		 "RF_2T2R" : "RF_1T1R");
}

static void rtl88eu_hal_notch_filter(struct ieee80211_hw *hw, bool enable)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	if (enable) {
		rtl_write_byte(rtlpriv, ROFDM0_RXDSP+1,
			       rtl_read_byte(rtlpriv, ROFDM0_RXDSP + 1) |
					     BIT(1));
	} else {
		rtl_write_byte(rtlpriv, ROFDM0_RXDSP+1,
			       rtl_read_byte(rtlpriv, ROFDM0_RXDSP + 1) &
					     ~BIT(1));
	}
}

static s32 _rtl88eu_llt_write(struct ieee80211_hw *hw, u32 address, u32 data)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	s32 status = true;
	s32 count = 0;
	u32 value = _LLT_INIT_ADDR(address) | _LLT_INIT_DATA(data) |
		    _LLT_OP(_LLT_WRITE_ACCESS);
	u16 llt_reg = REG_LLT_INIT;

	rtl_write_dword(rtlpriv, llt_reg, value);

	do {
		value = rtl_read_dword(rtlpriv, llt_reg);
		if (_LLT_NO_ACTIVE == _LLT_OP_VALUE(value))
			break;

		if (count > POLLING_LLT_THRESHOLD) {
			RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
				 "Failed to polling write LLT done at address %d!\n", address);
			status = false;
			break;
		}
	} while (count++);

	return status;
}

s32 rtl88eu_init_llt_table(struct ieee80211_hw *hw, u8 boundary)
{
	s32 status = false;
	u32 i;
	u32 last_entry_of_tx_pkt_buf = LAST_ENTRY_OF_TX_PKT_BUFFER;

	if (rtl88eu_iol_applied(hw)) {
		status = _rtl88eu_iol_init_llt_table(hw, boundary);
	} else {
		for (i = 0; i < (boundary - 1); i++) {
			status = _rtl88eu_llt_write(hw, i, i + 1);
			if (true != status)
				return status;
		}

		/* end of list */
		status = _rtl88eu_llt_write(hw, (boundary - 1), 0xFF);
		if (true != status)
			return status;

		/*  Make the other pages as ring buffer
		 *  This ring buffer is used as beacon buffer
		 *  if we config this MAC as two MAC transfer.
		 *  Otherwise used as local loopback buffer. */
		for (i = boundary; i < last_entry_of_tx_pkt_buf; i++) {
			status = _rtl88eu_llt_write(hw, i, (i + 1));
			if (true != status)
				return status;
		}

		/* Let last entry point to the start entry of ring buffer */
		status = _rtl88eu_llt_write(hw, last_entry_of_tx_pkt_buf,
					    boundary);
		if (true != status) {
			return status;
		}
	}

	return status;
}

static void _rtl88eu_read_pwrvalue_from_prom(struct ieee80211_hw *hw,
			struct txpower_info_2g *pwrinfo24g, u8 *hwinfo)
{
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	u32 rfpath, eeaddr = EEPROM_TX_PWR_INX, group, txcnt = 0;

	memset(pwrinfo24g, 0, sizeof(struct txpower_info_2g));

	if (rtlefuse->autoload_failflag) {
		for (rfpath = 0; rfpath < MAX_RF_PATH; rfpath++) {
			/* 2.4G default value */
			for (group = 0; group < MAX_CHNL_GROUP_24G; group++) {
				pwrinfo24g->index_cck_base[rfpath][group] =
					EEPROM_DEFAULT_24G_INDEX;
				pwrinfo24g->index_bw40_base[rfpath][group] =
					EEPROM_DEFAULT_24G_INDEX;
			}
			for (txcnt = 0; txcnt < MAX_TX_COUNT; txcnt++) {
				if (txcnt == 0) {
					pwrinfo24g->bw20_diff[rfpath][0] =
						EEPROM_DEFAULT_24G_HT20_DIFF;
					pwrinfo24g->ofdm_diff[rfpath][0] =
						EEPROM_DEFAULT_24G_OFDM_DIFF;
				} else {
					pwrinfo24g->bw20_diff[rfpath][txcnt] =
						EEPROM_DEFAULT_DIFF;
					pwrinfo24g->bw40_diff[rfpath][txcnt] =
						EEPROM_DEFAULT_DIFF;
					pwrinfo24g->cck_diff[rfpath][txcnt] =
						EEPROM_DEFAULT_DIFF;
					pwrinfo24g->ofdm_diff[rfpath][txcnt] =
						EEPROM_DEFAULT_DIFF;
				}
			}
		}
		return;
	}

	for (rfpath = 0; rfpath < MAX_RF_PATH; rfpath++) {
		/* 2.4G default value */
		for (group = 0; group < MAX_CHNL_GROUP_24G; group++) {
			pwrinfo24g->index_cck_base[rfpath][group] =
				hwinfo[eeaddr++];
			if (pwrinfo24g->index_cck_base[rfpath][group] == 0xFF)
				pwrinfo24g->index_cck_base[rfpath][group] =
					EEPROM_DEFAULT_24G_INDEX;
		}
		for (group = 0; group < MAX_CHNL_GROUP_24G - 1; group++) {
			pwrinfo24g->index_bw40_base[rfpath][group] =
				hwinfo[eeaddr++];
			if (pwrinfo24g->index_bw40_base[rfpath][group] == 0xFF)
				pwrinfo24g->index_bw40_base[rfpath][group] =
					EEPROM_DEFAULT_24G_INDEX;
		}
		for (txcnt = 0; txcnt < MAX_TX_COUNT; txcnt++) {
			if (txcnt == 0) {
				pwrinfo24g->bw40_diff[rfpath][txcnt] = 0;
				if (hwinfo[eeaddr] == 0xFF) {
					pwrinfo24g->bw20_diff[rfpath][txcnt] =
						EEPROM_DEFAULT_24G_HT20_DIFF;
				} else {
					pwrinfo24g->bw20_diff[rfpath][txcnt] =
						(hwinfo[eeaddr] & 0xf0) >> 4;
					if (pwrinfo24g->bw20_diff[
							rfpath][txcnt] & BIT(3))
						pwrinfo24g->bw20_diff[
							rfpath][txcnt] |= 0xF0;
				}

				if (hwinfo[eeaddr] == 0xFF) {
					pwrinfo24g->ofdm_diff[rfpath][txcnt] =
						EEPROM_DEFAULT_24G_OFDM_DIFF;
				} else {
					pwrinfo24g->ofdm_diff[rfpath][txcnt] =
						(hwinfo[eeaddr] & 0x0f);
					if (pwrinfo24g->ofdm_diff[
							rfpath][txcnt] & BIT(3))
						pwrinfo24g->ofdm_diff[
							rfpath][txcnt] |= 0xF0;
				}
				pwrinfo24g->cck_diff[rfpath][txcnt] = 0;
				eeaddr++;
			} else {
				if (hwinfo[eeaddr] == 0xFF) {
					pwrinfo24g->bw40_diff[rfpath][txcnt] =
						EEPROM_DEFAULT_DIFF;
				} else {
					pwrinfo24g->bw40_diff[rfpath][txcnt] =
						(hwinfo[eeaddr] & 0xf0) >> 4;
					if (pwrinfo24g->bw40_diff[
							rfpath][txcnt] & BIT(3))
						pwrinfo24g->bw40_diff[
							rfpath][txcnt] |= 0xF0;
				}

				if (hwinfo[eeaddr] == 0xFF) {
					pwrinfo24g->bw20_diff[rfpath][txcnt] =
						EEPROM_DEFAULT_DIFF;
				} else {
					pwrinfo24g->bw20_diff[rfpath][txcnt] =
							(hwinfo[eeaddr]&0x0f);
					if (pwrinfo24g->bw20_diff[
							rfpath][txcnt] & BIT(3))
						pwrinfo24g->bw20_diff[
							rfpath][txcnt] |= 0xF0;
				}
				eeaddr++;

				if (hwinfo[eeaddr] == 0xFF) {
					pwrinfo24g->ofdm_diff[rfpath][txcnt] =
						EEPROM_DEFAULT_DIFF;
				} else {
					pwrinfo24g->ofdm_diff[rfpath][txcnt] =
						(hwinfo[eeaddr] & 0xf0) >> 4;
					if (pwrinfo24g->ofdm_diff[
							rfpath][txcnt] & BIT(3))
						pwrinfo24g->ofdm_diff[
							rfpath][txcnt] |= 0xF0;
				}

				if (hwinfo[eeaddr] == 0xFF) {
					pwrinfo24g->cck_diff[rfpath][txcnt] =
						EEPROM_DEFAULT_DIFF;
				} else {
					pwrinfo24g->cck_diff[rfpath][txcnt] =
						(hwinfo[eeaddr]&0x0f);
					if (pwrinfo24g->cck_diff[
							rfpath][txcnt] & BIT(3))
						pwrinfo24g->cck_diff[
							rfpath][txcnt] |= 0xF0;
				}
				eeaddr++;
			}
		}
	}
}

static u8 _rtl88e_get_chnl_group(u8 chnl)
{
	u8 group = 0;

	if (chnl < 3)
		group = 0;
	else if (chnl < 6)
		group = 1;
	else if (chnl < 9)
		group = 2;
	else if (chnl < 12)
		group = 3;
	else if (chnl < 14)
		group = 4;
	else if (chnl == 14)
		group = 5;

	return group;
}

static void _rtl88eu_read_txpower_info(struct ieee80211_hw *hw, u8 *hwinfo)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	struct txpower_info_2g pwrinfo24g;
	u8 rf_path, index;
	u8 i;

	_rtl88eu_read_pwrvalue_from_prom(hw, &pwrinfo24g, hwinfo);

	/* TODO */
#if 0
	for (rfpath = 0; rfpath < 2; rfpath++) {
		for (ch = 0; ch < 14; ch++) {
			bIn24G = Hal_GetChnlGroup88E(ch, &group);
			if (bIn24G) {
				pHalData->Index24G_CCK_Base[rfpath][ch] =
					pwrinfo24g.index_cck_base[rfpath][group];
				if (ch == 14)
					pHalData->Index24G_BW40_Base[rfpath][ch] =
						pwrinfo24g.index_bw40_base[rfpath][4];
				else
					pHalData->Index24G_BW40_Base[rfpath][ch] =
						pwrinfo24g.index_bw40_base[rfpath][group];
			}
		}
		for (txcnt = 0; txcnt < MAX_TX_COUNT; txcnt++) {
			pHalData->CCK_24G_Diff[rfpath][txcnt] =
				pwrinfo24g.cck_diff[rfpath][txcnt];
			pHalData->OFDM_24G_Diff[rfpath][txcnt] =
				pwrinfo24g.ofdm_diff[rfpath][txcnt];
			pHalData->BW20_24G_Diff[rfpath][txcnt] =
				pwrinfo24g.bw20_diff[rfpath][txcnt];
			pHalData->BW40_24G_Diff[rfpath][txcnt] =
				pwrinfo24g.bw40_diff[rfpath][txcnt];
		}
	}
#else
	for (rf_path = 0; rf_path < 2; rf_path++) {
		for (i = 0; i < 14; i++) {
			index = _rtl88e_get_chnl_group(i+1);

			rtlefuse->txpwrlevel_cck[rf_path][i] =
				pwrinfo24g.index_cck_base[rf_path][index];
			rtlefuse->txpwrlevel_ht40_1s[rf_path][i] =
				pwrinfo24g.index_bw40_base[rf_path][index];
			rtlefuse->txpwr_ht20diff[rf_path][i] =
				pwrinfo24g.bw20_diff[rf_path][0];
			rtlefuse->txpwr_legacyhtdiff[rf_path][i] =
				pwrinfo24g.ofdm_diff[rf_path][0];
		}

		for (i = 0; i < 14; i++) {
			RTPRINT(rtlpriv, FINIT, INIT_TXPOWER,
				"RF(%d)-Ch(%d) [CCK / HT40_1S ] = [0x%x / 0x%x ]\n",
				rf_path, i,
				rtlefuse->txpwrlevel_cck[rf_path][i],
				rtlefuse->txpwrlevel_ht40_1s[rf_path][i]);
		}
	}
#endif
	if (!rtlefuse->autoload_failflag) {
		rtlefuse->eeprom_regulatory =
			(hwinfo[EEPROM_RF_BOARD_OPTION_88E] & 0x7);
		if (hwinfo[EEPROM_RF_BOARD_OPTION_88E] == 0xFF)
			rtlefuse->eeprom_regulatory =
				(EEPROM_DEFAULT_BOARD_OPTION & 0x7);
	} else {
		rtlefuse->eeprom_regulatory = 0;
	}
	RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
		 "eeprom_regulatory = 0x%x\n", rtlefuse->eeprom_regulatory);
}

static void _rtl88eu_read_adapter_info(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	u16 i, usvalue;
	u8 hwinfo[HWSET_MAX_SIZE] = {0};
	u16 eeprom_id;

	if (rtlefuse->epromtype == EEPROM_BOOT_EFUSE) {
		rtl_efuse_shadow_map_update(hw);
		memcpy((void *)hwinfo,
		       (void *)&rtlefuse->efuse_map[EFUSE_INIT_MAP][0],
		       HWSET_MAX_SIZE);
	} else if (rtlefuse->epromtype == EEPROM_93C46) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "RTL819X Not boot from eeprom, check it !!");
		return;
	} else {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "boot from neither eeprom nor efuse, check it !!");
		return;
	}
	
	eeprom_id = le16_to_cpu(*((__le16 *)hwinfo));
	if (eeprom_id != RTL_EEPROM_ID) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
			 "EEPROM ID(%#x) is invalid!!\n", eeprom_id);
		rtlefuse->autoload_failflag = true;
	} else {
		rtlefuse->autoload_failflag = false;
	}


	if (!rtlefuse->autoload_failflag) {
		rtlefuse->eeprom_vid =
			EF2BYTE(*(__le16 *)&hwinfo[EEPROM_VID_88EU]);
		rtlefuse->eeprom_did =
			EF2BYTE(*(__le16 *)&hwinfo[EEPROM_PID_88EU]);

		rtlefuse->eeprom_oemid = *(u8 *)&hwinfo[EEPROM_CUSTOMERID_88E];
	} else {
		rtlefuse->eeprom_vid = EEPROM_DEFAULT_VID;
		rtlefuse->eeprom_did = EEPROM_DEFAULT_PID;

		rtlefuse->eeprom_oemid = EEPROM_DEFAULT_CUSTOMERID;
	}

	RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
		 "EEPROM ID = 0x%04x\n", eeprom_id);
	RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
		 "VID = 0x%04X, PID = 0x%04X\n",
		 rtlefuse->eeprom_vid, rtlefuse->eeprom_did);
	RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
		 "Customer ID: 0x%02X", rtlefuse->eeprom_oemid);

	for (i = 0; i < 6; i += 2) {
		usvalue = *(u16 *)&hwinfo[EEPROM_MAC_ADDR + i];
		*((u16 *) (&rtlefuse->dev_addr[i])) = usvalue;
	}

	/* Hal_ReadPowerSavingMode88E(hw, eeprom->efuse_eeprom_data); */
	_rtl88eu_read_txpower_info(hw, hwinfo);

	if (!rtlefuse->autoload_failflag) {
		rtlefuse->eeprom_version = hwinfo[EEPROM_VERSION_88E];
		if (rtlefuse->eeprom_version == 0xFF)
			rtlefuse->eeprom_version = 0;
	} else {
		rtlefuse->eeprom_version = 1;
	}
	rtlefuse->txpwr_fromeprom = true; /* TODO */
	RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
		 "eeprom_version = %d\n", rtlefuse->eeprom_version);

	rtlefuse->eeprom_channelplan = hwinfo[EEPROM_CHANNELPLAN];
	rtlefuse->channel_plan = rtlefuse->eeprom_channelplan;
	
	if (!rtlefuse->autoload_failflag) {
		rtlefuse->crystalcap = hwinfo[EEPROM_XTAL_88E];
		if (rtlefuse->crystalcap == 0xFF)
			rtlefuse->crystalcap = EEPROM_DEFAULT_CRYSTALCAP_88E;
	} else {
		rtlefuse->crystalcap = EEPROM_DEFAULT_CRYSTALCAP_88E;
	}
	if (!rtlefuse->autoload_failflag) {
		rtlefuse->eeprom_oemid = hwinfo[EEPROM_CUSTOMERID_88E];
	} else {
		rtlefuse->eeprom_oemid = 0;
	}
	RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
		 "EEPROM Customer ID: 0x%2x\n", rtlefuse->eeprom_oemid);
	rtlefuse->antenna_div_cfg =
		(hwinfo[EEPROM_RF_BOARD_OPTION_88E] & 0x18) >> 3;
	if (hwinfo[EEPROM_RF_BOARD_OPTION_88E] == 0xFF)
		rtlefuse->antenna_div_cfg = 0;
	rtlefuse->antenna_div_type = hwinfo[EEPROM_RF_ANTENNA_OPT_88E];
	if (rtlefuse->antenna_div_type == 0xFF)
		rtlefuse->antenna_div_type = 0x01;
	if (rtlefuse->antenna_div_type == CG_TRX_HW_ANTDIV ||
		rtlefuse->antenna_div_type == CGCS_RX_HW_ANTDIV)
		rtlefuse->antenna_div_cfg = 1;

	if (!rtlefuse->autoload_failflag)
		rtlefuse->board_type = (hwinfo[EEPROM_RF_BOARD_OPTION_88E]
					& 0xE0) >> 5;
	else
		rtlefuse->board_type = 0;

	if (!rtlefuse->autoload_failflag)
		rtlefuse->eeprom_thermalmeter =
			hwinfo[EEPROM_THERMAL_METER_88E];
	else
		rtlefuse->eeprom_thermalmeter =
			EEPROM_DEFAULT_THERMALMETER;

	if (rtlefuse->eeprom_thermalmeter == 0xff ||
	    rtlefuse->autoload_failflag) {
		rtlefuse->apk_thermalmeterignore = true;
		rtlefuse->eeprom_thermalmeter = EEPROM_DEFAULT_THERMALMETER;
	}
	/* TODO */
	if (rtlhal->oem_id == RT_CID_DEFAULT) {
		switch (rtlefuse->eeprom_oemid) {
		case EEPROM_CID_DEFAULT:
			if (rtlefuse->eeprom_did == 0x8179) {
				if (rtlefuse->eeprom_svid == 0x1025) {
					rtlhal->oem_id = RT_CID_819X_ACER;
				} else if ((rtlefuse->eeprom_svid == 0x10EC &&
				     rtlefuse->eeprom_smid == 0x0179) ||
				     (rtlefuse->eeprom_svid == 0x17AA &&
				     rtlefuse->eeprom_smid == 0x0179)) {
					rtlhal->oem_id = RT_CID_819X_LENOVO;
				} else if (rtlefuse->eeprom_svid == 0x103c &&
					   rtlefuse->eeprom_smid == 0x197d) {
					rtlhal->oem_id = RT_CID_819X_HP;
				} else {
					rtlhal->oem_id = RT_CID_DEFAULT;
				}
			} else {
				rtlhal->oem_id = RT_CID_DEFAULT;
			}
			break;
		case EEPROM_CID_TOSHIBA:
			rtlhal->oem_id = RT_CID_TOSHIBA;
			break;
		case EEPROM_CID_QMI:
			rtlhal->oem_id = RT_CID_819X_QMI;
			break;
		case EEPROM_CID_WHQL:
		default:
			rtlhal->oem_id = RT_CID_DEFAULT;
			break;

		}
	}

}

static void _rtl88eu_config_normal_chip_out_ep(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_usb_priv *usb_priv = rtl_usbpriv(hw);
	struct rtl_usb *rtlusb = rtl_usbdev(usb_priv);

	switch (rtlusb->out_ep_nums) {
	case	3:
		rtlusb->out_queue_sel = TX_SELE_HQ | TX_SELE_LQ | TX_SELE_NQ;
		break;
	case	2:
		rtlusb->out_queue_sel = TX_SELE_HQ | TX_SELE_NQ;
		break;
	case	1:
		rtlusb->out_queue_sel = TX_SELE_HQ;
		break;
	default:
		break;
	}
	RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
		 "%s out_ep_queue_sel(0x%02x), out_ep_number(%d)\n",
		 __func__, rtlusb->out_queue_sel, rtlusb->out_ep_nums);
}

static void _rtl88eu_one_out_ep_mapping(struct ieee80211_hw *hw,
					struct rtl_ep_map *ep_map)
{
	ep_map->ep_mapping[RTL_TXQ_BE] = 2;
	ep_map->ep_mapping[RTL_TXQ_BK] = 2;
	ep_map->ep_mapping[RTL_TXQ_VI] = 2;
	ep_map->ep_mapping[RTL_TXQ_VO] = 2;
	ep_map->ep_mapping[RTL_TXQ_MGT] = 2;
	ep_map->ep_mapping[RTL_TXQ_BCN] = 2;
	ep_map->ep_mapping[RTL_TXQ_HI] = 2;
}

static void _rtl88eu_two_out_ep_mapping(struct ieee80211_hw *hw,
					struct rtl_ep_map *ep_map)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_usb_priv *usb_priv = rtl_usbpriv(hw);
	struct rtl_usb *rtlusb = rtl_usbdev(usb_priv);

	if (rtlusb->wmm_enable) { /* for WMM */
		RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG,
			 "USB Chip-B & WMM Setting.....\n");
		ep_map->ep_mapping[RTL_TXQ_BE] = 3;
		ep_map->ep_mapping[RTL_TXQ_BK] = 2;
		ep_map->ep_mapping[RTL_TXQ_VI] = 2;
		ep_map->ep_mapping[RTL_TXQ_VO] = 3;
		ep_map->ep_mapping[RTL_TXQ_MGT] = 2;
		ep_map->ep_mapping[RTL_TXQ_BCN] = 2;
		ep_map->ep_mapping[RTL_TXQ_HI] = 2;
	} else { /* typical setting */
		RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG,
			 "USB typical Setting.....\n");
		ep_map->ep_mapping[RTL_TXQ_BE] = 3;
		ep_map->ep_mapping[RTL_TXQ_BK] = 3;
		ep_map->ep_mapping[RTL_TXQ_VI] = 2;
		ep_map->ep_mapping[RTL_TXQ_VO] = 2;
		ep_map->ep_mapping[RTL_TXQ_MGT] = 2;
		ep_map->ep_mapping[RTL_TXQ_BCN] = 2;
		ep_map->ep_mapping[RTL_TXQ_HI] = 2;
	}
}

static void _rtl88eu_three_out_ep_mapping(struct ieee80211_hw *hw,
					  struct rtl_ep_map *ep_map)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_usb_priv *usb_priv = rtl_usbpriv(hw);
	struct rtl_usb *rtlusb = rtl_usbdev(usb_priv);
	
	if (rtlusb->wmm_enable) { /* for WMM */
		RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG,
			 "USB 3EP Setting for WMM.....\n");
		ep_map->ep_mapping[RTL_TXQ_BE] = 5;
		ep_map->ep_mapping[RTL_TXQ_BK] = 3;
		ep_map->ep_mapping[RTL_TXQ_VI] = 3;
		ep_map->ep_mapping[RTL_TXQ_VO] = 2;
		ep_map->ep_mapping[RTL_TXQ_MGT] = 2;
		ep_map->ep_mapping[RTL_TXQ_BCN] = 2;
		ep_map->ep_mapping[RTL_TXQ_HI] = 2;
	} else { /* typical setting */
		RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG,
			 "USB 3EP Setting for typical.....\n");
		ep_map->ep_mapping[RTL_TXQ_BE] = 5;
		ep_map->ep_mapping[RTL_TXQ_BK] = 5;
		ep_map->ep_mapping[RTL_TXQ_VI] = 3;
		ep_map->ep_mapping[RTL_TXQ_VO] = 2;
		ep_map->ep_mapping[RTL_TXQ_MGT] = 2;
		ep_map->ep_mapping[RTL_TXQ_BCN] = 2;
		ep_map->ep_mapping[RTL_TXQ_HI] = 2;
	}
}

static int _rtl88eu_out_ep_mapping(struct ieee80211_hw *hw)
{
	struct rtl_usb_priv *usb_priv = rtl_usbpriv(hw);
	struct rtl_usb *rtlusb = rtl_usbdev(usb_priv);
	struct rtl_ep_map *ep_map = &(rtlusb->ep_map);
	int err = 0;

	switch (rtlusb->out_ep_nums) {
	case 2:
		_rtl88eu_two_out_ep_mapping(hw, ep_map);
		break;
	case 3:
		/* TODO */
		_rtl88eu_three_out_ep_mapping(hw, ep_map);
		err = -EINVAL;
		break;
	case 1:
		_rtl88eu_one_out_ep_mapping(hw, ep_map);
		break;
	default:
		err = -EINVAL;
		break;
	}
	return err;
}

int rtl88eu_endpoint_mapping(struct ieee80211_hw *hw)
{
	struct rtl_usb_priv *usb_priv = rtl_usbpriv(hw);
	struct rtl_usb *rtlusb = rtl_usbdev(usb_priv);
	bool result = false;

	_rtl88eu_config_normal_chip_out_ep(hw);
	
	/* Normal chip with one IN and one OUT doesn't have interrupt IN EP. */
	if (1 == rtlusb->out_ep_nums) {
		if (1 != rtlusb->in_ep_nums)
			return result;
	}
	/* All config other than above support one Bulk IN and one
	 * Interrupt IN. */
	result = _rtl88eu_out_ep_mapping(hw);

	return result;
}

static u32 _rtl88eu_init_power_on(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u16 value16;
	/* HW Power on sequence */
#if 0
	if (haldata->bMacPwrCtrlOn)
		return true;
#endif
	
	/* TODO Rtl8188E_NIC... -> RTL8188eu_NIC...*/
	if (!rtl_hal_pwrseqcmdparsing(rtlpriv, PWR_CUT_ALL_MSK,
				      PWR_FAB_ALL_MSK, PWR_INTF_USB_MSK,
				      RTL8188EE_NIC_PWR_ON_FLOW)) {
		RT_TRACE(rtlpriv, COMP_POWER, DBG_LOUD,
			 "run power on flow fail\n");
		return false;
	}

	/* Enable MAC DMA/WMAC/SCHEDULE/SEC block
	 * Set CR bit10 to enable 32k calibration. */
	rtl_write_word(rtlpriv, REG_CR, 0x00);

	/* Enable MAC DMA/WMAC/SCHEDULE/SEC block */
	value16 = rtl_read_word(rtlpriv, REG_CR);
	value16 |= (HCI_TXDMA_EN | HCI_RXDMA_EN | TXDMA_EN | RXDMA_EN
				| PROTOCOL_EN | SCHEDULE_EN | ENSEC | CALTMR_EN);
	/* for SDIO - Set CR bit10 to enable 32k calibration. */

	rtl_write_word(rtlpriv, REG_CR, value16);
	/* TODO */
#if 0
	haldata->bMacPwrCtrlOn = true;
#endif

	return true;
}

void rtl88eu_set_qos(struct ieee80211_hw *hw, int aci)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	rtl88e_dm_init_edca_turbo(hw);
	switch (aci) {
	case AC1_BK:
		rtl_write_dword(rtlpriv, REG_EDCA_BK_PARAM, 0xa44f);
		break;
	case AC0_BE:
		break;
	case AC2_VI:
		rtl_write_dword(rtlpriv, REG_EDCA_VI_PARAM, 0x005ea324);
		break;
	case AC3_VO:
		rtl_write_dword(rtlpriv, REG_EDCA_VO_PARAM, 0x002fa226);
		break;
	default:
		RT_ASSERT(false, "invalid aci: %d !\n", aci);
		break;
	}
}

void rtl88eu_enable_interrupt(struct ieee80211_hw *hw)
{
}

void rtl88eu_disable_interrupt(struct ieee80211_hw *hw)
{
}


/*  Shall USB interface init this? */
static void _rtl88eu_init_interrupt(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_usb_priv *usb_priv = rtl_usbpriv(hw);
	struct rtl_usb *rtlusb = rtl_usbdev(usb_priv);
	u32 imr, imr_ex;
	u8  usb_opt;

	/* HISR write one to clear */
	rtl_write_dword(rtlpriv, REG_HISR_88E, 0xffffffff);
	/*  HIMR - */
	imr = IMR_PSTIMEOUT_88E | IMR_TBDER_88E | IMR_CPWM_88E | IMR_CPWM2_88E;
	rtl_write_dword(rtlpriv, REG_HIMR_88E, imr);
	rtlusb->irq_mask[0] = imr;

	imr_ex = IMR_TXERR_88E | IMR_RXERR_88E | IMR_TXFOVW_88E | IMR_RXFOVW_88E;
	rtl_write_dword(rtlpriv, REG_HIMRE_88E, imr_ex);
	rtlusb->irq_mask[1] = imr_ex;

	/* REG_USB_SPECIAL_OPTION - BIT(4) */
	/* 0; Use interrupt endpoint to upload interrupt pkt */
	/* 1; Use bulk endpoint to upload interrupt pkt, */
	usb_opt = rtl_read_byte(rtlpriv, REG_USB_SPECIAL_OPTION);

	if (!IS_HIGH_SPEED_USB(rtlusb->udev))
		usb_opt = usb_opt & (~INT_BULK_SEL);
	else
		usb_opt = usb_opt | (INT_BULK_SEL);
	
	rtl_write_byte(rtlpriv, REG_USB_SPECIAL_OPTION, usb_opt);
}

static void _rtl88eu_init_queue_reserved_page(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_usb_priv *usb_priv = rtl_usbpriv(hw);
	struct rtl_usb *rtlusb = rtl_usbdev(usb_priv);

	u32 num_hq = 0;
	u32 num_lq = 0;
	u32 num_nq = 0;
	u32 num_pubq;
	u32 value32;
	u8 value8;

	if (rtlusb->wmm_enable) {
		if (rtlusb->out_queue_sel & TX_SELE_HQ)
			num_hq = 0x29;

		if (rtlusb->out_queue_sel & TX_SELE_LQ)
			num_lq = 0x1c;

		/* NOTE: This step shall be proceed before writting REG_RQPN. */
		if (rtlusb->out_queue_sel & TX_SELE_NQ)
			num_nq = 0x1c;
		value8 = (u8)_NPQ(num_nq);
		rtl_write_byte(rtlpriv, REG_RQPN_NPQ, value8);

		num_pubq = 0xa8 - num_hq - num_lq - num_nq;

		/*  TX DMA */
		value32 = _HPQ(num_hq) | _LPQ(num_lq) | _PUBQ(num_pubq) |
			  LD_RQPN;
		rtl_write_dword(rtlpriv, REG_RQPN, value32);
	} else {
		rtl_write_word(rtlpriv, REG_RQPN_NPQ, 0x0000);
		rtl_write_word(rtlpriv, REG_RQPN_NPQ, 0x0d);
		rtl_write_dword(rtlpriv, REG_RQPN, 0x808e000d);
	}
}

static void _rtl88eu_init_txbuffer_boundary(struct ieee80211_hw *hw,
					    u8 boundary)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	rtl_write_byte(rtlpriv, REG_TXPKTBUF_BCNQ_BDNY, boundary);
	rtl_write_byte(rtlpriv, REG_TXPKTBUF_MGQ_BDNY, boundary);
	rtl_write_byte(rtlpriv, REG_TXPKTBUF_WMAC_LBK_BF_HD, boundary);
	rtl_write_byte(rtlpriv, REG_TRXFF_BNDY, boundary);
	rtl_write_byte(rtlpriv, REG_TDECTRL + 1, boundary);
}

static void _rtl88eu_init_page_boundary(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u16 rxff_bndy = 0x2400 - 1;

	rtl_write_word(rtlpriv, (REG_TRXFF_BNDY + 2), rxff_bndy);
}

static void _rtl88eu_init_normal_chip_reg_priority(struct ieee80211_hw *hw,
						   u16 be_q, u16 bk_q,
						   u16 vi_q, u16 vo_q,
						   u16 mgt_q, u16 hi_q)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u16 value16 = (rtl_read_word(rtlpriv, REG_TRXDMA_CTRL) & 0x7);

	value16 |= _TXDMA_BEQ_MAP(be_q)	| _TXDMA_BKQ_MAP(bk_q) |
		   _TXDMA_VIQ_MAP(vi_q)	| _TXDMA_VOQ_MAP(vo_q) |
		   _TXDMA_MGQ_MAP(mgt_q) | _TXDMA_HIQ_MAP(hi_q);

	rtl_write_word(rtlpriv, REG_TRXDMA_CTRL, value16);
}

static void _rtl88eu_init_normal_chip_one_out_ep_priority(
							struct ieee80211_hw *hw)
{
	struct rtl_usb_priv *usb_priv = rtl_usbpriv(hw);
	struct rtl_usb *rtlusb = rtl_usbdev(usb_priv);
	u16 value = 0;

	switch (rtlusb->out_queue_sel) {
	case TX_SELE_HQ:
		value = QUEUE_HIGH;
		break;
	case TX_SELE_LQ:
		value = QUEUE_LOW;
		break;
	case TX_SELE_NQ:
		value = QUEUE_NORMAL;
		break;
	default:
		break;
	}
	_rtl88eu_init_normal_chip_reg_priority(hw, value, value, value, value,
					       value, value);
}

static void _rtl88eu_init_normal_chip_two_out_ep_priority(
							struct ieee80211_hw *hw)
{
	struct rtl_usb_priv *usb_priv = rtl_usbpriv(hw);
	struct rtl_usb *rtlusb = rtl_usbdev(usb_priv);

	u16 be_q, bk_q, vi_q, vo_q, mgt_q, hi_q;
	u16 value_hi = 0;
	u16 value_low = 0;

	switch (rtlusb->out_queue_sel) {
	case (TX_SELE_HQ | TX_SELE_LQ):
		value_hi = QUEUE_HIGH;
		value_low = QUEUE_LOW;
		break;
	case (TX_SELE_NQ | TX_SELE_LQ):
		value_hi = QUEUE_NORMAL;
		value_low = QUEUE_LOW;
		break;
	case (TX_SELE_HQ | TX_SELE_NQ):
		value_hi = QUEUE_HIGH;
		value_low = QUEUE_NORMAL;
		break;
	default:
		break;
	}

	if (!rtlusb->wmm_enable) {
		be_q	= value_low;
		bk_q	= value_low;
		vi_q	= value_hi;
		vo_q	= value_hi;
		mgt_q	= value_hi;
		hi_q	= value_hi;
	} else {/* for WMM ,CONFIG_OUT_EP_WIFI_MODE */
		be_q	= value_low;
		bk_q	= value_hi;
		vi_q	= value_hi;
		vo_q	= value_low;
		mgt_q	= value_hi;
		hi_q	= value_hi;
	}
	_rtl88eu_init_normal_chip_reg_priority(hw, be_q, bk_q, vi_q, vo_q, mgt_q, hi_q);
}

static void _rtl88eu_init_normal_chip_three_out_ep_priority(
							struct ieee80211_hw *hw)
{
	struct rtl_usb_priv *usb_priv = rtl_usbpriv(hw);
	struct rtl_usb *rtlusb = rtl_usbdev(usb_priv);

	u16 be_q, bk_q, vi_q, vo_q, mgt_q, hi_q;

	if (!rtlusb->wmm_enable) {/* typical setting */
		be_q = QUEUE_LOW;
		bk_q = QUEUE_LOW;
		vi_q = QUEUE_NORMAL;
		vo_q = QUEUE_HIGH;
		mgt_q = QUEUE_HIGH;
		hi_q = QUEUE_HIGH;
	} else {/* for WMM */
		be_q = QUEUE_LOW;
		bk_q = QUEUE_NORMAL;
		vi_q = QUEUE_NORMAL;
		vo_q = QUEUE_HIGH;
		mgt_q = QUEUE_HIGH;
		hi_q = QUEUE_HIGH;
	}
	_rtl88eu_init_normal_chip_reg_priority(hw, be_q, bk_q, vi_q, vo_q,
					       mgt_q, hi_q);
}

static void _rtl88eu_init_queue_priority(struct ieee80211_hw *hw)
{
	struct rtl_usb_priv *usb_priv = rtl_usbpriv(hw);
	struct rtl_usb *rtlusb = rtl_usbdev(usb_priv);

	switch (rtlusb->out_ep_nums) {
	case 1:
		_rtl88eu_init_normal_chip_one_out_ep_priority(hw);
		break;
	case 2:
		_rtl88eu_init_normal_chip_two_out_ep_priority(hw);
		break;
	case 3:
		_rtl88eu_init_normal_chip_three_out_ep_priority(hw);
		break;
	default:
		break;
	}
}

static void _rtl88eu_enable_bcn_sub_func(struct ieee80211_hw *hw)
{
	_rtl88eu_set_bcn_ctrl_reg(hw, 0, BIT(1));
}

static void _rtl88eu_disable_bcn_sub_func(struct ieee80211_hw *hw)
{
	_rtl88eu_set_bcn_ctrl_reg(hw, BIT(1), 0);
}

static void _rtl88eu_resume_tx_beacon(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 tmp1byte;

	tmp1byte = rtl_read_byte(rtlpriv, REG_FWHW_TXQ_CTRL + 2);
	rtl_write_byte(rtlpriv, REG_FWHW_TXQ_CTRL + 2, tmp1byte | BIT(6));
	rtl_write_byte(rtlpriv, REG_TBTT_PROHIBIT + 1, 0xff);
	tmp1byte = rtl_read_byte(rtlpriv, REG_TBTT_PROHIBIT + 2);
	tmp1byte |= BIT(0);
	rtl_write_byte(rtlpriv, REG_TBTT_PROHIBIT + 2, tmp1byte);
}

static void _rtl88eu_stop_tx_beacon(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 tmp1byte;

	tmp1byte = rtl_read_byte(rtlpriv, REG_FWHW_TXQ_CTRL + 2);
	rtl_write_byte(rtlpriv, REG_FWHW_TXQ_CTRL + 2, tmp1byte & (~BIT(6)));
	rtl_write_byte(rtlpriv, REG_TBTT_PROHIBIT + 1, 0x64);
	tmp1byte = rtl_read_byte(rtlpriv, REG_TBTT_PROHIBIT + 2);
	tmp1byte &= ~(BIT(0));
	rtl_write_byte(rtlpriv, REG_TBTT_PROHIBIT + 2, tmp1byte);
}

static int _rtl88eu_set_media_status(struct ieee80211_hw *hw,
				     enum nl80211_iftype type)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 bt_msr = rtl_read_byte(rtlpriv, MSR);
	enum led_ctl_mode ledaction = LED_CTL_NO_LINK;

	rtl_write_byte(rtlpriv, REG_BCN_CTRL,
		       rtl_read_byte(rtlpriv, REG_BCN_CTRL) | BIT(4));
	bt_msr &= 0x0c;
	switch (type) {
	case NL80211_IFTYPE_UNSPECIFIED:
		bt_msr |= MSR_NOLINK;
		ledaction = LED_CTL_LINK;
		RT_TRACE(rtlpriv, COMP_ERR, DBG_TRACE,
			 "Set Network type to NO LINK!\n");
		break;
	case NL80211_IFTYPE_ADHOC:
		bt_msr |= MSR_ADHOC;
		RT_TRACE(rtlpriv, COMP_ERR, DBG_TRACE,
			 "Set Network type to Ad Hoc!\n");
		break;
	case NL80211_IFTYPE_STATION:
		bt_msr |= MSR_INFRA;
		ledaction = LED_CTL_LINK;
		RT_TRACE(rtlpriv, COMP_ERR, DBG_TRACE,
			 "Set Network type to STA!\n");
		break;
	case NL80211_IFTYPE_AP:
		bt_msr |= MSR_AP;
		RT_TRACE(rtlpriv, COMP_ERR, DBG_TRACE,
			 "Set Network type to AP!\n");
		break;
	default:
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "Network type %d not supported!\n", type);
		goto error_out;
	}
	rtl_write_byte(rtlpriv, MSR, bt_msr);
	rtlpriv->cfg->ops->led_control(hw, ledaction);

	if (type == NL80211_IFTYPE_UNSPECIFIED ||
	    type == NL80211_IFTYPE_STATION) {
		_rtl88eu_stop_tx_beacon(hw);
		rtl_write_byte(rtlpriv, REG_BCN_CTRL, 0x19);
	} else if (type == NL80211_IFTYPE_ADHOC) {
		_rtl88eu_resume_tx_beacon(hw);
		rtl_write_byte(rtlpriv, REG_BCN_CTRL, 0x1a);
	} else if (type == NL80211_IFTYPE_AP) {
		_rtl88eu_resume_tx_beacon(hw);
		rtl_write_byte(rtlpriv, REG_BCN_CTRL, 0x12);
		rtl_write_dword(rtlpriv, REG_RCR, 0x7000208e);
		rtl_write_word(rtlpriv, REG_RXFLTMAP2, 0xffff);
		rtl_write_word(rtlpriv, REG_RXFLTMAP1, 0x0400);
		rtl_write_byte(rtlpriv, REG_BCNDMATIM, 0x02);
		rtl_write_byte(rtlpriv, REG_ATIMWND, 0x0a);
		rtl_write_word(rtlpriv, REG_BCNTCFG, 0x00);
		rtl_write_word(rtlpriv, REG_TBTT_PROHIBIT, 0xff04);
		rtl_write_word(rtlpriv, REG_TSFTR_SYN_OFFSET, 0x7fff);
		rtl_write_byte(rtlpriv, REG_DUAL_TSF_RST, BIT(0));
		rtl_write_byte(rtlpriv, REG_MBID_NUM,
			       rtl_read_byte(rtlpriv, REG_MBID_NUM) |
					     BIT(3) | BIT(4));
		rtl_write_byte(rtlpriv, REG_BCN_CTRL,
			       (DIS_TSF_UDT0_NORMAL_CHIP|EN_BCN_FUNCTION |
				BIT(1)));
		rtl_write_byte(rtlpriv, REG_BCN_CTRL_1,
			       rtl_read_byte(rtlpriv, REG_BCN_CTRL_1) | BIT(0));

	} else {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
			 "Set HW_VAR_MEDIA_STATUS:No such media status(%x)\n",
			 type);
	}
	return 0;
error_out:
	return 1;
}

void rtl88eu_set_check_bssid(struct ieee80211_hw *hw, bool check_bssid)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 reg_rcr;

	if (rtlpriv->psc.rfpwr_state != ERFON)
		return;

	rtlpriv->cfg->ops->get_hw_reg(hw, HW_VAR_RCR, (u8 *)(&reg_rcr));

	if (check_bssid) {
		u8 tmp;
		reg_rcr |= (RCR_CBSSID_DATA | RCR_CBSSID_BCN);
		tmp = BIT(4);
		rtlpriv->cfg->ops->set_hw_reg(hw, HW_VAR_RCR,
					      (u8 *) (&reg_rcr));
		_rtl88eu_set_bcn_ctrl_reg(hw, 0, tmp);
	} else {
		u8 tmp;
		reg_rcr &= ~(RCR_CBSSID_DATA | RCR_CBSSID_BCN);
		tmp = BIT(4);
		reg_rcr &= (~(RCR_CBSSID_DATA | RCR_CBSSID_BCN));
		rtlpriv->cfg->ops->set_hw_reg(hw,
					      HW_VAR_RCR, (u8 *) (&reg_rcr));
		_rtl88eu_set_bcn_ctrl_reg(hw, tmp, 0);
	}
}

int rtl88eu_set_network_type(struct ieee80211_hw *hw, enum nl80211_iftype type)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	if (_rtl88eu_set_media_status(hw, type))
		return -EOPNOTSUPP;

	if (rtlpriv->mac80211.link_state == MAC80211_LINKED) {
		if (type != NL80211_IFTYPE_AP)
			rtlpriv->cfg->ops->set_chk_bssid(hw, true);
	} else {
		rtlpriv->cfg->ops->set_chk_bssid(hw, false);
	}

	return 0;
}

#if 0
static void _InitNetworkType(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 value32;

	value32 = rtl_read_dword(rtlpriv, REG_CR);
	/*  TODO: use the other function to set network type */
	value32 = (value32 & ~MASK_NETTYPE) | _NETTYPE(NT_LINK_AP);

	rtl_write_dword(rtlpriv, REG_CR, value32);
}
#endif

static void _rtl88eu_init_transfer_page_size(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 value8;

	value8 = _PSRX(PBP_128) | _PSTX(PBP_128);
	rtl_write_byte(rtlpriv, REG_PBP, value8);
}

static void _rtl88eu_init_driver_info_size(struct ieee80211_hw *hw,
					   u8 drv_info_size)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	rtl_write_byte(rtlpriv, REG_RX_DRVINFO_SZ, drv_info_size);
}

static void _rtl88eu_init_wmac_setting(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));

	mac->rx_conf = RCR_AAP | RCR_APM | RCR_AM | RCR_AB |
				  RCR_CBSSID_DATA | RCR_CBSSID_BCN |
				  RCR_APP_ICV | RCR_AMF | RCR_HTC_LOC_CTRL |
				  RCR_APP_MIC | RCR_APP_PHYSTS;

	rtl_write_dword(rtlpriv, REG_RCR, mac->rx_conf);

	/* Accept all multicast address */
	rtl_write_dword(rtlpriv, REG_MAR, 0xFFFFFFFF);
	rtl_write_dword(rtlpriv, REG_MAR + 4, 0xFFFFFFFF);
}

static void _rtl88eu_init_adaptive_ctrl(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u16 value16;
	u32 value32;

	/* Response Rate Set */
	value32 = rtl_read_dword(rtlpriv, REG_RRSR);
	value32 &= ~RATE_BITMAP_ALL;
	value32 |= RATE_RRSR_CCK_ONLY_1M;
	rtl_write_dword(rtlpriv, REG_RRSR, value32);

	/* CF-END Threshold */

	/* SIFS (used in NAV) */
	value16 = _SPEC_SIFS_CCK(0x10) | _SPEC_SIFS_OFDM(0x10);
	rtl_write_word(rtlpriv, REG_SPEC_SIFS, value16);

	/* Retry Limit */
	value16 = _LRL(0x30) | _SRL(0x30);
	rtl_write_word(rtlpriv, REG_RL, value16);
}

static void _rtl88eu_init_edca(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	/*  Set Spec SIFS (used in NAV) */
	rtl_write_word(rtlpriv, REG_SPEC_SIFS, 0x100a);
	rtl_write_word(rtlpriv, REG_MAC_SPEC_SIFS, 0x100a);

	/*  Set SIFS for CCK */
	rtl_write_word(rtlpriv, REG_SIFS_CTX, 0x100a);

	/*  Set SIFS for OFDM */
	rtl_write_word(rtlpriv, REG_SIFS_TRX, 0x100a);

	/*  TXOP */
	rtl_write_dword(rtlpriv, REG_EDCA_BE_PARAM, 0x005EA42B);
	rtl_write_dword(rtlpriv, REG_EDCA_BK_PARAM, 0x0000A44F);
	rtl_write_dword(rtlpriv, REG_EDCA_VI_PARAM, 0x005EA324);
	rtl_write_dword(rtlpriv, REG_EDCA_VO_PARAM, 0x002FA226);
}

#if 0
static void _InitRDGSetting(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	rtl_write_byte(rtlpriv, REG_RD_CTRL, 0xFF);
	rtl_write_word(rtlpriv, REG_RD_NAV_NXT, 0x200);
	rtl_write_byte(rtlpriv, REG_RD_RESP_PKT_TH, 0x05);
}
#endif

static void _rtl88eu_init_retry_function(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 value8;

	value8 = rtl_read_byte(rtlpriv, REG_FWHW_TXQ_CTRL);
	value8 |= EN_AMPDU_RTY_NEW;
	rtl_write_byte(rtlpriv, REG_FWHW_TXQ_CTRL, value8);

	/* Set ACK timeout */
	rtl_write_byte(rtlpriv, REG_ACKTO, 0x40);
}

static void _rtl88eu_agg_setting_tx_update(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_usb_priv *usb_priv = rtl_usbpriv(hw);
	struct rtl_usb *rtlusb = rtl_usbdev(usb_priv);

	u32 value32;

	if (rtlusb->wmm_enable)
		rtlusb->usb_tx_agg_mode = false;

	if (rtlusb->usb_tx_agg_mode) {
		value32 = rtl_read_dword(rtlpriv, REG_TDECTRL);
		value32 = value32 & ~(BLK_DESC_NUM_MASK << BLK_DESC_NUM_SHIFT);
		value32 |= ((6 & BLK_DESC_NUM_MASK) << BLK_DESC_NUM_SHIFT);

		rtl_write_dword(rtlpriv, REG_TDECTRL, value32);
	}
}

static void _rtl88eu_agg_setting_rx_update( struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_usb_priv *usb_priv = rtl_usbpriv(hw);
	struct rtl_usb *rtlusb = rtl_usbdev(usb_priv);

	u8 value_dma;
	u8 value_usb;

	value_dma = rtl_read_byte(rtlpriv, REG_TRXDMA_CTRL);
	value_usb = rtl_read_byte(rtlpriv, REG_USB_SPECIAL_OPTION);

	/* TODO */
	rtlusb->usb_rx_agg_mode = USB_RX_AGG_DMA;

	switch (rtlusb->usb_rx_agg_mode) {
	case USB_RX_AGG_DMA:
		value_dma |= RXDMA_AGG_EN;
		value_usb &= ~USB_AGG_EN;
		break;
	case USB_RX_AGG_USB:
		value_dma &= ~RXDMA_AGG_EN;
		value_usb |= USB_AGG_EN;
		break;
	case USB_RX_AGG_DMA_USB:
		value_dma |= RXDMA_AGG_EN;
		value_usb |= USB_AGG_EN;
		break;
	case USB_RX_AGG_DISABLE:
	default:
		value_dma &= ~RXDMA_AGG_EN;
		value_usb &= ~USB_AGG_EN;
		break;
	}

	rtl_write_byte(rtlpriv, REG_TRXDMA_CTRL, value_dma);
	rtl_write_byte(rtlpriv, REG_USB_SPECIAL_OPTION, value_usb);

	switch (rtlusb->usb_rx_agg_mode) {
	case USB_RX_AGG_DMA:
		rtl_write_byte(rtlpriv, REG_RXDMA_AGG_PG_TH, 48);
		rtl_write_byte(rtlpriv, REG_RXDMA_AGG_PG_TH+1, 4);
		break;
	case USB_RX_AGG_USB:
		rtl_write_byte(rtlpriv, REG_USB_AGG_TH, 8);
		rtl_write_byte(rtlpriv, REG_USB_AGG_TO, 6);
		break;
	case USB_RX_AGG_DMA_USB:
		rtl_write_byte(rtlpriv, REG_RXDMA_AGG_PG_TH, 48);
		rtl_write_byte(rtlpriv, REG_RXDMA_AGG_PG_TH+1, (4 & 0x1F));
		rtl_write_byte(rtlpriv, REG_USB_AGG_TH, 8);
		rtl_write_byte(rtlpriv, REG_USB_AGG_TO, 6);
		break;
	case USB_RX_AGG_DISABLE:
	default:
		break;
	}
}

static void _rtl88eu_init_agg_setting(struct ieee80211_hw *hw)
{

	/* TODO */
#if 1
	_rtl88eu_agg_setting_tx_update(hw);
	_rtl88eu_agg_setting_rx_update(hw);
	/* haldata->UsbRxHighSpeedMode = false; */
#endif
}

static void _rtl88eu_init_beacon_parameters(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_usb_priv *usb_priv = rtl_usbpriv(hw);
	struct rtl_usb *rtlusb = rtl_usbdev(usb_priv);

	rtl_write_word(rtlpriv, REG_BCN_CTRL, 0x1010);
	rtl_write_word(rtlpriv, REG_TBTT_PROHIBIT, 0x6404);
	rtl_write_byte(rtlpriv, REG_DRVERLYINT, DRIVER_EARLY_INT_TIME);
	rtl_write_byte(rtlpriv, REG_BCNDMATIM, BCN_DMA_ATIME_INT_TIME);
	rtl_write_word(rtlpriv, REG_BCNTCFG, 0x660f);
	rtlusb->reg_bcn_ctrl_val = rtl_read_byte(rtlpriv, REG_BCN_CTRL);
}

static void _rtl88eu_beacon_function_enable(struct ieee80211_hw *hw)
	
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	rtl_write_byte(rtlpriv, REG_BCN_CTRL, (BIT(4) | BIT(3) | BIT(1)));
	rtl_write_byte(rtlpriv, REG_RD_CTRL + 1, 0x6F);
}

static void _rtl88eu_bb_turn_on_block(struct ieee80211_hw *hw)
{
	rtl_set_bbreg(hw, RFPGA0_RFMOD, BCCKEN, 0x1);
	rtl_set_bbreg(hw, RFPGA0_RFMOD, BOFDMEN, 0x1);
}

static void _rtl88eu_init_ant_selection(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));

	if (rtlefuse->antenna_div_cfg == 0)
		return;

	rtl_write_dword(rtlpriv, REG_LEDCFG0,
			rtl_read_dword(rtlpriv, REG_LEDCFG0)|BIT(23));
	rtl_set_bbreg(hw, RFPGA0_XAB_RFPARAMETER, BIT(13), 0x01);
}

enum rf_pwrstate rtl88eu_rf_on_off_detect(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_ps_ctl *ppsc = rtl_psc(rtl_priv(hw));
	u8 val8;
	enum rf_pwrstate rfpowerstate = ERFOFF;

	if (ppsc->pwrdown_mode) {
		val8 = rtl_read_byte(rtlpriv, REG_HSISR);
		RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
			 "pwrdown, 0x5c(BIT(7))=%02x\n", val8);
		rfpowerstate = (val8 & BIT(7)) ? ERFOFF : ERFON;
	} else {
		rtl_write_byte(rtlpriv, REG_MAC_PINMUX_CFG,
			       rtl_read_byte(rtlpriv, REG_MAC_PINMUX_CFG) &
					     ~(BIT(3)));
		val8 = rtl_read_byte(rtlpriv, REG_GPIO_IO_SEL);
		RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,"GPIO_IN=%02x\n", val8);
		rfpowerstate = (val8 & BIT(3)) ? ERFON: ERFOFF;
	}
	return rfpowerstate;
}

static int _rtl88eu_init_mac(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_usb_priv *usb_priv = rtl_usbpriv(hw);
	struct rtl_usb *rtlusb = rtl_usbdev(usb_priv);
	int err = 0;
	u32 boundary = 0;
	
	err = _rtl88eu_init_power_on(hw);
	if (err == false) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "Failed to init power on!\n");
		return err;
	}

	if (!rtlusb->wmm_enable)
		boundary = TX_PAGE_BOUNDARY_88E;
	else
		boundary = WMM_NORMAL_TX_PAGE_BOUNDARY_88E;

	if (false == rtl88eu_init_llt_table(hw, boundary)) {
	
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "Failed to init LLT table\n");
		return -EINVAL;
	}

	_rtl88eu_init_queue_reserved_page(hw);
	_rtl88eu_init_txbuffer_boundary(hw, 0);
	_rtl88eu_init_page_boundary(hw);
	_rtl88eu_init_transfer_page_size(hw);
	_rtl88eu_init_queue_priority(hw);
	_rtl88eu_init_driver_info_size(hw, DRVINFO_SZ);
	_rtl88eu_init_interrupt(hw);
	/* TODO */
#if 0
	_InitNetworkType(hw);/* set msr */
#endif
	_rtl88eu_init_wmac_setting(hw);
	_rtl88eu_init_adaptive_ctrl(hw);
	_rtl88eu_init_edca(hw);
	_rtl88eu_init_retry_function(hw);
	/* TODO */
#if 0
	_rtl88eu_init_agg_setting(hw);
#endif
	rtlpriv->cfg->ops->set_bw_mode(hw, NL80211_CHAN_HT20);
	_rtl88eu_init_beacon_parameters(hw);
	_rtl88eu_init_txbuffer_boundary(hw, boundary);
	return 0;
}

int rtl88eu_hw_init(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	struct rtl_phy *rtlphy = rtl_phy(rtlpriv);
	struct rtl_ps_ctl *ppsc = rtl_psc(rtl_priv(hw));
	struct rtl_usb_priv *usb_priv = rtl_usbpriv(hw);
	struct rtl_usb *rtlusb = rtl_usbdev(usb_priv);
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	u8 value8 = 0;
	u16 value16;
	u32 status = true;
	unsigned long flags;
	/* TODO */
#if 1
	rtlusb->wmm_enable = false;
#else
	rtlusb->wmm_enable = true;
#endif

	local_save_flags(flags);
	local_irq_disable();
	rtlhal->fw_ready = false;
	rtlhal->hw_type = HARDWARE_TYPE_RTL8188EU;
	status = _rtl88eu_init_mac(hw);
	if (status) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG, "init mac failed!\n");
		goto exit;
	}
	status = rtl88eu_download_fw(hw, false);
	if (status) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "Download Firmware failed!!\n");
		status = true;
		return status;
	}
	RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
		 "Download Firmware Success!!\n");
	local_irq_enable();
#if 0
	rtlhal->fw_ready = false;
#else
	rtlhal->fw_ready = true;
#endif
	rtlhal->last_hmeboxnum = 0;
	rtlphy->iqk_initialized = false;
	rtlphy->pwrgroup_cnt = 0;
	rtlhal->fw_ps_state = FW_PS_STATE_ALL_ON_88E;
	rtlhal->fw_clk_change_in_progress = false;
	rtlhal->allow_sw_to_change_hwclc = false;
	ppsc->fw_current_inpsmode = false;

	rtl88e_phy_mac_config(hw);
	rtl88e_phy_bb_config(hw);
	rtlphy->rf_mode = RF_OP_BY_SW_3WIRE;
	rtl88e_phy_rf_config(hw);
	rtlphy->rfreg_chnlval[0] = rtl_get_rfreg(hw, (enum radio_path)0,
						 RF_CHNLBW, RFREG_OFFSET_MASK);
	rtlphy->rfreg_chnlval[1] = rtl_get_rfreg(hw, (enum radio_path)1,
						 RF_CHNLBW, RFREG_OFFSET_MASK);
	status = rtl88eu_iol_efuse_patch(hw);
	if (status == false) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "rtl88eu_iol_efuse_patch failed\n");
		goto exit;
	}

	value16 = rtl_read_word(rtlpriv, REG_CR);
	value16 |= (MACTXEN | MACRXEN);
	rtl_write_byte(rtlpriv, REG_CR, value16);

	value8 = rtl_read_byte(rtlpriv, REG_TX_RPT_CTRL);
	rtl_write_byte(rtlpriv,  REG_TX_RPT_CTRL, (value8 | BIT(1) | BIT(0)));
	rtl_write_byte(rtlpriv,  REG_TX_RPT_CTRL+1, 2);
	rtl_write_word(rtlpriv, REG_TX_RPT_TIME, 0xCdf0);
	rtl_write_byte(rtlpriv, REG_EARLY_MODE_CONTROL, 0);
	rtl_write_word(rtlpriv, REG_PKT_VO_VI_LIFE_TIME, 0x0400);
	rtl_write_word(rtlpriv, REG_PKT_BE_BK_LIFE_TIME, 0x0400);
	_rtl88eu_bb_turn_on_block(hw);
	rtl_cam_reset_all_entry(hw);
	rtlpriv->cfg->ops->enable_hw_sec(hw);
	ppsc->rfpwr_state = ERFON;
	rtlpriv->cfg->ops->set_hw_reg(hw, HW_VAR_ETHER_ADDR, mac->mac_addr);
	rtl88e_phy_set_txpower_level(hw, rtlphy->current_channel);
	_rtl88eu_init_ant_selection(hw);
	rtl_write_dword(rtlpriv, REG_BAR_MODE_CTRL, 0x0201ffff);
	rtl_write_byte(rtlpriv, REG_HWSEQ_CTRL, 0xFF);

	if (rtlusb->wmm_enable)
		rtl_write_word(rtlpriv, REG_FAST_EDCA_CTRL, 0);
	rtl_write_byte(rtlpriv, 0x652, 0x0);
	rtl_write_byte(rtlpriv,  REG_FWHW_TXQ_CTRL+1, 0x0F);
	rtl_write_byte(rtlpriv, REG_EARLY_MODE_CONTROL+3, 0x01);
	rtl_write_word(rtlpriv, REG_TX_RPT_TIME, 0x3DF0);
	rtl_write_word(rtlpriv, REG_TXDMA_OFFSET_CHK,
		       (rtl_read_word(rtlpriv, REG_TXDMA_OFFSET_CHK) |
			DROP_DATA_EN));
	if (ppsc->rfpwr_state == ERFON) {
		if ((rtlefuse->antenna_div_type == CGCS_RX_HW_ANTDIV) ||
		    ((rtlefuse->antenna_div_type == CG_TRX_HW_ANTDIV) &&
		     (rtlhal->oem_id == RT_CID_819X_HP))) {
			rtl88e_phy_set_rfpath_switch(hw, true);
			rtlpriv->dm.fat_table.rx_idle_ant = MAIN_ANT;
		} else {
			rtl88e_phy_set_rfpath_switch(hw, false);
			rtlpriv->dm.fat_table.rx_idle_ant = AUX_ANT;
		}
		RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD, "rx idle ant %s\n",
			 (rtlpriv->dm.fat_table.rx_idle_ant == MAIN_ANT) ?
			 ("MAIN_ANT") : ("AUX_ANT"));

		if (rtlphy->iqk_initialized) {
				rtl88e_phy_iq_calibrate(hw, true);
		} else {
			rtl88e_phy_iq_calibrate(hw, false);
			rtlphy->iqk_initialized = true;
		}
		rtl88e_dm_check_txpower_tracking(hw);
		rtlpriv->cfg->ops->phy_lc_calibrate(hw, false);
	}

	rtl_write_byte(rtlpriv, REG_USB_HRPWM, 0);
	rtl_write_dword(rtlpriv, REG_FWHW_TXQ_CTRL,
			rtl_read_dword(rtlpriv, REG_FWHW_TXQ_CTRL)|BIT(12));
	rtl88e_dm_init(hw);
	rtl88eu_hal_notch_filter(hw, 0);
	local_irq_restore(flags);
	return 0;
exit:
	local_irq_restore(flags);
	rtlpriv->rtlhal.being_init_adapter = false;
	return status;
}

static void _rtl88eu_card_disable(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	u8 val8;

	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD, "rtl8188eu card disable\n");

	/* Stop Tx Report Timer. 0x4EC[Bit1]=b'0 */
	val8 = rtl_read_byte(rtlpriv, REG_TX_RPT_CTRL);
	rtl_write_byte(rtlpriv, REG_TX_RPT_CTRL, val8&(~BIT(1)));

	/*  stop rx */
	rtl_write_byte(rtlpriv, REG_CR, 0x0);

	/*  Run LPS WL RFOFF flow */
	rtl_hal_pwrseqcmdparsing(rtlpriv, PWR_CUT_ALL_MSK,
				 PWR_FAB_ALL_MSK, PWR_INTF_USB_MSK,
				 RTL8188EE_NIC_LPS_ENTER_FLOW);

	/*  2. 0x1F[7:0] = 0 turn off RF */
	val8 = rtl_read_byte(rtlpriv, REG_MCUFWDL);
	if ((val8 & RAM_DL_SEL) && rtlhal->fw_ready) { /* 8051 RAM code */
		/* Reset MCU 0x2[10]=0. */
		val8 = rtl_read_byte(rtlpriv, REG_SYS_FUNC_EN+1);
		/*  0x2[10], FEN_CPUEN */
		val8 &= ~BIT(2);
		rtl_write_byte(rtlpriv, REG_SYS_FUNC_EN+1, val8);
	}

	/* reset MCU ready status */
	rtl_write_byte(rtlpriv, REG_MCUFWDL, 0);

	/* Disable 32k */
	val8 = rtl_read_byte(rtlpriv, REG_32K_CTRL);
	rtl_write_byte(rtlpriv, REG_32K_CTRL, val8&(~BIT(0)));

	/*  Card disable power action flow */
	rtl_hal_pwrseqcmdparsing(rtlpriv, PWR_CUT_ALL_MSK,
				 PWR_FAB_ALL_MSK, PWR_INTF_USB_MSK,
				 RTL8188EE_NIC_DISABLE_FLOW);

	/*  Reset MCU IO Wrapper */
	val8 = rtl_read_byte(rtlpriv, REG_RSV_CTRL+1);
	rtl_write_byte(rtlpriv, REG_RSV_CTRL+1, (val8&(~BIT(3))));
	val8 = rtl_read_byte(rtlpriv, REG_RSV_CTRL+1);
	rtl_write_byte(rtlpriv, REG_RSV_CTRL+1, val8|BIT(3));

	val8 = rtl_read_byte(rtlpriv, GPIO_IN);
	rtl_write_byte(rtlpriv, GPIO_OUT, val8);
	rtl_write_byte(rtlpriv, GPIO_IO_SEL, 0xFF);

	val8 = rtl_read_byte(rtlpriv, REG_GPIO_IO_SEL);
	rtl_write_byte(rtlpriv, REG_GPIO_IO_SEL, (val8<<4));
	val8 = rtl_read_byte(rtlpriv, REG_GPIO_IO_SEL+1);
	rtl_write_byte(rtlpriv, REG_GPIO_IO_SEL+1, val8|0x0F);
	/* set LNA ,TRSW,EX_PA Pin to output mode */
	rtl_write_dword(rtlpriv, REG_BB_PAD_CTRL, 0x00080808);
	/* TODO */
#if 0
	haldata->bMacPwrCtrlOn = false;
#endif
	rtlhal->fw_ready = false;
}
static void _rtl88eu_hw_power_down(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	/*  we need to enable register block contrl reg at 0x1c.
	 *  Then enable power down control bit of register 0x04 BIT(4)
	 *  and BIT(15) as 1. */

	/*  Enable register area 0x0-0xc. */
	rtl_write_byte(rtlpriv, REG_RSV_CTRL, 0x0);
	rtl_write_word(rtlpriv, REG_APS_FSMCO, 0x8812);
}

void rtl88eu_card_disable(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_ps_ctl *ppsc = rtl_psc(rtl_priv(hw));
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	enum nl80211_iftype opmode;

	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD, "rtl8188eu card disable\n");
	mac->link_state = MAC80211_NOLINK;
	opmode = NL80211_IFTYPE_UNSPECIFIED;

	_rtl88eu_set_media_status(hw, opmode);

	rtlpriv->cfg->ops->led_control(hw, LED_CTL_POWER_OFF);
	RT_SET_PS_LEVEL(ppsc, RT_RF_OFF_LEVL_HALT_NIC);

	rtl_write_dword(rtlpriv, REG_HIMR_88E, IMR_DISABLED_88E);
	rtl_write_dword(rtlpriv, REG_HIMRE_88E, IMR_DISABLED_88E);

	_rtl88eu_card_disable(hw);
	/* TODO */
#if 0
	_rtl88eu_hw_power_down(hw);
#endif
	rtlpriv->phy.iqk_initialized = false;
}

void rtl88eu_read_eeprom_info(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	u8 eeValue;

	/* check system boot selection */
	eeValue = rtl_read_byte(rtlpriv, REG_9346CR);
	
	if (eeValue & BIT(4)) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_DMESG, "Boot from EEPROM\n");
		rtlefuse->epromtype = EEPROM_93C46;
	} else {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_DMESG, "Boot from EFUSE\n");
		rtlefuse->epromtype = EEPROM_BOOT_EFUSE;
	}
	if (eeValue & BIT(5)) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_LOUD, "Autoload OK\n");
		rtlefuse->autoload_failflag = false;
	} else {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG, "Autoload ERR!!\n");
	}

	_rtl88eu_read_adapter_info(hw);
}

void rtl88eu_get_hw_reg(struct ieee80211_hw *hw, u8 variable, u8 *val)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_ps_ctl *ppsc = rtl_psc(rtl_priv(hw));

	switch (variable) {
	case HW_VAR_RCR:
		*((u32 *) (val)) = rtl_read_dword(rtlpriv, REG_RCR);
		break;
	case HW_VAR_RF_STATE:
		*((enum rf_pwrstate *)(val)) = ppsc->rfpwr_state;
		break;
	case HW_VAR_FWLPS_RF_ON:{
		enum rf_pwrstate rfstate;
		u32 val_rcr;

		rtlpriv->cfg->ops->get_hw_reg(hw, HW_VAR_RF_STATE,
					      (u8 *)(&rfstate));
		if (rfstate == ERFOFF) {
			*((bool *)(val)) = true;
		} else {
			val_rcr = rtl_read_dword(rtlpriv, REG_RCR);
			val_rcr &= 0x00070000;
			if (val_rcr)
				*((bool *)(val)) = false;
			else
				*((bool *)(val)) = true;
		}
		break; }
	case HW_VAR_FW_PSMODE_STATUS:
		*((bool *)(val)) = ppsc->fw_current_inpsmode;
		break;
	case HW_VAR_CORRECT_TSF:{
		u64 tsf;
		u32 *ptsf_low = (u32 *)&tsf;
		u32 *ptsf_high = ((u32 *)&tsf) + 1;

		*ptsf_high = rtl_read_dword(rtlpriv, (REG_TSFTR + 4));
		*ptsf_low = rtl_read_dword(rtlpriv, REG_TSFTR);

		*((u64 *)(val)) = tsf;
		break; }
	default:
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "switch case not process %x\n", variable);
		break;
	}
}

void rtl88eu_set_hw_reg(struct ieee80211_hw *hw, u8 variable, u8 *val)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	struct rtl_ps_ctl *ppsc = rtl_psc(rtl_priv(hw));
	struct rtl_usb_priv *usb_priv = rtl_usbpriv(hw);
	struct rtl_usb *rtlusb = rtl_usbdev(usb_priv);

	u8 idx;

	switch (variable) {
	case HW_VAR_ETHER_ADDR:
		for (idx = 0; idx < ETH_ALEN; idx++) {
			rtl_write_byte(rtlpriv, (REG_MACID + idx),
				       val[idx]);
		}
		break;
	case HW_VAR_BASIC_RATE:{
		u16 b_rate_cfg = ((u16 *)val)[0];
		u8 rate_index = 0;
		b_rate_cfg = b_rate_cfg & 0x15f;
		b_rate_cfg |= 0x01;
		rtl_write_byte(rtlpriv, REG_RRSR, b_rate_cfg & 0xff);
		rtl_write_byte(rtlpriv, REG_RRSR + 1,
			       (b_rate_cfg >> 8) & 0xff);
		while (b_rate_cfg > 0x1) {
			b_rate_cfg = (b_rate_cfg >> 1);
			rate_index++;
		}
		rtl_write_byte(rtlpriv, REG_INIRTS_RATE_SEL,
			       rate_index);
		break;
		}
	case HW_VAR_BSSID:
		for (idx = 0; idx < ETH_ALEN; idx++) {
			rtl_write_byte(rtlpriv, (REG_BSSID + idx),
				       val[idx]);
		}
		break;
	case HW_VAR_SIFS:
		rtl_write_byte(rtlpriv, REG_SIFS_CTX + 1, val[0]);
		rtl_write_byte(rtlpriv, REG_SIFS_TRX + 1, val[1]);

		rtl_write_byte(rtlpriv, REG_SPEC_SIFS + 1, val[0]);
		rtl_write_byte(rtlpriv, REG_MAC_SPEC_SIFS + 1, val[0]);

		if (!mac->ht_enable)
			rtl_write_word(rtlpriv, REG_RESP_SIFS_OFDM,
				       0x0e0e);
		else
			rtl_write_word(rtlpriv, REG_RESP_SIFS_OFDM,
				       *((u16 *)val));
		break;
	case HW_VAR_SLOT_TIME:{
		u8 e_aci;

		RT_TRACE(rtlpriv, COMP_MLME, DBG_LOUD,
			 "HW_VAR_SLOT_TIME %x\n", val[0]);

		rtl_write_byte(rtlpriv, REG_SLOT, val[0]);

		for (e_aci = 0; e_aci < AC_MAX; e_aci++) {
			rtlpriv->cfg->ops->set_hw_reg(hw, HW_VAR_AC_PARAM,
						      &e_aci);
		}
		break;
		}
	case HW_VAR_ACK_PREAMBLE:{
		u8 reg_tmp;
		u8 short_preamble = (bool)*val;
		reg_tmp = rtl_read_byte(rtlpriv, REG_TRXPTCL_CTL+2);
		if (short_preamble) {
			reg_tmp |= 0x02;
			rtl_write_byte(rtlpriv, REG_TRXPTCL_CTL +
				       2, reg_tmp);
		} else {
			reg_tmp |= 0xFD;
			rtl_write_byte(rtlpriv, REG_TRXPTCL_CTL +
				       2, reg_tmp);
		}
		break; }
	case HW_VAR_WPA_CONFIG:
		rtl_write_byte(rtlpriv, REG_SECCFG, *val);
		break;
	case HW_VAR_AMPDU_MIN_SPACE:{
		u8 min_spacing_to_set;
		u8 sec_min_space;

		min_spacing_to_set = *val;
		if (min_spacing_to_set <= 7) {
			sec_min_space = 0;

			if (min_spacing_to_set < sec_min_space)
				min_spacing_to_set = sec_min_space;

			mac->min_space_cfg = ((mac->min_space_cfg &
					       0xf8) |
					      min_spacing_to_set);

			*val = min_spacing_to_set;

			RT_TRACE(rtlpriv, COMP_MLME, DBG_LOUD,
				 "Set HW_VAR_AMPDU_MIN_SPACE: %#x\n",
				  mac->min_space_cfg);

			rtl_write_byte(rtlpriv, REG_AMPDU_MIN_SPACE,
				       mac->min_space_cfg);
		}
		break; }
	case HW_VAR_SHORTGI_DENSITY:{
		u8 density_to_set;

		density_to_set = *val;
		mac->min_space_cfg |= (density_to_set << 3);

		RT_TRACE(rtlpriv, COMP_MLME, DBG_LOUD,
			 "Set HW_VAR_SHORTGI_DENSITY: %#x\n",
			  mac->min_space_cfg);

		rtl_write_byte(rtlpriv, REG_AMPDU_MIN_SPACE,
			       mac->min_space_cfg);
		break;
		}
	case HW_VAR_AMPDU_FACTOR:{
		u8 regtoset_normal[4] = { 0x41, 0xa8, 0x72, 0xb9 };
		u8 factor_toset;
		u8 *p_regtoset = NULL;
		u8 index = 0;

		p_regtoset = regtoset_normal;

		factor_toset = *val;
		if (factor_toset <= 3) {
			factor_toset = (1 << (factor_toset + 2));
			if (factor_toset > 0xf)
				factor_toset = 0xf;

			for (index = 0; index < 4; index++) {
				if ((p_regtoset[index] & 0xf0) >
				    (factor_toset << 4))
					p_regtoset[index] =
					    (p_regtoset[index] & 0x0f) |
					    (factor_toset << 4);

				if ((p_regtoset[index] & 0x0f) >
				    factor_toset)
					p_regtoset[index] =
					    (p_regtoset[index] & 0xf0) |
					    (factor_toset);

				rtl_write_byte(rtlpriv,
					       (REG_AGGLEN_LMT + index),
					       p_regtoset[index]);

			}

			RT_TRACE(rtlpriv, COMP_MLME, DBG_LOUD,
				 "Set HW_VAR_AMPDU_FACTOR: %#x\n",
				  factor_toset);
		}
		break; }
	case HW_VAR_AC_PARAM:{
		u8 e_aci = *val;
		rtl88e_dm_init_edca_turbo(hw);

		if (rtlusb->acm_method != EACMWAY2_SW)
			rtlpriv->cfg->ops->set_hw_reg(hw,
						      HW_VAR_ACM_CTRL,
						      &e_aci);
		break; }
	case HW_VAR_ACM_CTRL:{
		u8 e_aci = *val;
		union aci_aifsn *p_aci_aifsn =
		    (union aci_aifsn *)(&(mac->ac[0].aifs));
		u8 acm = p_aci_aifsn->f.acm;
		u8 acm_ctrl = rtl_read_byte(rtlpriv, REG_ACMHWCTRL);

		acm_ctrl = acm_ctrl |
			   ((rtlusb->acm_method == 2) ? 0x0 : 0x1);

		if (acm) {
			switch (e_aci) {
			case AC0_BE:
				acm_ctrl |= ACMHW_BEQEN;
				break;
			case AC2_VI:
				acm_ctrl |= ACMHW_VIQEN;
				break;
			case AC3_VO:
				acm_ctrl |= ACMHW_VOQEN;
				break;
			default:
				RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
					 "HW_VAR_ACM_CTRL acm set failed: eACI is %d\n",
					 acm);
				break;
			}
		} else {
			switch (e_aci) {
			case AC0_BE:
				acm_ctrl &= (~ACMHW_BEQEN);
				break;
			case AC2_VI:
				acm_ctrl &= (~ACMHW_VIQEN);
				break;
			case AC3_VO:
				acm_ctrl &= (~ACMHW_VOQEN);
				break;
			default:
				RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
					 "switch case not process\n");
				break;
			}
		}

		RT_TRACE(rtlpriv, COMP_QOS, DBG_TRACE,
			 "SetHwReg8190usb(): [HW_VAR_ACM_CTRL] Write 0x%X\n",
			 acm_ctrl);
		rtl_write_byte(rtlpriv, REG_ACMHWCTRL, acm_ctrl);
		break; }
	case HW_VAR_RCR:
		rtl_write_dword(rtlpriv, REG_RCR, ((u32 *)(val))[0]);
		mac->rx_conf = ((u32 *)(val))[0];
		break;
	case HW_VAR_RETRY_LIMIT:{
		u8 retry_limit = *val;

		rtl_write_word(rtlpriv, REG_RL,
			       retry_limit << RETRY_LIMIT_SHORT_SHIFT |
			       retry_limit << RETRY_LIMIT_LONG_SHIFT);
		break; }
	case HW_VAR_DUAL_TSF_RST:
		rtl_write_byte(rtlpriv, REG_DUAL_TSF_RST, (BIT(0) | BIT(1)));
		break;
	case HW_VAR_EFUSE_BYTES:
		rtlefuse->efuse_usedbytes = *((u16 *)val);
		break;
	case HW_VAR_EFUSE_USAGE:
		rtlefuse->efuse_usedpercentage = *val;
		break;
	case HW_VAR_IO_CMD:
		rtl88e_phy_set_io_cmd(hw, (*(enum io_type *)val));
		break;
	case HW_VAR_SET_RPWM:
		break;
	case HW_VAR_H2C_FW_PWRMODE:
		rtl88eu_set_fw_pwrmode_cmd(hw, *val);
		break;
	case HW_VAR_FW_PSMODE_STATUS:
		ppsc->fw_current_inpsmode = *((bool *)val);
		break;
	case HW_VAR_H2C_FW_JOINBSSRPT:
		rtl88eu_set_fw_joinbss_report_cmd(hw, (*(u8 *)val));
		break;
	case HW_VAR_H2C_FW_P2P_PS_OFFLOAD:
		break;
	case HW_VAR_AID:{
		u16 u2btmp;

		u2btmp = rtl_read_word(rtlpriv, REG_BCN_PSR_RPT);
		u2btmp &= 0xC000;
		rtl_write_word(rtlpriv, REG_BCN_PSR_RPT, (u2btmp |
			       mac->assoc_id));
		break; }
	case HW_VAR_CORRECT_TSF:{
		u8 btype_ibss = *val;

		if (btype_ibss)
			_rtl88eu_stop_tx_beacon(hw);

		_rtl88eu_set_bcn_ctrl_reg(hw, 0, BIT(3));

		rtl_write_dword(rtlpriv, REG_TSFTR,
				(u32)(mac->tsf & 0xffffffff));
		rtl_write_dword(rtlpriv, REG_TSFTR + 4,
				(u32)((mac->tsf >> 32) & 0xffffffff));

		_rtl88eu_set_bcn_ctrl_reg(hw, BIT(3), 0);

		if (btype_ibss)
			_rtl88eu_resume_tx_beacon(hw);
		break; }
	case HW_VAR_KEEP_ALIVE: {
		u8 array[2];

		array[0] = 0xff;
		array[1] = *((u8 *)val);
		rtlpriv->cfg->ops->fill_h2c_cmd(hw, H2C_88E_KEEP_ALIVE_CTRL,
				    2, array);
		break; }
	default:
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "switch case not process %x\n", variable);
		break;
	}
}

void rtl88eu_update_interrupt_mask(struct ieee80211_hw *hw,
				   u32 add_msr, u32 rm_msr)
{
}

void rtl88eu_set_beacon_interval(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	u16 bcn_interval = mac->beacon_interval;

	RT_TRACE(rtlpriv, COMP_BEACON, DBG_DMESG, "beacon_interval:%d\n",
		 bcn_interval);
	rtl_write_word(rtlpriv, REG_BCN_INTERVAL, bcn_interval);
}

void rtl88eu_set_beacon_related_registers(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	u32 value32;
	u32 bcn_ctrl_reg = REG_BCN_CTRL;
	/* reset TSF, enable update TSF, correcting TSF On Beacon */

	/* BCN interval */
	rtl_write_word(rtlpriv, REG_BCN_INTERVAL, mac->beacon_interval);
	rtl_write_byte(rtlpriv, REG_ATIMWND, 0x02);/* 2ms */

	_rtl88eu_init_beacon_parameters(hw);

	rtl_write_byte(rtlpriv, REG_SLOT, 0x09);

	value32 = rtl_read_dword(rtlpriv, REG_TCR);
	value32 &= ~TSFRST;
	rtl_write_dword(rtlpriv,  REG_TCR, value32);

	value32 |= TSFRST;
	rtl_write_dword(rtlpriv, REG_TCR, value32);

	/* NOTE: Fix test chip's bug (about contention windows's randomness) */
	rtl_write_byte(rtlpriv,  REG_RXTSF_OFFSET_CCK, 0x50);
	rtl_write_byte(rtlpriv, REG_RXTSF_OFFSET_OFDM, 0x50);

	_rtl88eu_beacon_function_enable(hw);

	_rtl88eu_resume_tx_beacon(hw);

	rtl_write_byte(rtlpriv, bcn_ctrl_reg,
		       rtl_read_byte(rtlpriv, bcn_ctrl_reg) | BIT(1));
}

static void rtl88eu_update_hal_rate_mask(struct ieee80211_hw *hw,
					 struct ieee80211_sta *sta,
					 u8 rssi_level)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = rtl_phy(rtlpriv);
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	struct rtl_sta_info *sta_entry = NULL;
	u32 ratr_bitmap;
	u8 ratr_index;
	u8 curtxbw_40mhz = (sta->ht_cap.cap & IEEE80211_HT_CAP_SUP_WIDTH_20_40)
				? 1 : 0;
	u8 curshortgi_40mhz = (sta->ht_cap.cap & IEEE80211_HT_CAP_SGI_40) ?
				1 : 0;
	u8 curshortgi_20mhz = (sta->ht_cap.cap & IEEE80211_HT_CAP_SGI_20) ?
				1 : 0;
	enum wireless_mode wirelessmode = 0;
	bool b_shortgi = false;
	u8 rate_mask[5];
	u8 macid = 0;
	/*u8 mimo_ps = IEEE80211_SMPS_OFF;*/

	sta_entry = (struct rtl_sta_info *)sta->drv_priv;
	wirelessmode = sta_entry->wireless_mode;
	if (mac->opmode == NL80211_IFTYPE_STATION ||
		mac->opmode == NL80211_IFTYPE_MESH_POINT)
		curtxbw_40mhz = mac->bw_40;
	else if (mac->opmode == NL80211_IFTYPE_AP ||
		mac->opmode == NL80211_IFTYPE_ADHOC)
		macid = sta->aid + 1;

	if (rtlhal->current_bandtype == BAND_ON_5G)
		ratr_bitmap = sta->supp_rates[1] << 4;
	else
		ratr_bitmap = sta->supp_rates[0];
	if (mac->opmode == NL80211_IFTYPE_ADHOC)
		ratr_bitmap = 0xfff;
	ratr_bitmap |= (sta->ht_cap.mcs.rx_mask[1] << 20 |
			sta->ht_cap.mcs.rx_mask[0] << 12);
	switch (wirelessmode) {
	case WIRELESS_MODE_B:
		ratr_index = RATR_INX_WIRELESS_B;
		if (ratr_bitmap & 0x0000000c)
			ratr_bitmap &= 0x0000000d;
		else
			ratr_bitmap &= 0x0000000f;
		break;
	case WIRELESS_MODE_G:
		ratr_index = RATR_INX_WIRELESS_GB;

		if (rssi_level == 1)
			ratr_bitmap &= 0x00000f00;
		else if (rssi_level == 2)
			ratr_bitmap &= 0x00000ff0;
		else
			ratr_bitmap &= 0x00000ff5;
		break;
	case WIRELESS_MODE_N_24G:
	case WIRELESS_MODE_N_5G:
		ratr_index = RATR_INX_WIRELESS_NGB;
		if (rtlphy->rf_type == RF_1T2R ||
		    rtlphy->rf_type == RF_1T1R) {
			if (curtxbw_40mhz) {
				if (rssi_level == 1)
					ratr_bitmap &= 0x000f0000;
				else if (rssi_level == 2)
					ratr_bitmap &= 0x000ff000;
				else
					ratr_bitmap &= 0x000ff015;
			} else {
				if (rssi_level == 1)
					ratr_bitmap &= 0x000f0000;
				else if (rssi_level == 2)
					ratr_bitmap &= 0x000ff000;
				else
					ratr_bitmap &= 0x000ff005;
			}
		} else {
			if (curtxbw_40mhz) {
				if (rssi_level == 1)
					ratr_bitmap &= 0x0f8f0000;
				else if (rssi_level == 2)
					ratr_bitmap &= 0x0f8ff000;
				else
					ratr_bitmap &= 0x0f8ff015;
			} else {
				if (rssi_level == 1)
					ratr_bitmap &= 0x0f8f0000;
				else if (rssi_level == 2)
					ratr_bitmap &= 0x0f8ff000;
				else
					ratr_bitmap &= 0x0f8ff005;
			}
		}

		if ((curtxbw_40mhz && curshortgi_40mhz) ||
		    (!curtxbw_40mhz && curshortgi_20mhz)) {

			if (macid == 0)
				b_shortgi = true;
			else if (macid == 1)
				b_shortgi = false;
		}
		break;
	default:
		ratr_index = RATR_INX_WIRELESS_NGB;

		if (rtlphy->rf_type == RF_1T2R)
			ratr_bitmap &= 0x000ff0ff;
		else
			ratr_bitmap &= 0x0f0ff0ff;
		break;
	}
	sta_entry->ratr_index = ratr_index;

	RT_TRACE(rtlpriv, COMP_RATR, DBG_DMESG,
		 "ratr_bitmap :%x\n", ratr_bitmap);
	*(u32 *)&rate_mask = (ratr_bitmap & 0x0fffffff) |
			     (ratr_index << 28);
	rate_mask[4] = macid | (b_shortgi ? 0x20 : 0x00) | 0x80;
	RT_TRACE(rtlpriv, COMP_RATR, DBG_DMESG,
		 "Rate_index:%x, ratr_val:%x, %x:%x:%x:%x:%x\n",
		 ratr_index, ratr_bitmap,
		 rate_mask[0], rate_mask[1],
		 rate_mask[2], rate_mask[3],
		 rate_mask[4]);
#if 0
	rtlpriv->cfg->ops->fill_h2c_cmd(hw, H2C_88E_RA_MASK, 5, rate_mask);
#endif
	_rtl88eu_set_bcn_ctrl_reg(hw, BIT(3), 0);
}

static void rtl88eu_update_hal_rate_table(struct ieee80211_hw *hw,
					  struct ieee80211_sta *sta)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = rtl_phy(rtlpriv);
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	u32 ratr_value;
	u8 ratr_index = 0;
	u8 nmode = mac->ht_enable;
	u8 mimo_ps = IEEE80211_SMPS_OFF;
	u16 shortgi_rate;
	u32 tmp_ratr_value;
	u8 curtxbw_40mhz = mac->bw_40;
	u8 curshortgi_40mhz = (sta->ht_cap.cap & IEEE80211_HT_CAP_SGI_40) ?
			       1 : 0;
	u8 curshortgi_20mhz = (sta->ht_cap.cap & IEEE80211_HT_CAP_SGI_20) ?
			       1 : 0;
	enum wireless_mode wirelessmode = mac->mode;

	if (rtlhal->current_bandtype == BAND_ON_5G)
		ratr_value = sta->supp_rates[1] << 4;
	else
		ratr_value = sta->supp_rates[0];
	if (mac->opmode == NL80211_IFTYPE_ADHOC)
		ratr_value = 0xfff;

	ratr_value |= (sta->ht_cap.mcs.rx_mask[1] << 20 |
			sta->ht_cap.mcs.rx_mask[0] << 12);
	switch (wirelessmode) {
	case WIRELESS_MODE_B:
		if (ratr_value & 0x0000000c)
			ratr_value &= 0x0000000d;
		else
			ratr_value &= 0x0000000f;
		break;
	case WIRELESS_MODE_G:
		ratr_value &= 0x00000FF5;
		break;
	case WIRELESS_MODE_N_24G:
	case WIRELESS_MODE_N_5G:
		nmode = 1;
		if (mimo_ps == IEEE80211_SMPS_STATIC) {
			ratr_value &= 0x0007F005;
		} else {
			u32 ratr_mask;

			if (get_rf_type(rtlphy) == RF_1T2R ||
			    get_rf_type(rtlphy) == RF_1T1R)
				ratr_mask = 0x000ff005;
			else
				ratr_mask = 0x0f0ff005;

			ratr_value &= ratr_mask;
		}
		break;
	default:
		if (rtlphy->rf_type == RF_1T2R)
			ratr_value &= 0x000ff0ff;
		else
			ratr_value &= 0x0f0ff0ff;

		break;
	}

	ratr_value &= 0x0FFFFFFF;

	if (nmode && ((curtxbw_40mhz &&
			 curshortgi_40mhz) || (!curtxbw_40mhz &&
					       curshortgi_20mhz))) {

		ratr_value |= 0x10000000;
		tmp_ratr_value = (ratr_value >> 12);

		for (shortgi_rate = 15; shortgi_rate > 0; shortgi_rate--) {
			if ((1 << shortgi_rate) & tmp_ratr_value)
				break;
		}

		shortgi_rate = (shortgi_rate << 12) | (shortgi_rate << 8) |
		    (shortgi_rate << 4) | (shortgi_rate);
	}

	rtl_write_dword(rtlpriv, REG_ARFR0 + ratr_index * 4, ratr_value);

	RT_TRACE(rtlpriv, COMP_RATR, DBG_DMESG, "%x\n",
		 rtl_read_dword(rtlpriv, REG_ARFR0));
}

void rtl88eu_update_hal_rate_tbl(struct ieee80211_hw *hw,
		struct ieee80211_sta *sta, u8 rssi_level)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	if (rtlpriv->dm.useramask)
		rtl88eu_update_hal_rate_mask(hw, sta, rssi_level);
	else
		rtl88eu_update_hal_rate_table(hw, sta);
}

void rtl88eu_update_channel_access_setting(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	u16 sifs_timer;

	rtlpriv->cfg->ops->set_hw_reg(hw, HW_VAR_SLOT_TIME, &mac->slot_time);
	if (!mac->ht_enable)
		sifs_timer = 0x0a0a;
	else
		sifs_timer = 0x0e0e;
	rtlpriv->cfg->ops->set_hw_reg(hw, HW_VAR_SIFS, (u8 *)&sifs_timer);
}

bool rtl88eu_gpio_radio_on_off_checking(struct ieee80211_hw *hw, u8 *valid)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_ps_ctl *ppsc = rtl_psc(rtl_priv(hw));
	enum rf_pwrstate e_rfpowerstate_toset, cur_rfstate;
	u32 u4tmp;
	bool b_actuallyset = false;

	if (rtlpriv->rtlhal.being_init_adapter)
		return false;

	if (ppsc->swrf_processing)
		return false;

	spin_lock(&rtlpriv->locks.rf_ps_lock);
	if (ppsc->rfchange_inprogress) {
		spin_unlock(&rtlpriv->locks.rf_ps_lock);
		return false;
	} else {
		ppsc->rfchange_inprogress = true;
		spin_unlock(&rtlpriv->locks.rf_ps_lock);
	}

	cur_rfstate = ppsc->rfpwr_state;

	u4tmp = rtl_read_dword(rtlpriv, REG_GPIO_OUTPUT);
	e_rfpowerstate_toset = (u4tmp & BIT(31)) ? ERFON : ERFOFF;

	if (ppsc->hwradiooff && (e_rfpowerstate_toset == ERFON)) {
		RT_TRACE(rtlpriv, COMP_RF, DBG_DMESG,
			 "GPIOChangeRF  - HW Radio ON, RF ON\n");

		e_rfpowerstate_toset = ERFON;
		ppsc->hwradiooff = false;
		b_actuallyset = true;
	} else if ((!ppsc->hwradiooff) &&
		   (e_rfpowerstate_toset == ERFOFF)) {
		RT_TRACE(rtlpriv, COMP_RF, DBG_DMESG,
			 "GPIOChangeRF  - HW Radio OFF, RF OFF\n");

		e_rfpowerstate_toset = ERFOFF;
		ppsc->hwradiooff = true;
		b_actuallyset = true;
	}

	if (b_actuallyset) {
		spin_lock(&rtlpriv->locks.rf_ps_lock);
		ppsc->rfchange_inprogress = false;
		spin_unlock(&rtlpriv->locks.rf_ps_lock);
	} else {
		if (ppsc->reg_rfps_level & RT_RF_OFF_LEVL_HALT_NIC)
			RT_SET_PS_LEVEL(ppsc, RT_RF_OFF_LEVL_HALT_NIC);

		spin_lock(&rtlpriv->locks.rf_ps_lock);
		ppsc->rfchange_inprogress = false;
		spin_unlock(&rtlpriv->locks.rf_ps_lock);
	}

	*valid = 1;
	return !ppsc->hwradiooff;

}

void rtl88eu_set_key(struct ieee80211_hw *hw, u32 key_index,
		     u8 *p_macaddr, bool is_group, u8 enc_algo,
		     bool is_wepkey, bool clear_all)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	u8 *macaddr = p_macaddr;
	u32 entry_id = 0;
	bool is_pairwise = false;
	static u8 cam_const_addr[4][6] = {
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x02},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x03}
	};
	static u8 cam_const_broad[] = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};

	if (clear_all) {
		u8 idx = 0;
		u8 cam_offset = 0;
		u8 clear_number = 5;

		RT_TRACE(rtlpriv, COMP_SEC, DBG_DMESG, "clear_all\n");

		for (idx = 0; idx < clear_number; idx++) {
			rtl_cam_mark_invalid(hw, cam_offset + idx);
			rtl_cam_empty_entry(hw, cam_offset + idx);

			if (idx < 5) {
				memset(rtlpriv->sec.key_buf[idx], 0,
				       MAX_KEY_LEN);
				rtlpriv->sec.key_len[idx] = 0;
			}
		}

	} else {
		switch (enc_algo) {
		case WEP40_ENCRYPTION:
			enc_algo = CAM_WEP40;
			break;
		case WEP104_ENCRYPTION:
			enc_algo = CAM_WEP104;
			break;
		case TKIP_ENCRYPTION:
			enc_algo = CAM_TKIP;
			break;
		case AESCCMP_ENCRYPTION:
			enc_algo = CAM_AES;
			break;
		default:
			RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
				 "switch case not process\n");
			enc_algo = CAM_TKIP;
			break;
		}

		if (is_wepkey || rtlpriv->sec.use_defaultkey) {
			macaddr = cam_const_addr[key_index];
			entry_id = key_index;
		} else {
			if (is_group) {
				macaddr = cam_const_broad;
				entry_id = key_index;
			} else {
				if (mac->opmode == NL80211_IFTYPE_AP ||
				    mac->opmode == NL80211_IFTYPE_MESH_POINT) {
					entry_id =
					  rtl_cam_get_free_entry(hw, p_macaddr);
					if (entry_id >=  TOTAL_CAM_ENTRY) {
						RT_TRACE(rtlpriv, COMP_SEC,
							 DBG_EMERG,
							 "Can not find free hw security cam entry\n");
						return;
					}
				} else {
					entry_id = CAM_PAIRWISE_KEY_POSITION;
				}
				key_index = PAIRWISE_KEYIDX;
				is_pairwise = true;
			}
		}

		if (rtlpriv->sec.key_len[key_index] == 0) {
			RT_TRACE(rtlpriv, COMP_SEC, DBG_DMESG,
				 "delete one entry, entry_id is %d\n",
				 entry_id);
			if (mac->opmode == NL80211_IFTYPE_AP ||
				mac->opmode == NL80211_IFTYPE_MESH_POINT)
				rtl_cam_del_entry(hw, p_macaddr);
			rtl_cam_delete_one_entry(hw, p_macaddr, entry_id);
		} else {
			RT_TRACE(rtlpriv, COMP_SEC, DBG_DMESG,
				 "add one entry\n");
			if (is_pairwise) {
				RT_TRACE(rtlpriv, COMP_SEC, DBG_DMESG,
					 "set Pairwise key\n");

				rtl_cam_add_one_entry(hw, macaddr, key_index,
						      entry_id, enc_algo,
						      CAM_CONFIG_NO_USEDK,
						      rtlpriv->sec.key_buf[key_index]);
			} else {
				RT_TRACE(rtlpriv, COMP_SEC, DBG_DMESG,
					 "set group key\n");

				if (mac->opmode == NL80211_IFTYPE_ADHOC) {
					rtl_cam_add_one_entry(hw,
							rtlefuse->dev_addr,
							PAIRWISE_KEYIDX,
							CAM_PAIRWISE_KEY_POSITION,
							enc_algo,
							CAM_CONFIG_NO_USEDK,
							rtlpriv->sec.key_buf
							[entry_id]);
				}

				rtl_cam_add_one_entry(hw, macaddr, key_index,
						      entry_id, enc_algo,
						      CAM_CONFIG_NO_USEDK,
						      rtlpriv->sec.key_buf[entry_id]);
			}

		}
	}
}

void rtl88eu_enable_hw_security_config(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 sec_reg_value;

	RT_TRACE(rtlpriv, COMP_ERR, DBG_DMESG,
		 "PairwiseEncAlgorithm = %d GroupEncAlgorithm = %d\n",
		  rtlpriv->sec.pairwise_enc_algorithm,
		  rtlpriv->sec.group_enc_algorithm);

	if (rtlpriv->cfg->mod_params->sw_crypto || rtlpriv->sec.use_sw_sec) {
		RT_TRACE(rtlpriv, COMP_SEC, DBG_DMESG,
			 "not open hw encryption\n");
		return;
	}

	sec_reg_value = SCR_TXENCENABLE | SCR_RXDECENABLE;

	if (rtlpriv->sec.use_defaultkey) {
		sec_reg_value |= SCR_TXUSEDK;
		sec_reg_value |= SCR_RXUSEDK;
	}

	sec_reg_value |= (SCR_RXBCUSEDK | SCR_TXBCUSEDK);

	rtl_write_byte(rtlpriv, REG_CR + 1, 0x02);

	RT_TRACE(rtlpriv, COMP_SEC, DBG_DMESG,
		 "The SECR-value %x\n", sec_reg_value);

	rtlpriv->cfg->ops->set_hw_reg(hw, HW_VAR_WPA_CONFIG, &sec_reg_value);
}
