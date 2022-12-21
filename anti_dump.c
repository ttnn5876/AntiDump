/*
CONFIGS:
AD_CONFIG_HIDE_MEMORY_FILES
AD_CONFIG_BLOCK_MODULES
AD_CONFIG_BLOCK_RESOURCE_MEM
*/
#include "anti_dump.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include<linux/slab.h>

#ifdef AD_CONFIG_BLOCK_MODULES
#include <linux/notifier.h>
int module_block_init(void)
{
    printk(KERN_INFO "anti_dump: Module Blocked\n");
    return -ENOENT;
}

int module_handler(struct notifier_block *nb, unsigned long action, void *data)
{
    struct module *mod;
    unsigned long flags;
    DEFINE_SPINLOCK(modules_spinlock);

    mod = data;

    spin_lock_irqsave(&modules_spinlock, flags);

    if (mod->state == MODULE_STATE_COMING)
    {
        mod->init = module_block_init;
        printk(KERN_INFO "anti_dump: Module %s Loaded\n", mod->name);
    }
    spin_unlock_irqrestore(&modules_spinlock, flags);

    return 0;
}

struct notifier_block modules_notifier = {
    .notifier_call = module_handler,
    .priority = INT_MAX
};
#endif

#ifdef AD_CONFIG_BLOCK_RESOURCE_MEM
#include <linux/ioport.h>
#include <asm/unistd.h>

struct changed_resources {
    struct list_head list;
    struct resource *changed_resource;
};
struct changed_resources resources_list;

struct resource *iomem = &iomem_resource;

int set_page_write(unsigned long addr)
{
    unsigned int level;
    pte_t *pte = lookup_address(addr, &level);
    if (pte->pte & ~_PAGE_RW)
    {
        pte->pte |= _PAGE_RW;
        printk(KERN_INFO "anti_dump: modify the page frame to write");
        return 1;
    }
    return 0;
}

void set_page_no_write(unsigned long addr)
{
    unsigned int level;
    pte_t *pte = lookup_address(addr, &level);
    pte->pte = pte->pte & ~_PAGE_RW;
    printk(KERN_INFO "anti_dump: modify the page frame to read only");

}

#endif

int __init anti_dump_init(void)
{
    int ret = 0;

    printk(KERN_INFO "anti_dump: init\n");
    
    #ifdef AD_CONFIG_BLOCK_MODULES
    ret = register_module_notifier(&modules_notifier);
    if(ret)
    {
        printk(KERN_INFO "anti_dump: notifier register failed\n");
        return -1;
    }
    #endif

    #ifdef AD_CONFIG_BLOCK_RESOURCE_MEM

    // Manage list of modified resources
    INIT_LIST_HEAD(&resources_list.list);
    struct changed_resources *entry;
    for (iomem = iomem->child; iomem != NULL; iomem = iomem->sibling)
    {
        if (!strcmp(iomem->name, "System RAM"))
        {   
            // Append resource to string
            entry = (struct changed_resources *)kmalloc(sizeof(struct changed_resources), GFP_KERNEL);
            entry->changed_resource = iomem;
            list_add(&(entry->list), &(resources_list.list));

            // Change resource visible name
            set_page_write((unsigned long)iomem->name);
            strcpy((char *)iomem->name, "Video ROM "); // Very benign string here
            set_page_no_write((unsigned long)iomem->name);
            printk(KERN_INFO "anti_dump: overridden System RAM\n");
            break;
        }
    }
    #endif

    return ret;
}

void anti_dump_exit(void)
{
    printk(KERN_INFO "anti_dump: exit\n");

    #ifdef AD_CONFIG_BLOCK_MODULES
    unregister_module_notifier(&modules_notifier);
    #endif

    #ifdef AD_CONFIG_BLOCK_RESOURCE_MEM
    for (iomem = iomem->child; iomem != NULL; iomem = iomem->sibling)
    {
        if (!strcmp(iomem->name, "Video ROM "))
        {
            set_page_write((unsigned long)iomem->name);
            strcpy((char *)iomem->name, "System RAM");
            set_page_no_write((unsigned long)iomem->name);
            printk(KERN_INFO "anti_dump: restored System RAM\n");
            break;
        }
    }

    struct list_head *current_entry;
    struct resource *current_resource;

    list_for_each(current_entry, &resources_list.list)
    {
        current_resource = list_entry(current_entry, struct changed_resources, list)->changed_resource;

        set_page_write((unsigned long)current_resource->name);
        strcpy((char *)current_resource->name, "System RAM");
        set_page_no_write((unsigned long)current_resource->name);
        printk(KERN_INFO "anti_dump: restored System RAM\n");
        break;
    }

    #endif

    return;
}

module_init(anti_dump_init);
module_exit(anti_dump_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LITzman");