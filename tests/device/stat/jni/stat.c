/*
 * Copyright (C) 2013 The Android Open Source Project
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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

int main()
{
    int i;
    struct stat sb;
    char *paths[] = {
        "/system", "/dev/null", "doesn't exit", "stat"
    };

    for(i=0; i<sizeof(paths)/sizeof(paths[0]); i++)
    {
        if (stat(paths[i], &sb) != 0)
            printf("Can't stat path %s\n", paths[i]);
        else
        {
            printf("%14s %6lld: %s%s%s %s %s %s %s\n", paths[i], sb.st_size,
                   ((sb.st_mode & S_IRUSR)? "r" : " "),
                   ((sb.st_mode & S_IWUSR)? "w" : " "),
                   ((sb.st_mode & S_IXUSR)? "x" : " "),
                   (((sb.st_mode & S_IFMT) == S_IFREG)? "REG" : "   "),
                   (((sb.st_mode & S_IFMT) == S_IFDIR)? "DIR" : "   "),
                   (((sb.st_mode & S_IFMT) == S_IFLNK)? "LNK" : "   "),
                   (((sb.st_mode & S_IFMT) == S_IFCHR)? "CHR" : "   "));
        }
    }
   return 0;
}
