/*Adapted from:
https://coherentmusings.wordpress.com/2014/06/10/implementing-mmap-for-transferring-data-from-user-space-to-kernel-space/
*/

#include <linux/uaccess.h> /* copy_from_user */
#include <asm/io.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h> /* min */
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>

int page_order = 10;

static const char *filename = "centrifuge_mmap";
unsigned long kaddr = 0;
char *buf;

static int mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long pfn;
    pfn = virt_to_phys((void*)kaddr) >> PAGE_SHIFT;

    if(remap_pfn_range(vma, vma->vm_start, pfn, (vma->vm_end - vma->vm_start),
                vma->vm_page_prot))
    {
        printk("remap failed...");
        return -1;
    }
    vma->vm_flags |= (VM_DONTDUMP|VM_DONTEXPAND);
	return 0;
}

static int release(struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operations fops = {
    .mmap = mmap,
    .release = release
};

static int myinit(void)
{
    int i;
    proc_create(filename, 0, NULL, &fops);

    for (i = 0; i < 16; i ++) {
	    unsigned long kaddr_1 = __get_free_pages(GFP_KERNEL, page_order);
        if ((kaddr_1 != kaddr - 0x400000) && (i != 0)) {
            printk("Error allocating centrifuge memory");
            dump_stack(); 
        }
        kaddr = kaddr_1;
    }

    if (!kaddr) {
        printk("Allocate memory failure!/n");
    } else {
        unsigned long MEMSZ = ((1 << page_order) * PAGE_SIZE);
        buf = (char *)kaddr;

        for(i = 0; i < MEMSZ; i += PAGE_SIZE) {
            sprintf(buf + i, "%d", i >> PAGE_SHIFT);
        }
    }
    printk("Loaded centrifuge_mmap module\n");
    return 0;
}

static void myexit(void)
{
    pr_info("centrifuge_mmap module exiting\n");
    free_pages(kaddr, page_order);
    remove_proc_entry(filename, NULL);
    return;
}

module_init(myinit)
module_exit(myexit)
MODULE_LICENSE("GPL");
