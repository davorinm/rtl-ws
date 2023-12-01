#ifndef LIST_H
#define LIST_H

typedef struct list list;

list* list_alloc();

void list_add(list* l, void* data);

void list_apply(list* l, void (*func)(void*));

void list_apply2(list* l, void (*func)(void*,void*), void* user);

int list_length(const list* l);

void* list_peek(list* l);

void* list_poll(list* l);

void list_poll_to_list(list* from, list* to);

void list_clear(list* l);

void list_free(list* l);

#endif