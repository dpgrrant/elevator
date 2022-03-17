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
#include <linux/delay.h>

#define ENTRY_NAME "elevator"
#define PERMS 0644
#define PARENT NULL

#define ENTRY_SIZE 1000
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

int printingQueue(void);
struct thread_parameter{   //Elevator thread parameters
    struct list_head list;
    struct mutex my_mutex;
    struct task_struct *kthread;
    int c_state;                //c_ == current_ n_ == next_
    int c_floor;
    int c_weight;
    int c_occupants;
    bool issue_requested;
    bool deactivated;
    int total_pets_served;    
};

typedef struct{    //Pet Attributes
    struct list_head list;
    int pet_type;
    int boarding_floor;
    int destination_floor;
}Pet;

int cat=0;   //Used in Proc to print count of pet types
int dog=0;
int lizard=0;

static struct file_operations fops;
static char *message;
static int read_p;


struct thread_parameter e;          //THIS IS THE ELEVATOR THREAD e FOR SHORTHAND 
struct list_head passengerInEachQueue[10];
struct list_head passengersInsideElev;

extern int (*STUB_start_elevator)(void);   //START Systemcall
int start_elevator(void){
    if(e.c_state!=OFFLINE){
        return 1;
    }
    else{
        if(mutex_lock_interruptible(&e.my_mutex)==0){
            e.c_state = IDLE;
            e.c_floor=1;
            e.c_occupants=0;
            e.deactivated=false;
            e.total_pets_served=0;
            e.issue_requested=false;
        }
        mutex_unlock(&e.my_mutex);
        if(e.c_state == OFFLINE){
            return -1;
        }
        return 0;
    }
}

extern int (*STUB_stop_elevator)(void);    //STOP Systemcall
int stop_elevator(void){

    if(e.deactivated==true){
        return 1;
    }
    if(mutex_lock_interruptible(&e.my_mutex)==0){
        e.deactivated=true;
        e.c_state=OFFLINE;
    }
    mutex_unlock(&e.my_mutex);
    return 0;
}

extern int (*STUB_issue_request)(int,int,int);    //ISSUE request systemcall
int issue_request(int boarding_floor, int final_floor,int pet_type){
     Pet *entry;
    if(mutex_lock_interruptible(&e.my_mutex)==0){
    	e.issue_requested=true;  
    	   entry = kmalloc(sizeof(Pet), __GFP_RECLAIM | __GFP_IO | __GFP_FS);
		   entry->pet_type = pet_type; 
		   entry->boarding_floor = boarding_floor;
		   entry->destination_floor = final_floor;
		   list_add_tail(&entry->list, &passengerInEachQueue[boarding_floor - 1]);  
    }
    mutex_unlock(&e.my_mutex);
    return 1;
}

int countQueuePassengers(void) { // Counts the queue at each floor.
  struct list_head *ptr;
  Pet* pet;
  int numWaitingQueue=0;
  int i = 0;
  while (i < 10) {
    list_for_each(ptr, &passengerInEachQueue[i]) {
      pet = list_entry(ptr, Pet, list);
      numWaitingQueue++;
    }
   i++;
  }
  return numWaitingQueue;
}

void countPetType(void) { // Counts the pet types inside the elevator
  struct list_head *pos;
  Pet* entry;
  int i = 0;
  cat=0;
  dog=0;
  lizard=0;
    list_for_each(pos, &passengersInsideElev) {
      entry = list_entry(pos, Pet, list);
      if(entry->pet_type==PET_CAT){
          cat++;
      }
      else if(entry->pet_type==PET_DOG){
          dog++;
      }
      else if(entry->pet_type==PET_LIZARD){
          lizard++;
      }
    }
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
    return 0;    
}

bool canLoad(void){       //Function to check if a pet is eligible to be loaded
    struct list_head * pos;
	struct list_head * temp;
    Pet * tempPet=NULL;
    if(mutex_lock_interruptible(&e.my_mutex)==0){
        list_for_each_safe(pos, temp, &passengerInEachQueue[e.c_floor-1]){
            tempPet=list_entry(pos, Pet, list);
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
     mutex_unlock(&e.my_mutex);
    return false;

}
void startLoad(void){    //function to start loading of pets on elevator
    struct list_head * pos;
	struct list_head * temp;
    Pet *tempPet=NULL;
    Pet *petOnElev=NULL;
    
    if(mutex_lock_interruptible(&e.my_mutex)==0){
        list_for_each_safe(pos, temp, &passengerInEachQueue[e.c_floor-1]){
            tempPet=list_entry(pos, Pet, list);
            if(tempPet->boarding_floor == e.c_floor){
            	     
                if(tempPet->boarding_floor < tempPet->destination_floor && e.c_state==UP){
                	 if(e.c_weight + typeToWeight(tempPet->pet_type) > MAX_WEIGHT){
                        mutex_unlock(&e.my_mutex);
                        break;
                    }
                    if(e.c_occupants + 1 > MAX_PETS){
                        mutex_unlock(&e.my_mutex);
                        break;
                    }
                	e.c_state=LOADING;
                    ssleep(1);
                    petOnElev = kmalloc(sizeof(Pet), __GFP_RECLAIM | __GFP_IO | __GFP_FS);
                    petOnElev->pet_type=tempPet->pet_type;
                    petOnElev->boarding_floor=tempPet->boarding_floor;
                    petOnElev->destination_floor=tempPet->destination_floor;
                    list_add_tail(&petOnElev->list, &passengersInsideElev);
                    e.c_weight= e.c_weight + typeToWeight(tempPet->pet_type);
                    e.c_occupants++;
                     list_del(pos);
                    kfree(tempPet);
                    	e.c_state=UP;
                }else if(tempPet->boarding_floor > tempPet->destination_floor && e.c_state==DOWN){
                	if(e.c_weight + typeToWeight(tempPet->pet_type) > MAX_WEIGHT){
                        mutex_unlock(&e.my_mutex);
                        break;
                    }
                    if(e.c_occupants + 1 > MAX_PETS){
                        mutex_unlock(&e.my_mutex);
                        break;
                    }
                	e.c_state=LOADING;
                    ssleep(1);
                    petOnElev = kmalloc(sizeof(Pet), __GFP_RECLAIM | __GFP_IO | __GFP_FS);
                    petOnElev->pet_type=tempPet->pet_type;
                    petOnElev->boarding_floor=tempPet->boarding_floor;
                    petOnElev->destination_floor=tempPet->destination_floor;
                    list_add_tail(&petOnElev->list, &passengersInsideElev);
                    e.c_weight= e.c_weight + typeToWeight(tempPet->pet_type);
                    e.c_occupants++;
                     list_del(pos);
                    kfree(tempPet);
                    	e.c_state=DOWN;
                    
                }
            } 
        }
    }
     mutex_unlock(&e.my_mutex);
}


int canUnload(void) {     //function to check if a pet can be unloaded from elevator
  Pet *pet=NULL; 
  struct list_head *ptr;
  if(mutex_lock_interruptible(&e.my_mutex)==0){
    list_for_each(ptr, &passengersInsideElev) {
    pet = list_entry(ptr, Pet, list);
    if (pet->destination_floor == e.c_floor) {
    	mutex_unlock(&e.my_mutex);
      return 1;
    }
  }
}
   mutex_unlock(&e.my_mutex);
  return 0;
}

void startUnload(void) {    //function to start unloading a pet from elevator
  Pet *pet;
  int tempState=e.c_state;
  struct list_head *ptr, *loc;
  if(mutex_lock_interruptible(&e.my_mutex)==0){
  list_for_each_safe(ptr, loc, &passengersInsideElev) {
    pet = list_entry(ptr, Pet, list);
    if (pet->destination_floor == e.c_floor) { 
     e.c_state=LOADING;
     e.c_occupants--;
     e.c_weight=e.c_weight- typeToWeight(pet->pet_type);
     e.total_pets_served++;
     ssleep(1);
     list_del(ptr);    
     kfree(pet);      
    }
   }
  }
  e.c_state=tempState;
  mutex_unlock(&e.my_mutex);
}

int elevator(void *data)        //function used in kthread_run as the elevator module
{
    while(!kthread_should_stop()){
    	if(e.deactivated && e.c_occupants==0 && e.c_state!=OFFLINE){   //implement stop thread function 
    		e.c_state=OFFLINE;
    		continue;
		}
        if(e.c_state==OFFLINE){
            continue;
        }
        else if(e.c_state==IDLE){  
        	if(e.issue_requested==false){
                continue;
            }
        	e.c_state=UP;
            if(!e.deactivated && canLoad()){            
                startLoad();               
                continue;
            }
            else{
                e.c_state=UP;
                continue;
            }
        }
        else if(e.c_state==UP){   // checks if current state is UP
        	 if(canUnload()){
                 startUnload();
             }             
            if(e.c_floor==10){
            	e.c_state=DOWN;
            	if(!e.deactivated && canLoad()){
            	 	startLoad();
				}
                
                continue;
            }else{
                ssleep(2);
                e.c_floor=e.c_floor+1;
            }
            if(!e.deactivated && canLoad()){
                startLoad();
                continue;
            }else {
                e.c_state=UP;
                continue;
            }
        }       
        else if(e.c_state==DOWN){        		
             if(canUnload()){            	
                 startUnload();
             }            
            if(e.c_floor==1){
            	 e.c_state=UP;
              if(!e.deactivated && canLoad()){
                startLoad();
            }
                continue;
            }else{
                ssleep(2);
                e.c_floor=e.c_floor-1;
            }        
            if(!e.deactivated && canLoad()){
                startLoad();
                continue;
            }else {
                e.c_state=DOWN;
                continue;
            }
        }
    }
    return 0;
}

int print_elevator_proc(void){   //to be printed in proc elevator   
    Pet *pet=NULL;
	struct list_head *temp;
    struct list_head *pos;

    char* currentState;   
	char *buf = kmalloc(sizeof(char) * 100, __GFP_RECLAIM);
	if (buf == NULL) {
		printk(KERN_WARNING "print_animals");
		return -ENOMEM;
	}

	/* init message buffer */
	strcpy(message, "");

    if(e.c_state==OFFLINE) {
        currentState="OFFLINE";
    }else if(e.c_state==IDLE) {
         currentState="IDLE";
        
    }else if(e.c_state==UP) {
         currentState="UP";
        
    }else if(e.c_state==DOWN) {
         currentState="DOWN";
        
    } else if(e.c_state==LOADING) {
         currentState="LOADING";
    }           
	/* headers, print to temporary then append to message buffer */
	sprintf(buf, "Elevator state: %s\n", currentState);       strcat(message, buf);
	sprintf(buf, "Current floor: %d\n", e.c_floor);   strcat(message, buf);
	sprintf(buf, "Current weight: %d\n", e.c_weight);   strcat(message, buf);
    countPetType();
	sprintf(buf, "Elevator status: %d C, %d D, %d L\n", cat, dog,lizard );           strcat(message, buf);
    sprintf(buf, "Number of passengers: %d\n", e.c_occupants);   strcat(message, buf);
    sprintf(buf, "Number of passengers waiting: %d\n", countQueuePassengers());   strcat(message, buf);
    sprintf(buf, "Number passengers serviced: %d\n", e.total_pets_served);   strcat(message, buf);

    int i=10;
    char cfloor=' ';
    int numEachQueue;
    while (i > 0) {
        numEachQueue=0;
        if (i==e.c_floor){
            cfloor='*';
        }else {
            cfloor=' ';
        }
        sprintf(buf,"[ %c ] Floor: %d: ", cfloor, i); strcat(message, buf);
        list_for_each(pos, &passengerInEachQueue[i-1]) {
        pet = list_entry(pos, Pet, list);
        numEachQueue++;
        }

        if(numEachQueue!=0){
        	sprintf(buf,"%d ",numEachQueue); strcat(message, buf);
            list_for_each(pos, &passengerInEachQueue[i-1]) {
                pet = list_entry(pos, Pet, list);
                if(pet->pet_type==PET_CAT){
                    sprintf(buf,"C "); strcat(message, buf);
                }else if(pet->pet_type==PET_DOG){
                    sprintf(buf,"D "); strcat(message, buf);
                }else if(pet->pet_type==PET_LIZARD){
                    sprintf(buf,"L "); strcat(message, buf);
                }
            }
            sprintf(buf,"\n", numEachQueue); strcat(message, buf);
        }else {
            sprintf(buf,"%d\n", numEachQueue); strcat(message, buf);
        }

        i--;
    }

	/* trailing newline to separate file from commands */
	strcat(message, "\n");
	kfree(buf);
	return 0;
}


int elevator_proc_open(struct inode *sp_inode, struct file *sp_file) {
	read_p = 1;
	message = kmalloc(sizeof(char) * ENTRY_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
	if (message == NULL) {
		printk(KERN_WARNING "animal_proc_open");
		return -ENOMEM;
	}
	return print_elevator_proc();
}

ssize_t elevator_proc_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset) {
	int len = strlen(message);
	
	read_p = !read_p;
	if (read_p)
		return 0;
		
	copy_to_user(buf, message, len);
	return len;
}

int elevator_proc_release(struct inode *sp_inode, struct file *sp_file) {
	kfree(message);
	return 0;
}


int m_init(void){
    mutex_init(&e.my_mutex);
    STUB_start_elevator=start_elevator;
    STUB_stop_elevator=stop_elevator;
    STUB_issue_request=issue_request;

    fops.open = elevator_proc_open;
	fops.read = elevator_proc_read;
	fops.release = elevator_proc_release;

    if (!proc_create(ENTRY_NAME, PERMS, NULL, &fops)) {
		printk(KERN_WARNING "creating proc ERR\n");
		remove_proc_entry(ENTRY_NAME, NULL);
		return -ENOMEM;
	}

    e.kthread=kthread_run(elevator,&e,"elevator thread");
    int i;
    INIT_LIST_HEAD(&passengersInsideElev);
    for(i=0;i<10;i++){
        INIT_LIST_HEAD(&passengerInEachQueue[i]);
    }
    return 0;
}
module_init(m_init);
void m_exit(void){
	printk(KERN_NOTICE "Removing /proc/%s\n", ENTRY_NAME);
    STUB_start_elevator = NULL;
	STUB_issue_request = NULL;
	STUB_stop_elevator = NULL;
	
    kthread_stop(e.kthread);
	remove_proc_entry(ENTRY_NAME, NULL);
    mutex_destroy(&e.my_mutex);
	
}
module_exit(m_exit);
