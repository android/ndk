#include <locale.h>
#include <pthread.h>

static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static pthread_key_t s_locale_key;

static void init_key(void) {
  pthread_key_create(&s_locale_key, NULL);
}

locale_t uselocale(locale_t loc)
{
  pthread_once(&s_once, init_key);
  locale_t old_loc = pthread_getspecific(s_locale_key);
  if (loc)
    pthread_setspecific(s_locale_key, loc);
  return old_loc;
}
