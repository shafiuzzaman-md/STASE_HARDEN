//#include "conv.h"
//#include "heap.h"

//MODULE_LICENSE("GPL");
//MODULE_AUTHOR("SEFCOM");
//MODULE_DESCRIPTION("CONV");

#define DEV_NAME "conv"
#define CONV_UTF32_UTF8 1024
#define CONV_RELEASE_STRING 2048
#define CONV_ALLOC_STRING 4096
#define CONV_ERR -1
#define CONV_SUCCESS 0
#define CONV_LIST_SIZE 128
void *alloc_chunk(size_t size);
static int major_number;
//static struct cdev cdev;
static struct device *dev_file = NULL;
static struct class *conv_class = NULL;

typedef enum {
	UTF8,
	UTF32,
} FORMAT;

struct user_data_t {
	char *in;
	unsigned long inlen;
	char *out;
	unsigned int index;
	FORMAT f;
};

struct conv_entry_t {
	void *string;
	unsigned long size;
	FORMAT f;
};

struct conv_entry_t conv_list[CONV_LIST_SIZE];

void escape_utf8(uint8_t *utf8, size_t last, size_t j)
{klee_assert(0);
	switch (last) {
	case 1:
        klee_assert(j <= last);  // Auto-inserted assertion
		utf8[j++] = '\xf0';
		utf8[j++] = '\x9f';
		utf8[j++] = '\xa4';
		utf8[j++] = '\x94';
		break;
	case 2:
		utf8[j++] = '\xf0';
		utf8[j++] = '\x9f';
		utf8[j++] = '\xa4';
		break;
	case 3:
		utf8[j++] = '\xf0';
		utf8[j++] = '\x9f';
		break;
	case 4:
		utf8[j++] = '\xf0';
		break;
	}
}

size_t utf32_char_to_utf8(uint32_t codepoint, uint8_t *utf8)
{
	if (codepoint > 0x10FFFF ||
	    (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
		return CONV_ERR;
	}

	if (codepoint <= 0x7F) {
		utf8[0] = (uint8_t)codepoint;
		return 1;
	} else if (codepoint <= 0x7FF) {
		utf8[0] = 0xC0 | (codepoint >> 6);
		utf8[1] = 0x80 | (codepoint & 0x3F);
		return 2;
	} else if (codepoint <= 0xFFFF) {
		utf8[0] = 0xE0 | (codepoint >> 12);
		utf8[1] = 0x80 | ((codepoint >> 6) & 0x3F);
		utf8[2] = 0x80 | (codepoint & 0x3F);
		return 3;
	} else {
		utf8[0] = 0xF0 | (codepoint >> 18);
		utf8[1] = 0x80 | ((codepoint >> 12) & 0x3F);
		utf8[2] = 0x80 | ((codepoint >> 6) & 0x3F);
		utf8[3] = 0x80 | (codepoint & 0x3F);
		return 4;
	}
}

size_t utf32_string_to_utf8(const uint32_t *utf32, uint8_t *utf8,
			    size_t utf32_len)
{
	size_t i = 0, j = 0, utf8_len = 0;
    
	for (; i < utf32_len; i++) {
		uint8_t utf8_char[4];
		utf8_len = utf32_char_to_utf8(utf32[i], utf8_char);

		if (utf8_len == CONV_ERR) {
			return CONV_ERR;
		}

		for (size_t k = 0; k < utf8_len; k++) {
			utf8[j++] = utf8_char[k];
		}
	}

	escape_utf8(utf8, utf8_len, j);
	return j;
}

size_t utf8_encoding_length(uint32_t codepoint)
{
	if (codepoint > 0x10FFFF ||
	    (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
		return CONV_ERR;
	}

	if (codepoint <= 0x7F) {
		return 1;
	} else if (codepoint <= 0x7FF) {
		return 2;
	} else if (codepoint <= 0xFFFF) {
		return 3;
	} else {
		return 4;
	}
}

size_t count_utf8_bytes(const uint32_t *utf32, size_t utf32_len)
{
	size_t i = 0, j = 0;

	for (; i < utf32_len; i++) {
		size_t utf8_len = utf8_encoding_length(utf32[i]);

		if (utf8_len == CONV_ERR) {
			return CONV_ERR;
		}

		j += utf8_len;
	}

	return j;
}

static long conv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	char *inchunk;
	struct user_data_t ud = { 0 };

	if (copy_from_user(&ud, (void *)arg, sizeof(struct user_data_t)))
		goto err;

	switch (cmd) {
	case CONV_UTF32_UTF8:
		if (ud.index >= CONV_LIST_SIZE ||
		    conv_list[ud.index].string == 0 ||
		    conv_list[ud.index].size == 0 ||
		    conv_list[ud.index].f != UTF32)
			goto err;

		inchunk = conv_list[ud.index].string;
		size_t outlen = count_utf8_bytes((uint32_t *)inchunk,
						 conv_list[ud.index].size / 4);

		if (outlen == CONV_ERR)
			goto err;

		void *outchunk = alloc_chunk(outlen);
		utf32_string_to_utf8((uint32_t *)inchunk, (uint8_t *)outchunk,
				     conv_list[ud.index].size / 4);

	// 	if (copy_to_user(ud.out, outchunk, outlen))
	// 		goto err;

	// 	free_chunk(inchunk);
	// 	conv_list[ud.index].string = outchunk;
	// 	conv_list[ud.index].size = outlen;
	// 	conv_list[ud.index].f = UTF8;

	// 	return outlen;
	// case CONV_ALLOC_STRING:
	// 	if (ud.index >= CONV_LIST_SIZE ||
	// 	    conv_list[ud.index].string != 0 ||
	// 	    conv_list[ud.index].size != 0)
	// 		goto err;

	// 	char *inchunk = alloc_chunk(ud.inlen);
	// 	if (copy_from_user(inchunk, ud.in, ud.inlen))
	// 		goto err;

	// 	conv_list[ud.index].string = inchunk;
	// 	conv_list[ud.index].size = ud.inlen;
	// 	conv_list[ud.index].f = ud.f;

	// 	return CONV_SUCCESS;
	// case CONV_RELEASE_STRING:
	// 	if (ud.index >= CONV_LIST_SIZE ||
	// 	    conv_list[ud.index].string == 0 ||
	// 	    conv_list[ud.index].size == 0)
	// 		goto err;

	// 	free_chunk(conv_list[ud.index].string);
	// 	conv_list[ud.index].string = 0;
	// 	conv_list[ud.index].size = 0;

		return CONV_SUCCESS;
	default:
		goto err;
	}

err:
	return 0;
}

// static struct file_operations fops = {
// 	.owner = THIS_MODULE,
// 	.unlocked_ioctl = conv_ioctl,
// };

// static int __init conv_driver_init(void)
// {
// 	printk(KERN_INFO "[CONV] Initializing Conversion Driver\n");

// 	int ret;
// 	ret = alloc_chrdev_region(&major_number, 0, 1, DEV_NAME);
// 	if (ret < 0) {
// 		printk(KERN_ALERT "[CONV] Device registration failed\n");
// 		return ret;
// 	}

// 	conv_class = class_create(DEV_NAME);
// 	if (IS_ERR(conv_class)) {
// 		unregister_chrdev_region(major_number, 1);
// 		printk(KERN_ALERT "[CONV] Device class creation failed\n");
// 		return PTR_ERR(conv_class);
// 	}

// 	cdev_init(&cdev, &fops);
// 	ret = cdev_add(&cdev, major_number, 1);
// 	if (ret < 0) {
// 		class_destroy(conv_class);
// 		unregister_chrdev_region(major_number, 1);
// 		printk(KERN_ALERT "[CONV] Device addition failed\n");
// 		return ret;
// 	}

// 	dev_file =
// 		device_create(conv_class, NULL, major_number, NULL, DEV_NAME);
// 	if (IS_ERR(dev_file)) {
// 		cdev_del(&cdev);
// 		class_destroy(conv_class);
// 		unregister_chrdev_region(major_number, 1);
// 		printk(KERN_ALERT "[CONV] Device file creation failed\n");
// 		return PTR_ERR(dev_file);
// 	}

// 	printk(KERN_INFO "[CONV] Driver registered and device created\n");
// 	return 0;
// }

// static int __init conv_init(void)
// {
// 	conv_driver_init();
// 	init_heap();
// 	return 0;
// }

//device_initcall(conv_init);