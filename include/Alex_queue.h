#ifndef QUEUE_H
#define QUEUE_H


#include <stdlib.h>


typedef struct ListNode {
    int val;
    struct ListNode* next;
} ListNode;


typedef struct {
    ListNode* head;
    int size;
} Queue;


static inline int dequeue(Queue* q) {
    if(!q || q->size < 1) // Valdate inputs
        return -1;

    // Get leading value and remove head
    ListNode* head = q->head;
    q->head = head->next;
    int val = head->val;
    free(head);
    q->size--;

    return val;
}


static inline int enqueue(Queue* q, int val) {
    if(!q) // Validate input
        return 0;

    // Create new node
    ListNode* head = malloc(sizeof(ListNode));
    head->val = val;
    head->next = q->head;

    q->head = head;
    q->size++;

    return 1;
}


static inline int queueSize(Queue* q) {
    if(!q) return 0;
    return q->size;
}


#endif 