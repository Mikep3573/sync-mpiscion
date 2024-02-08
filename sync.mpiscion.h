/**
 * This file is the .h file for Programming Assignment #4: Synchronization
 * This was written by Michael Piscione
*/

/** Constants **/
#define SHBUFLEN 10  // Shared Buffer Length
#define NUM_CONSUMERS 2  // Number of consumers
#define NUM_TO_WRITE 10  // Number of characters to write

/** Structs **/
// Buffer Information
typedef struct {
    int buffer[SHBUFLEN];       // The buffer
    int in;                     // location to which a producer will write
    int out;                    // location from which a consumer will read
    int count;                  // total # items in the buffer
    int active;                 // whether consumers are still active
    pthread_mutex_t lock;       // the lock
} BufferInfo;

// Producer Thread Information
typedef struct {
    int myid;                   // id of the producer
    int numWritten;             // total number of characters written by this producer
    BufferInfo *bufferInfo;     // pointer to an instance of the buffer information struct
} ProducerInfo;

// Consumer Thread Information
typedef struct {
    int myid;                   // id of the consumer
    int numRead;                // total number of characters read by this consumer
    BufferInfo *bufferInfo;     // pointer to an instance of the buffer information struct
} ConsumerInfo;

/** Function Declarations **/
// Producer thread function
void *producer(void *);

// Consumer thread function
void *consumer(void *);
