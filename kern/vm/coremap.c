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

void
coremap_bootstrap(void) {
        // Get the first and last free address
        paddr_t lastpaddr  = ram_getsize();
        paddr_t firstpaddr = ram_getfirstfree();

        // Align first paddr to a page start (ie. multiple of PAGE_SIZE)
        int mod = firstpaddr % PAGE_SIZE;
        if (mod != 0) {
                firstpaddr += mod;
        }

        // Calculate the number of physical frames that are
        // possible
        int num_phy_frames = lastpaddr >> 12;

        // Calculate number of physical frames of kernel
        int num_kernel_frames = firstpaddr >> 12;

        // Get the size of a single coremap entry
        size_t coremap_entry_sz = sizeof(struct coremap_entry);
        
        // Total size for coremap
        size_t coremap_total_sz = num_phy_frames * coremap_entry_sz;

        // Find the number of physical frames required for coremap
        mod = coremap_total_sz % PAGE_SIZE;
        if (mod != 0) {
                coremap_total_sz += mod;
        }
        int num_coremap_frames = coremap_total_sz / PAGE_SIZE;

        // Total fixed physical frames
        int total_fixed_frames = num_kernel_frames + num_coremap_frames;

        num_total_frames = num_phy_frames;
        num_free_frames  = num_total_frames - total_fixed_frames;
        coremap = (struct coremap_entry *) PADDR_TO_KVADDR(firstpaddr);

        // Zero out the coremap area
        bzero((void *)coremap, coremap_total_sz);

        // Initialize the coremap
        for (int i = 0; i < num_phy_frames; i++) {
                if (i < total_fixed_frames) {
                        coremap[i].state = FIXED;
                }
                else {
                        coremap[i].state = FREE;
                }
        }

        // First free usable frame is one after the coremap frame
        first_index = total_fixed_frames;
}

paddr_t
coremap_getpages(unsigned long npages)
{
        paddr_t ret = 0;

        if (num_free_frames >= npages) {
                int num_pages = (int) npages;
                int len = num_total_frames - num_pages + 1; 
                for (int i = first_index; i < len; i++) {
                        bool free = true;
                        // Check if continuous npages are availabe
                        int j;
                        for (j = i; j < i + num_pages; j++) {
                                if (coremap[j].state != FREE) {
                                        free = false;
                                        break;
                                }
                        }
                        
                        if (free) {
                                // Mark free pages as allocated
                                for (int j = i; j < i + num_pages; j++) {
                                        coremap[j].state = ALLOCATED;
                                }

                                // Store the number of physical frames allocated
                                coremap[i].chunksize = npages;

                                // Return physical address of starting physical frame
                                ret = (paddr_t) (i * PAGE_SIZE); 

                                // Decrement the number of free frames
                                num_free_frames -= num_pages;
                                break;
                        }
                        else {
                                i = j;
                        }
                }
        }

        return ret;
}

void
coremap_freepages(vaddr_t addr)
{
        vaddr_t tmp_addr;
        int len = num_total_frames;
        for (int i = first_index; i < len; i++) {
                tmp_addr = PADDR_TO_KVADDR(i * PAGE_SIZE);
                if ( tmp_addr == addr) {
                        int npages = coremap[i].chunksize;
                        for(int j = i; j < i + npages; j++) {
                                // Mark frame as free
                                coremap[j].state = FREE;
                                coremap[j].chunksize = 0;
                                
                                // Zero out the page being freed
                                bzero((void *)PADDR_TO_KVADDR(j * PAGE_SIZE), 
                                      PAGE_SIZE);
                        }
                        
                        // Increment the number of free frames
                        num_free_frames += npages;
                        break;
                }
        }
}
