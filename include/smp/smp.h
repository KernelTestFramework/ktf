/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef KTF_SMP_H
#define KTF_SMP_H

#include <ktf.h>
#include <lib.h>
#include <processor.h>

#define INVALID_CPU (~0U)

/* External declarations */

extern void smp_init(void);

/* Static declarations */

static inline unsigned int smp_processor_id(void) {
    return (unsigned int) rdmsr(MSR_TSC_AUX);
}

#endif /* KTF_SMP_H */
