#include "linux/wait.h"
#include "linux/rwlock.h"
#include <linux/types.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/stackprotector.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/initrd.h>
#include <linux/bootmem.h>
#include <linux/acpi.h>
#include <linux/tty.h>
#include <linux/percpu.h>
#include <linux/kmod.h>
#include <linux/vmalloc.h>
#include <linux/kernel_stat.h>
#include <linux/start_kernel.h>
#include <linux/security.h>
#include <linux/smp.h>
#include <linux/profile.h>
#include <linux/rcupdate.h>
#include <linux/moduleparam.h>
#include <linux/kallsyms.h>
#include <linux/writeback.h>
#include <linux/cpu.h>
#include <linux/cpuset.h>
#include <linux/cgroup.h>
#include <linux/efi.h>
#include <linux/tick.h>
#include <linux/interrupt.h>
#include <linux/taskstats_kern.h>
#include <linux/delayacct.h>
#include <linux/unistd.h>
#include <linux/rmap.h>
#include <linux/mempolicy.h>
#include <linux/key.h>
#include <linux/buffer_head.h>
#include <linux/page_cgroup.h>
#include <linux/debug_locks.h>
#include <linux/debugobjects.h>
#include <linux/lockdep.h>
#include <linux/kmemleak.h>
#include <linux/pid_namespace.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/idr.h>
#include <linux/kgdb.h>
#include <linux/ftrace.h>
#include <linux/async.h>
#include <linux/kmemcheck.h>
#include <linux/sfi.h>
#include <linux/shmem_fs.h>
#include <linux/slab.h>
#include <linux/perf_event.h>

#include <asm/io.h>
#include <asm/bugs.h>
#include <asm/setup.h>
#include <asm/sections.h>
#include <asm/cacheflush.h>

#ifdef CONFIG_X86_LOCAL_APIC
#include <asm/smp.h>
#endif



static DECLARE_WAIT_QUEUE_HEAD(myevent_waitqueue);
rwlock_t myevent_lock;
unsigned int myevent_id;
extern char myevent_handler[];

void wakeup_mythread(void)
{
    printk("+++wakeup mythread\n");
    wake_up_interruptible(&myevent_waitqueue);
}
EXPORT_SYMBOL(wakeup_mythread);

static void run_umode_handler(int event_id){
    int i = 0;
    char *argv[2], *envp[4], *buffer = NULL;
    int value;

    argv[i++] = myevent_handler;

    buffer = kmalloc(32, GFP_KERNEL);
    if(buffer == NULL)
        return;

    sprintf(buffer, "TROUBLED_DS=%d", event_id);

    if(!argv[0])
    {
        printk("NO myevent handler\n");
        return;
    }

    i = 0;
    envp[i++] = "HOME=/";
    envp[i++] = "PATH=/sbin:/vendor/bin:/system/sbin:/system/bin:/system/xbin:/system/busybox/bin:/system/busybox/sbin:/system/busybox/usr/bin:/system/busybox/usr/sbin";
    //envp[i++] = buffer;
    envp[i] = 0;

    printk("%s: call usermodehleper, handler=%s\n",__FUNCTION__,argv[0]);
    value = call_usermodehelper(argv[0], argv, envp, 0);

    kfree(buffer);
}

int mykthread(void *unused)
{
    unsigned int event_id = 0;
    DECLARE_WAITQUEUE(wait, current);

    daemonize("mykthead");

    allow_signal(SIGKILL);

    /*sleep on this wait queue until it's woken up by parts
    of the kernel in charge of sensing the health of the data
    struction of interest */
    add_wait_queue(&myevent_waitqueue, &wait);

    for(;;)
    {
       set_current_state(TASK_INTERRUPTIBLE);
       schedule();

       printk("+++%s has wakeup\n", __FUNCTION__);
       if(signal_pending(current)) break;

       read_lock(&myevent_lock);
       if(myevent_id){
           printk("+++%s: myevent_id=%u\n", __FUNCTION__, myevent_id);
           event_id = myevent_id;
           read_unlock(&myevent_lock);

           run_umode_handler(myevent_id);
       }
       else{
           read_unlock(&myevent_lock);
       }

    }//for
    set_current_state(TASK_RUNNING);
    remove_wait_queue(&myevent_waitqueue, &wait);
    return 0;
}

EXPORT_SYMBOL(mykthread);
