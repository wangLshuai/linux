#include <linux/init.h>
#include <linux/module.h>

static int __init test_init(void)
{
    printk("hello,test\n");
    return 0;
}

static void __exit test_exit(void)
{
    printk("%s exit\n",__func__);
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wang shuai");