#ifndef __KTEMP_DRV__H
#define __KTEMP_DRV__H

#define WIFI_SSID_MAX 32
#define WIFI_PASS_MAX 63
#define DEV_NAME "ktemp25"

#define CON_IOCTL 0x2532

struct net_creds {
  char net_ssid[WIFI_SSID_MAX - 1];
  char net_passw[WIFI_PASS_MAX - 1];
};

struct ktemp_dev {
  struct net_creds net_creds;
  void (*connect_net)(void);
  unsigned int temp;
  unsigned int humidity;
};


#endif /* __KTEMP_DRV__H */
