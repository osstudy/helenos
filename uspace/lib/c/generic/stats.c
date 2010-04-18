/*
 * Copyright (c) 2010 Stanislav Kozina
 * Copyright (c) 2010 Martin Decky
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <stats.h>
#include <sysinfo.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <inttypes.h>

#define SYSINFO_STATS_MAX_PATH  64

/** Thread states
 *
 */
static const char *thread_states[] = {
	"Invalid",
	"Running",
	"Sleeping",
	"Ready",
	"Entering",
	"Exiting",
	"Lingering"
};

/** Get CPUs statistics
 *
 * @param count Number of records returned.
 *
 * @return Array of stats_cpu_t structures.
 *         If non-NULL then it should be eventually freed
 *         by free().
 *
 */
stats_cpu_t *stats_get_cpus(size_t *count)
{
	size_t size = 0;
	stats_cpu_t *stats_cpus =
	    (stats_cpu_t *) sysinfo_get_data("system.cpus", &size);
	
	assert((size % sizeof(stats_cpu_t)) == 0);
	
	*count = size / sizeof(stats_cpu_t);
	return stats_cpus;
}

/** Get physical memory statistics
 *
 *
 * @return Pointer to the stats_physmem_t structure.
 *         If non-NULL then it should be eventually freed
 *         by free().
 *
 */
stats_physmem_t *stats_get_physmem(void)
{
	size_t size = 0;
	stats_physmem_t *stats_physmem =
	    (stats_physmem_t *) sysinfo_get_data("system.physmem", &size);
	
	assert((size == sizeof(stats_physmem_t)) || (size == 0));
	
	return stats_physmem;
}

/** Get task IDs
 *
 * @param count Number of IDs returned.
 *
 * @return Array of IDs (task_id_t).
 *         If non-NULL then it should be eventually freed
 *         by free().
 *
 */
task_id_t *stats_get_tasks(size_t *count)
{
	size_t size = 0;
	task_id_t *ids =
	    (task_id_t *) sysinfo_get_data("system.tasks", &size);
	
	assert((size % sizeof(task_id_t)) == 0);
	
	*count = size / sizeof(task_id_t);
	return ids;
}

/** Get single task statistics
 *
 * @param task_id Task ID we are interested in.
 *
 * @return Pointer to the stats_task_t structure.
 *         If non-NULL then it should be eventually freed
 *         by free().
 *
 */
stats_task_t *stats_get_task(task_id_t task_id)
{
	char name[SYSINFO_STATS_MAX_PATH];
	snprintf(name, SYSINFO_STATS_MAX_PATH, "system.tasks.%" PRIu64, task_id);
	
	size_t size = 0;
	stats_task_t *stats_task =
	    (stats_task_t *) sysinfo_get_data(name, &size);
	
	assert((size == sizeof(stats_task_t)) || (size == 0));
	
	return stats_task;
}

/** Get thread IDs
 *
 * @param count Number of IDs returned.
 *
 * @return Array of IDs (thread_id_t).
 *         If non-NULL then it should be eventually freed
 *         by free().
 *
 */
thread_id_t *stats_get_threads(size_t *count)
{
	size_t size = 0;
	thread_id_t *ids =
	    (thread_id_t *) sysinfo_get_data("system.threads", &size);
	
	assert((size % sizeof(thread_id_t)) == 0);
	
	*count = size / sizeof(thread_id_t);
	return ids;
}

/** Get single thread statistics
 *
 * @param thread_id Thread ID we are interested in.
 *
 * @return Pointer to the stats_thread_t structure.
 *         If non-NULL then it should be eventually freed
 *         by free().
 *
 */
stats_thread_t *stats_get_thread(thread_id_t thread_id)
{
	char name[SYSINFO_STATS_MAX_PATH];
	snprintf(name, SYSINFO_STATS_MAX_PATH, "system.threads.%" PRIu64, thread_id);
	
	size_t size = 0;
	stats_thread_t *stats_thread =
	    (stats_thread_t *) sysinfo_get_data(name, &size);
	
	assert((size == sizeof(stats_thread_t)) || (size == 0));
	
	return stats_thread;
}

/** Get system load
 *
 * @param count Number of load records returned.
 *
 * @return Array of load records (load_t).
 *         If non-NULL then it should be eventually freed
 *         by free().
 *
 */
load_t *stats_get_load(size_t *count)
{
	size_t size = 0;
	load_t *load =
	    (load_t *) sysinfo_get_data("system.load", &size);
	
	assert((size % sizeof(load_t)) == 0);
	
	*count = size / sizeof(load_t);
	return load;
}

/** Get system uptime
 *
 * @return System uptime (in seconds).
 *
 */
sysarg_t stats_get_uptime(void)
{
	sysarg_t uptime;
	if (sysinfo_get_value("system.uptime", &uptime) != EOK)
		uptime = 0;
	
	return uptime;
}

/** Print load fixed-point value
 *
 * Print the load record fixed-point value in decimal
 * representation on stdout.
 *
 * @param upper      Load record.
 * @param dec_length Number of decimal digits to print.
 *
 */
void stats_print_load_fragment(load_t upper, unsigned int dec_length)
{
	/* Magic value from BSD */
	load_t lower = 65536;
	
	/* Print the whole part */
	printf("%u.", upper / lower);
	
	load_t rest = (upper % lower) * 10;
	
	unsigned int i;
	for (i = 0; i < dec_length; i++) {
		printf("%u", rest / lower);
		rest = (rest % lower) * 10;
	}
}

const char *thread_get_state(state_t state)
{
	if (state <= Lingering)
		return thread_states[state];
	
	return thread_states[Invalid];
}

/** @}
 */