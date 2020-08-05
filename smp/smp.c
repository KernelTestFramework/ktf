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
#include <ktf.h>
#include <lib.h>
#include <console.h>

#include <smp/smp.h>
#include <smp/mptables.h>

static unsigned nr_cpus;

void smp_init(void) {
    nr_cpus = mptables_init();

    if (nr_cpus == 0) {
        nr_cpus = 1;
        return;
    }

    printk("Initializing SMP support (CPUs: %u)\n", nr_cpus);

}
