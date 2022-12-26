#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/elf.h>

int module_block_init(void)
{
    printk(KERN_INFO "anti_dump: Module Blocked\n");

    // Throw an arbitrary error
    return -ENOENT;
}

int module_handler(struct notifier_block *nb, unsigned long action, void *data)
{
    struct module *mod;
    struct mod_kallsyms *kallsyms;
    int symbol_index;
    char *symbol_name;
    unsigned long flags;
    
    DEFINE_SPINLOCK(modules_spinlock);

    mod = data;

    spin_lock_irqsave(&modules_spinlock, flags);
    
    // If a new module is being loaded
    if (mod->state == MODULE_STATE_COMING)
    {
        // Iterate it's symbols
        kallsyms = mod->kallsyms;
        for (symbol_index = 0; symbol_index <= kallsyms->num_symtab + 1; symbol_index++)
        {
            // /kernel/module/kallsyms.c:kallsyms_symbol_name
            symbol_name = kallsyms->strtab + kallsyms->symtab[symbol_index].st_name;

            // Look for a sus symbol
            if (!strcmp(symbol_name, "iomem_resource"))
            {
                // Block module -  replace it's init with our dummy init
                mod->init = module_block_init;
                break;
            }
        }
    }

    spin_unlock_irqrestore(&modules_spinlock, flags);
    return 0;
}

struct notifier_block modules_notifier = {
    .notifier_call = module_handler,
    .priority = INT_MAX
};

int  module_blocker_init(void)
{
    int ret;
    ret = register_module_notifier(&modules_notifier);
    if(ret)
    {
        printk(KERN_INFO "anti_dump: notifier register failed\n");
    }
    return ret;
}

void module_blocker_exit(void)
{
    // Stop the modules callback
    unregister_module_notifier(&modules_notifier);
    return;
}