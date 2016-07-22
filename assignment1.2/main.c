#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <sys/resource.h>
#include <stdio.h>
#include <unistd.h>
#define random(x) (rand()%x)
#define MSGLEN 85
#define QUEUELEN 11
#define MAXLEN 2000
#define MAXNUM 10
//using namespace std;
typedef struct msg{
    int seqnum,source,destination;
    char str[85];
}MSG;
pthread_mutex_t id_lck,rcv_lck;
int bus_in,bus_out1,bus_out2,bus_out3;
//QUEUE bus_in_q;
//QUEUE bus_out_q[3];
const int period_multiplier[] = {12, 22, 24, 26, 28, 15, 17, 19};
const int thread_priority[] = {14, 11, 16, 15, 9, 15, 12, 17};
//	const char*[] = ["/dev/bus_in","/dev/bus_out1","/dev/bus_out2","/dev/bus_out3"];
int bus_out_map[3];
int msg_id,rcv_len;
void enumstr(char *s,int len){
    for(int i=0;i<len;++i){
        int flag = rand() % 3;
        switch (flag){
            case 0:
                s[i] = 'A' + rand() % 26;
                break;
            case 1:
                s[i] = 'a' + rand() % 26;
                break;
            case 2:
                s[i] = '0' + rand() % 10;
                break;
            default:
                s[i] = 'x';
                break;
        }
    }
    s[len - 1] = '\0';
}
void init(){
    msg_id=1,rcv_len=0;
    srand((unsigned)time(NULL));
    bus_in = open("/dev/bus_in", O_RDWR);
    bus_out_map[0] =bus_out1 = open("/dev/bus_out1", O_RDWR);
    bus_out_map[1] =bus_out2 = open("/dev/bus_out2", O_RDWR);
    bus_out_map[2] =bus_out3 = open("/dev/bus_out3", O_RDWR);
	if (bus_in < 0 || bus_out1<0 ||bus_out2<0||bus_out3<0){
		printf("Can not open device file.\n");
		return;
	}
    pthread_mutex_init(&id_lck,NULL);
    pthread_mutex_init(&rcv_lck,NULL);
}
void quit(){
	int i=0;
	close(bus_in);
	for(i=0;i<3;++i) close(bus_out_map[i]);
	//pthread_mutex_destory(&id_lck);
	//pthread_mutex_destory(&rcv_lck);
}
void *send_thread(void *i){
    int tmp = *(int *)i;
    printf("thread id %d:",tmp+1);
    struct timespec next;
    //sprintf();
    clock_gettime(CLOCK_MONOTONIC, &next);
    setpriority(PRIO_PROCESS, getpid(), thread_priority[tmp]);
    while(rcv_len<MAXLEN){    
        pthread_mutex_lock(&id_lck);
        int tmp_id = msg_id;
        MSG m = {tmp_id,tmp+1,random(3)+1,""};
        int len = 10+random(70);
        enumstr(m.str,len);
        if(write(bus_in,&m,1)!=0){
            msg_id++;
            printf("sender : seqnum %d sender: %d destination %d\n",m.seqnum,m.source,m.destination);
        }
        pthread_mutex_unlock(&id_lck);
        next.tv_sec += period_multiplier[tmp] / 1000;
        next.tv_nsec += period_multiplier[tmp] * 100;
	    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next,0);
	    //printf("thread %d wakeup=============================\n",tmp+1);
    }
    printf("data is enough,sender quit\n");
    return NULL;
}

void *daemon_thread(void *ptr){
    struct timespec next;
    MSG buffer[15];
    //clock_gettime(CLOCK_MONOTONIC, &next);
    setpriority(PRIO_PROCESS,getpid(), thread_priority[4]);
    while(rcv_len < MAXLEN){
        clock_gettime(CLOCK_MONOTONIC, &next);
        int index=0;
        //while(sq_read(&bus_in_q,&buffer[index])!=-1 && index<10){
        while(read(bus_in,&buffer[index],1)!=0 && index<10){
            printf("daemon received: seqnum %d sender: %d destination %d\n",buffer[index].seqnum,buffer[index].source,buffer[index].destination);
            index++;
        }
        printf("daemon : bus_in empty!\n");

        for(int i=0;i<index;++i){
            MSG m = buffer[i];
            int des = m.destination-1;

            if(write(bus_out_map[des],&m,1)!=0){
                printf("daemon sent: #%d success!\n",m.seqnum);
            }
            else{
                printf("destination queue%d is full,drop%d\n",des+1,m.seqnum);
            }

        }
        if(rcv_len>=MAXLEN){
            printf("daemon complete!quit\n");
            break;
        }
        clock_gettime(CLOCK_MONOTONIC, &next);
        next.tv_sec+= period_multiplier[4] / 1000;
        next.tv_nsec+= period_multiplier[4] * 100;
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, 0);
    }
    return NULL;
}

void *receiver_thread(void *i){
    int tmp = *(int *)i;
    struct timespec next;
    setpriority(PRIO_PROCESS, getpid(), thread_priority[tmp+5]);
    //clock_gettime(CLOCK_MONOTONIC, &next);
    while(1){
        //printf("%d\n",tmp);
        pthread_mutex_lock(&rcv_lck);
        MSG m2;
        while(read(bus_out_map[tmp],&m2,1)!=0 && rcv_len<MAXLEN){
            printf("receiver%d:seqnum %d sender: %d destination %d\n",tmp+1,m2.seqnum,m2.source,m2.destination);
            printf("all received %d data===================\n",rcv_len);
            rcv_len+=strlen(m2.str);
        }
        if(rcv_len>=MAXLEN) {
            pthread_mutex_unlock(&rcv_lck);
            break;
        }
        pthread_mutex_unlock(&rcv_lck);
        printf("receiver%d:nothing to read\n",tmp+1);

        clock_gettime(CLOCK_MONOTONIC, &next);
        next.tv_sec += period_multiplier[tmp+5] / 1000;
        next.tv_nsec += period_multiplier[tmp+5] * 100;
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, 0);
    }
    printf("received %d data,is enough!==================\n",rcv_len);
    return NULL;
}
void run(){
	pthread_t sender[4],daemon,receiver[3];
	for(int i=0;i<4;i++){
        int *p = (int *)malloc(sizeof(int));
        *p = i;
        pthread_create(&sender[i],NULL,send_thread,p);
        usleep(10);
    }
    pthread_create(&daemon, NULL,daemon_thread,NULL);
    usleep(100);
    for(int i=0;i<3;i++){
        int *p = (int *)malloc(sizeof(int));
        *p = i;
        pthread_create(&receiver[i],NULL,receiver_thread,p);
        usleep(100);
    }
    pthread_join (sender[0], NULL);
    pthread_join (sender[1], NULL);
    pthread_join (sender[2], NULL);
    pthread_join (sender[3], NULL);
    pthread_join (daemon,NULL);
    pthread_join (receiver[0], NULL);
    pthread_join (receiver[1], NULL);
    pthread_join (receiver[2], NULL);

}
int main(){
    init();
    run();
    printf("finish\n");
    quit();
    return 0;
}