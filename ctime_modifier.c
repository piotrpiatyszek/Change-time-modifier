#include<linux/init.h>
#include<linux/module.h>
#include<linux/cdev.h>
#include<linux/fs.h>
#include<linux/errno.h>
#include<linux/uaccess.h>
#include<linux/namei.h>
#include<linux/path.h>
#include<linux/mount.h>
#include<linux/version.h>
#include<linux/limits.h>
#include<linux/slab.h>

#define IOCTL_CMD _IOW(0xCD, 0x19, char*)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Piotr Piątyszek");

struct ctm_data {
	char* pathname;
	size_t pathsize;
	struct timespec time;
}; 

static long ctm_change_ctime(struct inode* inode, struct path* path, struct timespec* time){
	if(inode->i_flags & S_NOCMTIME)return -ENOTSUPP;
	#ifdef HAS_UNMAPPED_ID
	if(HAS_UNMAPPED_ID(inode))return -ENOTSUPP;
	#endif
	if(IS_NOCMTIME(inode))return -ENOTSUPP;
	if(mnt_want_write(path->mnt) != 0)return -ECANCELED;
	inode->i_ctime = *time;
	mark_inode_dirty_sync(inode);
	mnt_drop_write(path->mnt);
	return 0;
}

long ctm_ioctl(struct file* filep, unsigned int cmd, unsigned long ptr){
	char* pathname;
	struct path path;
	struct inode* inode;
	struct ctm_data data;	
	if(cmd != IOCTL_CMD)return -EINVAL;
	if(copy_from_user(&data, (const void*) ptr, sizeof(struct ctm_data)))return -EINVAL;
	if(data.pathsize > PATH_MAX)return -ENAMETOOLONG;
	pathname = kmalloc(data.pathsize, GFP_KERNEL);
	if(copy_from_user(pathname, (const char*) data.pathname, data.pathsize))return -EFAULT;
	if(kern_path(pathname, LOOKUP_FOLLOW, &path) < 0)return -ENOENT;
	inode = path.dentry->d_inode;
	return ctm_change_ctime(inode, &path, &data.time);
}

struct {
	struct file_operations fops;
	dev_t dev;
	struct cdev chardev;
} ctm_device = {
	.fops = {
		.owner = THIS_MODULE,
		.unlocked_ioctl = ctm_ioctl
	}
};

static int __init ctm_init(void){
	int err;
	err = alloc_chrdev_region(&ctm_device.dev, 0, 1, "ctimem");
	if(err)goto fail;
	cdev_init(&ctm_device.chardev, &ctm_device.fops);
	ctm_device.chardev.owner = THIS_MODULE;
	err = cdev_add(&ctm_device.chardev,ctm_device.dev, 1);
	if(err)goto fail;
	printk(KERN_INFO "Ctime_modifier started\n");
	return 0;
fail:
	printk(KERN_INFO "Cannot create a ctime_modifier's device\n");
	return -1;
}

static void __exit ctm_exit(void){
	printk(KERN_INFO "Ctime_modifier exited\n");
	cdev_del(&ctm_device.chardev);
	unregister_chrdev_region(ctm_device.dev, 1);
}

module_init(ctm_init);
module_exit(ctm_exit);
