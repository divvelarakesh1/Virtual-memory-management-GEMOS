#include <types.h>
#include <mmap.h>
#include <fork.h>
#include <v2p.h>
#include <page.h>
/*
 * You may define macros and other helper functions here
 * You must not declare and use any static/global variables
 * */

/**
 * mprotect System call Implementation.
 */
/* To remove  pages of vma which are removed partially  */
void change_flags(struct exec_context *current, struct vm_area *vm, u64 start, u64 end, int prot)
{

    u64* pgd = (u64 *)osmap(current->pgd);
    for (u64 addr = start; addr < (end); addr += 4096)
    {
        int pgd_off = (addr >> 39) & 0x1FF;
        int pud_off = (addr >> 30) & 0x1FF;
        int pmd_off = (addr >> 21) & 0x1FF;
        int pte_off = (addr >> 12) & 0x1FF;
        if (pgd[pgd_off] & 0x1)
        { // //("bye1\n");
            u64* pud = (u64 * )osmap((pgd[pgd_off]) >> 12);
            if (pud[pud_off] & 0x1)
            { //  //("bye2\n");
                u64* pmd = (u64 *)osmap(((pud[pud_off])) >> 12);
                if (pmd[pmd_off] & 0x1)
                { //   //("bye3\n");
                    u64* pte = (u64 *)osmap(pmd[pmd_off] >> 12);
                    if (pte[pte_off] & 0x1)
                    { //  //("bye4\n");
                        int x = pte[pte_off] & 0x7;
                        pte[pte_off] = pte[pte_off] >> 4;

                        if (prot == PROT_READ)
                        {

                            pte[pte_off] = pte[pte_off] << 4;
                            pte[pte_off] = pte[pte_off] + x;
                        }
                        else
                        {
                            pte[pte_off] = pte[pte_off] << 1;
                            pte[pte_off] = pte[pte_off] + 1;
                            pte[pte_off] = pte[pte_off] << 3;
                            pte[pte_off] += x;
                        }
                          //("%x\n",pte[pte_off]);
                    }
                    else
                    { // //("bye5\n");
                        continue;
                    }
                }
                else
                { // //("bye6\n");
                    continue;
                }
            }
            else
            { // //("bye7\n");
                continue;
            }
        }
        else
        { // //("bye8\n");
            continue;
        }
        asm volatile("invlpg (%0);" ::"r"(addr) : "memory");
    }
   
}
void partial_rem(struct exec_context *current, struct vm_area *vm, int length, u64 addr1)
{

    u64 *pgd = (u64 *)osmap(current->pgd);
    // //("bye\n");
    for (u64 addr = addr1; addr < (addr1 + length); addr = addr + 4096)
    {

        int pgd_off = (addr >> 39) & 0x1FF;
        int pud_off = (addr >> 30) & 0x1FF;
        int pmd_off = (addr >> 21) & 0x1FF;
        int pte_off = (addr >> 12) & 0x1FF;
        if (pgd[pgd_off] & 0x1)
        { // //("bye1\n");
            u64 *pud = (u64 *)osmap((pgd[pgd_off]) >> 12);
            if (pud[pud_off] & 0x1)
            { //  //("bye2\n");
                u64 *pmd = (u64 *)osmap(((pud[pud_off])) >> 12);
                if (pmd[pmd_off] & 0x1)
                { //   //("bye3\n");
                    u64 *pte = (u64 *)osmap(pmd[pmd_off] >> 12);
                    if (pte[pte_off] & 0x1)
                    { //  //("bye4\n");
                    //    printk("hello\n");
                        // if (get_pfn_refcount(pte[pte_off] >> 12))
                        // {
                        //    os_pfn_free(USER_REG, pte[pte_off] >> 12);
                        // }
                        // else
                        // {
                        //     put_pfn(pte[pte_off] >> 12);
                        // }
                        pte[pte_off] = pte[pte_off] & 0x0;
                    }
                    else
                    { // //("bye5\n");
                        continue;
                    }
                }
                else
                { // //("bye6\n");
                    continue;
                }
            }
            else
            { // //("bye7\n");
                continue;
            }
        }
        else
        { // //("bye8\n");
            continue;
        }
    }
}
/* To remove pages of vmas which are completely removed*/
void rem(struct exec_context *current, struct vm_area *vm)
{
    u64 *pgd = (u64 *)osmap(current->pgd);

    for (u64 addr = vm->vm_start; addr < vm->vm_end; addr = addr + 4096)
    {

        int pgd_off = (addr >> 39) & 0x1FF;
        int pud_off = (addr >> 30) & 0x1FF;
        int pmd_off = (addr >> 21) & 0x1FF;
        int pte_off = (addr >> 12) & 0x1FF;
        if (pgd[pgd_off] & 0x1)
        {
            u64 *pud = (u64 *)osmap((pgd[pgd_off]) >> 12);
            if (pud[pud_off] & 0x1)
            {
                u64 *pmd = osmap(((pud[pud_off])) >> 12);
                if (pmd[pmd_off] & 0x1)
                {
                    u64 *pte = osmap(pmd[pmd_off] >> 12);
                    if (pte[pte_off] & 0x1)
                    { // printk("hello\n");
                        put_pfn(USER_REG);
                        // if (get_pfn_refcount(pte[pte_off] >> 12))
                        // {
                       //     os_pfn_free(USER_REG, pte[pte_off] >> 12);
                        // }
                        pte[pte_off] = pte[pte_off] & 0x0;
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    continue;
                }
            }
            else
            {
                continue;
            }
        }
        else
        {
            continue;
        }
    }
}
/* Total vms which are removed compleely will call rem */
void rem_full(struct exec_context *current, struct vm_area *vm1, struct vm_area *vm2, int a)
{
    if (a == 0)
    {
        // start and final passed

        while (vm1 != vm2)
        {
            rem(current, vm1);
            vm1 = vm1->vm_next;
        }
        if (vm1 == vm2)
        {
            rem(current, vm1);
        }
    }
    // start and final-bef
    if (a == 1)
    {

        while (vm1 != vm2 && vm1->vm_next != vm2)
        {
            rem(current, vm1);
            vm1 = vm1->vm_next;
        }
    } // start->next and final
    if (a == 2)
    {
        //("2\n");
        
        if (vm1 == vm2)
        {
            ;
        }
        else
        { vm1 = vm1->vm_next;
            while (vm1 != vm2)
            {
                rem(current, vm1);
                vm1 = vm1->vm_next;
            }
            if (vm1 == vm2)
            {
                rem(current, vm1);
            }
        }
    }
    if (a == 3)
    {
        if (vm1 != vm2)
        {
            vm1 = vm1->vm_next;
            while (vm1 != vm2)
            {
                rem(current, vm1);
                vm1 = vm1->vm_next;
            }
        }
    }

    return;
}

long vm_area_mprotect(struct exec_context *current, u64 addr, int length, int prot)
{
    struct vm_area *start = current->vm_area;
    struct vm_area *start_bef;
    if (length % 4096 != 0)
    {
        length = length + (4096 - (length % 4096));
    }
    while (start)
    {

        /* vm is totally included in the range for changing access flags */
        if (addr <= start->vm_start && (addr + length) >= start->vm_end)
        { // //("hello\n");
            /* check for if there is a chance for merging*/
            change_flags(current, start, start->vm_start, start->vm_end, prot);
            if (start_bef && prot == start_bef->access_flags && start->vm_start == start_bef->vm_end)
            {
                start_bef->vm_end = start->vm_end;

                if (start->vm_next && start->vm_next->vm_start == start->vm_end && prot == start->vm_next->access_flags)
                {
                    start_bef->vm_end = start->vm_next->vm_end;
                    start_bef->vm_next = start->vm_next;
                    stats->num_vm_area--;
                    start = start->vm_next;
                }
                else
                {
                    start_bef->vm_next = start->vm_next;
                    start = start->vm_next;
                    stats->num_vm_area--;
                }
            }
            else
            {
                start->access_flags = prot;
                if (start->vm_next && start->vm_next->vm_start == start->vm_end && prot == start->vm_next->access_flags)
                {
                    start->vm_end = start->vm_next->vm_end;
                    start->vm_next = start->vm_next->vm_next;
                    stats->num_vm_area--;
                    start_bef = start;
                    start = start->vm_next;
                }
                else
                {
                    start_bef = start;
                    start = start->vm_next;
                }
            }

            continue;
        }
        /* partially  in the range ,flags changing for front part  */
        if (addr <= start->vm_start && (addr + length) < start->vm_end)
        {
            //  //("hello1\n");
            change_flags(current, start, start->vm_start, addr + length, prot);
            struct vm_area *sec = os_alloc(sizeof(struct vm_area));
            sec->vm_end = start->vm_end;
            sec->vm_next = start->vm_next;
            sec->vm_start = addr + length;
            sec->access_flags = prot;

            if (start_bef && start_bef->access_flags == prot && start_bef->vm_end == start->vm_start)
            {
                start_bef->vm_end = addr + length;
                start_bef->vm_next = sec;
            }
            else
            {
                start->vm_next = sec;
                start->vm_end = addr + length;
                start->access_flags = prot;
                stats->num_vm_area++;
            }
            continue;
        }
        /* partially  in the range flags changing for second part  */
        if (addr > start->vm_start && addr < start->vm_end && (addr + length) >= start->vm_end)
        { //  //("hello2\n");
            change_flags(current, start, addr, start->vm_end, prot);
            struct vm_area *sec = os_alloc(sizeof(struct vm_area));

            sec->vm_start = addr;
            sec->access_flags = prot;
            if (start->vm_next && start->vm_next->access_flags == prot && start->vm_end == start->vm_next->vm_start)
            {
                sec->vm_end = start->vm_next->vm_end;
                sec->vm_next = start->vm_next->vm_next;
                stats->num_vm_area--;
            }
            else
            {
                sec->vm_end = start->vm_end;
                sec->vm_next = start->vm_next;
            }
            start->vm_end = addr;
            start->vm_next = sec;
            start_bef = sec;
            start = sec->vm_next;
            continue;
        }
        /* Totally inside a vm*/
        if (addr > start->vm_start && (addr + length) < start->vm_end)
        {
            change_flags(current, start, addr, addr + length, prot);
            //("hello3\n");
            struct vm_area *d1 = os_alloc(sizeof(struct vm_area));
            struct vm_area *d2 = os_alloc(sizeof(struct vm_area));
            d1->vm_start = addr;
            d1->vm_end = addr + length;
            d1->access_flags = prot;
            d2->vm_start = addr + length;
            d2->vm_end = start->vm_end;
            start->vm_end = addr;
            d2->access_flags = start->access_flags;
            //(" second access flags %d\n", start->access_flags);
            //(" second access flags %d\n", d2->access_flags);
            d2->vm_next = start->vm_next;

            start->vm_next = d1;
            d1->vm_next = d2;
            stats->num_vm_area += 2;
            break;
        }
        start_bef = start;
        start = start->vm_next;
    }
    asm volatile("invlpg (%0);" ::"r"(addr) : "memory");
    // printf("%d\n",count_vm_areas(current));
    return 0;
}
/**
 * mmap system call implementation.
 */
long vm_area_map(struct exec_context *current, u64 addr, int length, int prot, int flags)
{ /* checking if there is an overlap*/
    if (length < 0)
    {
        return -EINVAL;
    }
    
    if (current->vm_area == NULL)
    {
        struct vm_area *dummy = os_alloc(sizeof(struct vm_area));
        dummy->vm_start = MMAP_AREA_START;
        dummy->vm_end = MMAP_AREA_START + 4096;
        dummy->access_flags = 0x0;
        dummy->vm_next = NULL;
        current->vm_area = dummy;
        stats->num_vm_area++;
    };
    struct vm_area *curr_vma = current->vm_area;

    if (length % 4096 != 0)
    {
        length = length + (4096 - (length % 4096));
    }

    u64 start_addr = addr;
    u64 end_addr = addr + length - 1;

    int overlap = 0;
    if (addr != 0)
    {
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
    }
    if (overlap == 1)
    { /* overlaaped and location is fixed*/
        if (flags == MAP_FIXED)
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
                    u64 addr2 = curr_vma2->vm_end + 1;
                    /* checking if merging is possible*/
                    int back_merge = 0;  // back merge with curr_vma1
                    int front_merge = 0; // front merge with curr_vma1->next
                    int prev = 0;
                    int next = 0;
                    back_merge = 1;
                    prev = (curr_vma2->access_flags == prot);

                    if ((addr2 + length) == curr_vma2->vm_next->vm_start && curr_vma2->vm_next)
                    {
                        front_merge = 1;
                        next = (curr_vma2->vm_next->access_flags == prot);
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
                            struct vm_area *new_one = os_alloc(sizeof(struct vm_area));

                            new_one->vm_start = addr2;
                            new_one->vm_end = addr2 + length - 1;
                            new_one->vm_next = curr_vma2->vm_next;
                            new_one->access_flags = prot;
                            curr_vma2->vm_next = new_one;
                            stats->num_vm_area++;
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
        if (addr != 0)
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
                        prev = (curr_vma1->access_flags == prot);
                    }
                    if ((addr + length) == curr_vma1->vm_next->vm_start && curr_vma1->vm_next)
                    {
                        front_merge = 1;
                        next = (curr_vma1->vm_next->access_flags == prot);
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
                            struct vm_area *new_one = os_alloc(sizeof(struct vm_area));
                            new_one->vm_start = addr;
                            new_one->vm_end = addr + length - 1;
                            new_one->vm_next = curr_vma1->vm_next;
                            new_one->access_flags = prot;
                            curr_vma1->vm_next = new_one;
                            stats->num_vm_area++;
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
                    u64 addr3 = curr_vma3->vm_end;
                    // printf("Ready for allocation at %d\n",addr3);
                    /* checking if merging is possible*/
                    int back_merge = 0;  // back merge with curr_vma1
                    int front_merge = 0; // front merge with curr_vma1->next
                    int prev = 0;
                    int next = 0;

                    back_merge = 1;
                    prev = (curr_vma3->access_flags == prot);

                    if (curr_vma3->vm_next && (addr3 + length) == curr_vma3->vm_next->vm_start)
                    {
                        front_merge = 1;
                        next = (curr_vma3->vm_next->access_flags == prot);
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
                            //    printf("creating a new vm\n");
                            struct vm_area *new_one = os_alloc(sizeof(struct vm_area));

                            new_one->vm_start = addr3;
                            new_one->vm_end = addr3 + length;
                            new_one->vm_next = curr_vma3->vm_next;
                            new_one->access_flags = prot;
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
    asm volatile("invlpg (%0);" ::"r"(addr) : "memory");
    return -EINVAL;
}

/**
 * munmap system call implemenations
 */
long vm_area_unmap(struct exec_context *current, u64 addr, int length)
{
    if (length <= 0 || addr < MMAP_AREA_START || addr + length - 1 > MMAP_AREA_END)
        return -1;
    u64 start_addr = addr;

    if (length % 4096 != 0)
    {
        length = length + (4096 - (length % 4096));
    }
    u64 end_addr = addr + length;
    int a = 0;
    int b = 0;

    // Find start and final VM areas
    struct vm_area *start = current->vm_area;
    struct vm_area *start_bef = NULL;
    struct vm_area *final = current->vm_area;
    struct vm_area *final_bef = NULL;

    while (start->vm_next)
    {
        if (addr >= start->vm_start && addr < start->vm_end)
        {
            break;
        }
        if (start->vm_end > addr && start->vm_next->vm_start > addr)
        {
            break;
        }
        a++;
        start_bef = start;
        start = start->vm_next;
    }

    while (final->vm_next)
    {
        if ((addr + length) > final->vm_start && (addr + length) <= final->vm_end)
        {
            break;
        }
        if (final->vm_end < (addr + length) && (final->vm_next->vm_start > (addr + length)))
        {
            final = final->vm_next;
            break;
        }
        b++;
        final_bef = final;
        final = final->vm_next;
    }
//    printk("%x",start->vm_start);
//    printk("%x",final->vm_start);
    int start_coin = 0;
    int final_coin = 0;
    if (addr == start->vm_start)
    {
        start_coin = 1;
    }
    if ((addr + length) == final->vm_end)
    {
        final_coin = 1;
    }

    // Case 1: Start coincides with VM area start
    if (start_coin)
    {
        if (final_coin)
        {
            // Free all VM areas from start to final
            struct vm_area *temp = start;
            struct vm_area *next;

            rem_full(current, start, final, 0);

            while (temp && temp != final)
            {
                next = temp->vm_next;
                os_free(temp, sizeof(struct vm_area));
                temp = next;
            }

            // Don't forget to free the final node
            if (temp)
            {
                os_free(temp, sizeof(struct vm_area));
            }

            if (start == current->vm_area)
            {
                if (final->vm_next)
                {
                    current->vm_area = final->vm_next;
                    stats->num_vm_area = 0;
                    return 0;
                }
                else
                {
                    current->vm_area = NULL;
                    stats->num_vm_area = stats->num_vm_area - (b - a + 1);
                    return 0;
                }
            }
            else
            {
                if (final->vm_next)
                {
                    start_bef->vm_next = final->vm_next;
                    stats->num_vm_area = stats->num_vm_area - (b - a + 1);
                    return 0;
                }
                else
                {
                    start_bef->vm_next = NULL;
                    stats->num_vm_area = stats->num_vm_area - (b - a + 1);
                    return 0;
                }
            }
        }
        else // start_coin but not final_coin
        {
            stats->num_vm_area = stats->num_vm_area - (b - a);

            // Free all VM areas from start to the one before final
            struct vm_area *temp = start;
            struct vm_area *next;

            rem_full(current, start, final, 1);

            while (temp && temp != final)
            {
                next = temp->vm_next;
                os_free(temp, sizeof(struct vm_area));
                temp = next;
            }

            if (final->vm_start > (addr + length))
            {
                if (start == current->vm_area)
                {
                    current->vm_area = final;
                    return 0;
                }
                else
                {
                    start_bef->vm_next = final;
                    return 0;
                }
            }
            else
            {
                partial_rem(current, final->vm_start, addr + length - final->vm_start, final->vm_start);
                final->vm_start = addr + length;
                if (start == current->vm_area)
                {
                    current->vm_area = final;
                    return 0;
                }
                else
                {
                    start_bef->vm_next = final;
                    return 0;
                }
            }
        }
    }
    else // not start_coin
    {
        if (final_coin)
        {
            rem_full(current, start, final, 2);
            stats->num_vm_area = stats->num_vm_area - (b - a);

            // Free all VM areas between start->vm_next and final
            if (start->vm_next != final)
            {
                struct vm_area *temp = start->vm_next;
                struct vm_area *next;

                while (temp && temp != final)
                {
                    next = temp->vm_next;
                    os_free(temp, sizeof(struct vm_area));
                    temp = next;
                }
            }

            if (start->vm_end >= addr)
            {
                partial_rem(current, start, start->vm_end - addr, addr);
                if (final->vm_next)
                {
                    start->vm_next = final->vm_next;
                    os_free(final, sizeof(struct vm_area));
                    return 0;
                }
                else
                {
                    start->vm_next = NULL;
                    os_free(final, sizeof(struct vm_area));
                    return 0;
                }
            }
            else
            {
                start->vm_end = addr;
                if (final->vm_next)
                {
                    start->vm_next = final->vm_next;
                    os_free(final, sizeof(struct vm_area));
                    return 0;
                }
                else
                {
                    start->vm_next = NULL;
                    os_free(final, sizeof(struct vm_area));
                    return 0;
                }
            }
        }
        else // not start_coin and not final_coin
        {   
            stats->num_vm_area = stats->num_vm_area - (b - a - 1);
            rem_full(current, start, final, 3);

            // Free all VM areas between start->vm_next and final-1
            if (start->vm_next != final)
            {
                struct vm_area *temp = start->vm_next;
                struct vm_area *next;

                while (temp && temp != final)
                {
                    next = temp->vm_next;
                    os_free(temp, sizeof(struct vm_area));
                    temp = next;
                }
            }
         
            if (addr >= start->vm_end)
            {   
                if ((addr + length) <= final->vm_start)
                {
                    start->vm_next = final;
                    return 0;
                }
                else
                {
                    partial_rem(current, final, addr + length - final->vm_start, final->vm_start);
                    final->vm_start = addr + length;
                    start->vm_next = final;
                    return 0;
                }
            }
            else
            {   // printk("%d\n",start->vm_end-addr);   
                partial_rem(current, start, start->vm_end-addr, addr);
                start->vm_end = addr ;
                if ((addr + length) <= final->vm_start)
                {
                    start->vm_next = final;
                    return 0;
                }
                else
                {  if(final!=start){
                    partial_rem(current, final, addr + length - final->vm_start, final->vm_start);
                }
                    if (b == a)
                    {
                        struct vm_area *new_one = os_alloc(sizeof(struct vm_area));
                        new_one->vm_start = addr + length;
                        new_one->vm_end = final->vm_end;
                        // We need a way to free this if it's not used
                        // However, it seems your code doesn't use this allocated structure
                        os_free(new_one, sizeof(struct vm_area));

                        start->vm_next = final;
                        return 0;
                    }
                    final->vm_start = addr + length;
                    start->vm_next = final;
                    return 0;
                }
            }
        }
    }
    asm volatile("invlpg (%0);" ::"r"(addr) : "memory");
    return -EINVAL;
}
/**
 * Function will invoked whenever there is page fault for an address in the vm area region
 * created using mmap
 */

long vm_area_pagefault(struct exec_context *current, u64 addr, int error_code)
{
    // //("entered the function\n");
    
    if (addr < 0)
    {
        return -EINVAL;
    }
    int is_present = 0;
    struct vm_area *start = current->vm_area->vm_next;
    //  printf("%d\n",count_vm_areas(current));
    ////("%d\n", error_code);
    
    while (start)
    { // //("looping\n");
      //  //("%x\n",start->vm_start);
      // //("%x\n",start->vm_end);
        if (addr >= start->vm_start && addr < start->vm_end)
        {
            is_present = 1;
            break;
        }

        start = start->vm_next;
    }
    // //("hio\n");
    if (is_present == 0)
    {
        return -1;
    }
    //  //("%d\n",error_code);
    // //("%d\n",error_code);
    if (error_code == 0x6 && start->access_flags == PROT_READ)
    {
        return -1;
    }
    if (error_code == 0x7)
    {
        if (!(start->access_flags & PROT_WRITE))
            return -1;
        handle_cow_fault(current, addr, start->access_flags);
        return 1;
    }
    int alpha = 1;
    if (start->access_flags == PROT_READ)
    {
        alpha = 0;
    }

    // //("hello\n");
    if (error_code == 0x4 || error_code == 0x6)
    { // //("Error code is 6 or 4\n");

        u64* pgd = (u64 *)osmap(current->pgd);
        // //("hello\n");
        int pgd_off = (addr >> 39) & 0x1FF;
        int pud_off = (addr >> 30) & 0x1FF;
        int pmd_off = (addr >> 21) & 0x1FF;
        int pte_off = (addr >> 12) & 0x1FF;
        if ((pgd[pgd_off]) & 0x1)
        {
            ;
        }
        else
        {
            // //("hi1\n");
            u32 pud_entry = os_pfn_alloc(OS_PT_REG);

            (pgd[pgd_off]) = pud_entry << 12 | 0x19;

            // else
            // {
            //     (pgd[pgd_off]) = pud_entry << 12 | 0x11;
            // }
        }
        u64* pud = (u64 *)osmap((pgd[pgd_off]) >> 12);
        if ((pud[pud_off]) & 0x1)
        {
            ;
        }
        else
        {
            //("hi2\n");
            u32 pmd_entry = os_pfn_alloc(OS_PT_REG);

            (pud[pud_off]) = pmd_entry << 12 | 0x19;
        }
        u64* pmd = (u64 *)osmap(((pud[pud_off])) >> 12);
        if (pmd[pmd_off] & 0x1)
        {
            ;
        }
        else
        {
            // //("hi3\n");
            u32 pte_entry = os_pfn_alloc(OS_PT_REG);

            pmd[pmd_off] = pte_entry << 12 | 0x19;
        }
        u64* pte = (u64 *)osmap(pmd[pmd_off] >> 12);

        u32 pt = os_pfn_alloc(USER_REG);

        if (alpha == 1)
        { // //("hi4\n");
            (pte[pte_off]) = pt << 12 | 0x19;

            // //("hello\n");
        }
        else
        { // //("hi4\n");
            (pte[pte_off]) = pt << 12 | 0x11;
        }
        //("%d\n",pte[pte_off]&0b11111);
        return 1;
    }
  asm volatile("invlpg (%0);" ::"r"(addr) : "memory");
    return -1;
}

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

