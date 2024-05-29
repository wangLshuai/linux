#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/rcupdate.h>
#include <linux/delay.h>

#define COUNTER 100000
static uint64_t num = 0;
static atomic_t num_unlock;
static uint64_t num_spin = 0;
static DEFINE_SPINLOCK(spinlock);
static uint64_t num_semphe = 0;
static DEFINE_SEMAPHORE(sem);
static uint64_t num_mutext = 0;
static DEFINE_MUTEX(m);
static bool read_running, write_running;

struct student {
	char *name;
	int old;
	char id[32];
	struct rcu_head rcu;
};

struct student *sp;
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
	return 0;
}

int read_thread(void *argc)
{
	struct student *p;
	while (read_running) {
		rcu_read_lock();
		p = rcu_dereference(sp);
		if (p != sp) {
			printk("p:%llu,sp:%llu\n", (uint64_t)p, (uint64_t)sp);
			printk("p name:%s old:%d \n", p->name, p->old);
		}
		if (!strncmp(p->name, "alice", 128)) {
			if (p->old != 14) {
				printk("error,name:%s old:%d\n", p->name,
				       p->old);
			}
		} else if (!strncmp(p->name, "blob", 128)) {
			if (p->old != 15) {
				printk("error,name:%s old:%d\n", p->name,
				       p->old);
			}
		} else {
			printk("error,name:%s old:%d\n", p->name, p->old);
		}
		rcu_read_unlock();
	}

	return 0;
}

static void myrcu_del(struct rcu_head *rh)
{
	struct student *s;
	s = container_of(rh, struct student, rcu);
	kfree(s);
}
int write_thread(void *argv)
{
	int i;
	struct student *new_p, *old_p;
	i = 2000;
	while (i-- > 0 && write_running) {
		msleep(1);
		new_p = kmalloc(sizeof(struct student), GFP_KERNEL);
		old_p = sp;
		*new_p = *old_p;

		if (i % 2) {
			new_p->name = "blob";
			new_p->id[0] = 3;
			new_p->id[1] = 2;
			new_p->old = 15;

		} else {
			new_p->name = "alice";
			new_p->id[0] = 3;
			new_p->id[1] = 2;
			new_p->old = 14;
		}

		rcu_assign_pointer(sp, new_p);
		printk("sp:%llu\n", (uint64_t)sp);
		call_rcu(&old_p->rcu, myrcu_del);
	}
	read_running = false;
	return 0;
}
static int __init sync_lock_init(void)
{
	printk("sync_lock init\n");
	sp = kmalloc(sizeof(struct student), GFP_KERNEL);
	sp->name = "alice";
	sp->id[0] = 0;
	sp->id[1] = 1;
	sp->old = 14;
	kthread_run(threadfn, NULL, "test thread1");
	kthread_run(threadfn, NULL, "test thread2");
	read_running = true;
	write_running = true;
	kthread_run(read_thread, NULL, "read thread1");
	kthread_run(read_thread, NULL, "read thread2");
	kthread_run(write_thread, NULL, "write thread");

	return 0;
}

static void __exit sync_lock_exit(void)
{
	read_running = write_running = false;
	msleep(1000);
	call_rcu(&sp->rcu, myrcu_del);
	printk("exit sync_lock\n");
}

module_init(sync_lock_init);
module_exit(sync_lock_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("wangshuai");
MODULE_DESCRIPTION("sync and lock test");