#ifndef MYQUEUE_INCLUDED
#define MYQUEUE_INCLUDED
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <pthread.h>
#define random(x) (rand()%x)
#define MSGLEN 85
#define QUEUELEN 11
using namespace std;

typedef struct msg{
    int seqnum;
    int source,destination;
    char *str;
}MSG;
typedef struct queue{
    MSG *base;
    pthread_mutex_t lck;
    int qfront,qrear;
    int maxsize;
}QUEUE,*PQUEUE;

void sq_create(PQUEUE q,int maxsize);
int  sq_write(PQUEUE q,MSG *value);
int  sq_read(PQUEUE q,MSG value);
void sq_delete(PQUEUE q);

void sq_create(PQUEUE q,int maxs){
    q->base = (MSG *)malloc(sizeof(MSG)*maxs);
    if (NULL == q->base){
        printf("Malloc Error!\n");
        return;
    }
    q->qfront = q->qrear = 0;
    q->maxsize = maxs;
    pthread_mutex_init(&(q->lck),NULL);
}
int sq_write(PQUEUE q,MSG value){
    int ret = pthread_mutex_lock(&(q->lck));
    if(ret){
        printf("Thread lock failed\n");
        pthread_exit(NULL);
    }
    int tmp2 = (q->qrear+1)%q->maxsize;
    if(q->qfront == tmp2){
        printf("queue is full\n");
        pthread_mutex_unlock(&(q->lck));
        return -1;
    }
    q->base[q->qrear] = value;
    q->qrear = (q->qrear+1)%q->maxsize;
    pthread_mutex_unlock(&(q->lck));
    return 1;
}
int sq_read(PQUEUE q,MSG *value){
    int ret = pthread_mutex_lock(&(q->lck));
    if(ret){
        printf("Thread lock failed\n");
        pthread_exit(NULL);
    }
    if(q->qfront == q->qrear){
        //printf("queue is empty!\n");
        pthread_mutex_unlock(&(q->lck));
        return -1;
    }
    *value = q->base[q->qfront];
    q->qfront = (q->qfront +1)%q->maxsize;
    pthread_mutex_unlock(&(q->lck));
    return 1;
}
void sq_delete(PQUEUE q){
    free(q->base);
    q->qfront = q->qrear = 0;
    q->maxsize = 0;
    pthread_mutex_init(&(q->lck),NULL);
}

#endif // MYQUEUE_INCLUDED
