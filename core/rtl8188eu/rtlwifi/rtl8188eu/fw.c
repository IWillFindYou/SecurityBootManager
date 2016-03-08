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
#include "../base.h"
#include "../core.h"
#include "reg.h"
#include "def.h"
#include "fw.h"

static void _rtl88eu_enable_fw_download(struct ieee80211_hw *hw, bool enable)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 tmp;

	if (enable) {
		tmp = rtl_read_byte(rtlpriv, REG_MCUFWDL);
		rtl_write_byte(rtlpriv, REG_MCUFWDL, tmp | 0x01);

		tmp = rtl_read_byte(rtlpriv, REG_MCUFWDL + 2);
		rtl_write_byte(rtlpriv, REG_MCUFWDL + 2, tmp & 0xf7);
	} else {
		tmp = rtl_read_byte(rtlpriv, REG_MCUFWDL);
		rtl_write_byte(rtlpriv, REG_MCUFWDL, tmp & 0xfe);

		rtl_write_byte(rtlpriv, REG_MCUFWDL + 1, 0x00);
	}
}

static void _rtl88eu_fw_block_write(struct ieee80211_hw *hw,
				   const u8 *buffer, u32 size)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 blocksize = sizeof(u32);
	u8 *bufferptr = (u8 *)buffer;
	u32 *pu4BytePtr = (u32 *)buffer;
	u32 i, offset, blockcount, remainsize;

	blockcount = size / blocksize;
	remainsize = size % blocksize;

	for (i = 0; i < blockcount; i++) {
		offset = i * blocksize;
		rtl_write_dword(rtlpriv, (FW_8192C_START_ADDRESS + offset),
				*(pu4BytePtr + i));
	}

	if (remainsize) {
		offset = blockcount * blocksize;
		bufferptr += offset;
		for (i = 0; i < remainsize; i++) {
			rtl_write_byte(rtlpriv, (FW_8192C_START_ADDRESS +
						 offset + i), *(bufferptr + i));
		}
	}
}

static void _rtl88eu_fw_page_write(struct ieee80211_hw *hw,
				  u32 page, const u8 *buffer, u32 size)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 value8;
	u8 u8page = (u8) (page & 0x07);

	value8 = (rtl_read_byte(rtlpriv, REG_MCUFWDL + 2) & 0xF8) | u8page;

	rtl_write_byte(rtlpriv, (REG_MCUFWDL + 2), value8);
	_rtl88eu_fw_block_write(hw, buffer, size);
}

static void _rtl88eu_fill_dummy(u8 *pfwbuf, u32 *pfwlen)
{
	u32 fwlen = *pfwlen;
	u8 remain = (u8) (fwlen % 4);

	remain = (remain == 0) ? 0 : (4 - remain);

	while (remain > 0) {
		pfwbuf[fwlen] = 0;
		fwlen++;
		remain--;
	}

	*pfwlen = fwlen;
}

static void _rtl88eu_write_fw(struct ieee80211_hw *hw,
			     enum version_8188e version, u8 *buffer, u32 size)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 *bufferptr = (u8 *)buffer;
	u32 pagenums, remainsize;
	u32 page, offset;

	RT_TRACE(rtlpriv, COMP_FW, DBG_LOUD, "FW size is %d bytes,\n", size);

	_rtl88eu_fill_dummy(bufferptr, &size);

	pagenums = size / FW_8192C_PAGE_SIZE;
	remainsize = size % FW_8192C_PAGE_SIZE;

	if (pagenums > 8) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "Page numbers should not greater then 8\n");
	}

	for (page = 0; page < pagenums; page++) {
		offset = page * FW_8192C_PAGE_SIZE;
		_rtl88eu_fw_page_write(hw, page, (bufferptr + offset),
				      FW_8192C_PAGE_SIZE);
	}

	if (remainsize) {
		offset = pagenums * FW_8192C_PAGE_SIZE;
		page = pagenums;
		_rtl88eu_fw_page_write(hw, page, (bufferptr + offset),
				      remainsize);
	}
}

static int _rtl88eu_fw_free_to_go(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	int err = -EIO;
	u32 counter = 0;
	u32 value32;

	do {
		value32 = rtl_read_dword(rtlpriv, REG_MCUFWDL);
		if (value32 & FWDL_CHKSUM_RPT)
			break;
	} while (counter++ < FW_8192C_POLLING_TIMEOUT_COUNT);

	if (counter >= FW_8192C_POLLING_TIMEOUT_COUNT) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "chksum report faill ! REG_MCUFWDL:0x%08x .\n",
			  value32);
		goto exit;
	}

	RT_TRACE(rtlpriv, COMP_FW, DBG_TRACE,
		 "Checksum report OK ! REG_MCUFWDL:0x%08x .\n", value32);

	value32 = rtl_read_dword(rtlpriv, REG_MCUFWDL);
	value32 |= MCUFWDL_RDY;
	value32 &= ~WINTINI_RDY;
	rtl_write_dword(rtlpriv, REG_MCUFWDL, value32);

	rtl88eu_firmware_selfreset(hw);
	counter = 0;

	do {
		value32 = rtl_read_dword(rtlpriv, REG_MCUFWDL);
		if (value32 & WINTINI_RDY) {
			RT_TRACE(rtlpriv, COMP_FW, DBG_TRACE,
				 "Polling FW ready success!! REG_MCUFWDL:0x%08x.\n",
				  value32);
			err = 0;
			goto exit;
		}

		udelay(FW_8192C_POLLING_DELAY);

	} while (counter++ < FW_8192C_POLLING_TIMEOUT_COUNT);

	RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
		 "Polling FW ready fail!! REG_MCUFWDL:0x%08x .\n", value32);

exit:
	return err;
}

int rtl88eu_download_fw(struct ieee80211_hw *hw,
		       bool buse_wake_on_wlan_fw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	struct rtl92c_firmware_header *pfwheader;
	u8 *pfwdata;
	u32 fwsize;
	int err;
	enum version_8188e version = rtlhal->version;

	if (!rtlhal->pfirmware)
		return 1;

	pfwheader = (struct rtl92c_firmware_header *)rtlhal->pfirmware;
	pfwdata = rtlhal->pfirmware;
	fwsize = rtlhal->fwsize;
	RT_TRACE(rtlpriv, COMP_FW, DBG_DMESG,
		 "normal Firmware SIZE %d\n", fwsize);

	if (IS_FW_HEADER_EXIST(pfwheader)) {
		RT_TRACE(rtlpriv, COMP_FW, DBG_DMESG,
			 "Firmware Version(%d), Signature(%#x), Size(%d)\n",
			  pfwheader->version, pfwheader->signature,
			  (int)sizeof(struct rtl92c_firmware_header));

		pfwdata = pfwdata + sizeof(struct rtl92c_firmware_header);
		fwsize = fwsize - sizeof(struct rtl92c_firmware_header);
	}

	if (rtl_read_byte(rtlpriv, REG_MCUFWDL) & BIT(7)) {
		rtl_write_byte(rtlpriv, REG_MCUFWDL, 0);
		rtl88eu_firmware_selfreset(hw);
	}
	_rtl88eu_enable_fw_download(hw, true);
	_rtl88eu_write_fw(hw, version, pfwdata, fwsize);
	rtl_write_byte(rtlpriv, REG_MCUFWDL,
		       rtl_read_byte(rtlpriv, REG_MCUFWDL) | FWDL_CHKSUM_RPT);
	_rtl88eu_enable_fw_download(hw, false);

	err = _rtl88eu_fw_free_to_go(hw);
	if (err) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "Firmware is not ready to run!\n");
	} else {
		RT_TRACE(rtlpriv, COMP_FW, DBG_LOUD,
			 "Firmware is ready to run!\n");
	}

	return 0;
}

#define RTL88eu_MAX_H2C_BOX_NUMS         4
#define RTL88eu_MAX_CMD_LEN              7
#define RTL88eu_MESSAGE_BOX_SIZE         4
#define RTL88eu_EX_MESSAGE_BOX_SIZE      4

static u8 _is_fw_read_cmd_down(struct ieee80211_hw *hw, u8 msgbox_num)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 read_down = false;
	int retry_cnts = 100;
	u8 valid;

	do {
		valid = rtl_read_byte(rtlpriv, REG_HMETFR) & BIT(msgbox_num);
		if (0 == valid)
			read_down = true;
	} while((!read_down) && (retry_cnts--));

	return read_down;
}

void rtl88eu_fill_h2c_cmd(struct ieee80211_hw *hw,
			  u8 element_id, u32 cmd_len, u8 *cmdbuffer)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	u8 bcmd_down = false;
	s32 retry_cnts = 100;
	u8 h2c_box_num;
	u32 msgbox_addr;
	u32 msgbox_ex_addr;
	u8 cmd_idx, ext_cmd_len;
	u32 h2c_cmd = 0;
	u32 h2c_cmd_ex = 0;

	if (!rtlhal->fw_ready) {
		RT_ASSERT(false,
			  "return H2C cmd because of Fw download fail!!!\n");
		return;
	}

	if (!cmdbuffer) {
		RT_ASSERT(false,
			  "return H2C cmd, cmdbuffer is NULL\n");
		return;
	}
	if (cmd_len > RTL88eu_MAX_CMD_LEN) {
		RT_ASSERT(false,
			  "return H2C cmd, cmd_len is so short\n");
		return;
	}

	RT_TRACE(rtlpriv, COMP_CMD, DBG_LOUD, "come in\n");

	do {
		h2c_box_num = rtlhal->last_hmeboxnum;

		if (!_is_fw_read_cmd_down(hw, h2c_box_num)) {
			RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
				 "return H2C cmd, fw read cmd failed...\n");
			return;
		}

		*(u8 *)(&h2c_cmd) = element_id;

		if (cmd_len <= 3) {
			memcpy((u8 *)(&h2c_cmd) +1, cmdbuffer, cmd_len);
		} else {
			memcpy((u8 *)(&h2c_cmd) +1, cmdbuffer, 3);
			ext_cmd_len = cmd_len - 3;
			memcpy((u8 *)(&h2c_cmd_ex), cmdbuffer + 3, ext_cmd_len);

			msgbox_ex_addr = REG_HMEBOX_EXT_0 +
				(h2c_box_num * RTL88eu_EX_MESSAGE_BOX_SIZE);

			for (cmd_idx = 0; cmd_idx < ext_cmd_len; cmd_idx++)
				rtl_write_byte(rtlpriv, msgbox_ex_addr + cmd_idx,
					       *((u8 *)(&h2c_cmd_ex) +
						cmd_idx));
		}

		msgbox_addr = REG_HMEBOX_0 + (h2c_box_num *
					     RTL88eu_MESSAGE_BOX_SIZE);

		cmd_idx = 0;
		for (; cmd_idx < RTL88eu_MESSAGE_BOX_SIZE; cmd_idx++) {
			rtl_write_byte(rtlpriv, msgbox_addr + cmd_idx,
				       *((u8 *)(&h2c_cmd) + cmd_idx));
		}
		bcmd_down = true;

		rtlhal->last_hmeboxnum = (h2c_box_num + 1) %
					  RTL88eu_MAX_H2C_BOX_NUMS;
	} while ((!bcmd_down) && (retry_cnts--));

	RT_TRACE(rtlpriv, COMP_CMD, DBG_LOUD, "go out\n");
}

void rtl88eu_firmware_selfreset(struct ieee80211_hw *hw)
{
	u8 u1b_tmp;
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	u1b_tmp = rtl_read_byte(rtlpriv, REG_SYS_FUNC_EN+1);
	rtl_write_byte(rtlpriv, REG_SYS_FUNC_EN+1, (u1b_tmp & (~BIT(2))));
	rtl_write_byte(rtlpriv, REG_SYS_FUNC_EN+1, (u1b_tmp | BIT(2)));
	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD, "8051 reset success\n");

}

void rtl88eu_set_fw_pwrmode_cmd(struct ieee80211_hw *hw, u8 mode)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 u1_h2c_set_pwrmode[H2C_88E_PWEMODE_LENGTH] = { 0 };
	struct rtl_ps_ctl *ppsc = rtl_psc(rtl_priv(hw));
	u8 rlbm = 0;

	RT_TRACE(rtlpriv, COMP_POWER, DBG_LOUD, "FW LPS mode = %d\n", mode);

	switch (mode) {
	case PS_MODE_ACTIVE:
		u1_h2c_set_pwrmode[0] = 0;
		break;
	case PS_MODE_MIN:
		u1_h2c_set_pwrmode[0] = 1;
		break;
	case PS_MODE_MAX:
		u1_h2c_set_pwrmode[0] = 1;
		break;
	case PS_MODE_DTIM:
		u1_h2c_set_pwrmode[0] = 1;
		break;
	case PS_MODE_UAPSD_WMM:
		u1_h2c_set_pwrmode[0] = 2;
		break;
	default:
		u1_h2c_set_pwrmode[0] = 0;
		break;
	}

	u1_h2c_set_pwrmode[1] = (((ppsc->smart_ps << 4) & 0xf0) | (rlbm & 0x0f));
	u1_h2c_set_pwrmode[2] = 1;
	u1_h2c_set_pwrmode[3] = 0;
	
	if (mode)
		u1_h2c_set_pwrmode[4] = 0x00;
	else
		u1_h2c_set_pwrmode[4] = 0x0c;
	
	RT_PRINT_DATA(rtlpriv, COMP_CMD, DBG_DMESG,
		      "rtl88eu_set_fw_pwrmode(): u1_h2c_set_pwrmode\n",
		      u1_h2c_set_pwrmode, H2C_88E_PWEMODE_LENGTH);
	rtlpriv->cfg->ops->fill_h2c_cmd(hw, H2C_88E_SETPWRMODE,
					H2C_88E_PWEMODE_LENGTH, u1_h2c_set_pwrmode);

}

void rtl88eu_set_fw_media_status_cmd(struct ieee80211_hw *hw,
				     __le16 status_rpt)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 opmode, macid;
	u16 mst_rpt = le16_to_cpu(status_rpt);
	
	opmode = (u8)mst_rpt;
	macid = (u8)(mst_rpt >> 8);

	rtlpriv->cfg->ops->fill_h2c_cmd(hw, 1,
					sizeof(mst_rpt), (u8 *)&mst_rpt);

}

#define BEACON_PG		0 /* ->1 */
#define PSPOLL_PG		2
#define NULL_PG			3
#define PROBERSP_PG		4 /* ->5 */

#define TOTAL_RESERVED_PKT_LEN	768

static u8 reserved_page_packet[TOTAL_RESERVED_PKT_LEN] = {
	/* page 0 beacon */
	0x80, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0x00, 0xE0, 0x4C, 0x76, 0x00, 0x42,
	0x00, 0x40, 0x10, 0x10, 0x00, 0x03, 0x50, 0x08,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x64, 0x00, 0x00, 0x04, 0x00, 0x0C, 0x6C, 0x69,
	0x6E, 0x6B, 0x73, 0x79, 0x73, 0x5F, 0x77, 0x6C,
	0x61, 0x6E, 0x01, 0x04, 0x82, 0x84, 0x8B, 0x96,
	0x03, 0x01, 0x01, 0x06, 0x02, 0x00, 0x00, 0x2A,
	0x01, 0x00, 0x32, 0x08, 0x24, 0x30, 0x48, 0x6C,
	0x0C, 0x12, 0x18, 0x60, 0x2D, 0x1A, 0x6C, 0x18,
	0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x3D, 0x00, 0xDD, 0x06, 0x00, 0xE0, 0x4C, 0x02,
	0x01, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	/* page 1 beacon */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x10, 0x00, 0x20, 0x8C, 0x00, 0x12, 0x10, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	/* page 2  ps-poll */
	0xA4, 0x10, 0x01, 0xC0, 0x00, 0x40, 0x10, 0x10,
	0x00, 0x03, 0x00, 0xE0, 0x4C, 0x76, 0x00, 0x42,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x18, 0x00, 0x20, 0x8C, 0x00, 0x12, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
	0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	/* page 3  null */
	0x48, 0x01, 0x00, 0x00, 0x00, 0x40, 0x10, 0x10,
	0x00, 0x03, 0x00, 0xE0, 0x4C, 0x76, 0x00, 0x42,
	0x00, 0x40, 0x10, 0x10, 0x00, 0x03, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x72, 0x00, 0x20, 0x8C, 0x00, 0x12, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
	0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	/* page 4  probe_resp */
	0x50, 0x00, 0x00, 0x00, 0x00, 0x40, 0x10, 0x10,
	0x00, 0x03, 0x00, 0xE0, 0x4C, 0x76, 0x00, 0x42,
	0x00, 0x40, 0x10, 0x10, 0x00, 0x03, 0x00, 0x00,
	0x9E, 0x46, 0x15, 0x32, 0x27, 0xF2, 0x2D, 0x00,
	0x64, 0x00, 0x00, 0x04, 0x00, 0x0C, 0x6C, 0x69,
	0x6E, 0x6B, 0x73, 0x79, 0x73, 0x5F, 0x77, 0x6C,
	0x61, 0x6E, 0x01, 0x04, 0x82, 0x84, 0x8B, 0x96,
	0x03, 0x01, 0x01, 0x06, 0x02, 0x00, 0x00, 0x2A,
	0x01, 0x00, 0x32, 0x08, 0x24, 0x30, 0x48, 0x6C,
	0x0C, 0x12, 0x18, 0x60, 0x2D, 0x1A, 0x6C, 0x18,
	0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x3D, 0x00, 0xDD, 0x06, 0x00, 0xE0, 0x4C, 0x02,
	0x01, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	/* page 5  probe_resp */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void rtl88eu_set_fw_rsvdpagepkt(struct ieee80211_hw *hw, bool b_dl_finished)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	struct sk_buff *skb = NULL;
	u32 totalpacketlen;
	bool rtstatus;
	u8 u1rsvdpageloc[5] = { 0 };
	bool b_dlok = false;
	u8 *beacon;
	u8 *p_pspoll;
	u8 *nullfunc;
	u8 *p_probersp;

	/*---------------------------------------------------------
	 *			(1) beacon
	 *---------------------------------------------------------
	 */
	beacon = &reserved_page_packet[BEACON_PG * 128];
	SET_80211_HDR_ADDRESS2(beacon, mac->mac_addr);
	SET_80211_HDR_ADDRESS3(beacon, mac->bssid);

	/*-------------------------------------------------------
	 *			(2) ps-poll
	 *--------------------------------------------------------
	 */
	p_pspoll = &reserved_page_packet[PSPOLL_PG * 128];
	SET_80211_PS_POLL_AID(p_pspoll, (mac->assoc_id | 0xc000));
	SET_80211_PS_POLL_BSSID(p_pspoll, mac->bssid);
	SET_80211_PS_POLL_TA(p_pspoll, mac->mac_addr);

	SET_H2CCMD_RSVDPAGE_LOC_PSPOLL(u1rsvdpageloc, PSPOLL_PG);

	/*--------------------------------------------------------
	 *			(3) null data
	 *---------------------------------------------------------
	 */
	nullfunc = &reserved_page_packet[NULL_PG * 128];
	SET_80211_HDR_ADDRESS1(nullfunc, mac->bssid);
	SET_80211_HDR_ADDRESS2(nullfunc, mac->mac_addr);
	SET_80211_HDR_ADDRESS3(nullfunc, mac->bssid);

	SET_H2CCMD_RSVDPAGE_LOC_NULL_DATA(u1rsvdpageloc, NULL_PG);

	/*---------------------------------------------------------
	 *			(4) probe response
	 *----------------------------------------------------------
	 */
	p_probersp = &reserved_page_packet[PROBERSP_PG * 128];
	SET_80211_HDR_ADDRESS1(p_probersp, mac->bssid);
	SET_80211_HDR_ADDRESS2(p_probersp, mac->mac_addr);
	SET_80211_HDR_ADDRESS3(p_probersp, mac->bssid);

	SET_H2CCMD_RSVDPAGE_LOC_PROBE_RSP(u1rsvdpageloc, PROBERSP_PG);

	totalpacketlen = TOTAL_RESERVED_PKT_LEN;

	RT_PRINT_DATA(rtlpriv, COMP_CMD, DBG_LOUD,
		      "rtl88eu_set_fw_rsvdpagepkt(): HW_VAR_SET_TX_CMD: ALL\n",
		      &reserved_page_packet[0], totalpacketlen);
	RT_PRINT_DATA(rtlpriv, COMP_CMD, DBG_DMESG,
		      "rtl88eu_set_fw_rsvdpagepkt(): HW_VAR_SET_TX_CMD: ALL\n",
		      u1rsvdpageloc, 3);

	skb = dev_alloc_skb(totalpacketlen);
	memcpy(skb_put(skb, totalpacketlen),
	       &reserved_page_packet, totalpacketlen);

#if 0
	rtstatus = rtl_cmd_send_packet(hw, skb);
#else
	rtstatus = true;
#endif

	if (rtstatus)
		b_dlok = true;

	if (b_dlok) {
		RT_TRACE(rtlpriv, COMP_POWER, DBG_LOUD,
			 "Set RSVD page location to Fw.\n");
		RT_PRINT_DATA(rtlpriv, COMP_CMD, DBG_DMESG,
			      "H2C_RSVDPAGE:\n", u1rsvdpageloc, 3);
		rtlpriv->cfg->ops->fill_h2c_cmd(hw, H2C_88E_RSVDPAGE,
						sizeof(u1rsvdpageloc), u1rsvdpageloc);
	} else
		RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
			 "Set RSVD page location to Fw FAIL!!!!!!.\n");
}


void rtl88eu_set_fw_joinbss_report_cmd(struct ieee80211_hw *hw, u8 mstatus)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));

	u8 dlbcn_cnt = 0;
	u32 poll = 0;
	u8 value8;
	bool send_beacon = false;
	bool bcn_valid = false;

	if (mstatus == 1 ) {
		rtl_write_word(rtlpriv, REG_BCN_PSR_RPT,
			       (0xc000 | mac->assoc_id));
		rtl_write_byte(rtlpriv, REG_CR + 1,
			       rtl_read_byte(rtlpriv, REG_CR + 1) | BIT(0));
		rtl_write_byte(rtlpriv, REG_BCN_CTRL,
			       rtl_read_byte(rtlpriv, REG_BCN_CTRL) &
					     (~BIT(3)));
		rtl_write_byte(rtlpriv, REG_BCN_CTRL,
			       rtl_read_byte(rtlpriv, REG_BCN_CTRL) | BIT(4));

		value8 = rtl_read_byte(rtlpriv, REG_FWHW_TXQ_CTRL + 2);
		if (value8 & BIT(6))
			send_beacon = true;

		rtl_write_byte(rtlpriv, REG_FWHW_TXQ_CTRL + 2,
			       (value8 & (~BIT(6))));
		rtl_write_byte(rtlpriv, REG_TDECTRL + 2,
			       rtl_read_byte(rtlpriv, REG_TDECTRL + 2) |
					     BIT(0));

		dlbcn_cnt = 0;
		poll = 0;

		do {
			rtl88eu_set_fw_rsvdpagepkt(hw, false);
			dlbcn_cnt++;
			do {
				yield();
				bcn_valid = (BIT(0) & rtl_read_byte(rtlpriv,
						REG_TDECTRL+2)) ? true : false;
				poll++;
			} while (!bcn_valid && (poll%10) != 0);
		} while ((!bcn_valid) && (dlbcn_cnt <= 100));

		if (!bcn_valid) {
			RT_TRACE(rtlpriv, COMP_FW, DBG_LOUD,
				 "Download RSVD page failed!\n");
		} else {
			RT_TRACE(rtlpriv, COMP_FW, DBG_LOUD,
				 "Download RSVD success!\n");
		}

		rtl_write_byte(rtlpriv, REG_BCN_CTRL,
			       rtl_read_byte(rtlpriv, REG_BCN_CTRL) | BIT(3));
		rtl_write_byte(rtlpriv, REG_BCN_CTRL,
			       rtl_read_byte(rtlpriv, REG_BCN_CTRL) &
					     (~BIT(4)));

		if (send_beacon) {
			rtl_write_byte(rtlpriv, REG_FWHW_TXQ_CTRL + 2,
				       rtl_read_byte(rtlpriv,
						     REG_FWHW_TXQ_CTRL + 2) |
						     BIT(6));
		}

		if (bcn_valid) {
			rtl_write_byte(rtlpriv, REG_TDECTRL + 2,
				rtl_read_byte(rtlpriv, REG_TDECTRL + 2) |
					      BIT(0));
		}

		rtl_write_byte(rtlpriv, REG_CR + 1,
			       rtl_read_byte(rtlpriv, REG_CR + 1) | (~BIT(0)));
	}
}
