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

#define MAX_PETS 10
#define MAX_WEIGHT 100
#define PET_CAT 0
#define PET_DOG 1
#define PET_LIZARD 2

#define OFFLINE 0
#define IDLE 1
#define UP 2
#define DOWN 3
#define LOADING 4




struct thread_parameter{
    struct list_head list;
    struct mutex my_mutex;
    struct task_struct *kthread;
    int c_state;                //c_ == current_ n_ == next_
    int c_floor;
    int c_weight;
    int c_occupants;
    bool issue_request;
    bool deactivated;
};

typedef struct{
    struct list_head list;
    int pet_type;
    int boarding_floor;
    int destination_floor;
}Pet;

struct thread_parameter e;          //THIS IS THE ELEVATOR e FOR SHORTHAND 
static struct file_operations fops;
struct list_head passengerInEachQueue[10];
struct list_head passengersInsideElev[10];

extern int (*STUB_start_elevator)(void);
int start_elevator(void){
    printk(KERN_NOTICE "started\n");
    if(e.c_state!=OFFLINE){
        return 1;
    }
    else{
        if(mutex_lock_interruptiple(&e.my_mutex)==0){
            e.c_state = IDLE;
            e.c_floor=1;
            e.c_occupants=0;
            e.deactivated=false;
        }
        mutex_unlock(&e.my_mutex);
        if(e.c_state==OFFLINE){
            return -1;
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
        if(mutex_lock_interruptiple(&e.my_mutex)==0 && e.c_occupants==0){
            e.c_state=OFFLINE;
        }
        mutex_unlock(&e.my_mutex);
        
    }while(e.c_occupants>0);
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

int typeToWeight(int type){
    //cat will weigh 15 lbs, each dog 45 lbs, and each lizard 5 lbs
    if(type == PET_CAT){
        return 15;
    }else if(type == PET_DOG){
        return 45;
    }else if(type == PET_LIZARD){
        return 5;
    }
}

bool canLoad(void){
    struct list_head * pos;
	struct list_head * temp;
    Pet * tempPet=NULL;

    if(mutex_lock_interruptiple(&e.my_mutex)==0){

        list_for_each_safe(pos, temp, &passengerInEachQueue){
            tempPet=list_entry(pos, Pet, passengerInEachQueue);

            if(tempPet->boarding_floor == e.c_floor){
                if(tempPet->boarding_floor < tempPet->destination_floor && e.c_state==UP){

                    if(e.c_occupants + 1 > MAX_PETS){
                        mutex_unlock(&e.my_mutex);
                        return false;
                    }else if(e.c_weight + typeToWeight(tempPet->pet_type) > MAX_WEIGHT){
                        mutex_unlock(&e.my_mutex);
                        return false;
                    }else {
                        mutex_unlock(&e.my_mutex);
                        return true;
                    }
                } 
                else if(tempPet->boarding_floor > tempPet->destination_floor && e.c_state==DOWN){

                    if(e.c_occupants + 1 > MAX_PETS){
                        mutex_unlock(&e.my_mutex);
                        return false;
                    }else if(e.c_weight + typeToWeight(tempPet->pet_type) > MAX_WEIGHT){
                        mutex_unlock(&e.my_mutex);
                        return false;
                    }else {
                        mutex_unlock(&e.my_mutex);
                        return true;
                    }
                }
            }
        }

    }

}

void startLoad(void){
    struct list_head * pos;
	struct list_head * temp;
    Pet *tempPet=NULL;
    Pet *petOnElev=NULL;
    
  
    if(mutex_lock_interruptiple(&e.my_mutex)==0){
        list_for_each_safe(pos, temp, &passengerInEachQueue){
            tempPet=list_entry(pos, Pet, passengerInEachQueue);
            if(tempPet->boarding_floor == e.c_floor){
                
                if(tempPet->boarding_floor < tempPet->destination_floor && e.c_state==UP){
                    petOnElev = kmalloc(sizeof(struct Pet), __GFP_RECLAIM | __GFP_IO | __GFP_FS);
                    petOnElev->pet_type=tempPet->pet_type;
                    petOnElev->boarding_floor=tempPet->boarding_floor;
                    petOnElev->destination_floor=tempPet->destination_floor;
                    list_add_tail(&petOnElev->list, &passengersInsideElev);
                    kfree(petOnElev);
                }else if(tempPet->boarding_floor > tempPet->destination_floor && e.c_state==DOWN){
                    petOnElev = kmalloc(sizeof(struct Pet), __GFP_RECLAIM | __GFP_IO | __GFP_FS);
                    petOnElev->pet_type=tempPet->pet_type;
                    petOnElev->boarding_floor=tempPet->boarding_floor;
                    petOnElev->destination_floor=tempPet->destination_floor;
                    list_add_tail(&petOnElev->list, &passengersInsideElev);
                    kfree(petOnElev);
                }
            } 
        }
    }
}

int elevator(void)        //function used in kthread_run as the elevator mmodule
{
    while(!kthread_should_stop()){
        if(e.c_state==OFFLINE){
            continue;
        }
        else if(e.c_state==IDLE){
            if(canLoad()){
                startLoad();
                continue;
            }else if(e.issue_request==false){
                continue;
            }
            else{
                e.c_state=UP;
                continue;
            }
        }
        else if(e.c_state==UP){
            if(e.c_floor==10){
                e.c_state=DOWN;
                continue;
            }else{
                ssleep(2);
                e.c_floor=e.c_floor+1;
            }

            if(canUnload()){
                startUnload();
            }

            if(canLoad()){
                startLoad();
                continue;

            }else {
                e.c_state=UP;
                continue;
            }
        }       
        else if(e.c_state==DOWN){
            if(e.c_floor==1){
                e.c_state=UP;
                continue;
            }else{
                ssleep(2);
                e.c_floor=e.c_floor-1;
            }

            if(canUnload()){
                startUnload();
            }
        
            if(canLoad()){
                startLoad();
                continue;

            }else {
                e.c_state=DOWN;
                continue;
            }
        }
    }
}

void m_init(void){
    mutex_init(&e.my_mutex);
    init_sys_calls();
    e.kthread=kthread_run(elevator,&e,"elevator thread");
    for(int i=0;i<10;i++){
        INIT_LIST_HEAD(passengerInEachQueue[i])
        INIT_LIST_HEAD(passengersInsideElev[i])
    }
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
