#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#define BUF_SIZE 3

int buffer[BUF_SIZE];	/*shared buffer */
int add=0;		/* place to add next element */
int rem=0;		/* place to remove next element */
int num=0;		/* number elements in buffer */
int done=0;     /* flag to indicate when producer is done */
pthread_mutex_t m=PTHREAD_MUTEX_INITIALIZER;	/* mutex lock for buffer */
pthread_cond_t c_cons=PTHREAD_COND_INITIALIZER; /* consumer waits on this cond var */
pthread_cond_t c_prod=PTHREAD_COND_INITIALIZER; /* producer waits on this cond var */

void *producer(void *param);
void *consumer_dividable_by_three(void *param);
void *consumer_dividable_by_five(void *param);
void *consumer_other_numbers(void *param);

int main (int argc, char *argv[])
{
	pthread_t tid[4];		/* thread identifiers */
	int i;

	/* create the threads; may be any number, in general */
    if (pthread_create(&tid[0], NULL, producer, NULL) != 0) {
        fprintf(stderr, "Unable to create producer thread\n");
        exit(1);
    }
    if (pthread_create(&tid[1], NULL, consumer_dividable_by_three, NULL) != 0) {
        fprintf(stderr, "Unable to create consumer thread\n");
        exit(1);
    }
    if (pthread_create(&tid[2], NULL, consumer_dividable_by_five, NULL) != 0) {
        fprintf(stderr, "Unable to create consumer thread\n");
        exit(1);
    }
    if (pthread_create(&tid[3], NULL, consumer_other_numbers, NULL) != 0) {
        fprintf(stderr, "Unable to create consumer thread\n");
        exit(1);
    }
	/* wait for created thread to exit */
    for (int i = 0; i < 4; i++) {
        pthread_join(tid[i], NULL);
    }
	printf ("Parent quitting\n");
	return 0;
}

/* Produce value(s) */
void *producer(void *param)
{
	int i;
	for (i=1; i<=100; i++) {
		/* Insert into buffer */
		pthread_mutex_lock (&m);
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
        usleep(rand() % 191 + 10);            // 随机休眠一定时间
    }
    pthread_mutex_lock(&m);
    done = 1;   /* let everyone know we're done */
    pthread_mutex_unlock(&m);
    pthread_cond_broadcast(&c_cons);
    printf("Producer quitting\n"); fflush(stdout);
    return NULL;
}

/* Consume value(s); Note the consumer never terminates */
void *consumer_dividable_by_three(void *param)
{
    int i;
	while (1) {
		pthread_mutex_lock (&m);
		if (num < 0) exit(1);   /* underflow */
		while (num == 0 || buffer[rem] % 3 != 0)	 /* block if buffer empty */
			pthread_cond_wait (&c_cons, &m);
		/* if executing here, buffer not empty so remove element */
		i = buffer[rem];
		printf ("Consumer three: consumed value %d\n", i);  fflush(stdout);
		rem = (rem+1) % BUF_SIZE;
		num--;
		pthread_mutex_unlock (&m);
		pthread_cond_signal (&c_prod);
		}
}

void *consumer_dividable_by_five(void *param)
{

    int i;
	while (1) {
		pthread_mutex_lock (&m);
		if (num < 0) exit(1);   /* underflow */
		while (num == 0 || buffer[rem] % 5 != 0)	 /* block if buffer empty */
			pthread_cond_wait (&c_cons, &m);
		/* if executing here, buffer not empty so remove element */
		i = buffer[rem];
		printf ("Consumer five: consumed value %d\n", i);  fflush(stdout);
		rem = (rem+1) % BUF_SIZE;
		num--;
		pthread_mutex_unlock (&m);
		pthread_cond_signal (&c_prod);
		}
}

void *consumer_other_numbers(void *param)
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
