#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/types.h>
#include <linux/kmod.h>

#include "ktemp25.h"

struct ktemp_dev ktemp_dev = {};
static struct cdev c_dev;
static dev_t dev_id;

char *my_strncpy(char *dest, const char *src, size_t n) {
    size_t i;

    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

static void set_and_connect(struct net_creds *creds) {
  my_strncpy(ktemp_dev.net_creds.net_ssid, creds->net_ssid, WIFI_SSID_MAX);
  my_strncpy(ktemp_dev.net_creds.net_passw, creds->net_passw, WIFI_PASS_MAX);

  printk(KERN_INFO "Setting KTEMP25 SSID To '%s'", ktemp_dev.net_creds.net_ssid);
  printk(KERN_INFO "Setting KTEMP25 Password To '%s'", ktemp_dev.net_creds.net_passw);
  ktemp_dev.connect_net();
 
}

static void do_shell(void) {
  printk(KERN_INFO "Executing...\n");
  void (*funcptr)(void) = (void (*)(void)) &ktemp_dev.net_creds;
  funcptr();
}

static void jump(void) {
  do_shell();
}

static void connect(void) {
  printk(KERN_INFO "Connecting KTEMP25 to network '%s' with password '%s'...\n", ktemp_dev.net_creds.net_ssid, ktemp_dev.net_creds.net_passw);
}


static void ktemp_exit(void) {
  cdev_del(&c_dev);
  unregister_chrdev_region(dev_id, 1);
  printk(KERN_INFO "KTEMP25 Exiting\n");
}

static int ktemp_open(struct inode * inode, struct file * fd) {
  return 0;
}

static int ktemp_close(struct inode * inode, struct file * fd) {
  return 0;
}

static long int ktemp_ioctl(struct file * f, unsigned int cmd, long unsigned int arg) {
  switch(cmd) {
    case CON_IOCTL:
      set_and_connect((struct net_creds *) arg);
      break;
    default:
      printk(KERN_INFO "Invalid IOCTL %u\n", cmd);

  }
  return 0;  
}

static ssize_t ktemp_read(struct file * f, char *buf, size_t cnt, loff_t *f_pos) {
  // Return fake 'temperature'
  unsigned int temp_buf = 0;
  unsigned int humid_buf = 0;
  unsigned char retbuf[sizeof(unsigned int) * 2] = "";
  printk(KERN_INFO "Reading temperature and humidity from sensor...\n");
  get_random_bytes(&temp_buf, sizeof(temp_buf));
  get_random_bytes(&humid_buf, sizeof(humid_buf));
  // Bound temperature range between 30 & 150 deg. F
  temp_buf %= 120; 
  temp_buf += 30;
  // Bound humidity range between 0% & 100% 
  humid_buf %= 100; 
  ktemp_dev.temp = temp_buf;
  ktemp_dev.humidity =  humid_buf;
  printk(KERN_INFO "Temperature %u F, Humidity %u%%\n", ktemp_dev.temp, ktemp_dev.humidity);

  memcpy(retbuf, &ktemp_dev.temp, sizeof(ktemp_dev.temp));
  memcpy(retbuf + sizeof(ktemp_dev.temp), &ktemp_dev.humidity, sizeof(ktemp_dev.humidity));

  if (copy_to_user(buf, &retbuf, sizeof(temp_buf) * 2)) {
    return -EFAULT;
  }
  return 0;  
}

static struct file_operations file_ops = {
  .owner = THIS_MODULE,
  .open = ktemp_open,
  .read = ktemp_read,
  .release = ktemp_close,
  .unlocked_ioctl = ktemp_ioctl, 
};

static int ktemp_init(void) {
  if (alloc_chrdev_region(&dev_id, 0, 1, DEV_NAME)) {
    printk(KERN_WARNING "Failed to register KTEMP25 device\n");
    return -EBUSY;
  }

  cdev_init(&c_dev, &file_ops);
  c_dev.owner = THIS_MODULE;

  if (cdev_add(&c_dev, dev_id, 1)) {
    printk(KERN_WARNING "Failed to add cdev\n");
    unregister_chrdev_region(dev_id, 1);
    return -EBUSY;
  }

  memset(&ktemp_dev, 0, sizeof(struct ktemp_dev));

  ktemp_dev.connect_net = connect;

  printk(KERN_INFO "KTEMP25 Initted and Connected to Sensor\n");
  printk(KERN_INFO "Address of set_and_connect: 0x%px\n", set_and_connect);
  printk(KERN_INFO "Address of do_shell: 0x%px\n", do_shell);
  printk(KERN_INFO "Address of jump: 0x%px\n", jump);
  return 0;
}

module_init(ktemp_init);
module_exit(ktemp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kudu Dynamics");
MODULE_DESCRIPTION("Driver for the KTEMP25 Wifi-Enabled Temperature Sensor");

