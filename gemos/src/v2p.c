#include <types.h>
#include <mmap.h>
#include <fork.h>
#include <v2p.h>
#include <page.h>
#include<memory.h>
/*
 * You may define macros and other helper functions here
 * You must not declare and use any static/global variables
 * */

/**
 * mprotect System call Implementation.
 */

long vm_area_mprotect(struct exec_context *current, u64 addr, int length, int prot)
{
    struct vm_area *start = current->vm_area;
    struct vm_area *start_bef=os_alloc(sizeof(struct vm_area));
    while (start)
    {
        if ((addr >= start->vm_start && addr <= start->vm_end) &&
            (((addr + length - 1) >= start->vm_start) && ((addr + length - 1) <= start->vm_end)))
        {
            break;
        }
        start_bef = start;
        start = start->vm_next;
    }
    if (addr > start->vm_start)
    {
        if (addr + length - 1 < (start->vm_end))
        { /* inside an vm*/
            struct vm_area *dummy1=os_alloc(sizeof(struct vm_area));;
            struct vm_area *dummy2=os_alloc(sizeof(struct vm_area));;
            dummy1->vm_start = addr;
            dummy1->vm_end = addr + length - 1;
            dummy2->vm_start = addr + length ;
            dummy2->vm_end = start->vm_end;
            dummy2->vm_next = start->vm_next;
            dummy1->vm_next = dummy2;
            start->vm_next = dummy1;
            dummy1->access_flags = prot;
            dummy2->access_flags = start->access_flags;
            return 0;
        }
        else
        { /* coiniciding backwards */
            if (start->vm_next->vm_start == (addr + length))
            { /* next vm is at next address*/
                if (start->vm_next->access_flags == prot)
                { /* same flags as new one,so merge*/
                    start->vm_next->vm_start = addr;
                    return 0;
                }
                else
                { /* different flags*/

                    struct vm_area *dummy=os_alloc(sizeof(struct vm_area));;
                    dummy->vm_start = addr;
                    dummy->vm_end = addr + length - 1;
                    dummy->vm_next = start->vm_next;
                    dummy->access_flags = prot;
                    start->vm_end = addr;
                    start->vm_next = dummy;
                    return 0;
                }
            }
            else
            { /* there is ahole between vms*/
                struct vm_area *dummy3=os_alloc(sizeof(struct vm_area));;
                dummy3->vm_start = addr;
                dummy3->vm_end = addr + length - 1;
                dummy3->vm_next = start->vm_next;
                dummy3->access_flags = prot;
                start->vm_end = addr;
                start->vm_next = dummy3;
                return 0;
            }
        }
    }
    else
    { /* co-inciding with initial one*/
        if (addr + length - 1 < (start->vm_end))
        { /* inside the vm */
            if (start != current->vm_area)
            {
                /* if the start is not current*/
                if ((addr - 1) == start_bef->vm_end)
                { /* no hole bewteen before vm and present*/
                    if (prot == start_bef->access_flags)
                    { /* previous has same flags*/
                        start_bef->vm_end = addr + length - 1;
                        start->vm_start = addr + length - 1;
                        return 0;
                    }
                    else
                    { /* doesn't have same flags*/
                        struct vm_area *dummy4 = current->vm_area;
                        dummy4->vm_start = addr;
                        dummy4->vm_end = addr + length - 1;
                        dummy4->access_flags = prot;
                        start->vm_start = addr + length;
                        start_bef->vm_next = dummy4;
                        dummy4->vm_next = start;
                        return 0;
                    }
                }
                else
                {
                    /* hole between before vm and present*/
                    struct vm_area *dummy5 = current->vm_area;
                    dummy5->vm_start = addr;
                    dummy5->vm_end = addr + length - 1;
                    dummy5->access_flags = prot;
                    start->vm_start = addr + length;
                    start_bef->vm_next = dummy5;
                    dummy5->vm_next = start;
                    return 0;
                }
            }
            else
            {
                /* start is current*/
                struct vm_area *dummy6 = current->vm_area;
                dummy6->vm_start = addr;
                dummy6->vm_end = addr + length - 1;
                dummy6->access_flags = prot;
                start->vm_start = addr + length;
                dummy6->vm_next = start;
                return 0;
            }
        }
        else
        {
            /* total vm*/
            start->access_flags = prot;
            return 0;
        }
    }

    return -1;
}

/**
 * mmap system call implementation.
 */
long vm_area_map(struct exec_context *current, u64 addr, int length, int prot, int flags)
{ /* checking if there is an overlap*/
    struct vm_area *curr_vma = current->vm_area;
    if (curr_vma == NULL)
    {
        struct vm_area *dummy=os_alloc(sizeof(struct vm_area));;
        dummy->vm_start = MMAP_AREA_START;
        dummy->vm_end = MMAP_AREA_START + 4095;
        dummy->access_flags = 0x0;
        dummy->vm_next = NULL;
        curr_vma = dummy;
        stats->num_vm_area++;
    };
    if(addr%4!=0){
      addr=addr+(4-(addr%4));
    }
    u64 start_addr = addr;
    u64 end_addr = addr + length - 1;
    int overlap = 0;
    // checking for newly requested address overlaps with existing vmas address
    while (curr_vma)
    {
        if (curr_vma->vm_start >= start_addr && end_addr <= curr_vma->vm_end)
        {
            overlap = 1;
            break;
        }
        if (curr_vma->vm_start <= start_addr && end_addr >= curr_vma->vm_end)
        {
            overlap = 1;
            break;
        }
        curr_vma = curr_vma->vm_next;
    }

    if (overlap == 1)
    { /* overlaaped and location is fixed*/
        if (flags = MAP_FIXED)
        {

            return -1;
        }
        /* location is not fixed so find the smallest address location at which vma can be fixed*/
        else
        {
            struct vm_area *curr_vma2 = current->vm_area;
            while (1)
            {
                /* Finding smallest address location for  the new vma*/
                if (curr_vma2->vm_next == NULL || curr_vma2->vm_next->vm_start - curr_vma2->vm_end >= length)
                {
                    int addr2 = curr_vma2->vm_end + 1;
                    /* checking if merging is possible*/
                    int back_merge = 0;  // back merge with curr_vma1
                    int front_merge = 0; // front merge with curr_vma1->next
                    int prev = 0;
                    int next = 0;
                    back_merge = 1;
                    prev = (curr_vma2->access_flags == flags);

                    if ((addr2 + length) == curr_vma2->vm_next->vm_start && curr_vma2->vm_next)
                    {
                        front_merge = 1;
                        next = (curr_vma2->vm_next->access_flags == flags);
                    }
                    if (prev && next)
                    {
                        curr_vma2->vm_end = curr_vma2->vm_next->vm_end;
                        curr_vma2->vm_next = curr_vma2->vm_next->vm_next;
                    }
                    else
                    {
                        if (prev == 1)
                        {
                            curr_vma2->vm_end = addr2 + length - 1;
                            return curr_vma2->vm_start;
                        }
                        if (next == 1)
                        {
                            curr_vma2->vm_next->vm_start = addr2;
                            return addr2;
                        }
                        if (prev == 0 && next == 0)
                        {
                            /* creating a new vma*/
                            struct vm_area *new_one=os_alloc(sizeof(struct vm_area));;
                            new_one->vm_start = addr2;
                            new_one->vm_end = addr2 + length - 1;
                            new_one->vm_next = curr_vma2->vm_next;
                            curr_vma2->vm_next = new_one;
                            return addr2;
                        }
                    }
                    break;
                }
                curr_vma2 = curr_vma2->vm_next;
            }
        }
    }
    if (overlap == 0)
    {
        if (addr != 0 )
        {
            struct vm_area *curr_vma1 = current->vm_area;

            while (1)
            {
                /* finding the appropriate position in the sorted linked list*/
                if (((addr >= curr_vma1->vm_end) && curr_vma1->vm_next == NULL) || (addr >= curr_vma1->vm_end) && ((addr + length - 1) <= curr_vma1->vm_next->vm_start))
                {
                    /* first check if there is chance  for merging*/
                    int back_merge = 0;  // back merge with curr_vma1
                    int front_merge = 0; // front merge with curr_vma1->next
                    int prev = 0;
                    int next = 0;
                    if (addr == (curr_vma1->vm_end + 1))
                    {
                        back_merge = 1;
                        prev = (curr_vma1->access_flags == flags);
                    }
                    if ((addr + length) == curr_vma1->vm_next->vm_start && curr_vma1->vm_next)
                    {
                        front_merge = 1;
                        next = (curr_vma1->vm_next->access_flags == flags);
                    }
                    /* three vma's merged*/
                    if (prev && next)
                    {
                        curr_vma1->vm_end = curr_vma1->vm_next->vm_end;
                        curr_vma1->vm_next = curr_vma1->vm_next->vm_next;
                        return curr_vma1->vm_start;
                    }
                    else
                    {
                        if (prev == 1)
                        {
                            curr_vma1->vm_end = addr + length - 1;
                            return curr_vma1->vm_start;
                        }
                        if (next == 1)
                        {
                            curr_vma1->vm_next->vm_start = addr;
                            return addr;
                        }
                        if (prev == 0 && next == 0)
                        {
                            /* creating a new vma*/
                            struct vm_area *new_one;
                            new_one->vm_start = addr;
                            new_one->vm_end = addr + length - 1;
                            new_one->vm_next = curr_vma1->vm_next;
                            curr_vma1->vm_next = new_one;
                            return addr;
                        }
                    }
                }
                curr_vma1 = curr_vma1->vm_next;
            }
        }
        else
        {

            struct vm_area *curr_vma3 = current->vm_area;
            while (1)
            {
                /* Finding smallest address location for  the new vma*/
                if (curr_vma3->vm_next == NULL || curr_vma3->vm_next->vm_start - curr_vma3->vm_end >= length)
                {
                    int addr3 = curr_vma3->vm_end + 1;
                    /* checking if merging is possible*/
                    int back_merge = 0;  // back merge with curr_vma1
                    int front_merge = 0; // front merge with curr_vma1->next
                    int prev = 0;
                    int next = 0;

                    back_merge = 1;
                    prev = (curr_vma3->access_flags == flags);

                    if ((addr3 + length) == curr_vma3->vm_next->vm_start && curr_vma3->vm_next)
                    {
                        front_merge = 1;
                        next = (curr_vma3->vm_next->access_flags == flags);
                    }
                    if (prev && next)
                    {
                        curr_vma3->vm_end = curr_vma3->vm_next->vm_end;
                        curr_vma3->vm_next = curr_vma3->vm_next->vm_next;
                        return curr_vma3->vm_start;
                    }
                    else
                    {
                        if (prev == 1)
                        {
                            curr_vma3->vm_end = addr3 + length - 1;
                            return curr_vma3->vm_start;
                        }
                        if (next == 1)
                        {
                            curr_vma3->vm_next->vm_start = addr3;
                            return addr3;
                        }
                        if (prev == 0 && next == 0)
                        {
                            /* creating a new vma*/
                            struct vm_area *new_one=os_alloc(sizeof(struct vm_area));;
                            new_one->vm_start = addr3;
                            new_one->vm_end = addr3 + length - 1;
                            new_one->vm_next = curr_vma3->vm_next;
                            curr_vma3->vm_next = new_one;
                            stats->num_vm_area++;
                            return addr3;
                        }
                    }
                    break;
                }
                curr_vma3 = curr_vma3->vm_next;
            }
        }
    }
    return -EINVAL;
}

/**
 * munmap system call implemenations
 */
long vm_area_unmap(struct exec_context *current, u64 addr, int length)
{

    u64 start_addr = addr;
    u64 end_addr = addr + length - 1;
    struct vm_area *start = current->vm_area;
    struct vm_area *start_bef;
    struct vm_area *final = current->vm_area;
    struct vm_area *final_aft;
    while (start->vm_next)
    {
        if (addr >= start->vm_start && addr <= start->vm_end)
        {
            break;
        }
        if (start->vm_end > addr && start->vm_next->vm_start > addr)
        {
            break;
        }
        start_bef = start;
        start = start->vm_next;
    }
    while (final->vm_next)
    {
        if ((addr + length - 1) >= final->vm_start && (addr + length - 1) <= final->vm_end)
        {

            break;
        }
        if (final->vm_end < (addr + length - 1) && (final->vm_next->vm_start > (addr + length - 1)))
        {
            final = final->vm_next;
            break;
        }
    }
    int start_coin = 0;
    int final_coin = 0;
    if (addr == start->vm_start)
    {
        start_coin = 1;
    }
    if ((addr + length - 1) == final->vm_end)
    {
        final_coin = 1;
    }
    if (start_coin)
    {
        if (final_coin)
        {
            if (start == current->vm_area)
            {
                if (final->vm_next)
                {
                    current->vm_area = NULL;
                    return 0;
                }
                else
                {
                    current->vm_area = final->vm_next;
                    return 0;
                }
            }
            else
            {
                if (final->vm_next)
                {
                    start_bef->vm_next = NULL;
                    current->vm_area = start_bef;
                    return 0;
                }
                else
                {
                    start_bef->vm_next = final->vm_next;
                    return 0;
                }
            }
        }
        else
        {
            if (final->vm_start > (addr + length - 1))
            {
                if (start == current->vm_area)
                {
                    current->vm_area = final;
                    return 0;
                }
                else
                {
                    start_bef->vm_next = final;
                    current->vm_area = start_bef;
                    return 0;
                }
            }
            else
            {
                final->vm_start = addr + length - 1;
                if (start == current->vm_area)
                {
                    current->vm_area = final;
                    return 0;
                }
                else
                {
                    start_bef->vm_next = final;
                    current->vm_area = start_bef;
                    return 0;
                }
            }
        }
    }
    else
    {
        if (final_coin)
        {

            if (start->vm_end >= addr)
            {

                if (final->vm_next)
                {
                    start->vm_next = final->vm_next;
                    return 0;
                }
                else
                {
                    start->vm_next = NULL;
                    return 0;
                }
            }
            else
            {
                start->vm_end = addr;
                if (final->vm_next)
                {
                    start->vm_next = final->vm_next;
                    return 0;
                }
                else
                {
                    start->vm_next = NULL;
                    return 0;
                }
            }
        }
        else
        {
            if (addr >= start->vm_end)
            {
                if ((addr + length - 1) <= final->vm_start)
                {
                    start->vm_next = final;
                    return 0;
                }
                else
                {
                    final->vm_start = addr + length - 1;
                    start->vm_next = final;
                    return 0;
                }
            }
            else
            {

                start->vm_end = addr;
                if ((addr + length - 1) <= final->vm_start)
                {
                    start->vm_next = final;
                    return 0;
                }
                else
                {
                    final->vm_start = addr + length - 1;
                    start->vm_next = final;
                    return 0;
                }
            }
        }
    }

    return -EINVAL;
}

/**
 * Function will invoked whenever there is page fault for an address in the vm area region
 * created using mmap
 */

// long vm_area_pagefault(struct exec_context *current, u64 addr, int error_code)
// {

// int is_present=0;
// struct vm_area * start=current->vm_area;
// while(start){
//     if(addr>=start->vm_start&&addr<=start->vm_end){
//         is_present=1;
//         break;
//     }
//     start=start->vm_next;
// }
// if(is_present==0){
//     return -1;
// }
// if(error_code!=start->access_flags){
// return -1;
// }
// u64 * pgd=(u64*)osmap(current->pgd);

// int pgd_off=(addr>>39)&0x1FF;
// int pud_off=(addr>>30)&0x1FF;
// int pmd_off=(addr>>21)&0x1FF;
// int pte_off=(addr>>12)&0x1FF;

// if(*(pgd+pgd_off)&0x1){
// ;
// }
// else{
   
//     u32 pud_entry = os_pfn_alloc(OS_PT_REG);
//     *(pgd+pgd_off)=pud_entry<<12|11001;
    
// }

// u64 *pud=*(pgd+pgd_off)>>12<<12;
// if(*(pud+pud_off)&0x1){
//     ;
// }
// else{
//     u32 pmd_entry=os_pfn_alloc(OS_PT_REG);
//     *(pud+pud_off)=pmd_entry<<12|11001;

// }
// u64 * pmd=*(pud+pud_off)>>12<<12;
// if(*(pmd+pmd_off)&0x1){
//     ;
// }
// else{
//     u32 pte_entry=os_pfn_alloc(OS_PT_REG);
//     *(pmd+pmd_off)=pte_entry<<12|11011;
// }
















//     return -1;
// }

/**
 * cfork system call implemenations
 * The parent returns the pid of child process. The return path of
 * the child process is handled separately through the calls at the
 * end of this function (e.g., setup_child_context etc.)
 */

long do_cfork()
{
    u32 pid;
    struct exec_context *new_ctx = get_new_ctx();
    struct exec_context *ctx = get_current_ctx();
    /* Do not modify above lines
     *
     * */
    /*--------------------- Your code [start]---------------*/

    /*--------------------- Your code [end] ----------------*/

    /*
     * The remaining part must not be changed
     */
    copy_os_pts(ctx->pgd, new_ctx->pgd);
    do_file_fork(new_ctx);
    setup_child_context(new_ctx);
    return pid;
}

/* Cow fault handling, for the entire user address space
 * For address belonging to memory segments (i.e., stack, data)
 * it is called when there is a CoW violation in these areas.
 *
 * For vm areas, your fault handler 'vm_area_pagefault'
 * should invoke this function
 * */

long handle_cow_fault(struct exec_context *current, u64 vaddr, int access_flags)
{
    return -1;
}
