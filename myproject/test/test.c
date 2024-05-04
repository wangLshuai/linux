#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mmzone.h>
#include <linux/page_ref.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/slab_def.h>
static void *p;
static void *vp0, *vp1;
static struct page *a_page;
static uintptr_t addr;
static struct kmem_cache *slab;
struct test_slab {
	int i;
	long j;
};

static void foreach_pages(void)
{
	unsigned long i = 0, free = 0, locked = 0, reserved = 0, swapcache = 0,
		      referenced = 0, slab = 0, private = 0, uptodate = 0,
		      dirty = 0, active = 0, writeback = 0, mappedtodisk = 0;
	struct page *pa;
	unsigned long num_phypages;

	num_phypages = get_num_physpages();
	printk("foreach %ld's phy pages\n", num_phypages);
	for (i = 0; i < num_phypages; i++) {
		pa = pfn_to_page(i);
		if (!page_count(pa)) {
			free++;
			continue;
		}
		if (PageLocked(pa))
			locked++;
		if (PageReserved(pa))
			reserved++;
		if (PageSwapCache(pa))
			swapcache++;
		if (PageReferenced(pa))
			referenced++;
		if (PageSlab(pa))
			slab++;
		if (PagePrivate(pa))
		private++;
		if (PageUptodate(pa))
			uptodate++;
		if (PageDirty(pa))
			dirty++;
		if (PageActive(pa))
			active++;
		if (PageWriteback(pa))
			writeback++;
		if (PageMappedToDisk(pa))
			mappedtodisk++;
	}
	printk("memory size : %lu MB\n",
	       num_phypages * PAGE_SIZE / 1024 / 1024);
	printk("free:%lu\n", free);
	printk("locked:%lu\n", locked);
	printk("reserved:%lu\n", reserved);
	printk("swapcache:%lu\n", swapcache);
	printk("referenced:%lu\n", referenced);
	printk("slab:%lu\n", slab);
	printk("private:%lu\n", private);
	printk("uptodata:%lu\n", uptodate);
	printk("dirty:%lu\n", dirty);
	printk("active:%lu\n", active);
	printk("writeback:%lu\n", writeback);
	printk("mappedtodisk:%lu\n", mappedtodisk);
}
static int __init test_init(void)
{
	int i;
	printk("hello,test\n");
	p = kmalloc(10, GFP_KERNEL);
	printk("kmalloc p addr:     %016lx\n", (uintptr_t)p);
	printk("__START_KERNEL_map: %016lx\addr\n", phys_base);
	printk("phy addr of p:      %016lx\n", __pa(p));

	vp0 = vmalloc(PAGE_SIZE);
	printk("vmalloc vp0 value:    %016lx\n", (uintptr_t)vp0);
	printk("vp0 is_vmmalloc_addr:%d\n", is_vmalloc_addr(vp0));
	struct page *vp0_page = vmalloc_to_page(vp0);
	printk("sizeof(struct page):%ld\n", sizeof(struct page));
	printk("vp0_page:             %016lx\n", (uintptr_t)vp0_page);
	// printk("mem_map:            %016lx\n",(uintptr_t)mem_map);
	uintptr_t phy_page_addr0 = page_to_phys(vp0_page);
	printk("phy page addr0:        %016lx\n", phy_page_addr0);
	printk("vp0 offset in page:   %016lx\n", offset_in_page(vp0));

	vp1 = vmalloc(PAGE_SIZE);
	printk("vmalloc vp1 value:    %016lx\n", (uintptr_t)vp1);
	struct page *vp1_page = vmalloc_to_page(vp1);
	printk("vp1_page:             %016lx\n", (uintptr_t)vp1_page);
	uintptr_t phy_page_addr1 = page_to_phys(vp1_page);
	printk("phy page addr:        %016lx\n", phy_page_addr1);

	printk("page array start:%016lx\n", (uintptr_t)vmemmap);
	long int index_delte = abs(vp1_page - vp0_page);
	printk("index_delte:%ld", index_delte);

	printk("%ld\n", abs((phy_page_addr0 >> PAGE_SHIFT) -
			    (phy_page_addr1 >> PAGE_SHIFT)));
	// BUG_ON(1);
	uint64_t cr3 = cr3 = __read_cr3();
	uint64_t *p = __va(cr3);
	printk("%016lx\n", (uintptr_t)p);
	printk("*p:%016llx\n", *p);
	printk("CR3:    %016llx\n", cr3);
	printk("%016lx\n", vp1_page->flags);
	printk("vp1_page->_mapcount.counter: %d,vp1_page->_refcount.counter: %d\n",
	       page_mapcount(vp1_page), page_count(vp1_page));

	a_page = alloc_page(GFP_KERNEL & ~__GFP_HIGHMEM);
	if (a_page != NULL) {
		printk("a_page value:%016lx\n", (uintptr_t)a_page);
		char *l_addr = page_address(a_page);
		for (i = 0; i < PAGE_SIZE; i++)
			l_addr[i] = 0x55;
	}

	uint64_t size;
	char *buf;
	for (size = PAGE_SIZE, i = 0; i < MAX_ORDER; i++, size *= 2) {
		printk("try tokmalloc %llu bytes\n", size);
		buf = kmalloc(size, GFP_ATOMIC);
		if (!buf) {
			pr_err("kmalloc failed\n");
			break;
		}
		kfree(buf);
	}

	addr = __get_free_pages(GFP_KERNEL, 10);
	if (addr) {
		printk("__get_free_pages return a virtual address from direct mapped zone:%016lx\n",
		       addr);
		*(char *)addr = 0xff;
	}
	foreach_pages();
	printk("-------------------------------------slab-------------------------------------\n");
	int cpu_nums = num_online_cpus();
	printk("cpu nums:%d\n", cpu_nums);
	slab = kmem_cache_create("test_slab", sizeof(struct test_slab),
				 SLAB_HWCACHE_ALIGN, 0, NULL);

	printk("kmeme_cache::cpu_slab is per cpu variable");
	printk("slab->size:%u,slab->limit:%u,slab->batchcount:%u,slab->num:%u,slab->align:%u,slab->object_size:%u\n",
	       slab->size, slab->limit, slab->batchcount, slab->num,
	       slab->align, slab->object_size);
	struct test_slab *test_s = kmem_cache_alloc(slab, GFP_KERNEL);
	test_s->i = 10;
	test_s->j = 100;
	printk("slab->size:%u,slab->limit:%u,slab->batchcount:%u,slab->num:%u,slab->align:%u,slab->object_size:%u\n",
	       slab->size, slab->limit, slab->batchcount, slab->num,
	       slab->align, slab->object_size);
	kmem_cache_free(slab, test_s);

	return 0;
}

static void __exit test_exit(void)
{
	kmem_cache_destroy(slab);
	if (addr)
		printk("*addr: %x", *(char *)addr);
	free_pages(addr, 10);
	__free_page(a_page);
	vfree(vp0);
	vfree(vp1);
	kfree(p);
	printk("%s exit\n", __func__);
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wang shuai");