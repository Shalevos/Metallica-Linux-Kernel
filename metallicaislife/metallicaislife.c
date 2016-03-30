#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/string.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shavelos");
MODULE_DESCRIPTION("Only Metallica! Ride The Lightning intro FUCK YES!!!!!!!!!");

static char *metallica_filename = "/home/shalevos/Music/Metallica/Metallica - 1984 - Ride The Lightning/02 - Ride The Lightning.mp3";

/*
 * Set up a module parameter for the filename.
 */
module_param(metallica_filename, charp, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(metallica_filename, "Full path to Ride The Lightning! Yea, bitch!");


/*
 * cr0 is an x86 control register, and the bit we're twiddling here controls
 * the write protection. We need to do this because the area of memory
 * containing the system call table is write protected, and modifying it would
 * case a protection fault.
 */
#define DISABLE_WRITE_PROTECTION (write_cr0(read_cr0() & (~ 0x10000)))
#define ENABLE_WRITE_PROTECTION (write_cr0(read_cr0() | 0x10000))


static unsigned long **find_sys_call_table(void);
asmlinkage long metallica_open(const char __user *filename, int flags, umode_t mode);

asmlinkage long (*original_sys_open)(const char __user *, int, umode_t);
asmlinkage unsigned long **sys_call_table;


static int __init metallica_init(void)
{
    if(!metallica_filename) {
	   printk(KERN_ERR "There is no file path for Ride The Lightning");
	   return -EINVAL;  /* invalid argument */
    }

    //We're finding the system call table so we can mess with it.
    sys_call_table = find_sys_call_table();

    if(!sys_call_table) {
	   printk(KERN_ERR "Couldn't find sys_call_table.\n");
	   return -EPERM;  /* operation not permitted; couldn't find general error */
    }

    /*
     * Replace the entry for open with our own function. We save the location
     * of the real sys_open so we can put it back when we're unloaded.
     */
    DISABLE_WRITE_PROTECTION;
    original_sys_open = (void *) sys_call_table[__NR_open];
    sys_call_table[__NR_open] = (unsigned long *) metallica_open;
    ENABLE_WRITE_PROTECTION;

    printk(KERN_INFO "AAAAAAHHHHHHHHHHHHHHHHHHHHHHH FUCK YES THIS INTRO IS SO BADASS I'M HAVING AN EARGASM PLEASE HELP!\n");
    return 0;  /* zero indicates success */
}


/*
 * Our replacement for sys_open, which forwards to the real sys_open unless the
 * file name ends with .mp3, in which case it unleashes badassintroisbadass.mp3 .
 */
asmlinkage long metallica_open(const char __user *filename, int flags, umode_t mode)
{
    int len = strlen(filename);

    /* See if we should hijack the open */
    if(strcmp(filename + len - 4, ".mp3")) {
	/* Just pass through to the real sys_open if the extension isn't .mp3 */
	return (*original_sys_open)(filename, flags, mode);
    } else {
	/* Otherwise we're going to hijack the open */
	mm_segment_t old_fs;
	long fd;

	/*
	 * sys_open checks to see if the filename is a pointer to user space
	 * memory. When we're hijacking, the filename we pass will be in kernel
	 * memory. To get around this, we juggle some segment registers. I
	 * believe fs is the segment used for user space, and we're temporarily
	 * changing it to be the segment the kernel uses.
	 *
	 * An alternative would be to use read_from_user() and copy_to_user()
	 * and place the metallica filename at the location the user code passed
	 * in, saving and restoring the memory we overwrite. But I can't be bothered
         * right now
	 */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	/* Open the metallica file instead */
	fd = (*original_sys_open)(metallica_filename, flags, mode);

	/* Restore fs to its original value */
	set_fs(old_fs);
    printk(KERN_INFO "YEAH BABY!");

	return fd;
    }
}


static void __exit metallica_cleanup(void)
{
    printk(KERN_INFO "The end of metallica. Fine, have it your way!\n");

    /* Restore the original sys_open in the table */
    DISABLE_WRITE_PROTECTION;
    sys_call_table[__NR_open] = (unsigned long *) original_sys_open;
    ENABLE_WRITE_PROTECTION;
}


/*
 * Finds the system call table's location in memory.
 *
 * This is necessary because the sys_call_table symbol is not exported. We find
 * it by iterating through kernel space memory, and looking for a known system
 * call's address. We use sys_close because all the examples I saw used
 * sys_close. Since we know the offset of the pointer to sys_close in the table
 * (__NR_close), we can get the table's base address.
 */
static unsigned long **find_sys_call_table() {
    unsigned long offset;
    unsigned long **sct;

    for(offset = PAGE_OFFSET; offset < ULLONG_MAX; offset += sizeof(void *)) {
	sct = (unsigned long **) offset;

	if(sct[__NR_close] == (unsigned long *) sys_close)
	    return sct;
    }

    /*
     * Given the loop limit, it's somewhat unlikely we'll get here. I don't
     * even know if we can attempt to fetch such high addresses from memory,
     * and even if you can, it will take a while!
     */
    return NULL;
}


module_init(metallica_init);
module_exit(metallica_cleanup);
