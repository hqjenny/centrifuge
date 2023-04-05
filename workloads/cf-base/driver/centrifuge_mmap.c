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

// Size of contiguous buffer to allocate (in bytes). Must be a multiple of the page size.
#define reserved_size (64*1024*1024)

//Maximum number of pages to allocate in a single call
#define page_order 10
//Number of allocations to make
#define nalloc (reserved_size / ((1 << page_order)*PAGE_SIZE))

static const char *alloc_fname = "centrifuge_mmap";

unsigned long kaddr = 0;
char *buf;

/*=============================================================================
 * Allocator
 * mmap'ing this file returns a physically contiguous region of memory. The
 * file location is not considered, only the size of the allocations. Multiple
 * mmaps of the same file region will return different physical addresses.
 *=============================================================================
 */
uintptr_t last_paddr = 0;

static int alloc_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long paddr = virt_to_phys((void*)kaddr);
    unsigned long pfn = paddr >> PAGE_SHIFT;

    //This will get more interesting when we do a proper allocator
    last_paddr = paddr;

    if(remap_pfn_range(vma, vma->vm_start, pfn, (vma->vm_end - vma->vm_start),
                vma->vm_page_prot))
    {
        printk("remap failed...");
        return -1;
    }
    
    vma->vm_flags |= (VM_DONTDUMP|VM_DONTEXPAND|VM_LOCKED);
	return 0;
}

static int alloc_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operations alloc_fops = {
    .mmap = alloc_mmap,
    .release = alloc_release
};

/*=============================================================================
 * Translator 
 * Reading the first 8B of this file returns the starting physical address of
 * the last allocated buffer.
 *=============================================================================
 */
static const char *vtop_fname = "centrifuge_vtop";

static ssize_t vtop_read(struct file *file, char __user *user_buffer,
                   size_t size, loff_t *offset)
{
    if(size < sizeof(uintptr_t) || *offset != 0) {
        printk("Centrifuge driver: invalid vtop read: size=%lu,off=%lld \n", size, *offset);
        return -EFAULT;
    }

    /* if(put_user(last_paddr, user_buffer) != 0) { */
    if(copy_to_user(user_buffer, &last_paddr, sizeof(last_paddr)) != 0) {
        printk("Centrifuge driver: failed to copy to user\n");
        return -EFAULT;
    }

    return size;
}

static const struct file_operations vtop_fops = {
    .read = vtop_read
};

/*=============================================================================
 * General centrifuge module 
 *=============================================================================
 */
static int cfmod_init(void)
{
    int i;
    if(proc_create(alloc_fname, 0, NULL, &alloc_fops) == NULL) {
        printk("centrifuge: failed to allocate proc file\n");
        return -ENOMEM;
    }
    if(proc_create(vtop_fname, 0, NULL, &vtop_fops) == NULL) {
        printk("centrifuge: failed to allocate proc file\n");
        return -ENOMEM;
    }

    for(i = 0; i < nalloc; i ++) {
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

static void cfmod_exit(void)
{
    pr_info("centrifuge_mmap module exiting\n");
    free_pages(kaddr, page_order);
    remove_proc_entry(alloc_fname, NULL);
    return;
}

module_init(cfmod_init)
module_exit(cfmod_exit)
MODULE_LICENSE("GPL");
