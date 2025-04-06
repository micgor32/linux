/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * smm_handler.h
 *
 * Internal header for the SMM handlers.
 *
 * Copyright 2025 9elements gmbh
 * Copyright 2025 Michal Gorlas <michal.gorlas@9elements.com>
 */

#ifndef __SMM_HANDLER_H
#define __SMM_HANDLER_H

void intel_common_reloc(int cpu, uintptr_t curr_smbase, uintptr_t perm_smbase);
void qemu35_reloc(int cpu, uintptr_t curr_smbase, uintptr_t perm_smbase);
// not implemented yet
void amd_common_reloc(void);
void intel_apollolake_reloc(int cpu, uintptr_t curr_smbase, uintptr_t perm_smbase);
void intel_denverton_reloc(int cpu, uintptr_t curr_smbase, uintptr_t perm_smbase);
void intel_baytrial_reloc(int cpu, uintptr_t curr_smbase, uintptr_t perm_smbase);
void intel_braswell_reloc(int cpu, uintptr_t curr_smbase, uintptr_t perm_smbase);
void intel_xeonsp_reloc(int cpu, uintptr_t curr_smbase, uintptr_t perm_smbase);
void intel_f2x_reloc(int cpu, uintptr_t curr_smbase, uintptr_t perm_smbase);
void intel_gen1_reloc(int cpu, uintptr_t curr_smbase, uintptr_t perm_smbase);
void intel_haswell_reloc(int cpu, uintptr_t curr_smbase, uintptr_t perm_smbase);

// locking



#endif 
