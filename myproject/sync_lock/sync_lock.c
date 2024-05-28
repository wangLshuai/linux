#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>

#define COUNTER 100000
static uint64_t num = 0;
static atomic_t num_unlock;
static uint64_t num_spin = 0;
static DEFINE_SPINLOCK(spinlock);
static uint64_t num_semphe = 0;
static DEFINE_SEMAPHORE(sem);
static uint64_t num_mutext = 0;
static DEFINE_MUTEX(m);

int threadfn(void *argc)
{
	unsigned long flags;
	uint64_t i;
	for (i = 0; i < COUNTER; i++) {
		num++;
		atomic_inc(&num_unlock);

		spin_lock_irqsave(&spinlock, flags);
		num_spin++;
		spin_unlock_irqrestore(&spinlock, flags);

		mutex_lock(&m);
		num_mutext++;
		mutex_unlock(&m);

		down(&sem);
		num_semphe++;
		up(&sem);
	}
	printk("num:%llu\n", num);
	printk("num_unlock:%d\n", atomic_read(&num_unlock));
	printk("num_spin:%llu\n", num_spin);
	printk("num_mutex:%llu\n", num_mutext);
	printk("num_semphe:%llu\n", num_semphe);
}
static int __init sync_lock_init(void)
{
	printk("sync_lock init\n");
	kthread_run(threadfn, NULL, "test thread1", NULL);
	kthread_run(threadfn, NULL, "test thread2", NULL);

	return 0;
}

static void __exit sync_lock_exit(void)
{
	printk("exit sync_lock\n");
}

module_init(sync_lock_init);
module_exit(sync_lock_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("wangshuai");
MODULE_DESCRIPTION("sync and lock test");