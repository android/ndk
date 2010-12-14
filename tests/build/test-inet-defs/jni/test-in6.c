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

#include <netinet/in6.h>
#ifndef IN6_IS_ADDR_MC_NODELOCAL
#error IN6_IS_ADDR_MC_NODELOCAL is not defined by <inet/in6.h>
#endif
#ifndef IN6_IS_ADDR_MC_GLOBAL
#error IN6_IS_ADDR_MC_GLOBAL is not defined by <inet/in6.h>
#endif
#ifndef IN6ADDR_ANY_INIT
#error IN6ADDR_ANY_INIT is not defined by <inet/in6.h>
#endif
#ifndef IN6_IS_ADDR_MULTICAST
#error IN6_IS_ADDR_MULTICAST is not defined by <inet/in6.h>
#endif
char dummy_in6;
