#include <linux/module.h>
#include <linux/kernel.h>
#include "anti_dump.h"

#ifdef AD_HIDE_RESOURCES
extern void resource_hider_init(void);
extern void resource_hider_exit(void);
#endif

#ifdef AD_BLOCK_MODULES
extern int  module_blocker_init(void);
extern void module_blocker_exit(void);
#endif

int __init anti_dump_init(void)
{
    int ret = 0;
    #ifdef AD_HIDE_RESOURCES
    resource_hider_init();
    #endif 

    #ifdef AD_BLOCK_MODULES
    ret =  module_blocker_init();
    #endif

    return ret;
}

void anti_dump_exit(void)
{
    #ifdef AD_HIDE_RESOURCES
    resource_hider_exit();
    #endif

    #ifdef AD_BLOCK_MODULES
    module_blocker_exit();
    #endif
}

module_init(anti_dump_init);
module_exit(anti_dump_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LITzman");