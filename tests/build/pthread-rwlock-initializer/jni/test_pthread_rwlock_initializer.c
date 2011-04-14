/* This test is here to check that PTHREAD_RWLOCK_INITIALIZER is correctly
 * Defined. It will fail to compile otherwise.
 *
 * It also checks that pthread_rwlock_t functions are properly exposed
 * by the C library. It will fail to link otherwise.
 */
#include <pthread.h>

pthread_rwlock_t  theLock = PTHREAD_RWLOCK_INITIALIZER;

void  build_check(void)
{
    pthread_rwlock_t  lock;
    pthread_rwlock_init(&lock, NULL);
    pthread_rwlock_rdlock(&lock);
    pthread_rwlock_unlock(&lock);
    pthread_rwlock_destroy(&lock);
}