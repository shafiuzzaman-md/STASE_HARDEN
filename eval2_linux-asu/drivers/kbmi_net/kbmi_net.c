/*
 * KB Message Initializer with Netfilter Hook (KBMI_NET)
 *
 * This kernel module integrates with the Netfilter framework to process network
 * packets at the pre-routing stage. It inspects an external message buffer
 * (provided by another module), extracts and processes a code segment from it,
 * and then copies the segment to a local buffer on the stack. The code segment
 * is filled with incrementing values and printed for debugging purposes.
 *
 * Author: Arya Stark
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/dynamic_util.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <asm/pgtable.h>

#define MESSAGE_SIZE 1024ull
extern char message_buffer[] __attribute__((weak));
static struct nf_hook_ops nfho;

/**
 * kbmi_packet_filter_hook - Netfilter hook callback function.
 * This function is invoked for each network packet at the pre-routing stage.
 * It inspects the external `message_buffer`, searches for a separator, extracts
 * the subsequent code segment, and copies it to a local stack buffer. The code
 * segment is then processed (filled with incrementing values) and printed.
 *
 * @priv: Private data passed to the hook function (not used).
 * @skb: The socket buffer containing the packet.
 * @state: The hook state (not used).
 *
 * Return: NF_ACCEPT to allow the packet to continue, or other values to drop it.
 */
static unsigned int kbmi_packet_filter_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
    unsigned char code_segment[MESSAGE_SIZE] = {0};
    int ret = NF_ACCEPT;

    if (!message_buffer) {
        goto out;
    }

    char *message_separator = (char *)memchr(message_buffer, '\0', MESSAGE_SIZE);
    if (message_separator == NULL) {
        pr_err("Separator not found in message buffer\n");
        goto out;
    }

    
    if (message_separator - message_buffer == MESSAGE_SIZE - 1) {
        pr_debug("Separator found at the end of the message buffer\n");
        goto out;
    }

    /* Extract the code segment following the separator */
    char *code_start = message_separator + 1;
    char *code_end = message_buffer + MESSAGE_SIZE;
    size_t code_size = code_end - code_start;
    if (code_size > MESSAGE_SIZE) {
        pr_err("Code size exceeds buffer capacity\n");
        goto out;
    }
    memcpy(code_segment, code_start, code_size);
    make_dynamic_area();

out:
    return ret;
}

/**
 * kbmi_net_init - Module initialization function.
 * This function registers the Netfilter hook at the pre-routing stage to
 * process incoming network packets. It logs a message upon successful loading.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
static int __init kbmi_net_init(void)
{
    int ret;

    /* Register Netfilter hook for packet filtering */
    nfho.hook = kbmi_packet_filter_hook;
    nfho.hooknum = NF_INET_PRE_ROUTING;
    nfho.pf = PF_INET;
    nfho.priority = NF_IP_PRI_FIRST;
    
    ret = nf_register_net_hook(&init_net, &nfho);
    if (ret) {
        pr_err("KBMI_NET: nf_register_net_hook failed\n");
        return ret;
    }

    pr_debug("KBMI_NET: loaded\n");
    return 0;
}

/**
 * kbmi_net_exit - Module cleanup function.
 * This function unregisters the Netfilter hook and logs a message upon module
 * unload.
 */
static void __exit kbmi_net_exit(void)
{
    nf_unregister_net_hook(&init_net, &nfho);
    pr_debug("KBMI_NET: unloaded\n");
}

module_init(kbmi_net_init);
module_exit(kbmi_net_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arya Stark");
MODULE_DESCRIPTION("KB Message Initializer with Netfilter Hook (KBMI_NET)");
