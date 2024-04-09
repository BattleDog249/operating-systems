#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/keyboard.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/slab.h>

#define PROC_FILE_NAME "kbd_demo"

struct notifier_block nb;

ssize_t read_simple(struct file *filp,char *buf,size_t count,loff_t *offp ) 
{
	return 0;
}

struct file_operations proc_fops = {
	read: read_simple,
};

int kb_notifier_fn(struct notifier_block *pnb, unsigned long action, void* data){
	struct keyboard_notifier_param *kp = (struct keyboard_notifier_param*)data;
	// also kp->dpwn, indicates if it's a press down or release
	printk("Key:  %d  Lights:  %d  Shiftmax:  %x\n", kp->value, kp->ledstate, kp->shift);
	return 0;
}

int init (void) {
	nb.notifier_call = kb_notifier_fn;
	register_keyboard_notifier(&nb);
	// proc_create(PROC_FILE_NAME,0,NULL,&proc_fops);
	return 0;
}

void cleanup(void) {
	unregister_keyboard_notifier(&nb);
	// remove_proc_entry(PROC_FILE_NAME,NULL);
}

MODULE_LICENSE("GPL"); 
module_init(init);
module_exit(cleanup);

