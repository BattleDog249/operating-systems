#include <linux/module.h> // Required for all kernel modules
#include <linux/kernel.h> // Required for printk, KERN_INFO, KERN_ERR
#include <linux/init.h> // Required for __init and __exit macros
#include <linux/netfilter.h> // Required for netfilter hooks
#include <linux/ip.h> // Required for iphdr
#include <linux/proc_fs.h> // Required for the proc file system
#include <linux/rbtree.h> // Required for Red-Black Trees
#include <linux/slab.h> // Required for kmalloc, kfree
#include <linux/rwsem.h> // Required for read-write semaphores

#define PROC_FILENAME "ip_traffic"

static struct nf_hook_ops netfilter_ops_in;
static struct proc_dir_entry *proc_file;
struct rb_root ip_tree = RB_ROOT;

// Define the structure for the Red-Black Tree nodes
struct ip_node {
	struct rb_node node;
	u32 ip_addr;
	int count;
};

// Function to insert an IP address into the Red-Black Tree
static void ip_tree_insert(u32 ip_addr) {
	//printk(KERN_INFO "netmon: ip_tree_insert() called\n");
	struct ip_node *data, *tmp;
	struct rb_node **new = &(ip_tree.rb_node), *parent = NULL;

	while (*new) {
		parent = *new;
		tmp = container_of(*new, struct ip_node, node);

		if (ip_addr < tmp->ip_addr) {
			new = &((*new)->rb_left);
		} else if (ip_addr > tmp->ip_addr) {
			new = &((*new)->rb_right);
		} else {
			tmp->count++;
			return;
		}
	}

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

// Declare a read-write semaphore
static DECLARE_RWSEM(ip_tree_rwsem);

// Function to insert an IP address into the Red-Black Tree
static unsigned int main_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state) {
	struct iphdr *ip;

	if (!skb)
		return NF_ACCEPT;

	ip = ip_hdr(skb);
	down_write(&ip_tree_rwsem);
	ip_tree_insert(ntohl(ip->saddr));
	up_write(&ip_tree_rwsem);

	return NF_ACCEPT;
}

// Function to read the contents of the Red-Black Tree
static ssize_t read_proc(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) {
	int len = 0;
	struct rb_node *node;
	struct ip_node *data;
	int num_nodes = 0;

	// Count the number of nodes in the tree
	for (node = rb_first(&ip_tree); node; node = rb_next(node)) {
		num_nodes++;
	}

	// Dynamically allocate the buffer based on the number of nodes
	char *buf = kmalloc(num_nodes * 20, GFP_KERNEL); // Assume each node needs 20 bytes
	if (!buf) {
		printk(KERN_ERR "Unable to allocate buffer.\n");
		return -ENOMEM;
	}

	down_read(&ip_tree_rwsem);
	for (node = rb_first(&ip_tree); node; node = rb_next(node)) {
		data = rb_entry(node, struct ip_node, node);
		u32 ip_addr_network_byte_order = htonl(data->ip_addr); // Convert IP address to network byte order
		len += snprintf(buf + len, num_nodes * 20 - len, "%pI4 - %d\n", &ip_addr_network_byte_order, data->count);
		if (len > count)
			break;
	}
	up_read(&ip_tree_rwsem);

	if (*ppos > 0 || len == 0) {
		kfree(buf);
		return 0;
	}

	if (copy_to_user(ubuf, buf, len)) {
		kfree(buf);
		return -EFAULT;
	}

	*ppos = len;
	kfree(buf);
	return len;
}

static struct proc_ops netmon_proc_ops = {
	proc_read: read_proc,
};

// Function to initialize the module
static int __init init_netmon(void) {
	printk(KERN_INFO "netmon: Loading module\n");
	netfilter_ops_in.hook = main_hook;
	netfilter_ops_in.hooknum = NF_INET_LOCAL_IN;
	netfilter_ops_in.pf = AF_INET;

	nf_register_net_hook(&init_net, &netfilter_ops_in);
	proc_file = proc_create(PROC_FILENAME, 0666, NULL, &netmon_proc_ops);
	if (!proc_file) return -ENOMEM;

	return 0;
}

// Function to clean up the module
static void __exit exit_netmon(void) {
	printk(KERN_INFO "netmon: Unloading module\n");
	struct rb_node *node;
	struct ip_node *data;

	nf_unregister_net_hook(&init_net, &netfilter_ops_in);
	remove_proc_entry(PROC_FILENAME, NULL);

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
