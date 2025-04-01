/*
 * Virtual Network Packet Filter Kernel Module
 *
 * This module filters network packets on a virtual network interface (e.g., tap0).
 * It inspects packets based on custom-defined rules and drops or forwards packets
 * depending on their content. It maintains statistics on dropped packets and
 * manages state across multiple packet streams.
 *
 * Author: Satoshi Nakamoto
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/if_ether.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <linux/hashtable.h>
#include <linux/spinlock.h>
#include <linux/jhash.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>
#include <linux/atomic.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Satoshi Nakamoto");
MODULE_DESCRIPTION("Virtual Network Packet Filter");
MODULE_VERSION("1.0");

#define MAX_FLOW_ENTRIES 10

/* Atomic variable to keep track of dropped packets */
static atomic_t dropped_packets = ATOMIC_INIT(0);

/* Spinlock to protect the flow table */
static DEFINE_SPINLOCK(flow_table_lock);

/**
 * struct flow_entry - Represents a network flow in the flow table
 * @src_ip: Source IP address
 * @dst_ip: Destination IP address
 * @src_port: Source port
 * @dst_port: Destination port
 * @bytes: Total bytes transmitted in this flow
 * @node: Hash table node
 */
struct flow_entry {
	__be32 src_ip;
	__be32 dst_ip;
	__be16 src_port;
	__be16 dst_port;
	atomic64_t bytes;
	struct hlist_node node;
};

/* Define a hash table for flow entries; 2^10 = 1024 buckets */
DEFINE_HASHTABLE(flow_table, 10);

/* Netfilter hook option struct */
static struct nf_hook_ops nfho;

/* Procfs entry */
static struct proc_dir_entry *proc_entry;

/**
 * packet_filter_hook - Netfilter hook function to inspect and filter packets
 * @priv: Private data (unused)
 * @skb: Pointer to the socket buffer
 * @state: Netfilter state
 *
 * This function is called for each packet at the specified hook point.
 * It inspects the packet and applies filtering rules.
 *
 * Returns NF_ACCEPT to allow the packet, NF_DROP to drop it.
 */
static unsigned int packet_filter_hook(void *priv,
				       struct sk_buff *skb,
				       const struct nf_hook_state *state)
{
	struct iphdr *ip_header;
	struct tcphdr *tcp_header;
	struct udphdr *udp_header;
	struct tcphdr _tcph;
	struct udphdr _udph;
	unsigned int sport = 0, dport = 0;
	struct flow_entry *flow;
	bool drop = false;
	u32 key;
	struct net_device *in_dev = state->in;

	/* Filter only on a specific virtual interface, e.g., "eth0" */
	if (strcmp(in_dev->name, "eth0") != 0)
		return NF_ACCEPT;

	/* Ensure this is an IP packet */
	if (skb->protocol != htons(ETH_P_IP))
		return NF_ACCEPT;

	/* Get IP header */
	ip_header = ip_hdr(skb);
	if (!ip_header)
		return NF_ACCEPT;

	/* Get transport layer ports */
	if (ip_header->protocol == IPPROTO_TCP) {
		tcp_header = skb_header_pointer(skb, ip_header->ihl * 4, sizeof(_tcph), &_tcph);
		if (!tcp_header)
			return NF_ACCEPT;
		sport = ntohs(tcp_header->source);
		dport = ntohs(tcp_header->dest);
	} else if (ip_header->protocol == IPPROTO_UDP) {
		udp_header = skb_header_pointer(skb, ip_header->ihl * 4, sizeof(_udph), &_udph);
		if (!udp_header)
			return NF_ACCEPT;
		sport = ntohs(udp_header->source);
		dport = ntohs(udp_header->dest);
	}

	/* Filtering rule: Drop packets from source IP 192.168.1.100 */
	if (ip_header->saddr == htonl(0xC0A80164)) { /* 192.168.1.100 */
		pr_debug("packet_filter: Dropping packet from 192.168.1.100\n");
		atomic_inc(&dropped_packets);
		return NF_DROP;
	}

	/* Filtering rule: Drop packets with payload size greater than 1500 bytes */
	if (ntohs(ip_header->tot_len) >= 1480) {
		atomic_inc(&dropped_packets);
		return NF_DROP;
	}

	/* Manage flow table size to ensure it does not exceed the limit */
    struct hlist_node *tmp;
    int bkt, flow_count = 0;

    /* Count the current number of flow entries */
    hash_for_each(flow_table, bkt, flow, node) {
        flow_count++;
    }

    /* If the number of flow entries exceeds the limit, remove the oldest one */
    if (flow_count >= MAX_FLOW_ENTRIES) {
        /* Removing the first entry found in the hash table */
        hash_for_each_safe(flow_table, bkt, tmp, flow, node) {
            hash_del(&flow->node);
            kfree(flow);
            pr_debug("packet_filter: Flow table exceeded limit, removed oldest flow.\n");
            break;
        }
    }

	/* Per-flow state management */
	key = jhash_3words((u32)ip_header->saddr, (u32)ip_header->daddr,
			   ((u32)sport << 16) | (u32)dport, 0);

	spin_lock(&flow_table_lock);

	hash_for_each_possible(flow_table, flow, node, key) {
		if (flow->src_ip == ip_header->saddr &&
		    flow->dst_ip == ip_header->daddr &&
		    flow->src_port == sport &&
		    flow->dst_port == dport) {
			/* Flow exists, update bytes */
			atomic64_add(skb->len, &flow->bytes);
			goto flow_found;
		}
	}

	/* Flow not found, create new flow entry */
	flow = kmalloc(sizeof(*flow), GFP_ATOMIC);
	if (!flow) {
		pr_err("packet_filter: kmalloc failed\n");
		spin_unlock(&flow_table_lock);
		return NF_DROP;
	}
	flow->src_ip = ip_header->saddr;
	flow->dst_ip = ip_header->daddr;
	flow->src_port = sport;
	flow->dst_port = dport;
	atomic64_set(&flow->bytes, skb->len);
	hash_add(flow_table, &flow->node, key);

flow_found:
	spin_unlock(&flow_table_lock);

	/* Per-flow filtering rule: Drop flows exceeding size */
	if (atomic64_read(&flow->bytes) > (1024 * 1024)) {
		atomic_inc(&dropped_packets);
		drop = true;
	}

	if (drop)
		return NF_DROP;

	/* Accept the packet */
	return NF_ACCEPT;
}

/**
 * dropped_packets_show - Display the number of dropped packets in procfs
 * @m: seq_file structure
 * @v: void pointer (unused)
 *
 * This function writes the number of dropped packets to the seq_file. It uses
 * a spinlock to ensure that no race conditions occur while reading the 
 * dropped_packets variable.
 *
 * Returns 0 on success, negative error code on failure.
 */
static int dropped_packets_show(struct seq_file *m, void *v)
{
	int dropped_count=0;

	/* Acquire spinlock to prevent race conditions */
	spin_lock(&flow_table_lock);

	/* Safely read the dropped packet count */
	dropped_count = atomic_read(&dropped_packets);

	/* Release the spinlock */
	spin_unlock(&flow_table_lock);

	/* Check if dropped_count is negative (should never happen) */
	if (unlikely(dropped_count < 0)) {
		return -EINVAL;
	}

	/* Write the dropped packet count to the seq_file */
	seq_printf(m, "Dropped Packets: %u\n", dropped_count);

	return 0;
}

/**
 * dropped_packets_open - Open function for procfs entry
 * @inode: inode pointer
 * @file: file pointer
 *
 * This function verifies the validity of the inode and file structures before 
 * opening the procfs entry. It returns appropriate error codes if validation
 * fails, or invokes single_open to display the dropped packets.
 *
 * Returns 0 on success, negative error code on failure.
 */
static int dropped_packets_open(struct inode *inode, struct file *file)
{
    int ret = 0;

	/* Verify inode and file pointers are valid */
	if (!inode || !file) {
		pr_err("dropped_packets_open: Invalid inode or file pointer\n");
		return -EINVAL;
	}

    /* Additional check: Ensure the file is a regular file (not a directory) */
	if (!S_ISREG(inode->i_mode)) {
		return -EISDIR;
	}

	/* Open the proc file to display dropped packets */
    ret = single_open(file, dropped_packets_show, NULL);
	return ret;
}

/* File operations for the procfs entry */
static const struct proc_ops dropped_packets_ops = {
	.proc_open = dropped_packets_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

/**
 * packet_filter_init - Module initialization function
 *
 * Registers the Netfilter hook and creates the procfs entry.
 *
 * Returns 0 on success, negative error code on failure.
 */
static int __init packet_filter_init(void)
{
	int ret;

	/* Register Netfilter hook */
	nfho.hook     = packet_filter_hook;
	nfho.hooknum  = NF_INET_PRE_ROUTING;
	nfho.pf       = PF_INET;
	nfho.priority = NF_IP_PRI_FIRST+1;

	ret = nf_register_net_hook(&init_net, &nfho);
	if (ret) {
		pr_err("packet_filter: nf_register_net_hook failed\n");
		return ret;
	}

	/* Create procfs entry */
	proc_entry = proc_create("dropped_packets", 0, NULL, &dropped_packets_ops);
	if (!proc_entry) {
		pr_err("packet_filter: proc_create failed\n");
		nf_unregister_net_hook(&init_net, &nfho);
		return -ENOMEM;
	}

	return 0;
}

/**
 * packet_filter_exit - Module cleanup function
 *
 * Unregisters the Netfilter hook, removes the procfs entry,
 * and cleans up the flow table.
 */
static void __exit packet_filter_exit(void)
{
	struct flow_entry *flow;
	struct hlist_node *tmp;
	int bkt;

	/* Remove procfs entry */
	proc_remove(proc_entry);

	/* Cleanup flow table */
	spin_lock(&flow_table_lock);
	hash_for_each_safe(flow_table, bkt, tmp, flow, node) {
		hash_del(&flow->node);
		kfree(flow);
	}
	spin_unlock(&flow_table_lock);

	pr_debug("packet_filter: Module unloaded\n");
	pr_debug("packet_filter: Dropped packets: %u\n", atomic_read(&dropped_packets));
}

module_init(packet_filter_init);
module_exit(packet_filter_exit);
