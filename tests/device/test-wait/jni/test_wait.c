/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[]) {

   pid_t cpid;
   int status;
   struct rusage usage;

//----------------------------------------------------
//----------------- Wait(); System Call --------------
//----------------------------------------------------
   printf("Testing Wait(); System Call\n");
   printf("\n");

   cpid = fork();               /* Creates fork */
   if (cpid == -1) {
      printf("For has failed, returned = %d\n", EXIT_FAILURE);
   }

   if (cpid == 0) {		/* This is the child operation */
      printf("Child Created\n");
      printf("Child = %d\n", getpid());
      printf("Parent = %d\n", getppid());
      sleep(2);
      exit(3);

   } else {                     /* This is the parent operation */
      printf("Waiting for child\n");
      wait(&status);
      printf("Waiting Complete\n");
      printf("Child Exit Code: %d\n", WEXITSTATUS(status));
   }

   printf("\n");               /* Just console space */
//----------------------------------------------------
//-------------- Waitpid(); System Call --------------
//----------------------------------------------------
   printf("Testing Waitpid(); System Call\n");
   printf("\n");


   cpid = fork();                 /* Creates fork */
   if (cpid == -1) {
      printf("Fork has failed, returned = %d\n", EXIT_FAILURE);
   }

   if (cpid == 0) {               /* This is the child operation */
      printf("Child Created\n");
      printf("Child = %d\n", getpid());
      printf("Parent = %d\n", getppid());
      sleep(2);
      exit(3);

   } else {                       /* This is the parent operation */
      printf("Waiting for child %d\n", cpid);
      waitpid(cpid, NULL, 0);
      printf("Waiting Complete\n");
      printf("Child Exit Code: %d\n", WEXITSTATUS(status));
   }

   printf("\n");
//----------------------------------------------------
//---------------- Wait3(); System Call --------------
//----------------------------------------------------
   printf("Testing Wait3(); System Call\n");
   printf("\n");

   cpid = fork();               /* Creates fork */
   if (cpid == -1) {
      printf("For has failed, returned = %d\n", EXIT_FAILURE);
   }

   if (cpid == 0) {             /* This is the child operation */
      printf("Child Created\n");
      printf("Child = %d\n", getpid());
      printf("Parent = %d\n", getppid());
      sleep(2);
      exit(3);

   } else {                     /* This is the parent operation */
      printf("Waiting for child\n");
      wait3(&status, 0, &usage);
      printf("Waiting Complete\n");
      printf("Child Exit Code: %d\n", WEXITSTATUS(status));
   }

   printf("\n");
   sleep(1);
//----------------------------------------------------
//---------------- Wait4(); System Call --------------
//----------------------------------------------------
   printf("Testing Wait4(); System Call\n");
   printf("\n");

   cpid = fork();               /* Creates fork */
   if (cpid == -1) {
      printf("For has failed, returned = %d\n", EXIT_FAILURE);
   }

   if (cpid == 0) {             /* This is the child operation */
      printf("Child Created\n");
      printf("Child = %d\n", getpid());
      printf("Parent = %d\n", getppid());
      sleep(2);
      exit(3);

   } else {                     /* This is the parent operation */
      printf("Waiting for child\n");
      wait4(cpid, &status, 0, &usage);
      //__wait4(cpid, &status, 0, &usage);  //  This function will work, the above which is delcared will not.
      printf("Waiting Complete\n");
      printf("Child Exit Code: %d\n", WEXITSTATUS(status));
   }

   printf("\n");
   sleep(1);

   return EXIT_SUCCESS;

}

