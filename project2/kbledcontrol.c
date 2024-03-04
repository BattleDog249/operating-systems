#include <linux/module.h>           // Needed by all modules
#include <linux/kernel.h>           // Needed for KERN_INFO and other kernel-related definitions
#include <linux/proc_fs.h>          // Needed for the proc file system
#include <linux/uaccess.h>          // Needed for copy_from_user
#include <linux/timer.h>            // Needed for the timer
#include <linux/tty.h>              // Needed for the tty driver
#include <linux/kd.h>               // Needed for the keyboard LED definitions
#include <linux/vt_kern.h>          // Needed for the console definitions
#include <linux/init.h>             // Needed for the module initialization and cleanup
#include <linux/console_struct.h>   // Needed for the console definitions

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

// Function to update the LED pattern
static void blink_timer_func(struct timer_list *t)
{
    kbledstatus = (kbledstatus == leds_on) ? 0 : leds_on;
    (my_driver->ops->ioctl)(vc_cons[fg_console].d->port.tty, KDSETLED, kbledstatus);
    mod_timer(&blink_timer, jiffies + blink_delay);
}

// Function to handle writes to the /proc file and update the LED pattern or blink delay
static ssize_t proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
    if (count > 3) return -EINVAL; // Input is too long
    char input[3]; // Declare new variable to hold the input, with space for string termination
    if (copy_from_user(input, buffer, count)) return -EFAULT; // Copy user space buffer to kernel space input and check for errors
    input[2] = '\0'; // Ensure string termination

    if (input[0] == 'L') {
        unsigned int leds; // Declare new variable to hold the updated LED pattern
        if (kstrtouint(input + 1, 0x10, &leds)) return -EINVAL; // Convert input to integer and store in leds variable and check for errors
        if (leds >= 0 && leds <= 7) {
            leds_on = leds; // Update leds_on with the new value
            printk(KERN_INFO "kbledcontrol: New LED pattern: %d", leds_on);
        } else {
            leds_on = DEFAULT_LEDS_ON; // Set leds_on to default value if input is out of range
            printk(KERN_INFO "kbledcontrol: Invalid LED pattern, using default: %d", leds_on);
        }
    } else if (input[0] == 'D') {
        unsigned int divisor; // Declare new variable to hold the updated divisor
        if (kstrtouint(input + 1, 0x10, &divisor)) return -EINVAL; // Convert input to integer and store in divisor variable and check for errors
        if (divisor >= 0 && divisor <= 9) {
            blink_delay = HZ / (divisor + 1); // Update blink_delay based on the new divisor
            printk(KERN_INFO "kbledcontrol: New blink delay: %d", blink_delay);
        } else {
            blink_delay = DEFAULT_BLINK_DELAY; // Set blink_delay to default value if input is out of range
            printk(KERN_INFO "kbledcontrol: Invalid divisor, using default blink delay: %d", blink_delay);
        }
    } else return -EINVAL; // Invalid input
    return count; // Return the number of bytes written
}

// Define the file operations for the /proc file
static struct proc_ops proc_fops = {
    proc_write: proc_write,
};

// Function to initialize the module and create the /proc file
static int __init kbledcontrol_init(void)
{
    printk(KERN_INFO "kbledcontrol: Loading module");
    my_driver = vc_cons[fg_console].d->port.tty->driver; // Get the tty driver from the console
    proc_file = proc_create(PROC_FILENAME, 0666, NULL, &proc_fops); // Create the /proc file
    if (!proc_file) return -ENOMEM; // Return error if file creation fails

    timer_setup(&blink_timer, blink_timer_func, 0); // Initialize the timer
    blink_timer.expires = jiffies + blink_delay; // Set the initial expiration time
    add_timer(&blink_timer);

    return 0;
}

// Function to remove the /proc file and clean up the module
static void __exit kbledcontrol_exit(void)
{
    printk(KERN_INFO "kbledcontrol: Unloading module");
    del_timer(&blink_timer); // Delete the timer
    (my_driver->ops->ioctl)(vc_cons[fg_console].d->port.tty, KDSETLED, 0); // Turn off LEDs
    remove_proc_entry(PROC_FILENAME, NULL); // Remove the /proc file
}

module_init(kbledcontrol_init);
module_exit(kbledcontrol_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Logan Gray");
MODULE_DESCRIPTION("Control keyboard LEDs via /proc file. L[0-7] to set LED pattern, D[0-9] to set blink delay.");
