#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/module.h>


static struct file_operations fileOps;
struct timespec oldTime;
static char *message;
static int read_p;
static int timesProcOpened=0;


#define ENTRY_SIZE 1000
#define PERMS 0644
#define PARENT NULL
#define TIMER_NAME "timer"

int proc_timer_open(struct inode *sp_inode, struct file *sp_file) {
	read_p = 1;
	timesProcOpened++;
	struct timespec currentTime = current_kernel_time();
    char* buff = kmalloc(sizeof(char) * ENTRY_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
	printk(KERN_INFO "proc opened\n");
	message=kmalloc(sizeof(char) * ENTRY_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
		if (buff == NULL || message==NULL) {
		printk(KERN_WARNING "proc open null");
		return -ENOMEM;
	}		
	strcpy(message, "");
	long int tv_sec=currentTime.tv_sec;
	long int tv_nsec=currentTime.tv_nsec;
	sprintf(buff, "current time: %ld.%ld\n", tv_sec, tv_nsec);
	strcat(message,buff);
	long int elapsedSecs=0;
	long int elapsedNsecs=0;
	
	if(oldTime.tv_sec){
	     elapsedSecs=currentTime.tv_sec- oldTime.tv_sec;
		
	}
	if(oldTime.tv_nsec){
		 elapsedNsecs=currentTime.tv_nsec- oldTime.tv_nsec;
		if(elapsedNsecs<0){
			elapsedNsecs+=1000000000;
			elapsedSecs--;
		}
	}
	if(timesProcOpened>1){
			sprintf(buff, "elapsed time: %ld.%09ld\n", elapsedSecs, elapsedNsecs);
			strcat(message,buff);
	}
	
	oldTime=current_kernel_time();
	return 0;
	
}

ssize_t proc_timer_read(struct file *sp_file, char __user *buff, size_t size, loff_t *offset) {
	int len = strlen(message);
	
	read_p = !read_p;
	if (read_p)
		return 0;
		
	copy_to_user(buff, message, len);
	return len;
}

int proc_timer_release(struct inode *sp_inode, struct file *sp_file) {
	kfree(message);
	return 0;
}

int init_m(void){
	printk(KERN_NOTICE "Creating /proc/%s \n",TIMER_NAME);
	fileOps.open = proc_timer_open;
	fileOps.read = proc_timer_read;
	fileOps.release = proc_timer_release;
	proc_create(TIMER_NAME, 0644, NULL, &fileOps);
	return 0;
	
}
static void exit_m(void){
	remove_proc_entry(TIMER_NAME, NULL);
	printk(KERN_NOTICE "Proc removed /proc/%s\n", TIMER_NAME);
}
module_init(init_m);
module_exit(exit_m);
