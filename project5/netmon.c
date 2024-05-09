#include <linux/module.h> // Required for all kernel modules
#include <linux/kernel.h> // Required for printk, KERN_INFO, KERN_ERR
#include <linux/init.h> // Required for __init and __exit macros
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h> // Required for iphdr
#include <linux/proc_fs.h> // Required for the proc file system
#include <linux/rbtree.h> // Required for Red-Black Trees
#include <linux/slab.h> // Required for kmalloc, kfree

#define PROC_FILENAME "ip_traffic"

static struct nf_hook_ops netfilter_ops_in;
static struct proc_dir_entry *proc_file;
struct rb_root ip_tree = RB_ROOT;

struct ip_node {
    struct rb_node node;
    u32 ip_addr;
    int count;
};

static void ip_tree_insert(u32 ip_addr) {
	printk(KERN_INFO "netmon: ip_tree_insert() called\n");
    struct ip_node *data, *tmp;
    struct rb_node **new = &(ip_tree.rb_node), *parent = NULL;

    while (*new) {
        printk(KERN_INFO "netmon: ip_tree_insert() while loop\n");
        parent = *new;
        tmp = container_of(*new, struct ip_node, node);

        if (ip_addr < tmp->ip_addr) {
            printk(KERN_INFO "netmon: ip_tree_insert() ip_addr < tmp->ip_addr\n");
            new = &((*new)->rb_left);
        } else if (ip_addr > tmp->ip_addr) {
            printk(KERN_INFO "netmon: ip_tree_insert() ip_addr > tmp->ip_addr\n");
            new = &((*new)->rb_right);
        } else {
            printk(KERN_INFO "netmon: ip_tree_insert() ip_addr == tmp->ip_addr\n");
            tmp->count++;
            return;
        }
    }

    printk(KERN_INFO "netmon: ip_tree_insert() kmalloc\n");
    data = kmalloc(sizeof(struct ip_node), GFP_KERNEL);
    if (!data) {
        printk(KERN_ERR "Unable to allocate IP node.\n");
        return;
    }

    data->ip_addr = ip_addr;
    data->count = 1;

    rb_link_node(&data->node, parent, new);
    rb_insert_color(&data->node, &ip_tree);
}

static unsigned int main_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state) {
	printk(KERN_INFO "netmon: main_hook() called\n");
    struct iphdr *ip;

    if (!skb)
        return NF_ACCEPT;

    ip = ip_hdr(skb);
    ip_tree_insert(ntohl(ip->saddr));

    return NF_ACCEPT;
}

static ssize_t read_proc(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) {
	printk(KERN_INFO "netmon: Reading /proc file\n");
    int len = 0;
    char buf[256];
    struct rb_node *node;
    struct ip_node *data;

    for (node = rb_first(&ip_tree); node; node = rb_next(node)) {
        data = rb_entry(node, struct ip_node, node);
        len += snprintf(buf + len, sizeof(buf) - len, "%pI4 - %d\n", &data->ip_addr, data->count);
        if (len > count)
            break;
    }

    if (*ppos > 0 || len == 0)
        return 0;

    if (copy_to_user(ubuf, buf, len))
        return -EFAULT;

    *ppos = len;
    return len;
}

static struct proc_ops netmon_proc_ops = {
    proc_read: read_proc,
};

static int __init init_netmon(void) {
	printk(KERN_INFO "netmon: Loading module\n");
    netfilter_ops_in.hook = main_hook;
    netfilter_ops_in.hooknum = NF_INET_PRE_ROUTING;
    netfilter_ops_in.pf = PF_INET;
    netfilter_ops_in.priority = NF_IP_PRI_FIRST;

    nf_register_net_hook(&init_net, &netfilter_ops_in);
    proc_file = proc_create(PROC_FILENAME, 0666, NULL, &netmon_proc_ops);
	if (!proc_file) return -ENOMEM; // Return error if file creation fails

    return 0;
}

static void __exit exit_netmon(void) {
	printk(KERN_INFO "netmon: Unloading module\n");
    struct rb_node *node;
    struct ip_node *data;

    nf_unregister_net_hook(&init_net, &netfilter_ops_in);
    remove_proc_entry(PROC_FILENAME, NULL); // Remove the /proc file

    for (node = rb_first(&ip_tree); node != NULL; node = rb_next(node)) {
        data = rb_entry(node, struct ip_node, node);
        rb_erase(node, &ip_tree);
        kfree(data);
    }
}

module_init(init_netmon);
module_exit(exit_netmon);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Logan Hunter Gray <lgray@proton.me>");
MODULE_DESCRIPTION("Module to track source IPs of incoming packets using Red-Black Trees.");
