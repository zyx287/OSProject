#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#define BUF_SIZE 20

int buffer[BUF_SIZE];   /*shared buffer */
int add=0;          /* place to add next element */
int rem=0;          /* place to remove next element */
int num=0;          /* number elements in buffer */
int done=0;         /* flag to indicate when producer is done */
int countB = 0, sumB = 0;
int countC = 0, sumC = 0;
int countD = 0, sumD = 0;
int pa,pb,pc;
pthread_mutex_t m=PTHREAD_MUTEX_INITIALIZER;  /* mutex lock for buffer */
pthread_cond_t c_cons=PTHREAD_COND_INITIALIZER; /* consumer waits on this cond var */
pthread_cond_t c_prod=PTHREAD_COND_INITIALIZER; /* producer waits on this cond var */

void *threadA(void *arg);
void *threadBC(void *arg);
void *threadD(void *arg);

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr,"usage: main <pa> <pb> <pc>\n");
        return 1;
    }
    pa = atoi(argv[1]);
    pb = atoi(argv[2]); // pb是全局变量
    pc = atoi(argv[3]); // pc是全局变量

    pthread_t threadA_id, threadB_id, threadC_id, threadD_id;
    pthread_create(&threadA_id, NULL, threadA, NULL);
    pthread_create(&threadB_id, NULL, threadBC, &pb);
    pthread_create(&threadC_id, NULL, threadBC, &pc);
    pthread_create(&threadD_id, NULL, threadD, NULL);
    pthread_join(threadA_id, NULL);
    pthread_join(threadB_id, NULL);
    pthread_join(threadC_id, NULL);
    pthread_join(threadD_id, NULL);
    printf("Thread B, %d, %d\n", countB, sumB);
    printf("Thread C, %d, %d\n", countC, sumC);
    printf("Thread D, %d, %d\n", countD, sumD);
    return 0;
}

void *threadA(void *arg)
{
    for (int i = 1; i <= pa; i++) {
        pthread_mutex_lock(&m);
        while (num == BUF_SIZE) {    /* block if buffer is full */
            if (done) {
                pthread_mutex_unlock(&m);
                return NULL;
            }
            pthread_cond_wait (&c_prod, &m);
        }
        buffer[add] = i;
        add = (add+1) % BUF_SIZE;
        num++;
        pthread_mutex_unlock (&m);
        pthread_cond_broadcast (&c_cons);    /* wake up all consumers */
        usleep(15000);            // 随机休眠一定时间
    }
    pthread_mutex_lock(&m);
    done = 1;   /* let everyone know we're done */
    pthread_mutex_unlock(&m);
    pthread_cond_broadcast(&c_cons);
    return NULL;
}

void *threadBC(void *arg)
{
    int i;
    int p = *((int*) arg);
    int q = (p==pb ? pc : pb);
    int count = 0, sum = 0;
    while (1) {
        pthread_mutex_lock (&m);
        if (num < 0 || (num == 0 && done)) {
            pthread_mutex_unlock(&m);
            return NULL;
        }
        while (num == 0 || buffer[rem] % p != 0) { /* block if buffer empty */
            if (done) {
                pthread_mutex_unlock(&m);
                return NULL;
            }
            pthread_cond_wait(&c_cons, &m);
        }
        /* if executing here, buffer not empty so remove element */
        i = buffer[rem];
        if (p==pb){
            countB++;
            sumB+=i;
            if (i % q == 0){
                countC++;
                sumC+=i;
            }
        }
        if (p==pc){
            countC++;
            sumC+=i;
            if (i % q == 0){
                countB++;
                sumB+=i;
            }
        }
        rem = (rem+1) % BUF_SIZE;
        num--;
        count++;
        sum += i;
        pthread_mutex_unlock (&m);
        pthread_cond_signal (&c_prod);
        usleep(p == pb ? 20000 : 15000);
    }
}

void *threadD(void *arg)
{
    int i;
    while (1) {
        pthread_mutex_lock (&m);
        if (num < 0 || (num == 0 && done)) {
            pthread_mutex_unlock(&m);
            return NULL;
        }
        while (num == 0 || buffer[rem] % 3 == 0 || buffer[rem] % 5 == 0) { /* block if buffer empty */
            if (done) {
                pthread_mutex_unlock(&m);
                return NULL;
            }
            pthread_cond_wait(&c_cons, &m);
        }
        /* if executing here, buffer not empty so remove element */
        i = buffer[rem];
        countD++;
        sumD+=i;
        rem = (rem+1) % BUF_SIZE;
        num--;
        pthread_mutex_unlock (&m);
        pthread_cond_signal (&c_prod);
        usleep(10000);
    }
}
