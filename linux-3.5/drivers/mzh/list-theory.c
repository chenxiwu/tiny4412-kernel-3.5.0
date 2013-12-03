#include "linux/wait.h"
#include "linux/rwlock.h"
#include <linux/types.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/list.h>


static int mydrv_worker(void* unused);

static struct _mydrv_wq{
    struct list_head mydrv_worklist;
    spinlock_t lock;
    wait_queue_head_t todo;
}mydrv_wq;

typedef struct _mydrv_work{
    struct list_head mydrv_workitem;
    void (*worker_func)(void*);
    void *worker_data;
}WORK;

static int __init mydrv_init(void)
{

    //spin lock init
    spin_lock_init(&mydrv_wq.lock);

    init_waitqueue_head(&mydrv_wq.todo);

    INIT_LIST_HEAD(&mydrv_wq.mydrv_worklist);

    kernel_thread(mydrv_worker, NULL, CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD);

    return 0;
}

int submit_work(void (*func)(void*data), void* data)
{
    WORK *mydrv_work;

    //alloc work struct 
    mydrv_work = kmalloc(sizeof(struct _mydrv_work));
    if(mydrv_work == NULL){
        printk("zmalloc failed\n");
        return -1;
    }

    //populate the work struct
    mydrv_work->worker_func = func;
    mydrv_wok->worker_data = data;
    
    //protect list
    spin_lock(&mydrv_wq.lock);
    list_add_tail(&mydrv_work->mydrv_workitem, 
            &mydrv_wq.mydrv_worklist);

    //wake up workqueue
    wake_up(&mydrv_wq.todo);

    spin_unlock(&mydrv_wq.lock);
    
    return 0;
}

static int mydrv_worker(void* unused){

    DECLARE_WAITQUEUE(wait, current);

    void* (worker_fucn)(void*);
    void* worker_data;
    WORK *mydrv_work;

    set_current_state(TASK_INTERRUPTIBLE);

    while(1){
        add_wait_queue(&mydrv_wq.todo, &wait);

        if(list_empty(&mydrv_wq.mydrv_worklist)){
            //if nothing todo
            schedlue();
        }
        else
            set_current_state(TASK_RUNNING);

        //remove itself frome waitqueue
        remove_wait_queue(&mydrv_wq.todo, &wait);

        spin_lock(&mydrv_wq.lock);

        while(!list_empty(&mydrv_wq.mydrv_worklist)){

            //get a workitem
            mydrv_work = list_entry(&mydrv.mydrv_worklist.next, 
                    struct _mydrv_work, mydrv_workitem);
            worker_func = mydrv_work->worker_func;
            worker_data = mydrv-work->worker_data;

            //this node has been processed, throw it
            list_del(&mydev.mydrv_worklist.next);
            free(mydrv_work);


            spin_unlock(&mydrv_wq.lock);

            //execute the work function in this node
            worker_func(worker_data);

            spin_lock(&mydrv_wq.lock);
        }
        spin_unlock(&mydrv_wq.lock);
        set_current_state(TASK_INTERRUPTIBLE);
    }

    set_current_state(TASK_RUNNING);
    return 0;
}
