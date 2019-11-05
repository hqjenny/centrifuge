/*
 * Remember: mmap, like most fops, does not work with debugfs as of 4.9! https://patchwork.kernel.org/patch/9252557/
 * Adapted from:
 * https://coherentmusings.wordpress.com/2014/06/10/implementing-mmap-for-transferring-data-from-user-space-to-kernel-space/
 * */

#include <asm/uaccess.h> /* copy_from_user */
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h> /* min */
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/backing-dev.h>
#include <linux/mm.h>
#include <linux/vmacache.h>
#include <linux/shm.h>
#include <linux/mman.h>
#include <linux/pagemap.h>
#include <linux/swap.h>
#include <linux/syscalls.h>
#include <linux/capability.h>
#include <linux/init.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/personality.h>
#include <linux/security.h>
#include <linux/hugetlb.h>
#include <linux/shmem_fs.h>
#include <linux/profile.h>
#include <linux/export.h>
#include <linux/mount.h>
#include <linux/mempolicy.h>
#include <linux/rmap.h>
#include <linux/mmu_notifier.h>
#include <linux/mmdebug.h>
#include <linux/perf_event.h>
#include <linux/audit.h>
#include <linux/khugepaged.h>
#include <linux/uprobes.h>
#include <linux/rbtree_augmented.h>
#include <linux/notifier.h>
#include <linux/memory.h>
#include <linux/printk.h>
#include <linux/userfaultfd_k.h>
#include <linux/moduleparam.h>
#include <linux/pkeys.h>
#include <linux/oom.h>

#include <linux/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/tlb.h>
#include <asm/mmu_context.h>

static const char *filename = "custom_mmap";

static struct mmap_info *info;

enum { BUFFER_SIZE = 4 , ORDER=8};

struct mmap_info {
    char *data;
};

/* After unmap. */
static void vm_close(struct vm_area_struct *vma)
{
    pr_info("vm_close\n");
}

/* First page access. */
//static int vm_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
static int vm_fault(struct vm_fault *vmf)
{
    struct vm_area_struct *vma = vmf->vma;
    unsigned long vm_start = vma->vm_start;
    unsigned long vm_end = vma->vm_end;
    unsigned long vmf_addr = vmf->address;
    pr_info("vm_start %llx, vm_end %llx, vmf_addr %llx.\n", vm_start, vm_end, vmf_addr);
    // Create linear mapping 
    unsigned long offset = vmf_addr - vm_start; 

    struct page *page;
    struct mmap_info *info;

    pr_info("vm_fault\n");
    info = (struct mmap_info *)vma->vm_private_data;
    // If the base logical addr of kernel buffer exists 
    if (info->data) {
        page = virt_to_page((info->data) + offset);
      
        pr_info("phy %llx virt %llx\n", page_to_phys(page), (info->data) + offset);
        get_page(page);
        vmf->page = page;
        int ret = remap_pfn_range(vma, vmf_addr, page_to_pfn(page), PAGE_SIZE, vma->vm_page_prot);
        if (ret)
            return ret;
  
    }
    return 0;
}

/* Aftr mmap. TODO vs mmap, when can this happen at a different time than mmap? */
static void vm_open(struct vm_area_struct *vma)
{
    pr_info("vm_open\n");
}

static struct vm_operations_struct vm_ops =
{
    .close = vm_close,
    .fault = vm_fault,
    .open = vm_open,
};

static int mmap(struct file *filp, struct vm_area_struct *vma)
{
    pr_info("mmap\n");
    vma->vm_ops = &vm_ops;
    vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
    vma->vm_private_data = filp->private_data;
    vm_open(vma);
    return 0;
}

static int open(struct inode *inode, struct file *filp)
{
 //   struct mmap_info *info;

    pr_info("open\n");
 //   info = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);
 //   pr_info("virt_to_phys = 0x%llx\n", (unsigned long long)virt_to_phys((void *)info));
 //   //info->data = (char *)get_zeroed_page(GFP_KERNEL);

 //   unsigned long order = ORDER; // TODO add module args 
 //   // data maps to logical address of the buffer
 //   info->data = (char* )__get_free_pages(GFP_KERNEL, order);
 //   if (!info->data) {
 //           pr_info("Fail to allocate free pages!\n");
 //           /* insufficient memory: you must handle this error! */
 //           return -ENOMEM;
 //   }
 //   pr_info("kernel logical addr 0x%llx\n", (unsigned long)(info->data));

 //   // By opening the file, a new vma struct is created
    memcpy(info->data, "asdf", BUFFER_SIZE);
    filp->private_data = info;
    return 0;
}

static ssize_t read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
    struct mmap_info *info;
    int ret;

    // off is the file offset dont use it!
    //pr_info("read offset %ld\n", off);
    unsigned long offset = 4500;
    pr_info("read offset %ld\n", offset);
    info = filp->private_data;
    ret = min(len, (size_t)BUFFER_SIZE);
    //if (copy_to_user(buf, (unsigned long)(info->data) + (unsigned long)off, ret)) {
    if (copy_to_user(buf, (unsigned long)(info->data) + (unsigned long)offset, ret)) {
        ret = -EFAULT;
    }
    return ret;
}

static ssize_t write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    struct mmap_info *info;

    unsigned long offset = 4500;
    pr_info("write offset %ld\n", offset);
    info = filp->private_data;
    if (copy_from_user((unsigned long)(info->data) + (unsigned long)offset, buf, min(len, (size_t)BUFFER_SIZE))) {
        return -EFAULT;
    } else {
        return len;
    }
}

static int release(struct inode *inode, struct file *filp)
{
//    struct mmap_info *info;

//    pr_info("release\n");
//    info = filp->private_data;
//    //free_page((unsigned long)info->data);
//    unsigned long order = ORDER;
//	free_pages(info->data, order);
//    printk("Freeing 2 ^ %d pages\n", order);
//    kfree(info);
//    filp->private_data = NULL;
    return 0;
}

static const struct file_operations fops = {
    .mmap = mmap,
    .open = open,
    .release = release,
    .read = read,
    .write = write,
};

static int myinit(void)
{
    proc_create(filename, 0, NULL, &fops);

    pr_info("open\n");
    info = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);
    //info->data = (char *)get_zeroed_page(GFP_KERNEL);

	unsigned long order = ORDER; // TODO add module args 
    // data maps to logical address of the buffer
	info->data = (char* )__get_free_pages(GFP_KERNEL, order);

    pr_info("virt_to_phys = 0x%llx\n", (unsigned long long)virt_to_phys((void *)info->data));
    if (!info->data) {
            pr_info("Fail to allocate free pages!\n");
            /* insufficient memory: you must handle this error! */
            return -ENOMEM;
    }
    pr_info("kernel logical addr 0x%llx\n", (unsigned long)(info->data));


    return 0;
}

static void myexit(void)
{
    //info = filp->private_data;
    //free_page((unsigned long)info->data);
    unsigned long order = ORDER;
	free_pages(info->data, order);
    printk("Freeing 2 ^ %d pages\n", order);
    kfree(info);

    remove_proc_entry(filename, NULL);
}

    module_init(myinit)
module_exit(myexit)
    MODULE_LICENSE("GPL");

