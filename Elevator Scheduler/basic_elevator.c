
#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>







        //THIS IS THE ELEVATOR e FOR SHORTHAND 

extern int (*STUB_start_elevator)(void);
int start_elevator(void){
    printk(KERN_NOTICE "started elevator\n");

    return 1;
}

extern int (*STUB_stop_elevator)(void);
int stop_elevator(void){
    printk(KERN_NOTICE "stop\n");


    return 1;
}

extern int (*STUB_issue_request)(int,int,int);
int issue_request(int boarding_floor, int final_floor,int pet_type){
     printk(KERN_NOTICE "issued reuest for elavaotr\n");
   
    return 1;
}

static int init_sys_calls(void)           //assign STUB's to functions
{
	 printk(KERN_NOTICE "indside init_sys_calls\n");
    STUB_start_elevator=start_elevator;
    STUB_stop_elevator=stop_elevator;
    STUB_issue_request=issue_request;
    return 1;
}




int elevator(void * tparams)        //function used in kthread_run as the elevator mmodule
{
//    while(!kthread_should_stop()){
//
//
//
//
//    }
}

module_init(init_sys_calls);
void m_exit(void){
    STUB_start_elevator = NULL;
	STUB_issue_request = NULL;
	STUB_stop_elevator = NULL;

//	kthread_stop(e.kthread);
//	mutex_destroy(&e.my_mutex);
	printk(KERN_NOTICE "Removing /proc/");
}
module_exit(m_exit);
