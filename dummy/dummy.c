#include <linux/module.h>
#include <linux/kernel.h>


int __init anti_dump_init(void)
{
    int ret = 0;

    printk(KERN_INFO "dummy: init\n");

    return ret;
}

void anti_dump_exit(void)
{
    printk(KERN_INFO "dummy: exit\n");
    return;
}

module_init(anti_dump_init);
module_exit(anti_dump_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LITzman");