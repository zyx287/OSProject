#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

/* Size of shared buffer */
#define BUF_SIZE 3

int buffer[BUF_SIZE];	/*shared buffer */
int add=0;		/* place to add next element */
int rem=0;		/* place to remove next element */
int num=0;		/* number elements in buffer */
int done=0;     /* flag to indicate when producer is done */
pthread_mutex_t m=PTHREAD_MUTEX_INITIALIZER;	/* mutex lock for buffer */
pthread_cond_t c_cons=PTHREAD_COND_INITIALIZER; /* consumer waits on this cond var */
pthread_cond_t c_prod=PTHREAD_COND_INITIALIZER; /* producer waits on this cond var */

void *threadA(void *arg);
void *threadBC(void *arg);
void *threadD(void *arg);

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        fprintf(stderr,"usage: main <pa> <pb> <pc>\n");
    }

    int pa = atoi(argv[1]);
    int pb = atoi(argv[2]);
    int pc = atoi(argv[3]);

    pthread_t tidA, tidB, tidC, tidD;

    // 创建线程A
    if (pthread_create(&tidA, NULL, threadA, &pa) != 0)
    {
        fprintf(stderr, "pthread_create for A failed\n");
        exit(1);
    }

    // 创建线程B和C
    if (pthread_create(&tidB, NULL, threadBC, &pb) != 0)
    {
        fprintf(stderr, "pthread_create for B failed\n");
        exit(1);
    }
    if (pthread_create(&tidC, NULL, threadBC, &pc) != 0)
    {
        fprintf(stderr, "pthread_create for C failed\n");
        exit(1);
    }

    // 创建线程D
    if (pthread_create(&tidD, NULL, threadD, &pa) != 0)
    {
        fprintf(stderr, "pthread_create for D failed\n");
        exit(1);
    }

    // 等待线程A结束
    if (pthread_join(tidA, NULL) != 0)
    {
        fprintf(stderr, "pthread_join for A failed\n");
        exit(1);
    }

    // 取消线程B和C
    if (pthread_cancel(tidB) != 0)
    {
        fprintf(stderr, "pthread_cancel for B failed\n");
        exit(1);
    }
    if (pthread_cancel(tidC) != 0)
    {
        fprintf(stderr, "pthread_cancel for C failed\n");
        exit(1);
    }

    // 等待线程D结束
    if (pthread_join(tidD, NULL) != 0)
    {
        fprintf(stderr, "pthread_join for D failed\n");
        exit(1);
    }

    // 输出结果
    printf("Thread B: %d, %d\n", buffer[1], buffer[2]);
    printf("Thread C: %d, %d\n", buffer[3], buffer[4]);
    printf("Thread D: %d, %d\n", buffer[5], buffer[6]);
    return 0;
}

void *threadA(void *arg)
{
    int pa = *((int*) arg);
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
		printf ("producer: inserted %d\n", i);  fflush (stdout);
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
    printf("Producer quitting\n"); fflush(stdout);
    return NULL;
}

void *threadBC(void *arg)
{
    int i;
    int p = *((int*) arg);
    int count = 0, sum = 0;
	while (1) {
		pthread_mutex_lock (&m);
		if (num < 0) exit(1);   /* underflow */
		while (num == 0 || buffer[rem] % p != 0)	 /* block if buffer empty */
			pthread_cond_wait (&c_cons, &m);
		/* if executing here, buffer not empty so remove element */
		i = buffer[rem];
		printf ("Consumer three: consumed value %d\n", i);  fflush(stdout);
		rem = (rem+1) % BUF_SIZE;
		num--;
        count++;
        sum += i;
		pthread_mutex_unlock (&m);
		pthread_cond_signal (&c_prod);
        usleep(20000);
		}
}

void *threadD(void *arg)
{
	int i;
	while (1) {
		pthread_mutex_lock (&m);
		if (num < 0) exit(1);   /* underflow */
		while (num == 0 || buffer[rem] % 3 == 0 || buffer[rem] % 5 == 0)	 /* block if buffer empty */
			pthread_cond_wait (&c_cons, &m);
		/* if executing here, buffer not empty so remove element */
		i = buffer[rem];
		printf ("Consumer other: consumed value %d\n", i);  fflush(stdout);
		rem = (rem+1) % BUF_SIZE;
		num--;
		pthread_mutex_unlock (&m);
		pthread_cond_signal (&c_prod);
		}
}


{
    int pa = *((int*) arg);
    int count = 0, sum = 0;
    while (1)
    {
        pthread_mutex_lock(&mutex);
        while (buffer[0] == 0)
            // Buffer is empty, wait until it has elements
            pthread_cond_wait(&empty, &mutex);
        int temp = buffer[buffer[0]--];
        if (temp % pa != 0)
        {
            count++;
            sum += temp;
        }
        pthread_cond_signal(&full);
        pthread_mutex_unlock(&mutex);

        usleep(10000);
    }
    return NULL;
}
我的需求是这个：
主进程从命令行接受三个参数pa,pb,pc(pa > pb > pc)。
其中，pa表示线程A产生的有序数组的长度，pb表示线程B的除数，pc表示线程C的除数
需要生成并运行四个线程，各线程的工作如下:
线程A每隔15毫秒按从1到pa顺序产生一个数，把它们放到长度不超过20的buffer数组中。如果buffer已满，则需要等待空闲时再插入:
线程B每隔20毫秒查询一次buffer，统计所有能被pb整除的数字的个数并求和;
线程C每隔15毫秒查询一次buffer，统计所有能被pc整除的数字的个数并求和；
线程D每隔10毫秒查询一次buffer，统计所有其他数字的个数并求和;
最后，主进程分别打印出进程B，C，D的数字个数与各自的求和。格式为Thread [id]，[count]，[sum]。例如:
Thread B，20，1059
Thread c，33，1683
Thread D，53，2632
使用Pthread实现上述功能，补全以下代码框架：
#include <stdio.h>
#include <pthread.h>
#define BUFFER SIZE 20
int buffer[BUFFER SIZE];
int main(int argc, char* argv[])(if (argc != 4)
fprintf(stderr,"usage: main <pa> <pb> <pc>\n");
void *threadA(...){}
void *threadBC(...){}
void*threadD(...){}
注意：线程B和线程C能够调用的函数接口均为threadBC；且需要考虑能同时被pb和pc整除的数需要同时被线程B和线程C读取到;线程B和线程C的等待时间不一样