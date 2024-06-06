#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/timer.h>

struct tasklet_struct t;
struct work_struct w;
static void time_fun(struct timer_list *t)
{
	// BUG_ON(in_interrupt());
	int count = softirq_count() >> SOFTIRQ_SHIFT;
	mylog("softirq count:%d\n", count);
	mylog("comm:%s pid:%d\n", current->comm, current->pid);
	mod_timer(t, jiffies + msecs_to_jiffies(1000));
}
DEFINE_TIMER(my_timer, time_fun);
static void tasklet_fun(unsigned long data)
{
	int count = softirq_count() >> SOFTIRQ_SHIFT;
	mylog("soft irq count:%d\n", count);
}

static void workqueue_fun(struct work_struct *data)
{
	BUG_ON(in_interrupt());
	mylog("comm:%s pid:%d\n", current->comm, current->pid);
}
static int __init interrupt_test_init(void)
{
	printk("simple char that support interrupt_test init \n");
	printk("NR_IRQS:%d\n", NR_IRQS);
	tasklet_init(&t, tasklet_fun, 0);
	tasklet_schedule(&t);
	INIT_WORK(&w, workqueue_fun);
	schedule_work(&w);
	my_timer.expires = jiffies + msecs_to_jiffies(1000);
	add_timer(&my_timer);

	return 0;
}

static void __exit interrupt_test_exit(void)
{
	del_timer(&my_timer);
}

module_init(interrupt_test_init);
module_exit(interrupt_test_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("interrupt_test io");