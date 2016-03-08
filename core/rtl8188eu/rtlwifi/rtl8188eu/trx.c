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
#include "../usb.h"
#include "../ps.h"
#include "../base.h"
#include "reg.h"
#include "def.h"
#include "phy.h"
#include "rf.h"
#include "dm.h"
#include "trx.h"
#include "../stats.h"

u16 rtl8188eu_mq_to_hwq(__le16 fc, u16 mac80211_queue_index)
{
	u16 hw_queue_index;

	if (unlikely(ieee80211_is_beacon(fc))) {
		hw_queue_index = RTL_TXQ_BCN;
		goto out;
	}
	if (ieee80211_is_mgmt(fc)) {
		hw_queue_index = RTL_TXQ_MGT;
		goto out;
	}
	switch (mac80211_queue_index) {
	case 0:
		hw_queue_index = RTL_TXQ_VO;
		break;
	case 1:
		hw_queue_index = RTL_TXQ_VI;
		break;
	case 2:
		hw_queue_index = RTL_TXQ_BE;
		break;
	case 3:
		hw_queue_index = RTL_TXQ_BK;
		break;
	default:
		hw_queue_index = RTL_TXQ_BE;
		RT_ASSERT(false, "QSLT_BE queue, skb_queue:%d\n",
			  mac80211_queue_index);
		break;
	}
out:
	return hw_queue_index;
}

static enum rtl_desc_qsel _rtl8188eu_mq_to_descq(struct ieee80211_hw *hw,
					 __le16 fc, u16 mac80211_queue_index)
{
	enum rtl_desc_qsel qsel;
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	if (unlikely(ieee80211_is_beacon(fc))) {
		qsel = QSLT_BEACON;
		goto out;
	}
	if (ieee80211_is_mgmt(fc)) {
		qsel = QSLT_MGNT;
		goto out;
	}
	switch (mac80211_queue_index) {
	case 0:	/* VO */
		qsel = QSLT_VO;
		RT_TRACE(rtlpriv, COMP_USB, DBG_DMESG,
			 "VO queue, set qsel = 0x%x\n", QSLT_VO);
		break;
	case 1:	/* VI */
		qsel = QSLT_VI;
		RT_TRACE(rtlpriv, COMP_USB, DBG_DMESG,
			 "VI queue, set qsel = 0x%x\n", QSLT_VI);
		break;
	case 3:	/* BK */
		qsel = QSLT_BK;
		RT_TRACE(rtlpriv, COMP_USB, DBG_DMESG,
			 "BK queue, set qsel = 0x%x\n", QSLT_BK);
		break;
	case 2:	/* BE */
	default:
		qsel = QSLT_BE;
		RT_TRACE(rtlpriv, COMP_USB, DBG_DMESG,
			 "BE queue, set qsel = 0x%x\n", QSLT_BE);
		break;
	}
out:
	return qsel;
}

/* =============================================================== */

/*----------------------------------------------------------------------
 *
 *	Rx handler
 *
 *---------------------------------------------------------------------- */

static void _rtl88ee_query_rxphystatus(struct ieee80211_hw *hw,
			struct rtl_stats *pstatus, u8 *pdesc,
			struct rx_fwinfo_88e *p_drvinfo,
			bool bpacket_match_bssid,
			bool bpacket_toself, bool packet_beacon)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_ps_ctl *ppsc = rtl_psc(rtlpriv);
	struct phy_sts_cck_8192s_t *cck_buf;
	struct phy_status_rpt *phystrpt =
		(struct phy_status_rpt *)p_drvinfo;
	struct rtl_dm *rtldm = rtl_dm(rtl_priv(hw));
	char rx_pwr_all = 0, rx_pwr[4];
	u8 rf_rx_num = 0, evm, pwdb_all;
	u8 i, max_spatial_stream;
	u32 rssi, total_rssi = 0;
	bool is_cck = pstatus->is_cck;
	u8 lan_idx, vga_idx;

	/* Record it for next packet processing */
	pstatus->packet_matchbssid = bpacket_match_bssid;
	pstatus->packet_toself = bpacket_toself;
	pstatus->packet_beacon = packet_beacon;
	pstatus->rx_mimo_signalquality[0] = -1;
	pstatus->rx_mimo_signalquality[1] = -1;

	if (is_cck) {
		u8 cck_highpwr;
		u8 cck_agc_rpt;
		/* CCK Driver info Structure is not the same as OFDM packet. */
		cck_buf = (struct phy_sts_cck_8192s_t *)p_drvinfo;
		cck_agc_rpt = cck_buf->cck_agc_rpt;

		/* (1)Hardware does not provide RSSI for CCK
		 * (2)PWDB, Average PWDB cacluated by
		 * hardware (for rate adaptive)
		 */
		if (ppsc->rfpwr_state == ERFON)
			cck_highpwr =
				(u8)rtl_get_bbreg(hw, RFPGA0_XA_HSSIPARAMETER2,
						  BIT(9));
		else
			cck_highpwr = false;

		lan_idx = ((cck_agc_rpt & 0xE0) >> 5);
		vga_idx = (cck_agc_rpt & 0x1f);
		switch (lan_idx) {
		case 7:
			if (vga_idx <= 27)
				/*VGA_idx = 27~2*/
				rx_pwr_all = -100 + 2*(27-vga_idx);
			else
				rx_pwr_all = -100;
			break;
		case 6:
			/*VGA_idx = 2~0*/
			rx_pwr_all = -48 + 2*(2-vga_idx);
			break;
		case 5:
			/*VGA_idx = 7~5*/
			rx_pwr_all = -42 + 2*(7-vga_idx);
			break;
		case 4:
			/*VGA_idx = 7~4*/
			rx_pwr_all = -36 + 2*(7-vga_idx);
			break;
		case 3:
			/*VGA_idx = 7~0*/
			rx_pwr_all = -24 + 2*(7-vga_idx);
			break;
		case 2:
			if (cck_highpwr)
				/*VGA_idx = 5~0*/
				rx_pwr_all = -12 + 2*(5-vga_idx);
			else
				rx_pwr_all = -6 + 2*(5-vga_idx);
			break;
		case 1:
			rx_pwr_all = 8-2*vga_idx;
			break;
		case 0:
			rx_pwr_all = 14-2*vga_idx;
			break;
		default:
			break;
		}
		rx_pwr_all += 6;
		pwdb_all = rtl_query_rxpwrpercentage(rx_pwr_all);
		/* CCK gain is smaller than OFDM/MCS gain,  */
		/* so we add gain diff by experiences, the val is 6 */
		pwdb_all += 6;
		if (pwdb_all > 100)
			pwdb_all = 100;
		/* modify the offset to make the same
		 * gain index with OFDM.
		 */
		if (pwdb_all > 34 && pwdb_all <= 42)
			pwdb_all -= 2;
		else if (pwdb_all > 26 && pwdb_all <= 34)
			pwdb_all -= 6;
		else if (pwdb_all > 14 && pwdb_all <= 26)
			pwdb_all -= 8;
		else if (pwdb_all > 4 && pwdb_all <= 14)
			pwdb_all -= 4;
		if (!cck_highpwr) {
			if (pwdb_all >= 80)
				pwdb_all = ((pwdb_all-80)<<1) +
					   ((pwdb_all-80)>>1) + 80;
			else if ((pwdb_all <= 78) && (pwdb_all >= 20))
				pwdb_all += 3;
			if (pwdb_all > 100)
				pwdb_all = 100;
		}

		pstatus->rx_pwdb_all = pwdb_all;
		pstatus->recvsignalpower = rx_pwr_all;

		/* (3) Get Signal Quality (EVM) */
		if (bpacket_match_bssid) {
			u8 sq;

			if (pstatus->rx_pwdb_all > 40)
				sq = 100;
			else {
				sq = cck_buf->sq_rpt;
				if (sq > 64)
					sq = 0;
				else if (sq < 20)
					sq = 100;
				else
					sq = ((64 - sq) * 100) / 44;
			}

			pstatus->signalquality = sq;
			pstatus->rx_mimo_signalquality[0] = sq;
			pstatus->rx_mimo_signalquality[1] = -1;
		}
	} else {
		rtlpriv->dm.rfpath_rxenable[0] =
		    rtlpriv->dm.rfpath_rxenable[1] = true;

		/* (1)Get RSSI for HT rate */
		for (i = RF90_PATH_A; i < RF6052_MAX_PATH; i++) {
			/* we will judge RF RX path now. */
			if (rtlpriv->dm.rfpath_rxenable[i])
				rf_rx_num++;

			rx_pwr[i] = ((p_drvinfo->gain_trsw[i] &
				      0x3f) * 2) - 110;

			/* Translate DBM to percentage. */
			rssi = rtl_query_rxpwrpercentage(rx_pwr[i]);
			total_rssi += rssi;

			/* Get Rx snr value in DB */
			rtlpriv->stats.rx_snr_db[i] =
				(long)(p_drvinfo->rxsnr[i] / 2);

			/* Record Signal Strength for next packet */
			if (bpacket_match_bssid)
				pstatus->rx_mimo_signalstrength[i] = (u8)rssi;
		}

		/* (2)PWDB, Average PWDB cacluated by
		 * hardware (for rate adaptive)
		 */
		rx_pwr_all = ((p_drvinfo->pwdb_all >> 1) & 0x7f) - 110;

		pwdb_all = rtl_query_rxpwrpercentage(rx_pwr_all);
		pstatus->rx_pwdb_all = pwdb_all;
		pstatus->rxpower = rx_pwr_all;
		pstatus->recvsignalpower = rx_pwr_all;

		/* (3)EVM of HT rate */
		if (pstatus->is_ht && pstatus->rate >= DESC92C_RATEMCS8 &&
		    pstatus->rate <= DESC92C_RATEMCS15)
			max_spatial_stream = 2;
		else
			max_spatial_stream = 1;

		for (i = 0; i < max_spatial_stream; i++) {
			evm = rtl_evm_db_to_percentage(p_drvinfo->rxevm[i]);

			if (bpacket_match_bssid) {
				/* Fill value in RFD, Get the first
				 * spatial stream onlyi
				 */
				if (i == 0)
					pstatus->signalquality =
						(u8)(evm & 0xff);
				pstatus->rx_mimo_signalquality[i] =
					(u8)(evm & 0xff);
			}
		}
	}

	/* UI BSS List signal strength(in percentage),
	 * make it good looking, from 0~100.
	 */
	if (is_cck)
		pstatus->signalstrength = (u8)(rtl_signal_scale_mapping(hw,
			pwdb_all));
	else if (rf_rx_num != 0)
		pstatus->signalstrength = (u8)(rtl_signal_scale_mapping(hw,
			total_rssi /= rf_rx_num));
	/*HW antenna diversity*/
	rtldm->fat_table.antsel_rx_keep_0 = phystrpt->ant_sel;
	rtldm->fat_table.antsel_rx_keep_1 = phystrpt->ant_sel_b;
	rtldm->fat_table.antsel_rx_keep_2 = phystrpt->antsel_rx_keep_2;
}

static void _rtl88ee_smart_antenna(struct ieee80211_hw *hw,
	struct rtl_stats *pstatus)
{
	struct rtl_dm *rtldm = rtl_dm(rtl_priv(hw));
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	u8 antsel_tr_mux;
	struct fast_ant_training *pfat_table = &rtldm->fat_table;

	if (rtlefuse->antenna_div_type == CG_TRX_SMART_ANTDIV) {
		if (pfat_table->fat_state == FAT_TRAINING_STATE) {
			if (pstatus->packet_toself) {
				antsel_tr_mux =
					(pfat_table->antsel_rx_keep_2 << 2) |
					(pfat_table->antsel_rx_keep_1 << 1) |
					pfat_table->antsel_rx_keep_0;
				pfat_table->ant_sum[antsel_tr_mux] +=
					pstatus->rx_pwdb_all;
				pfat_table->ant_cnt[antsel_tr_mux]++;
			}
		}
	} else if ((rtlefuse->antenna_div_type == CG_TRX_HW_ANTDIV) ||
	(rtlefuse->antenna_div_type == CGCS_RX_HW_ANTDIV)) {
		if (pstatus->packet_toself || pstatus->packet_matchbssid) {
			antsel_tr_mux = (pfat_table->antsel_rx_keep_2 << 2) |
					(pfat_table->antsel_rx_keep_1 << 1) |
					pfat_table->antsel_rx_keep_0;
			rtl88e_dm_ant_sel_statistics(hw, antsel_tr_mux, 0,
						     pstatus->rx_pwdb_all);
		}

	}
}

static void _rtl88ee_translate_rx_signal_stuff(struct ieee80211_hw *hw,
					       struct sk_buff *skb,
					       struct rtl_stats *pstatus,
					       u8 *pdesc,
					       struct rx_fwinfo_88e *p_drvinfo)
{
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	struct ieee80211_hdr *hdr;
	u8 *tmp_buf;
	u8 *praddr;
	u8 *psaddr;
	__le16 fc;
	bool packet_matchbssid, packet_toself, packet_beacon;

	tmp_buf = skb->data + pstatus->rx_drvinfo_size + pstatus->rx_bufshift;

	hdr = (struct ieee80211_hdr *)tmp_buf;
	fc = hdr->frame_control;
	praddr = hdr->addr1;
	psaddr = ieee80211_get_SA(hdr);
	memcpy(pstatus->psaddr, psaddr, ETH_ALEN);

	packet_matchbssid = ((!ieee80211_is_ctl(fc)) &&
	     (ether_addr_equal(mac->bssid, ieee80211_has_tods(fc) ?
			       hdr->addr1 : ieee80211_has_fromds(fc) ?
			       hdr->addr2 : hdr->addr3)) &&
			       (!pstatus->hwerror) &&
			       (!pstatus->crc) && (!pstatus->icv));

	packet_toself = packet_matchbssid &&
	    (ether_addr_equal(praddr, rtlefuse->dev_addr));

	if (ieee80211_is_beacon(hdr->frame_control))
		packet_beacon = true;
	else
		packet_beacon = false;

	_rtl88ee_query_rxphystatus(hw, pstatus, pdesc, p_drvinfo,
				   packet_matchbssid, packet_toself,
				   packet_beacon);
	_rtl88ee_smart_antenna(hw, pstatus);
	rtl_process_phyinfo(hw, tmp_buf, pstatus);
}
bool rtl88eu_rx_query_desc(struct ieee80211_hw *hw,
			   struct rtl_stats *status,
			   struct ieee80211_rx_status *rx_status,
			   u8 *pdesc, struct sk_buff *skb)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rx_fwinfo_88e *p_drvinfo;
	struct ieee80211_hdr *hdr;

	u32 phystatus = GET_RX_DESC_PHYST(pdesc);
	status->packet_report_type = (u8)GET_RX_STATUS_DESC_RPT_SEL(pdesc);
	if (status->packet_report_type == TX_REPORT2)
		status->length = (u16)GET_RX_RPT2_DESC_PKT_LEN(pdesc);
	else
		status->length = (u16)GET_RX_DESC_PKT_LEN(pdesc);
	status->rx_drvinfo_size = (u8)GET_RX_DESC_DRV_INFO_SIZE(pdesc) *
	    RX_DRV_INFO_SIZE_UNIT;
	status->rx_bufshift = (u8)(GET_RX_DESC_SHIFT(pdesc) & 0x03);
	status->icv = (u16)GET_RX_DESC_ICV(pdesc);
	status->crc = (u16)GET_RX_DESC_CRC32(pdesc);
	status->hwerror = (status->crc | status->icv);
	status->decrypted = !GET_RX_DESC_SWDEC(pdesc);
	status->rate = (u8)GET_RX_DESC_RXMCS(pdesc);
	status->shortpreamble = (u16)GET_RX_DESC_SPLCP(pdesc);
	status->isampdu = (bool) (GET_RX_DESC_PAGGR(pdesc) == 1);
	status->isfirst_ampdu = (bool)((GET_RX_DESC_PAGGR(pdesc) == 1) &&
				(GET_RX_DESC_FAGGR(pdesc) == 1));
	if (status->packet_report_type == NORMAL_RX)
		status->timestamp_low = GET_RX_DESC_TSFL(pdesc);
	status->rx_is40Mhzpacket = (bool) GET_RX_DESC_BW(pdesc);
	status->is_ht = (bool)GET_RX_DESC_RXHT(pdesc);

	//status->is_cck = RTL8188_RX_HAL_IS_CCK_RATE(status->rate);

	status->macid = GET_RX_DESC_MACID(pdesc);
	if (GET_RX_STATUS_DESC_MAGIC_MATCH(pdesc))
		status->wake_match = BIT(2);
	else if (GET_RX_STATUS_DESC_MAGIC_MATCH(pdesc))
		status->wake_match = BIT(1);
	else if (GET_RX_STATUS_DESC_UNICAST_MATCH(pdesc))
		status->wake_match = BIT(0);
	else
		status->wake_match = 0;
	if (status->wake_match)
		RT_TRACE(rtlpriv, COMP_RXDESC, DBG_LOUD,
		"GGGGGGGGGGGGGet Wakeup Packet!! WakeMatch=%d\n",
		status->wake_match);
	rx_status->freq = hw->conf.chandef.chan->center_freq;
	rx_status->band = hw->conf.chandef.chan->band;

	hdr = (struct ieee80211_hdr *)(skb->data + status->rx_drvinfo_size
			+ status->rx_bufshift);

	if (status->crc)
		rx_status->flag |= RX_FLAG_FAILED_FCS_CRC;

	if (status->rx_is40Mhzpacket)
		rx_status->flag |= RX_FLAG_40MHZ;

	if (status->is_ht)
		rx_status->flag |= RX_FLAG_HT;

	rx_status->flag |= RX_FLAG_MACTIME_START;

	/* hw will set status->decrypted true, if it finds the
	 * frame is open data frame or mgmt frame.
	 * So hw will not decryption robust managment frame
	 * for IEEE80211w but still set status->decrypted
	 * true, so here we should set it back to undecrypted
	 * for IEEE80211w frame, and mac80211 sw will help
	 * to decrypt it
	 */
	if (status->decrypted) {
		if ((!_ieee80211_is_robust_mgmt_frame(hdr)) &&
		    (ieee80211_has_protected(hdr->frame_control)))
			rx_status->flag |= RX_FLAG_DECRYPTED;
		else
			rx_status->flag &= ~RX_FLAG_DECRYPTED;
	}

	/* rate_idx: index of data rate into band's
	 * supported rates or MCS index if HT rates
	 * are use (RX_FLAG_HT)
	 * Notice: this is diff with windows define
	 */
	rx_status->rate_idx = rtlwifi_rate_mapping(hw, status->is_ht,
						   false, status->rate);

	rx_status->mactime = status->timestamp_low;
	if (phystatus == true) {
		p_drvinfo = (struct rx_fwinfo_88e *)(skb->data +
						     status->rx_bufshift);

		_rtl88ee_translate_rx_signal_stuff(hw,
						   skb, status, pdesc,
						   p_drvinfo);
	}
	rx_status->signal = status->recvsignalpower + 10;
	if (status->packet_report_type == TX_REPORT2) {
		status->macid_valid_entry[0] =
			 GET_RX_RPT2_DESC_MACID_VALID_1(pdesc);
		status->macid_valid_entry[1] =
			 GET_RX_RPT2_DESC_MACID_VALID_2(pdesc);
	}
	return true;
}

#define RTL_RX_DRV_INFO_UNIT		8

static void _rtl_rx_process(struct ieee80211_hw *hw, struct sk_buff *skb)
{
/* TODO */
#if 0
	struct ieee80211_rx_status *rx_status =
		 (struct ieee80211_rx_status *)IEEE80211_SKB_RXCB(skb);
	u32 skb_len, pkt_len, drvinfo_len;
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 *rxdesc;
	struct rtl_stats stats = {
		.signal = 0,
		.rate = 0,
	};
	struct rx_fwinfo_88e *p_drvinfo;
	bool bv;
	__le16 fc;
	struct ieee80211_hdr *hdr;

	memset(rx_status, 0, sizeof(*rx_status));
	rxdesc	= skb->data;
	skb_len	= skb->len;
	drvinfo_len = (GET_RX_DESC_DRVINFO_SIZE(rxdesc) * RTL_RX_DRV_INFO_UNIT);
	pkt_len		= GET_RX_DESC_PKT_LEN(rxdesc);
	/* TODO: Error recovery. drop this skb or something. */
	WARN_ON(skb_len < (pkt_len + RTL_RX_DESC_SIZE + drvinfo_len));
	stats.length = (u16) GET_RX_DESC_PKT_LEN(rxdesc);
	stats.rx_drvinfo_size = (u8)GET_RX_DESC_DRVINFO_SIZE(rxdesc) *
				RX_DRV_INFO_SIZE_UNIT;
	stats.rx_bufshift = (u8) (GET_RX_DESC_SHIFT(rxdesc) & 0x03);
	stats.icv = (u16) GET_RX_DESC_ICV(rxdesc);
	stats.crc = (u16) GET_RX_DESC_CRC32(rxdesc);
	stats.hwerror = (stats.crc | stats.icv);
	stats.decrypted = !GET_RX_DESC_SWDEC(rxdesc);
	stats.rate = (u8) GET_RX_DESC_RX_MCS(rxdesc);
	stats.shortpreamble = (u16) GET_RX_DESC_SPLCP(rxdesc);
	stats.isampdu = (bool) ((GET_RX_DESC_PAGGR(rxdesc) == 1)
				   && (GET_RX_DESC_FAGGR(rxdesc) == 1));
	stats.timestamp_low = GET_RX_DESC_TSFL(rxdesc);
	stats.rx_is40Mhzpacket = (bool) GET_RX_DESC_BW(rxdesc);
	stats.is_ht = (bool)GET_RX_DESC_RX_HT(rxdesc);
	/* TODO: is center_freq changed when doing scan? */
	/* TODO: Shall we add protection or just skip those two step? */
	rx_status->freq = hw->conf.chandef.chan->center_freq;
	rx_status->band = hw->conf.chandef.chan->band;
	if (GET_RX_DESC_CRC32(rxdesc))
		rx_status->flag |= RX_FLAG_FAILED_FCS_CRC;
	if (!GET_RX_DESC_SWDEC(rxdesc))
		rx_status->flag |= RX_FLAG_DECRYPTED;
	if (GET_RX_DESC_BW(rxdesc))
		rx_status->flag |= RX_FLAG_40MHZ;
	if (GET_RX_DESC_RX_HT(rxdesc))
		rx_status->flag |= RX_FLAG_HT;
	/* Data rate */
	rx_status->rate_idx = rtlwifi_rate_mapping(hw, stats.is_ht,
						   false, stats.rate);
	/*  There is a phy status after this rx descriptor. */
	if (GET_RX_DESC_PHY_STATUS(rxdesc)) {
		p_drvinfo = (struct rx_fwinfo_88e *)(rxdesc + RTL_RX_DESC_SIZE);
		_rtl88ee_translate_rx_signal_stuff(hw, skb, &stats,
						   rxdesc, p_drvinfo);
	}
	skb_pull(skb, (drvinfo_len + RTL_RX_DESC_SIZE));
	hdr = (struct ieee80211_hdr *)(skb->data);
	fc = hdr->frame_control;
	bv = ieee80211_is_probe_resp(fc);
	if (bv)
		RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG,
			 "Got probe response frame\n");
	if (ieee80211_is_beacon(fc))
		RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG, "Got beacon frame\n");
	if (ieee80211_is_data(fc))
		RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG, "Got data frame\n");
	RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG,
		 "Fram: fc = 0x%X addr1 = 0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X\n",
		 fc,
		 (u32)hdr->addr1[0], (u32)hdr->addr1[1],
		 (u32)hdr->addr1[2], (u32)hdr->addr1[3],
		 (u32)hdr->addr1[4], (u32)hdr->addr1[5]);
	memcpy(IEEE80211_SKB_RXCB(skb), rx_status, sizeof(*rx_status));
	ieee80211_rx(hw, skb);
#endif
}

void  rtl8188eu_rx_hdl(struct ieee80211_hw *hw, struct sk_buff * skb)
{
	_rtl_rx_process(hw, skb);
}

void rtl8188eu_rx_segregate_hdl(
	struct ieee80211_hw *hw,
	struct sk_buff *skb,
	struct sk_buff_head *skb_list)
{
}

/*----------------------------------------------------------------------
 *
 *	Tx handler
 *
 *---------------------------------------------------------------------- */
void rtl8188eu_tx_cleanup(struct ieee80211_hw *hw, struct sk_buff  *skb)
{
}

int rtl8188eu_tx_post_hdl(struct ieee80211_hw *hw, struct urb *urb,
			 struct sk_buff *skb)
{
	return 0;
}

struct sk_buff *rtl8188eu_tx_aggregate_hdl(struct ieee80211_hw *hw,
					   struct sk_buff_head *list)
{
	return skb_dequeue(list);
}

/*======================================== trx ===============================*/

/**
 *	For HW recovery information
 */
static void _rtl_tx_desc_checksum(u8 *txdesc)
{
	u16 *ptr = (u16 *)txdesc;
	u16	checksum = 0;
	u32 index;

	/* Clear first */
	SET_TX_DESC_TX_DESC_CHECKSUM(txdesc, 0);
	for (index = 0; index < 16; index++)
		checksum = checksum ^ (*(ptr + index));
	SET_TX_DESC_TX_DESC_CHECKSUM(txdesc, checksum);
}

void rtl88eu_tx_fill_desc(struct ieee80211_hw *hw,
			  struct ieee80211_hdr *hdr, u8 *pdesc_tx,
			  u8 *txbd, struct ieee80211_tx_info *info,
			  struct ieee80211_sta *sta,
			  struct sk_buff *skb,
			  u8 hw_queue, struct rtl_tcb_desc *ptcb_desc)

{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	u16 seq_number;
	__le16 fc = hdr->frame_control;
	u16 pktlen = skb->len;
	enum rtl_desc_qsel fw_qsel = _rtl8188eu_mq_to_descq(hw, fc,
						skb_get_queue_mapping(skb));
	u8 *txdesc;

	seq_number = (le16_to_cpu(hdr->seq_ctrl) & IEEE80211_SCTL_SEQ) >> 4;
	rtl_get_tcb_desc(hw, info, sta, skb, ptcb_desc);
	txdesc = (u8 *)skb_push(skb, RTL_TX_HEADER_SIZE);
	memset(txdesc, 0, RTL_TX_HEADER_SIZE);
	
	SET_TX_DESC_OWN(txdesc, 1);
	SET_TX_DESC_LAST_SEG(txdesc, 1);
	SET_TX_DESC_FIRST_SEG(txdesc, 1);
	SET_TX_DESC_PKT_SIZE(txdesc, pktlen);
	SET_TX_DESC_OFFSET(txdesc, 32);
	
	if (is_multicast_ether_addr(ieee80211_get_DA(hdr)) ||
	    is_broadcast_ether_addr(ieee80211_get_DA(hdr)))
		SET_TX_DESC_BMC(txdesc, 1);
	/* TODO */
	SET_TX_DESC_PKT_OFFSET(txdesc, RTL_DUMMY_OFFSET);
	SET_TX_DESC_USE_RATE(txdesc, 1);
	if (ieee80211_is_data(fc) || ieee80211_is_data_qos(fc)) {
		SET_TX_DESC_MACID(txdesc, ptcb_desc->mac_id);
		SET_TX_DESC_QUEUE_SEL(txdesc, fw_qsel);
		SET_TX_DESC_RATE_ID(txdesc, ptcb_desc->ratr_index);
	
		rcu_read_lock();
		sta = ieee80211_find_sta(mac->vif, mac->bssid);
		if (sta) {
			u8 ampdu_density = sta->ht_cap.ampdu_density;
			SET_TX_DESC_AMPDU_DENSITY(txdesc, ampdu_density);
		}
		rcu_read_unlock();
		if (info->control.hw_key) {
			struct ieee80211_key_conf *keyconf = info->control.hw_key;
			switch (keyconf->cipher) {
			case WLAN_CIPHER_SUITE_WEP40:
			case WLAN_CIPHER_SUITE_WEP104:
			case WLAN_CIPHER_SUITE_TKIP:
				SET_TX_DESC_SEC_TYPE(txdesc, 0x1);
				break;
			case WLAN_CIPHER_SUITE_CCMP:
				SET_TX_DESC_SEC_TYPE(txdesc, 0x3);
				break;
			default:
				SET_TX_DESC_SEC_TYPE(txdesc, 0x0);
				break;
			}
		}
		if (info->flags & IEEE80211_TX_CTL_AMPDU) {
			SET_TX_DESC_AGG_ENABLE(txdesc, 1);
			SET_TX_DESC_MAX_AGG_NUM(txdesc, 0x1f);
			SET_TX_DESC_MCSG1_MAX_LEN(txdesc, 6);
			SET_TX_DESC_MCSG2_MAX_LEN(txdesc, 6);
			SET_TX_DESC_MCSG3_MAX_LEN(txdesc, 6);
			SET_TX_DESC_MCS7_SGI_MAX_LEN(txdesc, 6);
		} else {
			SET_TX_DESC_AGG_BREAK(txdesc, 1);
		}
		SET_TX_DESC_SEQ(txdesc, seq_number);

		/* TODO */
		if (ieee80211_is_data_qos(fc))
			SET_TX_DESC_QOS(txdesc, 1);
		/* TODO */
		/* USB_TXAGG_NUM */
		
		if (!rtlpriv->ra.is_special_data) {
			/* SET_TX_DESC_RTS_ENABLE(txdesc, 1); */
			/* SET_TX_DESC_HW_RTS_ENABLE(txdesc, 1); */
			SET_TX_DESC_DATA_BW(txdesc, 1);
			SET_TX_DESC_TX_SUB_CARRIER(txdesc, 3);
			SET_TX_DESC_RTS_RATE(txdesc, 8);
			SET_TX_DESC_DATA_RATE_FB_LIMIT(txdesc, 0x1f);
			SET_TX_DESC_RTS_RATE_FB_LIMIT(txdesc, 0xf);
			if (ptcb_desc->use_shortgi)
				SET_TX_DESC_DATA_SHORTGI(txdesc, 1);
			SET_TX_DESC_TX_RATE(txdesc, ptcb_desc->hw_rate);
			/* TODO */
			/* SET_TX_DESC_PWR_STATUS(txdesc, pwr_status); */
		} else {
			SET_TX_DESC_AGG_BREAK(txdesc, 1);
			if (ptcb_desc->use_shortpreamble)
				SET_TX_DESC_DATA_SHORT(txdesc, 1);
			SET_TX_DESC_TX_RATE(txdesc, ptcb_desc->hw_rate);
		}
	} else if (ieee80211_is_mgmt(fc)){
		SET_TX_DESC_MACID(txdesc, ptcb_desc->mac_id);
		SET_TX_DESC_QUEUE_SEL(txdesc, fw_qsel);
		SET_TX_DESC_RATE_ID(txdesc, ptcb_desc->ratr_index);
		/* TODO */
#if 0
		if (pxmitframe->ack_report)
			SET_TX_DESC_CCX(txdesc, 1);
#endif
		SET_TX_DESC_SEQ(txdesc, seq_number);
		SET_TX_DESC_RETRY_LIMIT_ENABLE(txdesc, 1);
		/* TODO */
		SET_TX_DESC_DATA_RETRY_LIMIT(txdesc, 0x6);
		SET_TX_DESC_TX_RATE(txdesc, 1);
	} else {
		SET_TX_DESC_MACID(txdesc, ptcb_desc->mac_id);
		SET_TX_DESC_RATE_ID(txdesc, ptcb_desc->ratr_index);
		SET_TX_DESC_SEQ(txdesc, seq_number);
		SET_TX_DESC_TX_RATE(txdesc, 1);
	}
	if (!ieee80211_is_data_qos(fc)) {
		SET_TX_DESC_HWSEQ_EN(txdesc, 1);
		SET_TX_DESC_HWSEQ_SSN(txdesc, 1);
	}
	rtl88e_dm_set_tx_ant_by_tx_info(hw, txdesc, ptcb_desc->mac_id);
	_rtl_tx_desc_checksum(txdesc);
}

void rtl88eu_tx_fill_cmddesc(struct ieee80211_hw *hw,
			     u8 *pdesc, bool firstseg,
			     bool lastseg, struct sk_buff *skb)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 fw_queue = QSLT_BEACON;
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)(skb->data);
	__le16 fc = hdr->frame_control;

	memset((void *)pdesc, 0, RTL_TX_HEADER_SIZE);
	if (firstseg)
		SET_TX_DESC_OFFSET(pdesc, RTL_TX_HEADER_SIZE);
	SET_TX_DESC_TX_RATE(pdesc, DESC_RATE1M);
	SET_TX_DESC_SEQ(pdesc, 0);
	/* TODO */
	//SET_TX_DESC_LINIP(pdesc, 0);
	SET_TX_DESC_QUEUE_SEL(pdesc, fw_queue);
	SET_TX_DESC_FIRST_SEG(pdesc, 1);
	SET_TX_DESC_LAST_SEG(pdesc, 1);
	SET_TX_DESC_RATE_ID(pdesc, 7);
	SET_TX_DESC_MACID(pdesc, 0);
	SET_TX_DESC_OWN(pdesc, 1);
	SET_TX_DESC_PKT_SIZE(pdesc, (u16)skb->len);
	SET_TX_DESC_FIRST_SEG(pdesc, 1);
	SET_TX_DESC_LAST_SEG(pdesc, 1);
	SET_TX_DESC_OFFSET(pdesc, 0x20);
	SET_TX_DESC_USE_RATE(pdesc, 1);
	if (!ieee80211_is_data_qos(fc)) {
		SET_TX_DESC_HWSEQ_EN(pdesc, 1);
		/* TODO */
		//SET_TX_DESC_PKT_ID(pdesc, 8);
	}
	RT_PRINT_DATA(rtlpriv, COMP_CMD, DBG_LOUD, "H2C Tx Cmd Content",
		      pdesc, RTL_TX_DESC_SIZE);
}

bool rtl88eu_cmd_send_packet(struct ieee80211_hw *hw, struct sk_buff *skb)
{
	return true;
}
