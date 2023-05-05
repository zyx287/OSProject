#include <stdio.h>
#include <pthread.h>

#define BUFFER_SIZE 20

int buffer[BUFFER_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

void *threadA(void *arg)
{
    int pa = *((int*) arg);
    for (int i = 1; i <= pa; i++)
    {
        pthread_mutex_lock(&mutex);
        while (buffer[0] != 0 && BUFFER_SIZE - buffer[0] < 1)
            // Buffer is full, wait until it becomes available
            pthread_cond_wait(&full, &mutex);
        if (buffer[0] == 0)
        {
            // Buffer is empty, add the first element
            buffer[++buffer[0]] = i;
            pthread_cond_signal(&empty);
        }
        else
            buffer[++buffer[0]] = i;
        pthread_mutex_unlock(&mutex);

        usleep(15000);
    }
    return NULL;
}

void *threadBC(void *arg)
{
    int p = *((int*) arg);
    int count = 0, sum = 0;
    while (1)
    {
        pthread_mu tex_lock(&mutex);
        while (buffer[0] == 0)
            // Buffer is empty, wait until it has elements
            pthread_cond_wait(&empty, &mutex);
        int temp = buffer[buffer[0]--];
        if (temp % p == 0)
        {
            count++;
            sum += temp;
        }
        pthread_cond_signal(&full);
        pthread_mutex_unlock(&mutex);

        usleep(20000);
    }
    return NULL;
}

void *threadD(void *arg)
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

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        fprintf(stderr,"usage: main <pa> <pb> <pc>\n");
        return -1;
    }

    int pa = atoi(argv[1]);
    int pb = atoi(argv[2]);
    int pc = atoi(argv[3]);

    pthread_t tidA, tidB, tidC, tidD;

    // 创建线程A
    if (pthread_create(&tidA, NULL, threadA, &pa) != 0)
    {
        fprintf(stderr, "pthread_create for A failed\n");
        return -1;
    }

    // 创建线程B和C
    if (pthread_create(&tidB, NULL, threadBC, &pb) != 0)
    {
        fprintf(stderr, "pthread_create for B failed\n");
        return -1;
    }
    if (pthread_create(&tidC, NULL, threadBC, &pc) != 0)
    {
        fprintf(stderr, "pthread_create for C failed\n");
        return -1;
    }

    // 创建线程D
    if (pthread_create(&tidD, NULL, threadD, &pa) != 0)
    {
        fprintf(stderr, "pthread_create for D failed\n");
        return -1;
    }

    // 等待线程A结束
    if (pthread_join(tidA, NULL) != 0)
    {
        fprintf(stderr, "pthread_join for A failed\n");
        return -1;
    }

    // 取消线程B和C
    if (pthread_cancel(tidB) != 0)
    {
        fprintf(stderr, "pthread_cancel for B failed\n");
        return -1;
    }
    if (pthread_cancel(tidC) != 0)
    {
        fprintf(stderr, "pthread_cancel for C failed\n");
        return -1;
    }

    // 等待线程D结束
    if (pthread_join(tidD, NULL) != 0)
    {
        fprintf(stderr, "pthread_join for D failed\n");
        return -1;
    }

    // 输出结果
    printf("Thread B: %d, %d\n", buffer[1], buffer[2]);
    printf("Thread C: %d, %d\n", buffer[3], buffer[4]);
    printf("Thread D: %d, %d\n", buffer[5], buffer[6]);
    return 0;
}
##################################################################
#include <stdio.h>
#include <pthread.h>
#define BUFFER_SIZE 20

int buffer[BUFFER_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;

// 定义两组变量，用于存储线程B和线程C的计数和求和
int countB = 0, sumB = 0;
int countC = 0, sumC = 0;
int countD = 0, sumD = 0;

void *threadA(void *arg) {
    int pa = *(int*)arg;
    for (int i = 1; i <= pa; i++) {
        // wait until buffer has free space
        pthread_mutex_lock(&mutex);
        while (countD >= BUFFER_SIZE) {
            pthread_cond_wait(&empty, &mutex);
        }
        // insert new number to buffer
        buffer[countD] = i;
        countD++;
        printf("Thread A produced %d\n", i);
        // signal buffer has been filled
        pthread_cond_signal(&full);
        pthread_mutex_unlock(&mutex);
        usleep(15000);
    }
    return NULL;
}

void *threadBC(void *arg) {
    int divisor = *(int*)arg;
    while (1) {
        // wait until buffer has data
        pthread_mutex_lock(&mutex);
        while (countD == 0) {
            pthread_cond_wait(&full, &mutex);
        }
        // check buffer for numbers divisible by divisor or pb and pc
        int count = 0, sum = 0;
        for (int i = 0; i < countD; i++) {
            if (buffer[i] % divisor == 0 || (buffer[i] % pb == 0 && buffer[i] % pc == 0)) { // 修改内容
                count++;
                sum += buffer[i];
                // remove number from buffer
                buffer[i] = buffer[countD-1];
                countD--;
                i--;
            }
        }
        // signal buffer has free space
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&mutex);
        // update count and sum
        if (divisor == pb) {
            countB = count;
            sumB = sum;
        } else if (divisor == pc) {
            countC = count;
            sumC = sum;
        }
        usleep(divisor == pb ? 20000 : 15000);
    }
    return NULL;
}

void *threadD(void *arg) {
    while (1) {
        // wait until buffer has data
        pthread_mutex_lock(&mutex);
        while (countD == 0) {
            pthread_cond_wait(&full, &mutex);
        }
        // check buffer for numbers not divisible by pb or pc
        int count = 0, sum = 0;
        for (int i = 0; i < countD; i++) {
            if (buffer[i] % pb != 0 && buffer[i] % pc != 0) {
                count++;
                sum += buffer[i];
                //remove number from buffer
                buffer[i] = buffer[countD-1];
                countD--;
                i--;
            }
        }
        // signal buffer has free space
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&mutex);
        // update count and sum
        countD = 0;
        sumD = sum;
        usleep(10000);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr,"usage: main <pa> <pb> <pc>\n");
        return 1;
    }
    int pa = atoi(argv[1]);
    pb = atoi(argv[2]); // pb是全局变量
    pc = atoi(argv[3]); // pc是全局变量

    pthread_t threadA_id, threadB_id, threadC_id, threadD_id;
    pthread_create(&threadA_id, NULL, threadA, &pa);
    pthread_create(&threadB_id, NULL, threadBC, &pb);
    pthread_create(&threadC_id, NULL, threadBC, &pc);
    pthread_create(&threadD_id, NULL, threadD, NULL);

    while (1) {
        printf("Thread B, %d, %d\n", countB, sumB);
        printf("Thread C, %d, %d\n", countC, sumC);
        printf("Thread D, %d, %d\n", countD, sumD);
        usleep(1000000);
    }

    return 0;
}
