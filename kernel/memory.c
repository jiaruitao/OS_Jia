#include "memory.h"
#include "../lib/std_int.h"
#include "../lib/kernel/print.h"
#include "../lib/kernel/bitmap.h"
#include "global.h"
#include "debug.h"
#include "../lib/string.h"
#include "../thread/sync.h"
#include "../thread/thread.h"

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

#define PG_SIZE 4096
#define MEM_BITMAP_BASE 0xc009a000

#define K_HEAP_START 0xc0100000			// 虚拟地址，跨过 1M 的空间

struct pool{
	struct bitmap pool_bitmap;
	uint32_t phy_addr_start;
	uint32_t pool_size;					// 本内存池的字节容量
	struct lock lock;
};

struct pool kernel_pool, user_pool;		// 内核物理内存池，用户物理内存池
struct virtual_addr kernel_vaddr;		// 内核虚拟内存池


/* 在pf 表示的虚拟内存池中申请pg_cnt个虚拟页 */
/* 返回申请的虚拟内存的虚拟起始地址 */
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt)
{
	int vaddr_start = 0, bit_idx_start = -1;
	uint32_t cnt = 0;
	if (pf == PF_KERNEL){		// 内核虚拟内存池中申请内存
		bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
		if (bit_idx_start == -1){
			return NULL;
		}
		while(cnt < pg_cnt){
			bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);		// 把位图相应位置1
		}
		vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
	}
	else{	// 用户虚拟内存池中申请内存
		struct task_struct* cur = running_thread();
		bit_idx_start = bitmap_scan(&cur->userprog_vaddr.vaddr_bitmap, pg_cnt);
		if (bit_idx_start == -1){
			return NULL;
		}
		while(cnt < pg_cnt){
			bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);		// 把位图相应位置1
		}
		vaddr_start = cur->userprog_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
		
		ASSERT((uint32_t)vaddr_start < (0xc0000000 - PG_SIZE));
		
	}
	return (void*)vaddr_start;	
}

/* 得到虚拟地址 vaddr 对应的 pte 指针 */
uint32_t* pte_ptr(uint32_t vaddr)
{
	uint32_t*pte = (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
	return pte;
}

/* 得到虚拟地址 vaddr 对应的 pde 指针 */
uint32_t* pde_ptr(uint32_t vaddr)
{
	uint32_t* pde = (uint32_t*)(0xfffff000 + PDE_IDX(vaddr) * 4);
	return pde;
}
/* 物理内存池中申请 1 个物理页，返回物理地址 */
static void* palloc(struct pool* m_pool)
{
	int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
	if (bit_idx == -1){
		return NULL;
	}
	bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
	uint32_t page_phyaddr = (m_pool->phy_addr_start + (bit_idx * PG_SIZE));
	return (void*)page_phyaddr;
}

static void page_table_add(void* _vaddr, void* _page_phyaddr)
{
	uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
	uint32_t* pde = pde_ptr(vaddr);
	uint32_t* pte = pte_ptr(vaddr);
	
	if (*pde & 0x00000001){					// 判断页目录项中的 P 位，看页表是否存在
		ASSERT(!(*pte & 0x00000001));		// 页表项不能存在
		if (!(*pte & 0x00000001)){
			*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
		}else{
			// 应该不会执行到这里
			put_str("\n~!@#$%^&*()~!@#$%^&*()~!@#$%^&*()\n");
		}
	}else{									// 页表不存在
		uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
		*pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
		memset((void*)((int)pte & 0xfffff000), 0, PG_SIZE);
		ASSERT(!(*pte & 0x00000001));
		*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
	}
}

void* malloc_page(enum pool_flags pf, uint32_t pg_cnt)
{
	ASSERT(pg_cnt > 0 && pg_cnt < 3840);
	
	void* vaddr_start = vaddr_get(pf, pg_cnt);			// 在虚拟内存池中申请 pg_cnt 个页
	if (vaddr_start == NULL){
		return NULL;
	}
	
	uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
	struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;
	while (cnt--){
		void* page_phyaddr = palloc(mem_pool);			// 在物理内存池中申请 1 页
		if (page_phyaddr == NULL){
			return NULL;
		}
		page_table_add(vaddr, page_phyaddr);
		vaddr += PG_SIZE;
	}
	return vaddr_start;
}

/* 在内核空间申请 pg_cnt 页内存, 返回虚拟地址*/
void* get_kernel_pages(uint32_t pg_cnt)
{
	void* vaddr = malloc_page(PF_KERNEL, pg_cnt);
	if (vaddr != NULL){
		memset(vaddr, 0, pg_cnt * PG_SIZE);
	}
	return vaddr;
}

/* 在用户空间申请 pg_cnt 页内存, 返回虚拟地址 */
void* get_user_pages(uint32_t pg_cnt)
{
	lock_acquire(&user_pool.lock);
	void* vaddr = malloc_page(PF_USER, pg_cnt);
	if (vaddr != NULL) {
		memset(vaddr, 0, pg_cnt * PG_SIZE);
	}
	lock_release(&user_pool.lock);
	return vaddr;
}

/* 将地址vaddr与pf池中的物理地址关联,仅支持一页空间分配 */
void* get_a_page(enum pool_flags pf, uint32_t vaddr) 
{
	struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;
	lock_acquire(&mem_pool->lock);

	/* 先将虚拟地址对应的位图置1 */
	struct task_struct* cur = running_thread();
	int32_t bit_idx = -1;

/* 若当前是用户进程申请用户内存,就修改用户进程自己的虚拟地址位图 */
	if (cur->pgdir != NULL && pf == PF_USER) {
		bit_idx = (vaddr - cur->userprog_vaddr.vaddr_start) / PG_SIZE;
		ASSERT(bit_idx > 0);
		bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx, 1);

	} else if (cur->pgdir == NULL && pf == PF_KERNEL){
/* 如果是内核线程申请内核内存,就修改kernel_vaddr. */
		bit_idx = (vaddr - kernel_vaddr.vaddr_start) / PG_SIZE;
		ASSERT(bit_idx > 0);
		bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx, 1);
	} else {
		PANIC("get_a_page:not allow kernel alloc userspace or user alloc kernelspace by get_a_page");
	}

	void* page_phyaddr = palloc(mem_pool);
	if (page_phyaddr == NULL) {
		return NULL;
	}
	page_table_add((void*)vaddr, page_phyaddr); 
	lock_release(&mem_pool->lock);
	return (void*)vaddr;
}

/* 得到虚拟地址映射到的物理地址 */
uint32_t addr_v2p(uint32_t vaddr) 
{
	uint32_t* pte = pte_ptr(vaddr);
/* (*pte)的值是页表所在的物理页框地址,
 * 去掉其低12位的页表项属性+虚拟地址vaddr的低12位 */
	return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
}

/* 初始化内存池，参数是物理内存的大小 */ 
//本质就是初始化 kernel_pool, user_pool 这两个物理内存池 和 kernel_vaddr 内核虚拟内存池
static void mem_pool_init(uint32_t all_mem) 
{
	put_str("mem_pool_init start!\n");
	uint32_t page_table_size = PG_SIZE * 256;
	uint32_t used_mem = page_table_size + 0x100000;		//已用空间：页表+1M的内核
	uint32_t free_mem = all_mem - used_mem;
	
	uint16_t all_free_pages = free_mem / PG_SIZE;
	uint16_t kernel_free_pages = all_free_pages / 2;
	uint16_t user_free_pages = all_free_pages - kernel_free_pages;
	
	uint32_t kbm_length = kernel_free_pages / 8;
	uint32_t ubm_length = user_free_pages / 8;
	
	uint32_t kp_start = used_mem;					// 内核物理内存池的起始地址
	uint32_t up_start = used_mem + kernel_free_pages * PG_SIZE;//用户物理内存池的起始地址
	
	// 初始化 kernel_pool, user_pool 这两个全局变量
	kernel_pool.phy_addr_start = kp_start;
	user_pool.phy_addr_start = up_start;
	
	kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
	user_pool.pool_size = user_free_pages * PG_SIZE;	
	
	kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
	user_pool.pool_bitmap.btmp_bytes_len = ubm_length;
	
	kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;
	user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length);
	
	// 输出内存信息
	put_str("all_mem:");put_int(all_mem);
	put_str("\n");
	put_str(" kernel_pool_bitmap_start:");put_int((uint32_t)kernel_pool.pool_bitmap.bits);
	put_str(" kernel_pool_phy_addr_start:");put_int(kernel_pool.phy_addr_start);
	put_str("\n");
	put_str(" user_pool_bitmap_start:");put_int((uint32_t)user_pool.pool_bitmap.bits);
	put_str(" user_pool_phy_addr_start:");put_int(user_pool.phy_addr_start);
	put_str("\n");
	
	// 位图初始化
	bitmap_init(&kernel_pool.pool_bitmap);			
	bitmap_init(&user_pool.pool_bitmap);
	
	lock_init(&kernel_pool.lock);
	lock_init(&user_pool.lock);
	
	// 初始化 虚拟内存池
	kernel_vaddr.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);
	kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
	kernel_vaddr.vaddr_start = K_HEAP_START;
	bitmap_init(&kernel_vaddr.vaddr_bitmap);
	put_str("mem_pool_init done!\n");
}


void mem_init(void)
{
	put_str("mem_init start!\n");
	uint32_t mem_bytes_total = (*(uint32_t*)0xb03);
	mem_pool_init(mem_bytes_total);
	put_str("mem_init done!\n");
}

