/* 
** Copyright 2002-2004, The Haiku Team. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_VM_PAGE_H
#define _KERNEL_VM_PAGE_H


#include <vm.h>


struct kernel_args;


#ifdef __cplusplus
extern "C" {
#endif

status_t vm_page_init(struct kernel_args *args);
status_t vm_page_init_post_area(struct kernel_args *args);
status_t vm_page_init_post_thread(struct kernel_args *args);

status_t vm_mark_page_inuse(addr_t page);
status_t vm_mark_page_range_inuse(addr_t startPage, addr_t length);
status_t vm_page_set_state(vm_page *page, int state);

vm_page *vm_page_allocate_page(int state);
vm_page *vm_page_allocate_page_run(int state, addr_t length);
vm_page *vm_page_allocate_specific_page(addr_t page_num, int state);
vm_page *vm_lookup_page(addr_t page_num);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_VM_PAGE_H */
