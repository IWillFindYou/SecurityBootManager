#ifndef _USR_IWMGMT_H
#define _USR_IWMGMT_H

/** @file
 *
 * Wireless network interface management
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

struct net80211_device;
struct ap_info {
	char ssid[22];
	struct net80211_wlan *wlan;
	const char *crypt;
	const char *auth;
};

struct ap_list {
	struct ap_info ap[30];
	int len;
};

extern void iwstat ( struct net80211_device *dev );
extern int iwlist ( struct net80211_device *dev );
extern struct ap_list* get_iwlist ( struct net80211_device *dev );

#endif /* _USR_IWMGMT_H */
