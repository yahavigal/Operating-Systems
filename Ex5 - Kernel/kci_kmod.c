/*
 * kci_kmod.c
 *
 *  Created on: 28 Jan 2017
 *      Author: osboxes
 */

#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/keyboard.h>
#include <linux/debugfs.h>
#include <linux/syscalls.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <asm/paravirt.h>
#include "kci.h"

MODULE_LICENSE("GPL v2");

 /*	 globals  */
static int pid;
static int fdes;
static int cipher;

 /*   decs    */
static struct dentry *file;
static struct dentry *subdir;
static unsigned long **sysct;
static unsigned long oldcr0;
static char buff[BUFSIZE] = {0};
static size_t possition = 0;

 /* syscall decs */
asmlinkage long (* oldread)(int fd, char* __user buf, size_t count);
asmlinkage long (* oldwrite)(int fd, char* __user buf, size_t count);

static long device_ioctl(struct file* file, unsigned int ioctl_num, unsigned long ioctl_param);

asmlinkage long myread(int fd, char* __user buf, size_t count){
	int rc = 0;
	int i;
	long bytes = -1;
	char curr, log[LOGBUF] = {0};

	bytes = oldread(fd, buf, count);

	if(current->pid != pid || fdes != fd || cipher == 0)
		return bytes;

	for(i=0; i<bytes; i++){
		if ((rc = get_user(curr, buf + i))<0){
			printk("Error during get_user\n");
			return -1;
		}

		curr -= 1;

		if ((rc = put_user(curr, buf + i))<0){
			printk("Error during put_user\n");
			return -1;
		}
	}

	if((rc = sprintf(log, "PID = %d, FILE_DES = %d, READ %ld BYTES OUT OF %d\n", pid, fdes, bytes, count))<0){
		printk("Error during sprintf\n");
		return -1;
	}

	pr_debug("%s\n",log);

	if((possition+rc)>= BUFSIZE){
		memset(buff, 0, BUFSIZE);
		possition = 0;
	}

	strncpy(buff + possition, log, rc);
	rc += 1;
	possition += rc;

	return bytes;
}

asmlinkage long mywrite(int fd, char* __user buf,size_t count){
	int rc = 0;
	int i;
	long bytes = -1;
	char curr = 0, log[LOGBUF] = {0};

	if (cipher && (current->pid == pid) && (fd == fdes)){

		for (i=0; i<count; i++){
			if ((rc = get_user(curr, buf + i))<0){
				printk("Error during get_user\n");
				return -1;
			}

			curr += 1;

			if ((rc = put_user(curr, buf + i))<0){
				printk("Error during put_user\n");
				return -1;
			}
		}
	}

	bytes = oldwrite(fd, buf, count);

	for (i=0; i<count; i++){
		if ((rc = get_user(curr, buf + i))<0){
			printk("Error during get_user\n");
			return -1;
		}

		curr -= 1;

		if ((rc = put_user(curr, buf + i))<0){
			printk("Error during put_user\n");
			return -1;
		}
	}

	if((rc = sprintf(log, "PID = %d, FILE_DES = %d, WRITTEN %ld BYTES OUT OF %d\n", pid, fdes, bytes, count))<0){
		printk("Error during sprintf\n");
		return -1;
	}

	pr_debug("%s\n",log);

	if((possition+rc)>= BUFSIZE){
		memset(buff, 0, BUFSIZE);
		possition = 0;
	}

	strncpy(buff + possition, log, rc);
	rc+=1;
	possition += rc;

	return bytes;
}

static ssize_t logr(struct file *filp, char *buffer, size_t len, loff_t *offset){
	return simple_read_from_buffer(buffer, len, offset, buff, possition);
}

const struct file_operations dbgFops = {
	.owner = THIS_MODULE,
	.read = logr,
};


struct file_operations Fops = {
    .unlocked_ioctl= device_ioctl,
};

static long device_ioctl(struct file* file, unsigned int ioctl_num, unsigned long ioctl_param){
  switch(ioctl_num){
  case IOCTL_SET_PID:
	  pid = (int)ioctl_param;
	  break;
  case IOCTL_SET_FD:
	  fdes = (int)ioctl_param;
	  break;
  case IOCTL_CIPHER:
	  cipher = ioctl_param;
	  break;
  default:
	  return -1;
  }

  return 0;
}

static unsigned long** aquireSysct(void){
	unsigned long int offset = PAGE_OFFSET;
	unsigned long** sct;
	while (offset < ULLONG_MAX) {
		sct = (unsigned long **)offset;
		if (sct[__NR_close] == (unsigned long *) sys_close)
			return sct;
		offset += sizeof(void *);
	}
	return NULL;
}

/* Initialize the module */
static int __init mod_init(void){
	int rc = 0;
	possition = 0;

	// creating log file with its subdirectory
	subdir = debugfs_create_dir(SUBDIR, NULL);
	if (IS_ERR(subdir))
		return PTR_ERR(subdir);
	if (!subdir)
		return -ENOENT;

	file = debugfs_create_file(FILE, S_IRUSR, subdir, NULL, &dbgFops);
	if (!file) {
		debugfs_remove_recursive(subdir);
		return -ENOENT;
	}

    pid = -1;
    fdes = -1;
    cipher = 0;

    // Register a character device. Get newly assigned major num
	 if ((rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops))<0){
        printk(KERN_ALERT "%s failed with %d\n","Sorry, registering the character device ", MAJOR_NUM);
        return -1;
    }

    // Aquire the sysct pointer
    if (!(sysct = aquireSysct())){
    	printk("Error while aquiring sysct pointer\n");
    	return -1;
    }

    // intercepting and switching
    oldcr0 = read_cr0();
    write_cr0(oldcr0 & ~0x00010000);
    oldread = (void *)sysct[__NR_read];
    oldwrite = (void *)sysct[__NR_write];
    sysct[__NR_read] = (unsigned long *)myread;
    sysct[__NR_write] = (unsigned long *)mywrite;
    write_cr0(oldcr0);

    return 0;
}

/* Cleanup - unregister the appropriate file from /proc */
static void __exit mod_exit(void){

	debugfs_remove_recursive(subdir);

	write_cr0(oldcr0 & ~0x00010000);
	if (sysct){
		sysct[__NR_read] = (unsigned long*) oldread;
		sysct[__NR_write] = (unsigned long*) oldwrite;
	}
	write_cr0(oldcr0);
	msleep(2000);

    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

module_init(mod_init);
module_exit(mod_exit);
