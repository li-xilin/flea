#ifndef AX_LINK_H
#define AX_LINK_H
#include <stddef.h>

typedef struct _LINK
{
        struct _LINK *prev, *next;
} LINK, *LPLINK;

#define LIST_INITIALIZER(name) { &(name), &(name) }

#define CONTAINER_OF(ptr, type, member) (((type*)((char*)ptr - (offsetof(type, member)))))

inline static void ListInit(LPLINK head)
{
        head->next = head->prev = head;
}

inline static void ListInitLink(LPLINK link){
        link->next = link->prev = NULL;
}

inline static void __ListAdd(LPLINK new_link, LPLINK prev, LPLINK next)
{
        next->prev = new_link;
        new_link->next = next;
        new_link->prev = prev;
        prev->next = new_link;
}

inline static void ListAddFront(LPLINK head, LPLINK new_link)
{
        __ListAdd(new_link, head, head->next);
}

inline static void ListAddBack(LPLINK head, LPLINK new_link)
{
        __ListAdd(new_link, head->prev, head);
}

inline static void __ListDel(LPLINK  prev, LPLINK  next)
{
        next->prev = prev;
        prev->next = next;
}

inline static void ListDel(LPLINK link)
{
        if(link == 0 || link->prev == 0 || link->next == 0)
                return;

        __ListDel(link->prev, link->next);
        link->prev = link->next = 0;
}

inline static int ListIsEmpty(const LPLINK head)
{
        return head->next == head;
}

#define ListGetEntry(link, type, member) CONTAINER_OF(link, type, member)

#define ListFirstEntry(head, type, member) ListGetEntry((head)->next, type, member)

#define ListLastEntry(head, type, member) ListGetEntry((head)->prev, type, member)

#define ListFirstEntryOrNull(head, type, member) \
        (!ListIsEmpty(head) ? ListFirstEntry(head, type, member) : NULL)

#define ListForEach(head, pos) \
		for (pos = (head)->next; pos != (head); pos = pos->next)

#endif
