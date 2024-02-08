/**
 * This file is the .c file for Programming Assignment #4: Synchronization
 * This was written by Michael Piscione
*/

/** Dependencies **/
// Include dependencies
#include <pthread.h>
#include <stdio.h>
#include "sync.mpiscion.h"

/**
 * insertItem inserts an element into a bounded queue, implemented as a circular buffer.
 * It takes an int element, void *param, and int *index as parameters. It first casts
 * the void *param to be a ProducerInfo pointer. Using this object, it checks if
 * the current number of items in the buffer is max'd out. If it is, it will return false, if not,
 * it will grab the element at a given index, pass that given index back through the index parameter,
 * increment the index (in a circular fashion), and, finally, it will increment the count
 * of the number of items in the buffer.
 * Parameters: int, the element to be placed in the queue, void *, the ProducerInfo object, int *,
 * the index parameter to pass the index back through by reference.
 * Return: 0 if buffer is full, 1 otherwise.
*/
int insertItem(int element, void *param, int *index) {
    // Cast param to be pointer to instance of ProducerInfo
    ProducerInfo *pInfo = (ProducerInfo *) param;

    // If the queue is full
    if (pInfo->bufferInfo->count == SHBUFLEN) {
        // Return false
        return 0;
    }
    else {  // If the queue isn't full
        // Set the given space in the buffer to be the element given
        pInfo->bufferInfo->buffer[pInfo->bufferInfo->in] = element;

        // Pass the value of out back through the index pointer
        *index = pInfo->bufferInfo->in;

        // Set the new index
        pInfo->bufferInfo->in = (pInfo->bufferInfo->in + 1) % SHBUFLEN;

        // Increment the item count
        pInfo->bufferInfo->count = pInfo->bufferInfo->count + 1;

        // Return true
        return 1;
    }
}

/**
 * removeItem removes an element from a bounded queue, implemented as a circular buffer.
 * It takes an int *element, void *param, and int *index as parameters. It first casts
 * the void *param to be a ConsumerInfo pointer. Using this object, it checks if
 * the current number of items in the buffer is none (no items to remove). If it is, it will return false, if not,
 * it will grab the element at a given index, pass that given index and element back through the index 
 * and element parameters respectively, increment the index (in a circular fashion), 
 * and, finally, it will decrement the count of the number of items in the buffer.
 * Parameters: int *, the parameter to pass the element removed back through by reference, void *, 
 * the ConsumerInfo object, int *, the index parameter to pass the index back through by reference.
 * Return: 0 if buffer is empty, 1 otherwise.
*/
int removeItem(int *element, void *param, int *index) {
    // Cast param to be pointer to instance of ConsumerInfo
    ConsumerInfo *cInfo = (ConsumerInfo *) param;

    // If there are no elements to remove
    if (cInfo->bufferInfo->count == 0) {
        return 0;  // Return false
    }
    else {  // If there are elements to remove
        // Set the element removed to be the element in the given place in the buffer
        *element = cInfo->bufferInfo->buffer[cInfo->bufferInfo->out];

        // Pass the value of out back through the index pointer
        *index = cInfo->bufferInfo->out;

        // Set the new index
        cInfo->bufferInfo->out = (cInfo->bufferInfo->out + 1) % SHBUFLEN;

        // Decrement the item count
        cInfo->bufferInfo->count = cInfo->bufferInfo->count - 1;

        // Return true
        return 1;
    }
}

/**
 * NOTE: This function is meant to be used with threads.
 * producer takes a single parameter, void *param. It casts it to be a ProducerInfo pointer.
 * It goes through a loop. At the top of the loop it checks if it is the last element to write (if it is, it writes -1),
 * if it isn't, it will attempt to insert another integer into the buffer. If it does, it increments its count
 * of the number of times it's written to the buffer. If a single item has been written, it then goes back through this loop.
 * This function employs a mutex lock to access the shared buffer and its corresponding information.
 * Parameters: void *, the parameter to be cast to a ProducerInfo pointer.
 * Return: n/a
*/
void *producer(void *param) {
    // Cast param to be pointer to instance of ProducerInfo
    ProducerInfo *pInfo = (ProducerInfo *) param;

    // Declare local variables
    int valueToWrite, written, index;

    // For i = 0 to NUM_TO_WRITE
    for (int i = 0; i <= NUM_TO_WRITE; i++) {
        if (i < NUM_TO_WRITE) {  // If not equal to NUM_TO_WRITE, haven't reached the end, write a number
            valueToWrite = i;
        }
        else {  // Equal to NUM_TO_WRITE, write something to signify end of buffer
            valueToWrite = -1;
        }

        // Set written to false
        written = 0;

        // While written is false
        while (!written) {
            // Critical section start
            pthread_mutex_lock(&pInfo->bufferInfo->lock);  // Acquire the lock

            // Try to put valueToWrite into the shared buffer
            if (insertItem(valueToWrite, pInfo, &index)) {
                // Print message that write was successful
                printf("producer writes %d at position %d from producer %d\n", valueToWrite, index, pInfo->myid);

                // Increment numWritten for this thread
                pInfo->numWritten++;

                // Set written to be true
                written = 1;
            }

            // Critical section end
            pthread_mutex_unlock(&pInfo->bufferInfo->lock);  // Release the lock
        }
    }

    // End the thread's execution
    printf("producer is exiting\n");
    pthread_exit(NULL);
}

/**
 * NOTE: This function is meant to be used with threads.
 * consumer takes a single parameter, void *param. It casts it to be a ConsumerInfo pointer.
 * It goes through a loop. At the top of the loop it checks if this thread is needed (i.e., if there is anything else to read),
 * if it is, it will attempt to read another integer in the buffer. If it does, and there are elements to be read, it grabs the value
 * and increments its counter of the number of times this thread has read an item. If it reads a -1, it signals to the other reading
 * threads that they should exit.
 * This function employs a mutex lock to access the shared buffer and its corresponding information.
 * Parameters: void *, the parameter to be cast to a ProducerInfo pointer.
 * Return: n/a
*/
void *consumer(void *param) {
    // Cast param to be pointer to instance of ConsumerInfo
    ConsumerInfo *cInfo = (ConsumerInfo *) param;

    // Declare local variables
    int done, gotValue, index;

    // Initially set done to false
    done = 0;

    // While not done
    while (!done) {
        // Set gotValue to false
        gotValue = 0;

        // While not gotValue and not done
        while (!gotValue && !done) {
            // Critical section start
            pthread_mutex_lock(&cInfo->bufferInfo->lock);  // Acquire the lock

            // Create local variable to hold removed value
            int readVal;

            // If active
            if (cInfo->bufferInfo->active) {
                // Try to read a value from the shared buffer
                if (removeItem(&readVal, cInfo, &index)) {
                    // Set gotValue to true
                    gotValue = 1;

                    // Print a message for a successful read
                    printf("consumer reads %d at position %d from consumer %d\n", readVal, index, cInfo->myid);

                    // Increment numRead
                    cInfo->numRead++;

                    // If value read was -1, set active to false
                    if (readVal == -1) {
                        cInfo->bufferInfo->active = 0;
                    }
                }
            }  
            // If not active
            if (!cInfo->bufferInfo->active) {
                // Set done to true
                done = 1;
            } 

            // Critical section end
            pthread_mutex_unlock(&cInfo->bufferInfo->lock);  // Release the lock
        }
    }

    // End the thread's execution
    printf("consumer is exiting\n");
    pthread_exit(NULL);
}

int main() {
    // Initialize instance of BufferInfo
    BufferInfo bInfo;
    bInfo.active = 1;
    bInfo.count = 0;
    bInfo.in = 0;
    bInfo.out = 0;

    // Initialize mutex lock
    pthread_mutex_init(&bInfo.lock, NULL);

    // Initialize instance of ProducerInfo
    ProducerInfo pInfo;
    pInfo.bufferInfo = &bInfo;
    pInfo.myid = 0;
    pInfo.numWritten = 0;

    // Initialize locations to store thread ids
    pthread_t producerTid;
    pthread_t consumerTid[NUM_CONSUMERS];

    // Create a producer thread
    pthread_create(&producerTid, NULL, producer, &pInfo);

    // Create consumer threads
    ConsumerInfo cInfos[NUM_CONSUMERS];
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        cInfos[i].bufferInfo = &bInfo;
        cInfos[i].myid = i;
        cInfos[i].numRead = 0;
        pthread_create(&consumerTid[i], NULL, consumer, &cInfos[i]);
    }

    // Join the threads
    int sum;
    pthread_join(producerTid, NULL);
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_join(consumerTid[i], NULL);
        sum += cInfos[i].numRead;
    }

    printf("Are they the same?? %d\n", pInfo.numWritten == sum);

    // Return 0 upon successful execution
    return 0;
}