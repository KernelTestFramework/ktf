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

static inline bool list_is_empty(list_head_t *list) {
    return list->next == list;
}

#define list_entry(elem, type, member) container_of(elem, type, member)

#define list_next_entry(elem, member) list_entry((elem)->member.next, typeof(*(elem)), member)
#define list_prev_entry(elem, member) list_entry((elem)->member.prev, typeof(*(elem)), member)

#define list_first_entry(head, type, member) list_entry((head)->next, type, member)
#define list_last_entry(head, type, member) list_entry((head)->prev, type, member)

#define list_for_each(ptr, head) \
    for (ptr = (head)->next; ptr != (head); ptr = ptr->next)

#define list_for_each_safe(ptr, bak, head)    \
    for (ptr = (head)->next, bak = ptr->next; \
         ptr != (head);                       \
         ptr = bak, bak = ptr->next)

#define list_for_each_entry(elem, head, member)                \
    for (elem = list_first_entry(head, typeof(*elem), member); \
         &elem->member != (head);                              \
         elem = list_next_entry(elem, member))

#define list_for_each_entry_safe(elem, bak, head, member)      \
    for (elem = list_first_entry(head, typeof(*elem), member), \
         bak = list_next_entry(elem, member);                  \
         &elem->member != (head);                              \
         elem = bak, bak = list_next_entry(bak, member))

#endif /* KTF_LIST_H */
