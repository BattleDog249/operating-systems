#include <linux/module.h>
#include <linux/init.h>
#include <linux/ip.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/inet.h>
#include <linux/socket.h>

MODULE_LICENSE("GPL");

/* A "hook" is attached to the network interface
 * There are commonly used to implement a firewall or other routing logic */
struct nf_hook_ops demo_hook;

/* This is the function that will be called when our hook is triggered */
unsigned int hook_function(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
	/* There will be a lot more about this stuff in CS435, but understand this:
	 * 	1.  There are many Internet protocols, but we use IPv4 for most traffic
	 * 	2.  Since there are more than one, this kind of function can be used
	 * 			for protocols other than IPv4.  We know we're using IPv4, so we 
	 * 			can make this cast safely. */
	struct iphdr *ip_header = (struct iphdr *)skb_network_header(skb);
	/* The IP address is a 32-bit unsigned integer, usually interpreted as a 
	 * series of 4 bytes.  We usually write these in a form like 192.168.18.25,
	 * but for that, we need to apply a fair bit of logic and convert to a string.
	 * printk has a special placeholder, %pI4, that can do this for us. */
	printk("IP address:  %pI4\n", &(ip_header->saddr));
	/* If you return 0, the network interface will stop working!
	 * We could choose not to accept the incoming packet, which would be useful
	 * if we were making a firewall.  Returning NF_ACCEPT all the time will not
	 * filter incoming traffic in any way it's not already filtered. */
	return NF_ACCEPT;
}

int __init netmon_init(void)
{
	// The nf_nook_ops structure will store information used for nf_register_net_hook
	demo_hook.hook = hook_function;       // Function our hook will run
	demo_hook.hooknum = NF_INET_LOCAL_IN; // Look at incoming trafic
	demo_hook.pf = AF_INET;               // AF_INET is IPv4.  AF_INET6 is IPv6
	nf_register_net_hook(&init_net, &demo_hook); // init_net is defined in a header file
	return 0;
}

void __exit netmon_exit(void)
{
	// If you don't unregister the hook, the OS will crash when the module is removed
	nf_unregister_net_hook(&init_net, &demo_hook);
}

module_init(netmon_init);
module_exit(netmon_exit);
