/*
 * Copyright (c) 2018,2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel_internal.h>
#include <zephyr/spinlock.h>
#include <zephyr/llext/symbol.h>

bool z_spin_lock_valid(struct k_spinlock *l)
{
	uintptr_t thread_cpu = l->thread_cpu;

	if (thread_cpu != 0U) {
		if ((thread_cpu & 3U) == _current_cpu->id) {
			return false;
		}
	}
	return true;
}
EXPORT_SYMBOL(z_spin_lock_valid);

/* z_spin_unlock_valid() and z_spin_lock_set_owner() below use _raw_current
 * rather than _current: they run at points where z_current_thread_set() may
 * just have changed the current thread, which an architecture caching it in a
 * register (CONFIG_ARCH_HAS_CUSTOM_CURRENT_IMPL) would not report.  Both are
 * called from k_spin_lock() and k_spin_unlock() with the lock held, hence with
 * interrupts locked as _raw_current requires.
 */
bool z_spin_unlock_valid(struct k_spinlock *l)
{
	uintptr_t tcpu = l->thread_cpu;

	l->thread_cpu = 0;

	if (arch_is_in_isr() &&
	    (_raw_current->base.thread_state & _THREAD_DUMMY) != 0) {
		/* Edge case where an ISR aborted _current */
		return true;
	}
	if (tcpu != (_current_cpu->id | (uintptr_t)_raw_current)) {
		return false;
	}
	return true;
}
EXPORT_SYMBOL(z_spin_unlock_valid);

void z_spin_lock_set_owner(struct k_spinlock *l)
{
	l->thread_cpu = _current_cpu->id | (uintptr_t)_raw_current;
}
EXPORT_SYMBOL(z_spin_lock_set_owner);

#ifdef CONFIG_KERNEL_COHERENCE
bool z_spin_lock_mem_coherent(struct k_spinlock *l)
{
	return sys_cache_is_mem_coherent((void *)l);
}
EXPORT_SYMBOL(z_spin_lock_mem_coherent);
#endif /* CONFIG_KERNEL_COHERENCE */
