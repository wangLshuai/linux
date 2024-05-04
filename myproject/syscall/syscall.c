#include <linux/wait.h>
#include <linux/syscalls.h>
DECLARE_WAIT_QUEUE_HEAD(sleep);
SYSCALL_DEFINE0(mysyscall)
{
	printk("%s:current pid is:%d\n", __func__, current->pid);
#ifdef CONFIG_MYSYSCALL_PRINK
	printk("CONFIG_MYSYSCALL_PRINK\n");
#endif
	uint64_t cr3 = cr3 = __read_cr3();
	printk("CR3:    %016llx\n", cr3);
	struct zone *z;
	int i = 0;
	for_each_zone (z) {
		i++;
		printk("zone name:%s\n", z->name);
	}

	printk("%d of zone\n", i);
	printk("zone->freearea size:%d\n", MAX_ORDER);
	printk("z->free_area[0].free_list[MIGRATE_TYPES],MIGRATE_TYPES:%d\n",
	       MIGRATE_TYPES);
	struct task_struct *t = current;
	printk("sizeof(struct task_struct):%ld\n", sizeof(struct task_struct));
	printk("mmap_base:%016lx\n", t->mm->mmap_base);
	printk("pgd:%016llx, __pa(gpd):%016lx\n", (uint64_t)t->mm->pgd,
	       __pa((uint64_t)t->mm->pgd));
	printk("mm->start_brk:%016lx,mm->brk:%016lx\n", t->mm->start_brk,
	       t->mm->brk);
	printk("mm->start_code:%016lx,mm->start_data:%016lx,mm->start_stack:%016lx\n",
	       t->mm->start_code, t->mm->start_data, t->mm->start_stack);
	printk("foreach vm area\n");
	struct vm_area_struct *vm;
	vm = t->mm->mmap;
	i = 0;
	while (vm) {
		printk(" vm %d :vm->start:%016lx, vm->end:%016lx\n", i,
		       vm->vm_start, vm->vm_end);
		i++;
		vm = vm->vm_next;
	}
	wait_event_interruptible_timeout(sleep, 0, msecs_to_jiffies(5000));
	return current->pid;
}
