#include<stdio.h>
#include<stdlib.h>
#include<ucontext.h>
#include<stdbool.h>
#include "mythread.h"
typedef struct lNode{
    void* threadData;// defining it as void as thread struct is not yet defined
    struct lNode* next;
}Lnode;

typedef struct qNode{
    void* threadData;
    struct qNode* next;
}Qnode;

typedef struct queue{
    Qnode *front, *rear;
    int size;
}Queue;

typedef struct list{
    int size;
    Lnode* head;
}List;

typedef struct thread {
	// Define any fields you might need inside here.
	ucontext_t* ctx;
    struct thread* p;
    Queue* children;
    bool parentBlocked;
    bool joined;
    bool joinedAll;
} Thread;

typedef struct semaphore{
    int value;
    Queue* semaBlocked;
}Semaphore;



Queue* createQueue() 
{ 
    Queue* q = (Queue*)malloc(sizeof(Queue)); 
    q->front = q->rear = NULL;
    q->size = 0;
    return q; 
} 

Qnode * newNode(Thread* t) 
{ 
    Qnode * temp = (Qnode *)malloc(sizeof(Qnode)); 
    temp->threadData = t; 
    temp->next = NULL; 
    return temp; 
} 
  
// The function to add a key k to q 
void enQueue(Queue * q, Thread * t) 
{ 
    // Create a new LL node 
    Qnode * temp = newNode(t); 
  
    // If queue is empty, then new node is front and rear both 
    if (q->rear == NULL) { 
        q->front = q->rear = temp;
        q->size = 1; 
        return; 
    } 
  
    // Add the new node at the end of queue and change rear 
    q->rear->next = temp; 
    q->rear = temp;
    q->size++; 
} 

Qnode* deQueue(Queue* q) 
{ 
    // If queue is empty, return NULL. 
    if (q->front == NULL) 
        return NULL; 
  
    // Store previous front and move front one node ahead 
    Qnode* temp = q->front; 
  
    q->front = q->front->next; 
  
    // If front becomes NULL, then change rear also as NULL 
    if (q->front == NULL) 
        q->rear = NULL; 
    
    q->size--;
    return temp; 
}

bool searchNode(Queue* q, Thread* t){
    Qnode* curr = q->front;
    while(curr != NULL){
        if((Thread*)curr->threadData == t){
            return true;
        }
        curr = curr->next;
    }
    return false;
}

Qnode* getParticularNode(Queue*  q, Thread* t ){
     if (q->front == NULL) 
        return NULL; 
    Qnode* temp = q->front;
    if((Thread*)temp->threadData == t && q->size == 1){
        q->front = q->rear = NULL;
        q->size--;
        return temp;
    }
    if((Thread*)temp->threadData == t){
        q->front = q->front->next;
        q->size--;
        return temp;
    }
    Qnode* result = NULL;
    while(temp->next != NULL && temp->next->next != NULL){
        if((Thread*)temp->next->threadData == t){
            result = temp->next;
            temp->next = temp->next->next;
            q->size--;
            break;
        }
    temp= temp->next;
    }
    if((Thread*)temp->next->threadData ==t){
        result = temp->next;
        temp->next = NULL;
        q->rear = temp;
        q->size--;
    }

    if(q->size == 1){
        q->front = q->rear;
    }
    return result;
}
/*---------------------------------------------------------------------------------------------------------------*/
List* createList(){
    List* l = (List*)malloc(sizeof(List));
    l->head = NULL;
    l->size = 0;
}

Lnode* createLnode(Thread* t){
    Lnode* node = (Lnode*)malloc(sizeof(Lnode));
    node->next = NULL;
    node->threadData = t;
    return node;
}

void enList(List* l, Thread* t){
    Lnode* node = createLnode(t);
    if(l->size == 0){
        l->head = node;
        l->size++;
        return;
    }
    Lnode* curr = l->head;
    while(curr != NULL){
        curr= curr->next;
    }
    curr = node;
    l->size++;
}

static Queue *readyQueue, *blockedQueue;
static Thread *root, *running;
static ucontext_t* buffer;
//buffer = (ucontext_t *)malloc(sizeof(ucontext_t));
// ****** THREAD OPERATIONS ****** 
// Create a new thread.
MyThread MyThreadCreate(void(*start_funct)(void *), void *args){
    Thread *t;
    if(root == NULL){
        printf("MyThreadInit needs to be called first");
        exit(1);
    }
    t = (Thread*)malloc(sizeof(Thread));
    ucontext_t* context = (ucontext_t *)malloc(sizeof(ucontext_t));
    t->ctx = context;
    t->p = running;
    getcontext(t->ctx);
    t->ctx->uc_stack.ss_sp = (char*) malloc(sizeof(char)*16384);
    t->ctx->uc_stack.ss_size = sizeof(char)*16384;
    makecontext(t->ctx,(void(*)())start_funct,1, args);
    enQueue(readyQueue,t);
    if(running->children == NULL){
        running->children = createQueue();
    }
    enQueue(running->children,t);
    return (MyThread)t;
}

// Yield invoking thread
void MyThreadYield(void){
    Thread * nextThread, * temp;
    
    if(readyQueue->size == 0){
        return;
    }
    else{
        Thread * nextThread = (Thread*)deQueue(readyQueue)->threadData;
        enQueue(readyQueue,running);
        temp = running;
        running = nextThread;
        swapcontext(temp->ctx,running->ctx);
    }
}

// Join with a child thread
int MyThreadJoin(MyThread thread){
    if(!searchNode(running->children,(Thread*)thread)){
        printf("Thread is not present in children queue \n");
        return -1;
    }
    Thread* blockingchild = (Thread*)thread;
    blockingchild->parentBlocked = true;
    Thread* nextThread = (Thread*)deQueue(readyQueue)->threadData;
    //Thread* nextThread = (Thread*)getParticularNode(running->children,(Thread*)thread)->threadData;
    running->joined= true;
    enQueue(blockedQueue,running);
    //nextThread->parentBlocked = true;
    Thread* temp = running;
    running = nextThread;
    swapcontext(temp->ctx,running->ctx);
}

// Join with all children
void MyThreadJoinAll(void){
    if(running->children->size ==0){
        printf("There are no active child for the thread");
        return;
    }
    Thread* nextThread = (Thread*)deQueue(readyQueue)->threadData;
    running->joinedAll = true;
    enQueue(blockedQueue,running);
    Thread* temp = running;
    running = nextThread;
    swapcontext(temp->ctx,running->ctx);
}

// Terminate invoking thread
void MyThreadExit(void){
    Thread *torun, *temp;
    if(root==running){
        while(readyQueue->size > 0){
            Thread * nextThread = (Thread*)deQueue(readyQueue)->threadData;
            enQueue(readyQueue,running);
            temp = running;
            running = nextThread;
            swapcontext(temp->ctx,running->ctx);
        }
        return;
         //exit(0);
    }else{
        getParticularNode(running->p->children, running);
        if(running->parentBlocked){
            enQueue(readyQueue,(Thread*)getParticularNode(blockedQueue, running->p)->threadData);
            running->p->joined = false;
        }
        if(running->p->joinedAll && running->p->children->size == 0){
            enQueue(readyQueue,(Thread*)getParticularNode(blockedQueue, running->p)->threadData);
            running->p->joinedAll = false;
        }
        Thread* temp = running;
        //free(temp);
        running = (Thread*)deQueue(readyQueue)->threadData;
        swapcontext(temp->ctx,running->ctx);
    }
   
}

// ****** SEMAPHORE OPERATIONS ****** 
// Create a semaphore
MySemaphore MySemaphoreInit(int initialValue){
    if(root == NULL){
        printf("MyThreadInit needs to be called first");
        exit(1);
    }
    if(initialValue<0){
        printf("Intial valve should be postive");
        exit(1);
    }
    Semaphore* sem = (Semaphore*)malloc(sizeof(Semaphore));
    sem->semaBlocked = createQueue();
    sem->value = initialValue;
    return (MySemaphore)sem;

}

// Signal a semaphore
void MySemaphoreSignal(MySemaphore sem){
    Semaphore* sema;
    sema = (Semaphore*)sem;
    sema->value++;
    if(sema->value <= 0){
        enQueue(readyQueue,(Thread*)deQueue(sema->semaBlocked)->threadData);
    }
}

// Wait on a semaphore
void MySemaphoreWait(MySemaphore sem){
    Semaphore* sema;
    sema = (Semaphore*)sem;
    sema->value--;
    if(sema->value < 0){
        enQueue(sema->semaBlocked, running);
        Thread* temp = running;
        running = (Thread*)deQueue(readyQueue)->threadData;
        swapcontext(temp->ctx,running->ctx);
    }
}

// Destroy on a semaphore
int MySemaphoreDestroy(MySemaphore sem){
    if(sem == NULL){
        printf("MySemaphoreInit needs to be called first");
        return -1;
    }
    Semaphore* s;
    s = (Semaphore*)sem;
    if(s->semaBlocked->size!=0){
        printf("Can not destory as there are threads blocked on semaphore");
        return -1;
    }
    if(s->semaBlocked->size==0){
        free(sem);
        return 0;
    }
    return -1;
}
// ****** CALLS ONLY FOR UNIX PROCESS ****** 
// Create and run the "main" thread
void MyThreadInit(void(*start_funct)(void *), void *args){
    readyQueue = createQueue();
    blockedQueue = createQueue();
    root = (Thread *) malloc(sizeof(Thread));
	ucontext_t* context = (ucontext_t*) malloc(sizeof(ucontext_t));
    ucontext_t* returnctx = (ucontext_t*) malloc(sizeof(ucontext_t));
	root->ctx = context;
    root->p= NULL;
    root->children = NULL;
    getcontext(root->ctx);
	root->ctx->uc_stack.ss_sp = (char*) malloc(sizeof(char) * 16384);
	root->ctx->uc_stack.ss_size = 16384;
    root->ctx->uc_link = returnctx;
    makecontext(root->ctx, (void (*)()) start_funct, 1, args);
    running = root;
    swapcontext(returnctx,root->ctx);
    //setcontext(root->ctx);
}


