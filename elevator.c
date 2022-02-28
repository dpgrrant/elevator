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

#define ENTRY_NAME "animal_list"
#define PERMS 0644
#define PARENT NULL
static struct file_operations fops;

struct thread_parameter{
    struct list_head list;
    struct mutex my_mutex;
    struct task_struct *kthread;
    int id;
    int c_state;
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
}Pets;