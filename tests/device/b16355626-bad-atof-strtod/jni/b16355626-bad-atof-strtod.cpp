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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main()
{
   char buf[128];
   int error_count = 0;
   float f = atof("90.099998474121094");
   printf("%f\n", f);
   sprintf(buf, "%08x", *(unsigned*)&f);
   error_count += strcasecmp("42b43333", buf);
   
   f = atof("3.14");
   printf("%f\n", f);
   sprintf(buf, "%08x", *(unsigned*)&f);
   error_count += strcasecmp("4048f5c3", buf);

   return error_count;
}
