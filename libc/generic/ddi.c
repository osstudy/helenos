/*
 * Copyright (C) 2006 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */ 

#include <ddi.h>
#include <libc.h>
#include <task.h>
#include <kernel/ddi/ddi_arg.h>

/** Map piece of physical memory to task.
 *
 * Caller of this function must have the CAP_MEM_MANAGER capability.
 *
 * @param id Task ID.
 * @param pf Physical address of the starting frame.
 * @param vp Virtual address of the sterting page.
 * @param pages Number of pages to map.
 * @param writable If true, the mapping will be created writable.
 *
 * @return 0 on success, EPERM if the caller lacks the CAP_MEM_MANAGER capability,
 *	   ENOENT if there is no task with specified ID and ENOMEM if there
 *	   was some problem in creating address space area.
 */
int map_physmem(task_id_t id, void *pf, void *vp, unsigned long pages, int writable)
{
	task_id_t task_id;
	ddi_memarg_t arg;

	arg.task_id = id;
	arg.phys_base = pf;
	arg.virt_base = vp;
	arg.pages = pages;
	arg.writable = writable;

	return __SYSCALL1(SYS_MAP_PHYSMEM, (sysarg_t) &arg);
}

/** Enable I/O space range to task.
 *
 * Caller of this function must have the IO_MEM_MANAGER capability.
 *
 * @param id Task ID.
 * @param ioaddr Starting address of the I/O range.
 * @param size Size of the range.
 *
 * @return 0 on success, EPERM if the caller lacks the CAP_IO_MANAGER capability,
 *	   ENOENT if there is no task with specified ID and ENOMEM if there
 *	   was some problem in allocating memory.
 */
int iospace_enable(task_id_t id, void *ioaddr, unsigned long size)
{
	task_id_t task_id;
	ddi_ioarg_t arg;

	arg.task_id = id;
	arg.ioaddr = ioaddr;
	arg.size = size;

	return __SYSCALL1(SYS_IOSPACE_ENABLE, (sysarg_t) &arg);
}

/** Interrupt control
 *
 * @param enable 1 - enable interrupts, 0 - disable interrupts
 */
int preemption_control(int enable)
{
	return __SYSCALL1(SYS_PREEMPT_CONTROL, (sysarg_t) enable);
}
