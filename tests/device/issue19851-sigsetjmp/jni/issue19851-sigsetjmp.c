/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <signal.h>
#include <setjmp.h>

static sigjmp_buf sbuf;

void handler(int sig)
{
    printf("In handler()\n");
    siglongjmp(sbuf, 99);
    printf("After calling siglongjmp, but you won't see me here.\n");
}

void foo()
{
    struct sigaction sact;
    sigset_t sset;
    printf("In foo()\n");

 /* Setup signal handler for SIGUSR2 */
    sigemptyset(&sact.sa_mask);
    sact.sa_flags = 0;
    sact.sa_handler = handler;
    sigaction(SIGUSR2, &sact, NULL);

 /* Unblock SIGUSR2 */
    sigemptyset(&sset);
    sigaddset(&sset, SIGUSR2);
    sigprocmask(SIG_UNBLOCK, &sset, NULL);

 /* Action */
    kill(getpid(), SIGUSR2);

    printf("After calling kill, but you won't see me here.\n");
}

void bar()
{
    printf("In bar()\n");
    foo();
    printf("After calling foo, but you won't see me here.\n");
}

int main()
{
    int code, ret;
    sigset_t sset;
    printf("In main()\n");
    sigemptyset(&sset);
    sigaddset(&sset, SIGUSR1);
    sigaddset(&sset, SIGUSR2);
    sigprocmask(SIG_SETMASK, &sset, NULL);
    if ((code = sigsetjmp(sbuf, 1)) == 0)
    {
        bar();
        ret = 0xDEADBEEF;
    }
    else
    {
        printf("siglongjmp() back to main, code = %d\n", code);
      /* Make sure SIGUSR2 still blocked */
        sigprocmask(SIG_SETMASK, NULL, &sset);
        ret = sigismember(&sset, SIGUSR2)? 0 : -1;
    }
    printf("ret = %d\n", ret);

    return ret;
}
