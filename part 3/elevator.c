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
    int id;
    int c_state;                //c_ == current_ n_ == next_
    int n_state;
    int c_floor;
    int n_floor;
    int c_weight;
    int c_occupants;
}

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
    if(mutex_lock_interruptiple(&e.my_mutex)==0){

    }
    mutex_unlock(&e.my_mutex);

    return 1;
}

extern int (*STUB_stop_elevator)(void);
int stop_elevator(void){
    if(mutex_lock_interruptiple(&e.my_mutex)==0){

    }
    mutex_unlock(&e.my_mutex);

    return 1;
}

extern int (*STUB_issue_request)(void);
int issue_request(void){
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

}

void m_init(void){
    init_sys_calls();
    mutex_init(&e.my_mutex);
    e->kthread=kthread_run(elevator,&e,"elevator thread");
}
module_init(m_init);
void m_exit(void){


}
module_exit(m_exit);
