#include <linux/module.h> // Required for all kernel modules
#include <linux/kernel.h> // Required for printk
#include <linux/init.h> // Required for __init and __exit macros
#include <linux/input.h> // Required for input_dev, input_report_rel, input_report_key, input_sync, BTN_LEFT, REL_X, REL_Y
#include <linux/notifier.h> // Required for notifier_block, notifier_call, NOTIFY_OK
#include <linux/keyboard.h> // Required for keyboard_notifier_param, KBD_KEYCODE, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_SPACE

#include <linux/input.h> // Required for input_allocate_device, input_set_abs_params
#include <linux/uinput.h> // Required for UI_DEV_CREATE, UI_DEV_DESTROY

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
			// Report left mouse button press
			input_report_key(input_dev, BTN_LEFT, 1);
			input_sync(input_dev);
		}
	}

	return NOTIFY_OK;
}

static struct notifier_block keyboard_nb = {
	notifier_call: keyboard_notifier
};

static int __init init_module(void)
{
	register_keyboard_notifier(&keyboard_nb);
	printk(KERN_INFO "Mouse mover module initialized");
	return 0;
}

static void __exit exit_module(void)
{
	unregister_keyboard_notifier(&keyboard_nb);
	printk(KERN_INFO "Mouse mover module removed");
}

module_init(init_module);
module_exit(exit_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Logan Gray");
MODULE_DESCRIPTION("Keyboard to Mouse Control Module");
