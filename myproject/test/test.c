#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mmzone.h>
#include <linux/page_ref.h>
#include <linux/mm.h>
#include <linux/slab_def.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/sched/signal.h>
#include <linux/nsproxy.h>
#include <linux/utsname.h>
#include <linux/pid_namespace.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#define DEV_NAME "test_dev"

struct test_slab {
	int i;
	long j;
};

#define TEST_IOC_MEM _IO('C', 0x01)
#define TEST_IOC_TASK _IO('C', 0x02)
#define TEST_IOC_KERNEL_THREAD _IO('C', 0x03)

static bool running = false;
int kernel_thread_fun(void *arg)
{
	printk("kernel thread start\n");
	printk("kernel thread task->mm:%lx task->active_mm:%lx\n",
	       (uintptr_t)current->mm, (uintptr_t)current->active_mm);
	while (running) {
		DECLARE_WAIT_QUEUE_HEAD(sleep);
		wait_event_interruptible_timeout(sleep, 0,
						 msecs_to_jiffies(1000));
		printk("kernel thread running\n");
	}

	printk("kernel thread stop\n");
	return 0;
}

static int test_open(struct inode *inode, struct file *file)
{
	int minor = MINOR(inode->i_rdev);
	int major = MAJOR(inode->i_rdev);
	printk("open dev %d:%d\n", major, minor);
	return 0;
}
static int test_release(struct inode *inode, struct file *file)
{
	return 0;
}
static ssize_t test_read(struct file *file, char __user *buf, size_t count,
			 loff_t *ppos)
{
	return 0;
}

static ssize_t test_write(struct file *file, const char __user *buf,
			  size_t count, loff_t *ppos)
{
	return 0;
}

static void foreach_pages(void)
{
	unsigned long i = 0, free = 0, locked = 0, reserved = 0, swapcache = 0,
		      referenced = 0, slab = 0, private = 0, uptodate = 0,
		      dirty = 0, active = 0, writeback = 0, mappedtodisk = 0;
	struct page *pa;
	unsigned long num_phypages;

	num_phypages = get_num_physpages();
	printk("foreach %ld's phy pages\n", num_phypages);
	for (i = 0; i < num_phypages; i++) {
		pa = pfn_to_page(i);
		if (!page_count(pa)) {
			free++;
			continue;
		}
		if (PageLocked(pa))
			locked++;
		if (PageReserved(pa))
			reserved++;
		if (PageSwapCache(pa))
			swapcache++;
		if (PageReferenced(pa))
			referenced++;
		if (PageSlab(pa))
			slab++;
		if (PagePrivate(pa))
		private++;
		if (PageUptodate(pa))
			uptodate++;
		if (PageDirty(pa))
			dirty++;
		if (PageActive(pa))
			active++;
		if (PageWriteback(pa))
			writeback++;
		if (PageMappedToDisk(pa))
			mappedtodisk++;
	}
	printk("memory size : %lu MB\n",
	       num_phypages * PAGE_SIZE / 1024 / 1024);
	printk("free:%lu\n", free);
	printk("locked:%lu\n", locked);
	printk("reserved:%lu\n", reserved);
	printk("swapcache:%lu\n", swapcache);
	printk("referenced:%lu\n", referenced);
	printk("slab:%lu\n", slab);
	printk("private:%lu\n", private);
	printk("uptodata:%lu\n", uptodate);
	printk("dirty:%lu\n", dirty);
	printk("active:%lu\n", active);
	printk("writeback:%lu\n", writeback);
	printk("mappedtodisk:%lu\n", mappedtodisk);
}

static void test_memory(void)
{
	int i;
	void *p;
	void *vp0, *vp1;
	struct page *a_page;
	uintptr_t addr;
	struct kmem_cache *slab;
	printk("hello,test\n");
	p = kmalloc(10, GFP_KERNEL);
	printk("kmalloc p addr:     %016lx\n", (uintptr_t)p);
	printk("__START_KERNEL_map: %016lx\addr\n", phys_base);
	printk("phy addr of p:      %016lx\n", __pa(p));
	kfree(p);

	vp0 = vmalloc(PAGE_SIZE);
	printk("vmalloc vp0 value:    %016lx\n", (uintptr_t)vp0);
	printk("vp0 is_vmmalloc_addr:%d\n", is_vmalloc_addr(vp0));
	struct page *vp0_page = vmalloc_to_page(vp0);
	printk("sizeof(struct page):%ld\n", sizeof(struct page));
	printk("vp0_page:             %016lx\n", (uintptr_t)vp0_page);
	// printk("mem_map:            %016lx\n",(uintptr_t)mem_map);
	uintptr_t phy_page_addr0 = page_to_phys(vp0_page);
	printk("phy page addr0:        %016lx\n", phy_page_addr0);
	printk("vp0 offset in page:   %016lx\n", offset_in_page(vp0));

	vp1 = vmalloc(PAGE_SIZE);
	printk("vmalloc vp1 value:    %016lx\n", (uintptr_t)vp1);
	struct page *vp1_page = vmalloc_to_page(vp1);
	printk("vp1_page:             %016lx\n", (uintptr_t)vp1_page);
	uintptr_t phy_page_addr1 = page_to_phys(vp1_page);
	printk("phy page addr:        %016lx\n", phy_page_addr1);

	printk("page array start:%016lx\n", (uintptr_t)vmemmap);
	long int index_delte = abs(vp1_page - vp0_page);
	printk("index_delte:%ld", index_delte);

	printk("%ld\n", abs((phy_page_addr0 >> PAGE_SHIFT) -
			    (phy_page_addr1 >> PAGE_SHIFT)));

	vfree(vp0);
	vfree(vp1);

	// BUG_ON(1);
	uint64_t cr3 = cr3 = __read_cr3();
	uint64_t *pgd = __va(cr3);
	printk("%016lx\n", (uintptr_t)pgd);
	printk("*pgd:%016llx\n", *pgd);
	printk("CR3:    %016llx\n", cr3);
	printk("%016lx\n", vp1_page->flags);
	printk("vp1_page->_mapcount.counter: %d,vp1_page->_refcount.counter: %d\n",
	       page_mapcount(vp1_page), page_count(vp1_page));

	a_page = alloc_page(GFP_KERNEL & ~__GFP_HIGHMEM);
	if (a_page != NULL) {
		printk("a_page value:%016lx\n", (uintptr_t)a_page);
		char *l_addr = page_address(a_page);
		for (i = 0; i < PAGE_SIZE; i++)
			l_addr[i] = 0x55;
	}
	__free_page(a_page);

	uint64_t size;
	char *buf;
	for (size = PAGE_SIZE, i = 0; i < MAX_ORDER; i++, size *= 2) {
		printk("try tokmalloc %llu bytes\n", size);
		buf = kmalloc(size, GFP_ATOMIC);
		if (!buf) {
			pr_err("kmalloc failed\n");
			break;
		}
		kfree(buf);
	}

	addr = __get_free_pages(GFP_KERNEL, 10);
	if (addr) {
		printk("__get_free_pages return a virtual address from direct mapped zone:%016lx\n",
		       addr);
		*(char *)addr = 0xff;
	}
	if (addr)
		printk("*addr: %x", *(char *)addr);
	free_pages(addr, 10);

	foreach_pages();
	printk("-------------------------------------slab-------------------------------------\n");
	int cpu_nums = num_online_cpus();
	printk("cpu nums:%d\n", cpu_nums);
	slab = kmem_cache_create("test_slab", sizeof(struct test_slab),
				 SLAB_HWCACHE_ALIGN, 0, NULL);

	printk("kmeme_cache::cpu_slab is per cpu variable");
	printk("slab->size:%u,slab->limit:%u,slab->batchcount:%u,slab->num:%u,slab->align:%u,slab->object_size:%u\n",
	       slab->size, slab->limit, slab->batchcount, slab->num,
	       slab->align, slab->object_size);
	struct test_slab *test_s = kmem_cache_alloc(slab, GFP_KERNEL);
	test_s->i = 10;
	test_s->j = 100;
	printk("slab->size:%u,slab->limit:%u,slab->batchcount:%u,slab->num:%u,slab->align:%u,slab->object_size:%u\n",
	       slab->size, slab->limit, slab->batchcount, slab->num,
	       slab->align, slab->object_size);
	kmem_cache_free(slab, test_s);
	kmem_cache_destroy(slab);
}

static void test_task(void)
{
	int i;
	struct task_struct *t = current;
	struct pid *pid;
	struct pid_namespace *pidns;
	printk("task->pid:%d,this global\n", t->pid);
	printk("sizeof(pid):%lu\n", sizeof(struct pid));
	pid = task_session(t);
	printk("local ns task session id:%d\n", pid->numbers[pid->level].nr);
	pid = task_pgrp(t);
	printk("local ns task process group id:%d",
	       pid->numbers[pid->level].nr);
	printk("task nsproxy->pid_ns_for_children->level:%d",
	       t->nsproxy->pid_ns_for_children->level);
	int level = t->nsproxy->pid_ns_for_children->level;
	for (i = 0; i <= level; i++) {
		printk("this task pid :%d in pid namespace %d\n",
		       t->thread_pid->numbers[i].nr, i);
	}

	pidns = t->nsproxy->pid_ns_for_children;
	for (i = level; i >= 0; i--) {
		printk("1 process:%s in %d namespace",
		       find_task_by_pid_ns(1, pidns)->comm, i);
		pidns = pidns->parent;
	}

	printk("task->comm:%s\n", t->comm);
	printk("task->prio:%d\n", t->prio);
	printk("task->static_prio:%d\n", t->static_prio);
	printk("task->normal_prio:%d\n", t->normal_prio);
	printk("task->rt_priority:%d\n", t->rt_priority);
	printk("MAX_USER_RT_RPIO:%d MAX_RT_PRIO:%d,MAX_PRIO:%d", MAX_USER_PRIO,
	       MAX_RT_PRIO, MAX_PRIO);
	printk("task->policy:%d\n", t->policy);
	printk("task->cpus_allowed:%d\n", t->nr_cpus_allowed);
	printk("task->state:%ld\n", t->state);
	printk("task->flags:%x\n", t->flags);
	printk("task->real_start_time:%llu\n", t->real_start_time);
	printk("t->signal->rlim[RLIMIT_CORE].rlim_cur:%lu,t->signal->rlim[RLIMIT_CORE].rlim_max:%lu\n",
	       t->signal->rlim[RLIMIT_CORE].rlim_cur,
	       t->signal->rlim[RLIMIT_CORE].rlim_max); //ulimit -c 409600
	printk("t->signal->rlim[RLIMIT_NOFILE].rlim_cur:%lu,t->signal->rlim[RLIMIT_NOFILE].rlim_max:%lu\n",
	       t->signal->rlim[RLIMIT_NOFILE].rlim_cur,
	       t->signal->rlim[RLIMIT_NOFILE].rlim_max); //ulimit -c 409600

	printk("t->nsproxy->uts_ns->name. sysname:%s version:%s machine:%s\n",
	       t->nsproxy->uts_ns->name.sysname,
	       t->nsproxy->uts_ns->name.version,
	       t->nsproxy->uts_ns->name.machine);
	printk("sizeof(union thread_info):%lu,sizeof(struct task_struct):%lu,t->stack:%lx t:%lx\n",
	       sizeof(union thread_union), sizeof(struct task_struct),
	       (uintptr_t)t->stack, (uintptr_t)t);
	printk("user->processes:%d\n",
	       atomic_read(&t->real_cred->user->processes));
	printk("user thread task->mm:%lx task->active_mm:%lx\n",
	       (uintptr_t)current->mm, (uintptr_t)current->active_mm);
	set_current_state(TASK_RUNNING);

	printk("test_tsk_need_resched(t):%d\n", test_tsk_need_resched(t));
	printk("sched entity exec_start:%llu sum_exec_runtime:%llu,vruntime:%llu\n",
	       t->se.exec_start, t->se.sum_exec_runtime, t->se.vruntime);

}
long test_ioctl(struct file *file, unsigned int ioc, unsigned long args)
{
	printk("%s\n", __func__);
	switch (ioc) {
	case TEST_IOC_MEM:
		printk("ioctl mem\n");
		test_memory();
		break;
	case TEST_IOC_TASK:
		printk("ioctl task\n");
		test_task();
		break;
	case TEST_IOC_KERNEL_THREAD:
		if (args && !running) {
			running = true;
			kthread_run(kernel_thread_fun, 0, "test");
			printk("new kernel thread\n");
		}
		if (!args) {
			running = false;
		}
		break;
	}
	return 0;
}

static struct file_operations fops = {
	.open = test_open,
	.release = test_release,
	.read = test_read,
	.write = test_write,
	.unlocked_ioctl = test_ioctl,
};
static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEV_NAME,
	.fops = &fops,
};

static int __init test_init(void)
{
	if (misc_register(&misc)) {
		pr_err("misc register failed");
	}

	return 0;
}

static void __exit test_exit(void)
{
	misc_deregister(&misc);
	printk("%s exit\n", __func__);
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wang shuai");