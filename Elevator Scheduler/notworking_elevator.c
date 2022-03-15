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


struct thread_parameter{
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

typedef struct{
    struct list_head list;
    int pet_type;
    int boarding_floor;
    int destination_floor;
}Pet;

int cat=0;
int dog=0;
int lizard=0;

static struct file_operations fops;
static char *message;
static int read_p;


struct thread_parameter e;          //THIS IS THE ELEVATOR e FOR SHORTHAND 
struct list_head passengerInEachQueue[10];
struct list_head passengersInsideElev;

extern int (*STUB_start_elevator)(void);
int start_elevator(void){
    printk(KERN_NOTICE "started\n");
    if(e.c_state!=OFFLINE){
        return 1;
    }
    else{
        if(mutex_lock_interruptible(&e.my_mutex)==0){
            printk(KERN_NOTICE "Set to IDLE\n");
            e.c_state = IDLE;
            e.c_floor=1;
            e.c_occupants=0;
            e.deactivated=false;
            e.total_pets_served=0;
            e.issue_requested=false;
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
    if(mutex_lock_interruptible(&e.my_mutex)==0){
    	e.c_state=OFFLINE;
        e.deactivated=true;
        kthread_stop(e.kthread);
    }
    mutex_unlock(&e.my_mutex);
    return 0;
}

extern int (*STUB_issue_request)(int,int,int);
int issue_request(int boarding_floor, int final_floor,int pet_type){
     printk(KERN_NOTICE "issued \n");
     Pet *entry;
    if(mutex_lock_interruptible(&e.my_mutex)==0){
    	e.issue_requested=true;
    	printk("before add in to Queue\n");
  
    	   entry = kmalloc(sizeof(Pet), __GFP_RECLAIM | __GFP_IO | __GFP_FS);
		   entry->pet_type = pet_type; 
		   entry->boarding_floor = boarding_floor;
		   entry->destination_floor = final_floor;
		   list_add_tail(&entry->list, &passengerInEachQueue[boarding_floor - 1]);
		   printk("Added to Queue\n");
  
    }
    mutex_unlock(&e.my_mutex);
     printingQueue();
    return 1;
}

int printingQueue(void) { // Prints the queue at each floor.
  struct list_head *pos;
  Pet* entry;
  int currentPos = 0;
  int numWaitingQueue=0;
  int i = 0;
  printk("============Passenger Queue:==========\n");

  while (i < 10) {
    printk("Floor: %d\n", i+1);
    list_for_each(pos, &passengerInEachQueue[i]) {
      entry = list_entry(pos, Pet, list);
      printk("Queue pos: %d\nType: %d\nStart Floor: %d\nDest Floor: %d\n", currentPos, entry->pet_type, entry->boarding_floor, entry->destination_floor);
      ++currentPos;
      numWaitingQueue++;
    }
  i++;
  }
printk("\n");
  return numWaitingQueue;

}

void printElevator(void) { // Prints the elevtor at each floor.
  struct list_head *pos;
  Pet* entry;
  int i = 0;
  cat=0;
  dog=0;
  lizard=0;


  printk("===============Elevator Queue:=====================\n");
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
      printk("Type: %d\nStart Floor: %d\nDest Floor: %d\n", entry->pet_type, entry->boarding_floor, entry->destination_floor);
    }
 
  printk("\n");
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

bool canLoad(void){
    struct list_head * pos;
	struct list_head * temp;
    Pet * tempPet=NULL;
    if(mutex_lock_interruptible(&e.my_mutex)==0){

        list_for_each_safe(pos, temp, &passengerInEachQueue[e.c_floor-1]){
            tempPet=list_entry(pos, Pet, list);
            if(tempPet->boarding_floor == e.c_floor){
                if(tempPet->boarding_floor < tempPet->destination_floor && e.c_state==UP){
                     	printk(KERN_NOTICE " going UP while canload check\n" );
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
					printk(KERN_NOTICE " going DOWN while canload check\n" );
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
void startLoad(void){
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
                	printk(KERN_NOTICE "In UP startload \n");
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
                	printk(KERN_NOTICE "In DOWN startload \n");
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
int canUnload(void) { 
  Pet *entry=NULL; 
  struct list_head *pos;
   if(mutex_lock_interruptible(&e.my_mutex)==0){
  list_for_each(pos, &passengersInsideElev) {
    entry = list_entry(pos, Pet, list);
    if (entry->destination_floor == e.c_floor) {
    	printk(" canUnload" );
    	mutex_unlock(&e.my_mutex);
      return 1;
    }
  }
}
   mutex_unlock(&e.my_mutex);
  return 0;
}
void startUnload(void) {
  Pet *entry;
  int tempState=e.c_state;
  struct list_head *pos, *q;
  if(mutex_lock_interruptible(&e.my_mutex)==0){
  list_for_each_safe(pos, q, &passengersInsideElev) {
    entry = list_entry(pos, Pet, list);
    if (entry->destination_floor == e.c_floor) { 
     // total_pets_served++;
     e.c_state=LOADING;
     e.c_occupants--;
     e.c_weight=e.c_weight- typeToWeight(entry->pet_type);
     e.total_pets_served++;
     ssleep(1);
     list_del(pos);
      
      kfree(entry);
      
    }
   }
  }
  e.c_state=tempState;
  mutex_unlock(&e.my_mutex);
}

int elevator(void *data)        //function used in kthread_run as the elevator mmodule
{
    while(!kthread_should_stop()){
        if(e.c_state==OFFLINE){
            continue;
        }
        else if(e.c_state==IDLE){
        	printingQueue();
        	if(e.issue_requested==false){
                continue;
            }
        	e.c_state=UP;
            if(canLoad()){
            	printk(KERN_NOTICE "canLoad=true \n");            
                startLoad();               
                printElevator();
                printingQueue();
                continue;
            }
            else{
                e.c_state=UP;
                continue;
            }
        }
        else if(e.c_state==UP){
        	printk(" going up and Floor: %d\n", e.c_floor);	
        	 if(canUnload()){
        	 	
                 startUnload();
                 printElevator();
             }
             
            if(e.c_floor==10){
                e.c_state=DOWN;
                continue;
            }else{
                ssleep(2);
                e.c_floor=e.c_floor+1;
                
            }

            

            if(canLoad()){
                        
                startLoad();
                printingQueue();
                printElevator();
                continue;

            }else {
                e.c_state=UP;
                continue;
            }
        }       
        else if(e.c_state==DOWN){
        		printk(" going down and Floor: %d\n", e.c_floor);
        		
             if(canUnload()){            	
                 startUnload();
                 printElevator();
                 printingQueue();
             }
             
            if(e.c_floor==1){
                e.c_state=UP;
                continue;
            }else{
                ssleep(2);
                e.c_floor=e.c_floor-1;
            }

        
            if(canLoad()){
            
                startLoad();
                printElevator();
                printingQueue();
                continue;

            }else {
                e.c_state=DOWN;
                continue;
            }
        }
    }
    return 0;
}

int print_animals(void ){
   
    Pet *entry=NULL;
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
    printElevator();
	sprintf(buf, "Elevator status: %d C, %d D, %d L\n", cat, dog,lizard );           strcat(message, buf);
    sprintf(buf, "Number of passengers: %d\n", e.c_occupants);   strcat(message, buf);
    sprintf(buf, "Number of passengers waiting: %d\n", printingQueue());   strcat(message, buf);
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
        entry = list_entry(pos, Pet, list);
        numEachQueue++;
        }

        if(numEachQueue!=0){
            list_for_each(pos, &passengerInEachQueue[i-1]) {
                entry = list_entry(pos, Pet, list);
                if(entry->pet_type==PET_CAT){
                    sprintf(buf,"C "); strcat(message, buf);
                }else if(entry->pet_type==PET_DOG){
                    sprintf(buf,"D "); strcat(message, buf);
                }else if(entry->pet_type==PET_LIZARD){
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


int animal_proc_open(struct inode *sp_inode, struct file *sp_file) {
	read_p = 1;
	message = kmalloc(sizeof(char) * ENTRY_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
	if (message == NULL) {
		printk(KERN_WARNING "animal_proc_open");
		return -ENOMEM;
	}
	return print_animals();
}

ssize_t animal_proc_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset) {
	int len = strlen(message);
	
	read_p = !read_p;
	if (read_p)
		return 0;
		
	copy_to_user(buf, message, len);
	return len;
}

int animal_proc_release(struct inode *sp_inode, struct file *sp_file) {
	kfree(message);
	return 0;
}




int m_init(void){
    mutex_init(&e.my_mutex);


    STUB_start_elevator=start_elevator;
    STUB_stop_elevator=stop_elevator;
    STUB_issue_request=issue_request;


    fops.open = animal_proc_open;
	fops.read = animal_proc_read;
	fops.release = animal_proc_release;

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
