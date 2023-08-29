/******************************************************************************
 *
 * Name: acktf.h - OS specific defines, etc. for KTF
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2021, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/

#ifdef KTF_ACPICA

#ifndef __ACKTF_H__
#define __ACKTF_H__

/* Common (in-kernel/user-space) ACPICA configuration */

#define ACPI_USE_SYSTEM_CLIBRARY
#define ACPI_USE_DO_WHILE_0
#define ACPI_USE_LOCAL_CACHE

/* Kernel specific ACPICA configuration */

#ifdef KTF_DEBUG
#undef ACPI_NO_ERROR_MESSAGES
#define ACPI_DEBUG_OUTPUT
#endif

#include <spinlock.h>
#include <string.h>

#include "acenv.h"

#define ACPI_INIT_FUNCTION
#define ACPI_CACHE_T ACPI_MEMORY_LIST

/* Host-dependent types and defines for in-kernel ACPICA */

#define ACPI_EXPORT_SYMBOL(symbol)

#define ACPI_SPINLOCK  spinlock_t *
#define ACPI_CPU_FLAGS unsigned long

#define ACPI_MSG_ERROR     "ACPI Error: "
#define ACPI_MSG_EXCEPTION "ACPI Exception: "
#define ACPI_MSG_WARNING   "ACPI Warning: "
#define ACPI_MSG_INFO      "ACPI: "

#define ACPI_MSG_BIOS_ERROR   "ACPI BIOS Error (bug): "
#define ACPI_MSG_BIOS_WARNING "ACPI BIOS Warning (bug): "

#ifndef __init
#define __init
#endif
#ifndef __iomem
#define __iomem
#endif

#if defined(__x86_64__)
#define ACPI_MACHINE_WIDTH        64
#define COMPILER_DEPENDENT_INT64  long
#define COMPILER_DEPENDENT_UINT64 unsigned long
#else
#define ACPI_MACHINE_WIDTH        32
#define COMPILER_DEPENDENT_INT64  long long
#define COMPILER_DEPENDENT_UINT64 unsigned long long
#endif

#endif /* __ACKTF_H__ */
#endif /* KTF_ACPICA */
