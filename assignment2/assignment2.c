#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/resource.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/syscall.h>

const int HIGHSET_PRIORITY = 50;
pthread_mutex_t lck1,lck2;
const int thread_priority[] = {20, 50, 40, 30, 30, 30, 30, 10};
const int periodic_thread_iteration[3][3] = {{10,900,30},{400,500,100},{200,800,100}};
const int thread_period[] = {200,300,500};  //线程周期
int count[3] = {0,0,0};         //判断是否在deadline前截止
struct itimerval t;
pthread_mutex_t time_lck;
cpu_set_t cpuset;               //设置单核
sem_t sem1,sem2,sem_start;
pthread_t thread_p[3],thread_ap[2],getc_thread;

void sighand(int sig);          //周期性线程deadline时钟处理
void init();
void thread_init(pthread_t *id,struct sched_param *param,pthread_attr_t *attr,int priority);
void quit();
void *periodic_thread(void *i);     //周期性线程
void *aperiodic_thread(void *i);    //非周期性线程
void *keyboard_monitor(void *i);    //键盘监听线程
int main(){
    init();
    int i=0,ret;
    for(i=0;i<3;++i){
        int *p = (int *)malloc(sizeof(int));
        *p = i;
        struct sched_param *param = (struct sched_param *)malloc(sizeof(struct sched_param));
        pthread_attr_t *attr = (pthread_attr_t *)malloc(sizeof(pthread_attr_t));
        int priority = thread_priority[i];
        thread_init(&thread_p[i],param,attr,priority);
        ret = pthread_create(&thread_p[i],attr,periodic_thread,p);
        //printf("thread id %lu\n",(unsigned long int)thread_p[i]);
        if (ret!=0){
            printf("pthread_create\n%s\n", strerror(ret));
            exit(1);
        }
    }
    pthread_create(&getc_thread,NULL,keyboard_monitor,NULL);
    for(i=0;i<2;++i){
        int *p = (int *)malloc(sizeof(int));
        *p = i;
        pthread_create(&thread_ap[i],NULL,aperiodic_thread,p);
    }
    printf("Start all threads\n");
    for(i=0;i<6;++i)
        sem_post(&sem_start);       //start all thread
    sleep(1);
    quit();
    return 0;
}
void sighand(int sig){
//  处理线程timer的信号，判断是否deadline前截止
    pthread_t tid = pthread_self();
    int policy;
    int i,index=1e5;
    unsigned long tmp = (unsigned long) tid;
    unsigned long target;
    struct sched_param param;
    for(i=0;i<3;++i){
        target = (unsigned long)thread_p[i];
        if(tmp == target){
            index = i;
        }
    }
    if(index<1e4){
        printf("thread %d's deadline\n",index+1);
        int sum = periodic_thread_iteration[index][0]+periodic_thread_iteration[index][1]+periodic_thread_iteration[index][2];
        if(count[index]<sum){
            printf("counter value %d,should be %d\n",count[index],sum);
            pthread_getschedparam(pthread_self(), &policy, &param);
            printf("thread %d's priority = %d\n",index+1,param.__sched_priority);
            if(param.__sched_priority>10){
                param.__sched_priority = param.__sched_priority-5;
                int ret = pthread_setschedparam(pthread_self(),policy,&param);
                if (ret!=0){
                    printf(" pthread_attr_setschedparam/n%s/n", strerror(ret));
                    exit(1);
                }
                printf("Thread %d's priority decreased\n",index+1);
                pthread_getschedparam(pthread_self(), &policy, &param);
            }
        }
        else{
            printf("thread %d success in this period\n",index+1);
        }
    }
    return;
}
void init(){
//init
    pthread_mutex_init(&lck1,NULL);
    pthread_mutex_init(&lck2,NULL);
    CPU_ZERO(&cpuset);
    CPU_SET(1,&cpuset);
    sem_init(&sem1,0,0);
    sem_init(&sem2,0,0);
    sem_init(&sem_start,0,0);
    pthread_mutex_init(&time_lck,NULL);
}
void quit(){
    pthread_mutex_destroy(&lck1);
    pthread_mutex_destroy(&lck2);
    pthread_mutex_destroy(&time_lck);
    //CPU_FREE(&cpuset);
    sem_destroy(&sem1);
    sem_destroy(&sem2);
    sem_destroy(&sem_start);
}
void thread_init(pthread_t *id,struct sched_param *param,pthread_attr_t *attr,int priority){
//init a periodic_thread
    int ret,inher,policy;
    pthread_attr_init(attr);
    ret = pthread_attr_getinheritsched(attr, &inher);
    inher = PTHREAD_EXPLICIT_SCHED;
    ret = pthread_attr_setinheritsched(attr, inher);
    if (ret!=0){
        printf("pthread_attr_setinheritsched/n%s/n", strerror(ret));
        exit(1);
    }
    policy = SCHED_FIFO;        //need root
    ret = pthread_attr_setschedpolicy(attr, policy);
    if (ret!=0){
        printf(" pthread_attr_setschedpolicy\n%s\n", strerror(ret));
        exit(1);
    }
    (*param).sched_priority = priority;
    ret = pthread_attr_setschedparam(attr,param);
    if (ret!=0){
        printf(" pthread_attr_setschedparam/n%s/n", strerror(ret));
        exit(1);
    }
}

void *periodic_thread(void *i){
//periodic thread
    int tmp = *(int *)i;
    int j=0;
    timer_t timerid;
    struct itimerspec its;
    struct sigevent sev;
    sem_wait(&sem_start);
    printf("thread id %d on:\n",tmp+1);

    struct sigaction actions;
    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask); /* 将参数set信号集初始化并清空 */
    actions.sa_flags = 0;
    actions.sa_handler = sighand;
    sigaction(SIGALRM,&actions,NULL);

    sev.sigev_notify = SIGEV_THREAD_ID;
    sev.sigev_signo = SIGALRM;
    sev.sigev_value.sival_ptr = &timerid;
    sev._sigev_un._tid = syscall(SYS_gettid);
    timer_create(CLOCK_REALTIME, &sev, &timerid);
    struct timespec next;
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t),&cpuset) < 0){
        printf("set thread affinity failed\n");
    }
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 3*thread_period[tmp] * 10;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = thread_period[tmp] * 1e6;
    //printf("thread %d in loop\n",tmp);
    clock_gettime(CLOCK_MONOTONIC, &next);
    while(1){
        timer_settime(timerid, 0, &its, NULL);
        //printf("thread %d running\n",tmp);
        count[tmp] = 0;
        for(j=0;j<periodic_thread_iteration[tmp][0];++j){
            count[tmp]++;
        }
        //printf("before lock\n");
        pthread_mutex_lock(&lck1);
        for(j=0;j<periodic_thread_iteration[tmp][1];++j){
            count[tmp]++;
        }
        //printf("in lock\n");
        pthread_mutex_unlock(&lck1);
        for(j=0;j<periodic_thread_iteration[tmp][2];++j){
            count[tmp]++;
        }
        //printf("after lock\n");
        clock_gettime(CLOCK_MONOTONIC, &next);
        next.tv_sec += 0;
        next.tv_nsec += thread_period[tmp] * 1000000;
	    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next,0);
    }
    printf("Thread %d quit!",tmp);
}
void *aperiodic_thread(void *i){
//aperiodic_thread
    int tmp = *(int *)i;
    int j=0;
    sem_wait(&sem_start);
    printf("thread id %d on:\n",tmp+4);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t),&cpuset) < 0){
        printf("set thread affinity failed\n");
    }
    setpriority(PRIO_PROCESS, getpid(), thread_priority[tmp+4]);
    while(1){
        if(tmp){
            sem_wait(&sem2);
        }
        else{
            sem_wait(&sem1);
        }
        printf("Getting the key\n");
        printf("in thread %d\n",tmp+4);
        for(j=0;j<500;++j){
            ;
        }
    }
}
void *keyboard_monitor(void *i){
//监听键盘
    sem_wait(&sem_start);
    printf("thread keyboard_monitor on:\n");
    struct timespec next;
    char c;
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t),&cpuset) < 0){
        printf("set thread affinity failed\n");
    }
    clock_gettime(CLOCK_MONOTONIC, &next);
    setpriority(PRIO_PROCESS, getpid(),HIGHSET_PRIORITY);
    while(1){
        c = getchar();
        printf("%c\n",c);
        switch(c){
            case '1':
                sem_post(&sem1);
                break;
            case '2':
                sem_post(&sem2);
                break;
        }
    }
}
