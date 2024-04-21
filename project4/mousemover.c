#include <linux/module.h> // Required for all kernel modules
#include <linux/kernel.h> // Required for printk, KERN_INFO, KERN_ERR
#include <linux/init.h> // Required for __init and __exit macros
#include <linux/input.h> // Required for input_dev, input_allocate_device, input_report_rel, input_report_key, input_sync, BTN_LEFT, REL_X, REL_Y
#include <linux/keyboard.h> // Required for keyboard_notifier_param, KBD_KEYCODE, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_SPACE
#include <linux/notifier.h> // Required for notifier_block, notifier_call, NOTIFY_OK

static struct input_dev *input_dev;

static int keyboard_notifier(struct notifier_block *nblock, unsigned long code, void *_param)
{
	struct keyboard_notifier_param *param = _param;

	// Check if it's a key press event
	if (code == KBD_KEYCODE) {
		// Check for arrow key press
		if (param->value == KEY_UP) {
			// Move mouse cursor up
			input_report_rel(input_dev, REL_Y, -1);
			input_sync(input_dev);
		} else if (param->value == KEY_DOWN) {
			// Move mouse cursor down
			input_report_rel(input_dev, REL_Y, 1);
			input_sync(input_dev);
		} else if (param->value == KEY_LEFT) {
			// Move mouse cursor left
			input_report_rel(input_dev, REL_X, -1);
			input_sync(input_dev);
		} else if (param->value == KEY_RIGHT) {
			// Move mouse cursor right
			input_report_rel(input_dev, REL_X, 1);
			input_sync(input_dev);
		}
		// Check for space key press
		else if (param->value == KEY_SPACE) {
			// Report left mouse button press or release
            input_report_key(input_dev, BTN_LEFT, param->down);
            input_sync(input_dev);
		}
	}

	return NOTIFY_OK;
}

static struct notifier_block keyboard_nb = {
	notifier_call: keyboard_notifier
};

static int __init init_mousemover(void)
{
	int error;

	// Allocate a new input device
    input_dev = input_allocate_device();
    if (!input_dev) {
        printk(KERN_ERR "mousemover: Failed to allocate input device\n");
        return -ENOMEM;
    }

    // Set the name and physical location of the device
    input_dev->name = "Virtual Mouse";
    input_dev->phys = "vmouse/input0";

    // Set the type of events that this device can generate
    input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
    input_dev->keybit[BIT_WORD(BTN_MOUSE)] = BIT_MASK(BTN_LEFT) | BIT_MASK(BTN_MIDDLE) | BIT_MASK(BTN_RIGHT);
    input_dev->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y);

    // Register the input device
    error = input_register_device(input_dev);
    if (error) {
        printk(KERN_ERR "mousemover: Failed to register input device\n");
        input_free_device(input_dev);
        return error;
    }

	// Register the keyboard notifier
	register_keyboard_notifier(&keyboard_nb);
	printk(KERN_INFO "mousemover: Module initialized\n");
	return 0;
}

static void __exit exit_mousemover(void)
{
	unregister_keyboard_notifier(&keyboard_nb);
	input_unregister_device(input_dev);
	printk(KERN_INFO "mousemover: Module removed\n");
}

module_init(init_mousemover);
module_exit(exit_mousemover);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Logan Hunter Gray <lgray@proton.me>");
MODULE_DESCRIPTION("A kernel module that moves the mouse cursor based on keyboard input");
