
#include <stdio.h>
#include <stdlib.h>

#include "Linked_queue.h"



LinkedQueue* createNode()
{
	 LinkedQueue* q;
	 q = (LinkedQueue*)malloc(sizeof(LinkedQueue));
	 memset(q, 0, sizeof(LinkedQueue));
	 q->front = NULL;
	 q->rear  = NULL;
	 q->length = 0;
	 return q;
}

int isEmpty (LinkedQueue* q)
{
 return (q->length == 0);
}

int getLength (LinkedQueue* q)
{
	if(q == NULL)
		return 0;
	return q->length;
}


void enqueue (LinkedQueue* q, Element* item)
{
 QueueNode* newNode;
 newNode = (QueueNode*)malloc(sizeof(QueueNode));
 memset(newNode , 0, sizeof(QueueNode));
 newNode->data = item;
 newNode->link = NULL;
 
 if (q->length == 0)
 {
 q->front = q->rear = newNode;
 }
 
 else
 {
 q->rear->link =newNode;
 q->rear = newNode;
 }
 q->length++;
 if(q->length % 10 == 0)
 	printf("Queue -> length %d\n", q->length);
}

Element* dequeue(LinkedQueue* q)
{
 QueueNode* temp;
 Element* item = NULL;
 
 if (q->length == 0)
 {
 
 printf("[[dequeue]] 1 Queue Length %d\n", q->length);
//  printf("Queue is empty\n");
//  exit(1);
	//memset(item,0,sizeof(Element));
	return NULL;
 }
 else
 	{
 	
 item = q->front->data;
 temp = q->front;
 q->front =q->front->link;

 if (q->front == NULL) q->rear = NULL;
 q->length--;
free(temp);
 return item;
 }
}


int getSizeOfFirstItem(LinkedQueue* q)
{
 QueueNode* temp;
 Element* item = NULL;
 
 if (q->length == 0)
 {
 
 printf("[[dequeue]] 1 Queue Length %d\n", q->length);
//  printf("Queue is empty\n");
//  exit(1);
	//memset(item,0,sizeof(Element));
	return NULL;
 }
 else
 	{

	 return q->front->data->size;
 }
}



void delete(LinkedQueue* q)
{
	 QueueNode* temp;
	 if (q->length ==0)
	 {
	  //printf("Queue is empty\n");
	  //exit(1);
		return ;
	 }

	 else
	 {
		 temp =q->front;
		 q->front = q->front->link;
		 if (q->front == NULL) q->rear =NULL;
		 q->length--;
		 free(temp);
	 }
}

Element* peek(LinkedQueue* q)
{
Element* item;
 if (q->length == 0)
 {
  //printf("Queue is empty\m");
  //exit(1);
	memset(item,0,sizeof(Element));
	return item;
 }

 else return q->front->data;
}


