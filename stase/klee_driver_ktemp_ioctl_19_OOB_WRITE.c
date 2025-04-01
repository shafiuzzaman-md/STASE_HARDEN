// Auto-generated KLEE driver for ktemp_ioctl
#include "klee/klee.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define CON_IOCTL 0x1337
#define WIFI_SSID_MAX 32
#define WIFI_PASS_MAX 64
#define BUFFER_SIZE  WIFI_SSID_MAX - 1
typedef unsigned long size_t;  

// struct net_creds {
//     char net_ssid[WIFI_SSID_MAX];
//     char net_passw[WIFI_PASS_MAX];
// };

// struct ktemp_dev_t {
//     struct net_creds net_creds;
//     void (*connect_net)(void);
//     unsigned int temp;
//     unsigned int humidity;
// };

struct file {
    int dummy;
};

static long int ktemp_ioctl(struct file *f, unsigned int cmd, long unsigned int arg, size_t net_ssid_size_sym);

//struct ktemp_dev_t ktemp_dev;

void dummy_connect_net() {
    // Safe stub
}

#include "instrumented_code/ktemp25_19_OOB_WRITE.c"



int main() {
    struct file fake_file;
    unsigned int cmd = CON_IOCTL;
    char arg;
    size_t net_ssid_size_sym;

    klee_make_symbolic(&arg, sizeof(arg), "arg");
    klee_make_symbolic(&net_ssid_size_sym, sizeof(net_ssid_size_sym), "net_ssid_size_sym");
 

    ktemp_ioctl(&fake_file, cmd, arg, net_ssid_size_sym);

    return 0;
}
