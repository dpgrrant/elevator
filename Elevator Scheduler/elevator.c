#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <linux/kthread.h>

#define ENTRY_NAME "Elevator"
#define PERMS 0644
#define PARENT NULL
static struct file_operations fops;

struct thread_parameter{
    struct list_head list;
    struct mutex my_mutex;
    struct task_struct *kthread;
    int c_state="OFFLINE";                //c_ == current_ n_ == next_
    int c_floor=1;
    int c_weight=0;
    int c_occupants=0;
    bool deactivated=true;
};

#define MAX_PETS 10
#define MAX_WEIGHT 100
#define PET_DOG 0
#define PET_CAT 1
#define PET_LIZARD 2

typedef struct{
    struct list_head list;
    int pet_type;
    int boarding_floor;
    int destination_floor;
}Pet;

struct thread_parameter e;          //THIS IS THE ELEVATOR e FOR SHORTHAND 

extern int (*STUB_start_elevator)(void);
int start_elevator(void){
    printk(KERN_NOTICE "started\n");
    if(e.c_state!="OFFLINE"){
        return 1;
    }
    else{
        if(mutex_lock_interruptiple(&e.my_mutex)==0){
            e.c_state = "IDLE";
            e.c_floor=1;
            e.c_occupants=0;
            e.deactivated=false;
        }
        mutex_unlock(&e.my_mutex);
        if(e.c_state=="OFFLINE"){
            return -ERRORNUM;
        }
        return 0;
    }
}

extern int (*STUB_stop_elevator)(void);
int stop_elevator(void){
    printk(KERN_NOTICE "stop\n");

    if(e.deactivated==true){
        return 1;
    }

    if(mutex_lock_interruptiple(&e.my_mutex)==0){
        e.deactivated=true;
    }
    mutex_unlock(&e.my_mutex);
    
   
    do{
        if(mutex_lock_interruptiple(&e.my_mutex)==0 && e.c_occupants=0){
            e.c_state="OFFLINE";
        }
        mutex_unlock(&e.my_mutex);
        
    }while(c.occupants>0);
    return 0;
}

extern int (*STUB_issue_request)(void);
int issue_request(int boarding_floor, int final_floor,int pet_type){
     printk(KERN_NOTICE "issued \n");
    if(mutex_lock_interruptiple(&e.my_mutex)==0){
  
    }
    mutex_unlock(&e.my_mutex);

    return 1;
}

void init_sys_calls(void)           //assign STUB's to functions
{
    STUB_start_elevator=start_elevator;
    STUB_stop_elevator=stop_elevator;
    STUB_issue_request=issue_request;
}




int elevator(void * tparams)        //function used in kthread_run as the elevator mmodule
{
    while(!kthread_should_stop()){
        if(e.c_state=="OFFLINE"){
            break;
        }
        else if(e.c_state=="IDLE"){
            if(canLoad()){
                startLoad();
                continue;

            }else{
                e.c_state="UP"
                continue;
            }
        }
        else if(e.c_state=="UP"){

            if(canLoad()){
                startLoad();
                continue;

            }
            else if(e.c_floor==10){

            }else{
                ssleep(2);
                e.c_floor=e.c_floor+e.c_floor+1;

            }
        }
        else if(e.c_state=="LOADING"){

        }
       
        else if(e.c_state=="DOWN"){

        }
    }
}

void m_init(void){
    mutex_init(&e.my_mutex);
    init_sys_calls();
    
    e->kthread=kthread_run(elevator,&e,"elevator thread");
}
module_init(m_init);
void m_exit(void){
    STUB_start_elevator = NULL;
	STUB_issue_request = NULL;
	STUB_stop_elevator = NULL;

	kthread_stop(e.kthread);
	mutex_destroy(&e.my_mutex);
	printk(KERN_NOTICE "Removing /proc/%s\n", ENTRY_NAME);
}
module_exit(m_exit);
