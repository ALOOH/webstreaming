#ifndef __LINKED_QUEUE_H
#define __LINKED_QUEUE_H
#define MAXSIZEDATA 2048*20
#define MAXQUEUECOUNT 20
typedef struct
{
	unsigned char data[MAXSIZEDATA];
	unsigned char* outPointer;
	unsigned char* inPointer;
	unsigned char* initialPointer;
	unsigned char* endPointer;
}SPIQ;

SPIQ* createSPIQ();
void putSPIQ(SPIQ* q,unsigned char d);	// MAXSIZEDATA를 넘어가면 처음부터 그냥 덮어쓴다
unsigned char getSPIQ(SPIQ* q,unsigned char* d);

typedef struct
{
 unsigned char *data;
 int size;
 //unsigned char data;
}Element;

typedef struct
{
 Element *data;
 struct QueueNode* link;
}QueueNode;

typedef struct
{
 QueueNode* front;
 QueueNode* rear;
 int length;
}LinkedQueue;

LinkedQueue* createNode();
int isEmpty (LinkedQueue* q);
void enqueue (LinkedQueue* q, Element* item);
Element* dequeue(LinkedQueue* q);
void delete(LinkedQueue* q);
Element* peek(LinkedQueue* q);
int getLength (LinkedQueue* q);
int getSizeOfFirstItem(LinkedQueue* q);



#endif //__LINKED_QUEUE_H

