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
#include <list.h>
#include <console.h>
#include <percpu.h>

#include <mm/vmm.h>

static list_head_t percpu_frames;

void init_percpu(void) {
    printk("Initialize Per CPU structures\n");

    list_init(&percpu_frames);
}

percpu_t *get_percpu_page(unsigned int cpu) {
    percpu_t *percpu;

    list_for_each_entry(percpu, &percpu_frames, list) {
        if (percpu->id == cpu)
            return percpu;
    }

    /* Per CPU page must be identity mapped,
     * because GDT descriptor has 32-bit base.
     */
    percpu = get_free_page(GFP_IDENT | GFP_KERNEL);
    BUG_ON(!percpu);

    percpu->id = cpu;
    percpu->user_stack = get_free_page_top(GFP_USER);

    list_add(&percpu->list, &percpu_frames);
    return percpu;
}
