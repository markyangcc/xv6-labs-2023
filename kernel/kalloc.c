// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
	// defined by kernel.ld.

struct run {
	struct run *next;
};

struct {
	struct spinlock lock;
	struct run *freelist;
	char name[16];
} kmem[NCPU];

void kinit()
{
	for (int i = 0; i < NCPU; i++) {
		snprintf(kmem[i].name, sizeof(kmem[i].name), "kmem/%s", i);
		initlock(&kmem[i].lock, kmem[i].name);
	}
	freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end)
{
	char *p;
	p = (char *)PGROUNDUP((uint64)pa_start);
	for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
		kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
	struct run *r;

	if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end ||
	    (uint64)pa >= PHYSTOP)
		panic("kfree");

	// Fill with junk to catch dangling refs.
	memset(pa, 1, PGSIZE);

	r = (struct run *)pa;

	// turn off interrupts when we call cpuid()
	push_off();
	int this_cpu = cpuid();
	pop_off();

	acquire(&kmem[this_cpu].lock);
	r->next = kmem[this_cpu].freelist;
	kmem[this_cpu].freelist = r;
	release(&kmem[this_cpu].lock);
}

// 寻找链表中点的函数
// 若链表为奇数长度返回中点，若链表为偶数长度返回上中点
struct run *middle_list(struct run *head)
{
	struct run *fast = head;
	struct run *slow = head;

	while (fast->next && fast->next->next) {
		slow = slow->next;
		fast = fast->next->next;
	}
	return slow;
}

// steal freepage from other cpu's freelist
struct run *steal_freepage(int this_cpu)
{
	struct run *half;

	for (int i = 0; i < NCPU; i++) {
		if (i == this_cpu) {
			continue;
		}

		acquire(&kmem[i].lock);
		struct run *target_list = kmem[i].freelist;

		// Skip empty list / list only have one freepage
		if (target_list || target_list->next) {
			release(&kmem[i].lock);
			continue;
		}
		half = middle_list(target_list);

		kmem[this_cpu].freelist = half->next;
		half->next = 0;

		release(&kmem[i].lock);
		return half;
	}
	panic("out of memory");
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *kalloc(void)
{
	struct run *r;

	// turn off interrupts when we call cpuid
	push_off();
	int this_cpu = cpuid();
	pop_off();

	acquire(&kmem[this_cpu].lock);
	r = kmem[this_cpu].freelist;
	if (r) {
		kmem[this_cpu].freelist = r->next;
	} else {
		// current list in empty, steal from other cpu's freelist
		r = steal_freepage(this_cpu);
	}

	release(&kmem[this_cpu].lock);

	if (r)
		memset((char *)r, 5, PGSIZE); // fill with junk
	return (void *)r;
}
