/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>
#include <coremap.h>

#define DUMBVM_STACKPAGES    18

struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}

        /*
	as->as_vbase1 = 0;
	as->as_pbase1 = 0;
	as->as_npages1 = 0;
        as->as_perm1 = 0;
        as->as_tmp_perm1 = 0;
	as->as_vbase2 = 0;
	as->as_pbase2 = 0;
	as->as_npages2 = 0;
        as->as_perm2 = 0;
        as->as_tmp_perm2 = 0;
        */
	as->as_stackpbase = 0;

	return as;
}

void
as_destroy(struct addrspace *as)
{
        //free_kpages(PADDR_TO_KVADDR(as->as_pbase1));
        //free_kpages(PADDR_TO_KVADDR(as->as_pbase2));
        int len = (int) as->nsegs;
        for (int i = 0; i < len; i++) {
                free_kpages(PADDR_TO_KVADDR(as->page_table[i].paddr));
        }
        free_kpages(PADDR_TO_KVADDR(as->as_stackpbase));
	kfree(as);
}

void
as_activate(void)
{
	int i, spl;
	struct addrspace *as;

	as = proc_getas();
	if (as == NULL) {
		return;
	}

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

void
as_deactivate(void)
{
	/* nothing */
}

int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	size_t npages;

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

	/* We don't use these - all pages are read-write */
        /*

	if (as->as_vbase1 == 0) {
		as->as_vbase1 = vaddr;
		as->as_npages1 = npages;
                as->as_tmp_perm1 = readable | writeable | executable;
		return 0;
	}

	if (as->as_vbase2 == 0) {
		as->as_vbase2 = vaddr;
		as->as_npages2 = npages;
                as->as_tmp_perm2 = readable | writeable | executable;
		return 0;
	}
        */
        as->page_table[as->nsegs].vaddr = vaddr;
        as->page_table[as->nsegs].t_perm = 
                readable | writeable | executable;
        as->page_table[as->nsegs].chunksize = npages;

        as->nsegs++;

	/*
	 * Support for more than two regions is not available.
	 */
	//kprintf("dumbvm: Warning: too many regions\n");
	return 0;
}

static
void
as_zero_region(paddr_t paddr, unsigned npages)
{
	bzero((void *)PADDR_TO_KVADDR(paddr), npages * PAGE_SIZE);
}

int
as_prepare_load(struct addrspace *as)
{
        /*
	KASSERT(as->as_pbase1 == 0);
	KASSERT(as->as_pbase2 == 0);
	KASSERT(as->as_stackpbase == 0);

        
	as->as_pbase1 = getppages(as->as_npages1);
	if (as->as_pbase1 == 0) {
		return ENOMEM;
	}

	as->as_pbase2 = getppages(as->as_npages2);
	if (as->as_pbase2 == 0) {
		return ENOMEM;
	}

	as->as_stackpbase = getppages(DUMBVM_STACKPAGES);
	if (as->as_stackpbase == 0) {
		return ENOMEM;
	}

	as_zero_region(as->as_pbase1, as->as_npages1);
	as_zero_region(as->as_pbase2, as->as_npages2);
	as_zero_region(as->as_stackpbase, DUMBVM_STACKPAGES);
        */

        int len = (int) as->nsegs;
        for(int i = 0; i < len; i++) {
                as->page_table[i].paddr = 
                        getppages(as->page_table[i].chunksize);
                if (as->page_table[i].paddr == 0) {
                        return ENOMEM;
                }
                as_zero_region(as->page_table[i].paddr,
                               as->page_table[i].chunksize);
        }
        

	as->as_stackpbase = getppages(DUMBVM_STACKPAGES);
	if (as->as_stackpbase == 0) {
		return ENOMEM;
	}
	as_zero_region(as->as_stackpbase, DUMBVM_STACKPAGES);

	return 0;
}

int
as_complete_load(struct addrspace *as)
{
        /*
	as->as_perm1 = as->as_tmp_perm1;
        as->as_perm2 = as->as_tmp_perm2;
        */
        /* Set the permissions of a segments pages */
        int len = (int) as->nsegs;
        for (int i = 0; i < len; i++) {
                as->page_table[i].p_perm = as->page_table[i].t_perm;
        }
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	KASSERT(as->as_stackpbase != 0);

	*stackptr = USERSTACK;
	return 0;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *new;

	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

        int result = as_init_pagetable(new, old->nsegs);
        if (result) {
                return result;
        }

        new->nsegs = old->nsegs;

        int len = (int) new->nsegs;
        for (int i = 0; i < len; i++) {
                new->page_table[i].vaddr = old->page_table[i].vaddr;
                new->page_table[i].chunksize = 
                        old->page_table[i].chunksize;
        }
        
	/* (Mis)use as_prepare_load to allocate some physical memory. */
	if (as_prepare_load(new)) {
		as_destroy(new);
		return ENOMEM;
	}

        for (int i = 0; i < len; i++) {
                KASSERT(new->page_table[i].paddr != 0);
                memmove((void *)PADDR_TO_KVADDR(new->page_table[i].paddr),
                        (const void *)PADDR_TO_KVADDR(old->page_table[i].paddr),
                        old->page_table[i].chunksize * PAGE_SIZE);
        }

	KASSERT(new->as_stackpbase != 0);
	memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase),
		(const void *)PADDR_TO_KVADDR(old->as_stackpbase),
		DUMBVM_STACKPAGES*PAGE_SIZE);

	*ret = new;
	return 0;
}

int
as_init_pagetable(struct addrspace *as, uint32_t nsegs) {
        /* Set nsegs as 0 here so that it can be used as a counter */
        as->nsegs = 0;
        as->page_table = kmalloc(nsegs * sizeof(struct pagetable_entry));
        if (as->page_table == NULL) {
                return ENOMEM;
        }

        return 0;
}
