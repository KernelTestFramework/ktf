/*
 * Copyright © 2020 Amazon.com, Inc. or its affiliates.
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef KTF_LIST_H
#define KTF_LIST_H

#include <ktf.h>
#include <lib.h>

struct list_head {
    struct list_head *next;
    struct list_head *prev;
};
typedef struct list_head list_head_t;

/* Static declarations */

static inline void list_init(list_head_t *head) {
    head->next = head;
    head->prev = head;
}

static inline void list_insert(list_head_t *new, list_head_t *prev, list_head_t *next) {
    new->prev = prev;
    new->next = next;

    next->prev = new;
    prev->next = new;
}

static inline void list_add(list_head_t *new, list_head_t *top) {
    list_insert(new, top, top->next);
}

static inline void list_add_tail(list_head_t *new, list_head_t *top) {
    list_insert(new, top->prev, top);
}

static inline void list_unlink(list_head_t *entry) {
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
    entry->next = NULL;
    entry->prev = NULL;
}

static inline bool list_is_empty(list_head_t *list) { return list->next == list; }

#define list_entry(elem, type, member) container_of(elem, type, member)

#define list_next_entry(elem, member)                                                    \
    list_entry((elem)->member.next, typeof(*(elem)), member)
#define list_prev_entry(elem, member)                                                    \
    list_entry((elem)->member.prev, typeof(*(elem)), member)

#define list_first_entry(head, type, member) list_entry((head)->next, type, member)
#define list_last_entry(head, type, member)  list_entry((head)->prev, type, member)

#define list_for_each(ptr, head) for (ptr = (head)->next; ptr != (head); ptr = ptr->next)

#define list_for_each_safe(ptr, bak, head)                                               \
    for (ptr = (head)->next, bak = ptr->next; ptr != (head); ptr = bak, bak = ptr->next)

#define list_for_each_entry(elem, head, member)                                          \
    for (elem = list_first_entry(head, typeof(*elem), member); &elem->member != (head);  \
         elem = list_next_entry(elem, member))

#define list_for_each_entry_safe(elem, bak, head, member)                                \
    for (elem = list_first_entry(head, typeof(*elem), member),                           \
        bak = list_next_entry(elem, member);                                             \
         &elem->member != (head); elem = bak, bak = list_next_entry(bak, member))

#endif /* KTF_LIST_H */
