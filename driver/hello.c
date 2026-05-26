#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("LEON");

MODULE_DESCRIPTION("Hello universe!");


static int __init hello_init(void)
{
    pr_info("bme280_pi: hello from the module\n");
    return 0;
}

static void __exit hello_exit(void)
{
    pr_info("bme280_pi: goodbye\n");
}

module_init(hello_init);
module_exit(hello_exit);