/*
 * Keyboard Message Programmer Module (KBMP)
 *
 * This kernel module provides an interface to write and read messages using
 * the /proc filesystem. It exposes two proc entries:
 *
 * 1. /proc/kb_message - Allows writing an ASCII message into a fixed-size 
 *    buffer (1024 bytes) and reading it back. Messages are copied from the
 *    user space into a kernel buffer, where they can be retrieved or processed.
 *
 * 2. /proc/kb_disp - Allows reading a rotated version of the stored message.
 *    The message is cyclically rotated on each read, simulating a scrolling
 *    effect on the displayed message.
 *
 * The module ensures that only valid ASCII characters can be written, and
 * handles memory safely with proper checks for buffer overflow conditions.
 *
 * Author: Bill Shelley
 */


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#define MESSAGE_SIZE 1024ull
char message_buffer[MESSAGE_SIZE] = {0};

EXPORT_SYMBOL(message_buffer);

/*
 * get_message_from_buffer - Helper function to retrieve the stored message.
 * This function allocates memory for a copy of the message stored in the
 * global `message_buffer`, copies the message into the newly allocated space,
 * and returns it.
 *
 * Return: Pointer to the newly allocated message or NULL on failure.
 */
char* get_message_from_buffer(void) {
    size_t message_length = strlen(message_buffer);
    char* msg = (char*)kmalloc(message_length + 1, GFP_KERNEL);
    if (!msg) {
        return NULL;
    }
    strncpy(msg, message_buffer, message_length);
    msg[message_length] = '\0';
    return msg;
}

/*
 * message_write - Write function for /proc/kb_message.
 * This function handles writing data from user space into the kernel buffer.
 * It verifies that the filename is correct, ensures only valid ASCII characters
 * are written, and then copies the data into the `message_buffer`.
 *
 * @file: The file pointer for the /proc entry.
 * @buf: The buffer containing data from user space.
 * @count: The size of the user data.
 * @pos: The position pointer (unused).
 *
 * Return: Number of bytes written on success, -EINVAL on error.
 */
ssize_t message_write(struct file *file, const char __user *buf, size_t count, loff_t *pos) {
    // Ensure this is the correct file by name
    if (strcmp(file->f_path.dentry->d_name.name, "kb_message") != 0) {
        pr_err("incorrect filename\n");
        return -EINVAL;
    }

    if (count > MESSAGE_SIZE) {
        pr_err("count greater than message size\n");
        return -EINVAL;
    }

    for (size_t i = 0; i < count; ++i) {
        char ch;
        if (get_user(ch, &buf[i])) {
            return -EFAULT;
        }
        if (ch == '\0') {
            break;
        }
        if (ch > 'z' || ch < 'A') {
            return -EINVAL;
        }
    }

    unsigned long uncopied = copy_from_user(message_buffer, buf, count);
    if (uncopied > 0) {
        pr_err("tried to copy %lu but %lu failed to copy\n", count, uncopied);
        memset(message_buffer, 0, MESSAGE_SIZE);
        return -EINVAL;
    };

    if (count < MESSAGE_SIZE) {
        message_buffer[count] = '\0';
    } else {
        message_buffer[MESSAGE_SIZE - 1] = '\0';
    }
    pr_debug("KBMP: Message received (size: %zu bytes)\n", count);
    return count;
}

/*
 * message_read - Read function for /proc/kb_message.
 * This function handles reading the stored message from the kernel buffer
 * into a user space buffer. It only allows full reads of the message.
 *
 * @file: The file pointer for the /proc entry.
 * @buf: The buffer to copy data to in user space.
 * @count: The number of bytes to read.
 * @ppos: The position pointer.
 *
 * Return: Number of bytes read on success, 0 on EOF, -EINVAL or -EFAULT on error.
 */
ssize_t message_read(struct file* file, char __user *buf, size_t count, loff_t* ppos) {
    if (strcmp(file->f_path.dentry->d_name.name, "kb_message") != 0) {
        pr_err("incorrect filename");
        return -EINVAL;
    }

    if (ppos == NULL || *ppos > 0 || count < MESSAGE_SIZE) {
        return 0;
    }

    if (count > MESSAGE_SIZE) {
        count = MESSAGE_SIZE;
    }

    size_t msg_length = strnlen(message_buffer, MESSAGE_SIZE);
    if (count > msg_length) {
        count = msg_length;
    }
    unsigned long uncopied = copy_to_user(buf, message_buffer, count);
    if (uncopied > 0) {
        pr_err("tried to copy %lu but %lu failed to copy\n", count, uncopied);
        return -EFAULT;
    };
    
    *ppos += count;

    return count;
}

/*
 * rotate - Rotate the stored message by a given offset.
 * This function creates a rotated version of the input string by shifting
 * the characters based on the given index. It allocates a new buffer for
 * the rotated message and returns it.
 *
 * @s: The input string to be rotated.
 * @i: The index by which to rotate the string.
 *
 * Return: Pointer to the newly allocated rotated string.
 */
char* rotate(char* s, size_t i) {
    size_t length = strlen(s);
    char* rotated = (char*)kmalloc(length + 1, GFP_KERNEL);
    if (!rotated) {
        return NULL;
    }

    size_t index = i % length;
    size_t remainder_length = length - index;

    memcpy(rotated, s + index, remainder_length);
    memcpy(rotated + remainder_length, s, index);

    rotated[length] = '\0';
    return rotated;
}

/*
 * display - Read function for /proc/kb_disp.
 * This function reads the stored message, rotates it based on a static index,
 * and returns the rotated message to user space. The index is incremented
 * after each read to simulate scrolling.
 *
 * @file: The file pointer for the /proc entry.
 * @buf: The buffer to copy data to in user space.
 * @count: The number of bytes to read.
 * @ppos: The position pointer.
 *
 * Return: Number of bytes read on success, 0 on EOF, -EINVAL or -EFAULT on error.
 */
ssize_t display(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    static size_t message_index = 0;
    
    if (strcmp(file->f_path.dentry->d_name.name, "kb_disp") != 0) {
        pr_err("incorrect filename");
        return -EINVAL;
    }

    // only allow full reads
    if (ppos == NULL || *ppos > 0 || count < MESSAGE_SIZE) {
        return 0;
    }

    char* msg = get_message_from_buffer();
    if (!msg) {
        return count;
    }
    message_index = (message_index + 1) % MESSAGE_SIZE;
    char* rotated_message = rotate(msg, message_index);
    if (!rotated_message) {
        return count;
    }
    kfree(msg);

    count = strlen(rotated_message);

    ssize_t uncopied = copy_to_user(buf, rotated_message, count);
    kfree(rotated_message);

    if (uncopied > 0) {
        pr_err("tried to copy %lu but %lu failed to copy\n", count, uncopied);
        return -EINVAL;
    };

    *ppos += count;

    return count;
}

static const struct proc_ops proc_file_ops = {
    .proc_write = message_write,
    .proc_read = message_read,
};

static const struct proc_ops proc_disp_ops = {
    .proc_read = display,
};

static struct proc_dir_entry *proc_entry;
static struct proc_dir_entry *disp_entry;

/*
 * kbmp_init - Initialization function for the module.
 * This function sets up the procfs entries for writing and displaying messages
 * and initializes the message buffer.
 *
 * Return: 0 on success, negative error code on failure.
 */
static int __init kbmp_init(void) {
    pr_debug("KBMP: Loaded.\n");
    memset(message_buffer, '\0', MESSAGE_SIZE);

    proc_entry = proc_create("kb_message", 0666, NULL, &proc_file_ops);
    if (!proc_entry) {
        pr_err("KBMP: Failed to create proc entry\n");
        return -ENOMEM;
    }

    pr_debug("KBMP: /proc/kb_message created\n");

    disp_entry = proc_create("kb_disp", 0444, NULL, &proc_disp_ops);
    if (!disp_entry) {
        pr_err("KBMP: Failed to create display proc entry\n");
        return -ENOMEM;
    }

    pr_debug("KBMP: /proc/kb_disp created\n");

    return 0;
}

/*
 * kbmp_exit - Cleanup function for the module.
 * This function removes the procfs entries and logs the module unload.
 */
static void __exit kbmp_exit(void) {
    pr_debug("KBMP: Unloading and removing proc entries...\n");

    proc_remove(proc_entry);
    proc_remove(disp_entry);
}

module_init(kbmp_init);
module_exit(kbmp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bill Shelley");
MODULE_DESCRIPTION("KB Message Programmer");
