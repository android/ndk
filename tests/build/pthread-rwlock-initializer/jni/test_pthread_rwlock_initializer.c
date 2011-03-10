/* This test is here to check that PTHREAD_RWLOCK_INITIALIZER is correctly
 * Defined. It will fail to compile otherwise.
 */
#include <pthread.h>

pthread_rwlock_t  theLock = PTHREAD_RWLOCK_INITIALIZER;
