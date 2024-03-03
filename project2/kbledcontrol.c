#include <linux/module.h> // Needed by all modules
#include <linux/kernel.h> // Needed for KERN_INFO and other kernel-related definitions
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/tty.h>
#include <linux/kd.h>
#include <linux/vt_kern.h>
#include <linux/init.h>
#include <linux/console_struct.h>

// Define the module information
#define PROC_FILENAME "kbledcontrol"
#define DEFAULT_BLINK_DELAY HZ/5    // Should be 1/5 of a second, 200ms
#define DEFAULT_LEDS_ON 0x07        // Default LED pattern (all LEDs on)

// Define the module variables
static struct proc_dir_entry* proc_file;
static struct timer_list blink_timer;
static unsigned int blink_delay = DEFAULT_BLINK_DELAY;
static char leds_on = DEFAULT_LEDS_ON;
static struct tty_driver* my_driver;
static char kbledstatus = 0;

static void blink_timer_func(struct timer_list *t)
{
    kbledstatus = (kbledstatus == leds_on) ? 0 : leds_on;
    (my_driver->ops->ioctl)(vc_cons[fg_console].d->port.tty, KDSETLED, kbledstatus);
    mod_timer(&blink_timer, jiffies + blink_delay);
}

// Write to /proc file to update LED pattern or blink delay
static ssize_t proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
    char input[10];
    if (count > 9) return -EINVAL; // Input too long
    if (copy_from_user(input, buffer, count)) return -EFAULT;
    input[count] = '\0'; // Ensure string termination

    if (input[0] == 'L') {
        // Update LED pattern
        printk(KERN_INFO "kbledcontrol: Updating LED setting to L%c\n", input[1]);
        leds_on = simple_strtoul(input + 1, NULL, 10);
        printk(KERN_INFO "kbledcontrol: LEDs on: %d\n", leds_on)
        if (leds_on > 7) {
            printk(KERN_ERR "kbledcontrol: User passed value greater than %c, turning on all LEDs", DEFAULT_LEDS_ON);
            leds_on = DEFAULT_LEDS_ON; // Validate range
        }
    } else if (input[0] == 'D') {
        // Update blink delay
        printk(KERN_INFO "kbledcontrol: Updating blink speed to D%c\n", input[1]);
        unsigned int divisor = simple_strtoul(input + 1, NULL, 10);
        if (divisor >= 0 && divisor <= 9) {
            blink_delay = HZ / (divisor + 1); // Adjust delay based on divisor
        } else {
            blink_delay = DEFAULT_BLINK_DELAY; // Default if out of range
        }
    } else {
        printk(KERN_INFO "kbledcontrol: User passed invalid input, returning -EINVAL");
        return -EINVAL; // Invalid input
    }

    return count;
}

struct proc_ops proc_fops = {
    proc_write: proc_write,
};

// Function to initialize the module and create the /proc file
static int __init kbledcontrol_init(void)
{
    printk(KERN_INFO "kbledcontrol: Loading module");
    my_driver = vc_cons[fg_console].d->port.tty->driver;
    proc_file = proc_create(PROC_FILENAME, 0666, NULL, &proc_fops);
    if (!proc_file) return -ENOMEM;

    timer_setup(&blink_timer, blink_timer_func, 0);
    blink_timer.expires = jiffies + blink_delay;
    add_timer(&blink_timer);

    return 0;
}

// Function to remove the /proc file and clean up the module
static void __exit kbledcontrol_exit(void)
{
    printk(KERN_INFO "kbledcontrol: Unloading module");
    del_timer(&blink_timer);
    (my_driver->ops->ioctl)(vc_cons[fg_console].d->port.tty, KDSETLED, 0); // Turn off LEDs
    remove_proc_entry(PROC_FILENAME, NULL);
}

module_init(kbledcontrol_init);
module_exit(kbledcontrol_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Logan Gray");
MODULE_DESCRIPTION("Control keyboard LEDs via /proc file.");
