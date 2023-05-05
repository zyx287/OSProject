#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#define BUF_SIZE 20// Size of buffer is 20

int buffer[BUF_SIZE];      // Shared Buffer
int add=0;                 // Position for the next added element
int rem=0;                 // Position for the next pop out element
int num=0;                 // Numbers of the elements in Buffer
int done=0;                // Flag of thread done
int countB = 0, sumB = 0;  // Count/Sum num in Thread B
int countC = 0, sumC = 0;  // Count/Sum num in Thread C
int countD = 0, sumD = 0;  // Count/Sum num in Thread D
int pa, pb, pc;            // Parameters of Thread ABCD; Set as a global variable

pthread_mutex_t m=PTHREAD_MUTEX_INITIALIZER;        // Create and initialize the mutex
pthread_cond_t c_cons=PTHREAD_COND_INITIALIZER;    // Create conditional variable, consumers wait for this signal
pthread_cond_t c_prod=PTHREAD_COND_INITIALIZER;    // Create conditional variable, producer wait for this signal

void *threadA(void *arg);   // Thread A
void *threadBC(void *arg);  // Thread B and C
void *threadD(void *arg);   // Thread D

int main(int argc, char* argv[]) {

    if (argc != 4) {   // Check the number of input parameters
        fprintf(stderr,"usage: main <pa> <pb> <pc>\n");
        return 1;
    }
    // Read
    pa = atoi(argv[1]);
    pb = atoi(argv[2]);
    pc = atoi(argv[3]);

    pthread_t threadA_id, threadB_id, threadC_id, threadD_id;  // Thread ID
    // Create 4 threads, propagate the corresponding parameters
    pthread_create(&threadA_id, NULL, threadA, NULL);
    pthread_create(&threadB_id, NULL, threadBC, &pb);
    pthread_create(&threadC_id, NULL, threadBC, &pc);
    pthread_create(&threadD_id, NULL, threadD, NULL);
    // Ended all threads
    pthread_join(threadA_id, NULL);
    pthread_join(threadB_id, NULL);
    pthread_join(threadC_id, NULL);
    pthread_join(threadD_id, NULL);
    // Return the result of different threads
    printf("Thread B, %d, %d\n", countB, sumB);
    printf("Thread C, %d, %d\n", countC, sumC);
    printf("Thread D, %d, %d\n", countD, sumD);
    return 0;
}

void *threadA(void *arg)
{
    for (int i = 1; i <= pa; i++) {          // Generate 1, 2, 3, ... , pa
        pthread_mutex_lock(&m);              // Lock
        while (num == BUF_SIZE) {            // If the buffer is full, wait
            if (done) {
                pthread_mutex_unlock(&m);    // If the producer is finished, unlock and exit
                return NULL;
            }
            pthread_cond_wait (&c_prod, &m);  // Wait for the signal of the producer and release the mutex m
        }
        buffer[add] = i;                     // Add the element to the buffer
        add = (add+1) % BUF_SIZE;            // Update the addition position
        num++;                               // Increase the number of elements
        pthread_mutex_unlock (&m);           // Unlock
        pthread_cond_broadcast (&c_cons);    // Wake up the consumer (broadcast)
        usleep(15000);                       // Sleep for 15 ms
    }
    pthread_mutex_lock(&m);                   // Producer operation ends. Lock and set done flag
    done = 1;                                 // Set the done flag to indicate that the producer has finished
    pthread_mutex_unlock(&m);                 // Unlock
    pthread_cond_broadcast(&c_cons);          // Broadcast to the consumer
    return NULL;
}

void *threadBC(void *arg)
{
    int i;
    int p = *((int*) arg);          // Get the parameter
    int q = (p==pb ? pc : pb);      // Get the reverse parameter in a same thread
    int count = 0, sum = 0;
    if (p==pb) {          // Update countB, sumB, countC and sumC according to the parameter values. In case of the number could divided by both pb and pc
        while (1) {
        pthread_mutex_lock (&m);
        if (num < 0 || (num == 0 && done)) {    // If the buffer is empty and the producer operation is completed, unlock and exit the thread
            pthread_mutex_unlock(&m);
            return NULL;
        }
        while (num == 0 || buffer[rem] % p != 0) { // If the buffer is empty or the next element to be removed does not meet the requirements, wait
            if (done) {
                pthread_mutex_unlock(&m);   // If the producer is finished, unlock and exit the thread
                return NULL;
            }
            pthread_cond_wait(&c_cons, &m);    // Wait for the signal of the consumer and release m
        }
        i = buffer[rem];     // Remove the next element that meets the requirement from the buffer
        countB++;
        sumB+=i;
        if (i % q == 0){
            countC++;
            sumC+=i;
        }
        rem = (rem+1) % BUF_SIZE;   // Update the next element position to be removed
        num--;                      // Reduce the number of elements in the buffer
        pthread_mutex_unlock (&m);  // Unlock the mutex
        pthread_cond_signal (&c_prod);  // Wake up the producer
        usleep(20000);
        }
    }
    if (p==pc) {
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
        i = buffer[rem];
        countC++;
        sumC+=i;
        if (i % q == 0){
            countB++;
            sumB+=i;
        }
        rem = (rem+1) % BUF_SIZE;   // Update the next element position to be removed
        num--;                      // Reduce the number of elements in the buffer
        pthread_mutex_unlock (&m);  // Unlock the mutex
        pthread_cond_signal (&c_prod);  // Wake up the producer
        usleep(15000);
        }
    }
}

void *threadD(void *arg) 
{
    int i;
    while (1) {
        pthread_mutex_lock (&m);  // Lock
        if (num < 0 || (num == 0 && done)) {  // If the buffer is empty and the producer operation is completed, unlock and exit the thread
            pthread_mutex_unlock(&m);
            return NULL;
        }
        while (num == 0 || buffer[rem] % pb == 0 || buffer[rem] % pc == 0) { // If the buffer is empty or the next element to be removed does not meet the requirements, wait
            if (done) {
                pthread_mutex_unlock(&m);   // If the producer is finished, unlock and exit thread
                return NULL;
            }
            pthread_cond_wait(&c_cons, &m); // Wait for the signal of the consumer and release m
        }
        i = buffer[rem];    // Remove the next element that meets the requirement from the buffer
        countD++;           
        sumD+=i;            
        rem = (rem+1) % BUF_SIZE;  // Update the next element position to be removed
        num--;               // Reduce the number of elements in the buffer
        pthread_mutex_unlock (&m);  // Unlock the mutex
        pthread_cond_signal (&c_prod);  // Wake up the producer
        usleep(10000);  // Sleep for 10 ms
    }
}
