#include <linux/syscalls.h>

SYSCALL_DEFINE0(mysyscall)
{
	printk("%s:current pid is:%d\n",__func__,current->pid);
	#ifdef CONFIG_MYSYSCALL_PRINK
	printk("CONFIG_MYSYSCALL_PRINK\n");
	#endif
	return current->pid;
}
