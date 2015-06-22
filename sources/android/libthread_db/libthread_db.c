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

#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <thread_db.h>
#include <unistd.h>

extern pid_t ps_getpid(struct ps_prochandle*);

// We don't have any symbols to cache.
char const** td_symbol_list(void) {
    static char const* symbols[] = { NULL };
    return symbols;
}

//
// Thread agents.
//

td_err_e td_ta_new(struct ps_prochandle* proc_handle, td_thragent_t** agent_out) {
    td_thragent_t* agent = (td_thragent_t*) calloc(1, sizeof(td_thragent_t));
    if (!agent) {
        return TD_MALLOC;
    }

    agent->pid = ps_getpid(proc_handle);
    agent->ph = proc_handle;
    *agent_out = agent;

    return TD_OK;
}


td_err_e td_ta_delete(td_thragent_t* ta) {
    free(ta);
    return TD_OK;
}

td_err_e td_ta_map_lwp2thr(td_thragent_t const* agent, lwpid_t lwpid, td_thrhandle_t* th) {
    th->pid = ps_getpid(agent->ph);
    th->tid = lwpid;
    return TD_OK;
}

td_err_e td_ta_thr_iter(td_thragent_t const* agent,
                        td_thr_iter_f* func,
                        void* cookie,
                        td_thr_state_e state,
                        int32_t prio,
                        sigset_t* sigmask,
                        uint32_t user_flags) {
    td_err_e err = TD_OK;
    char path[32];
    DIR * dir;
    struct dirent * entry;
    td_thrhandle_t handle;

    snprintf(path, sizeof(path), "/proc/%d/task/", agent->pid);
    dir = opendir(path);
    if (!dir) {
        return TD_NOEVENT;
    }

    handle.pid = agent->pid;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        handle.tid = atoi(entry->d_name);
        if (func(&handle, cookie) != 0) {
	    err = TD_DBERR;
            break;
        }
    }

    closedir(dir);

    return err;
}

//
// Threads.
//

td_err_e td_thr_get_info(td_thrhandle_t const* handle, td_thrinfo_t* info) {
    info->ti_tid = handle->tid;
    info->ti_lid = handle->tid; // Our pthreads uses kernel ids for tids
    info->ti_state = TD_THR_SLEEP; /* XXX this needs to be read from /proc/<pid>/task/<tid>.
                                      This is only used to see if the thread is a zombie or not */
    return TD_OK;
}

//
// TLS.
//

td_err_e td_thr_tlsbase(const td_thrhandle_t* unused1, unsigned long int unused2, psaddr_t* unused3) {
  return TD_NOAPLIC; // TODO: fix this if/when we support ELF TLS.
}

td_err_e td_thr_tls_get_addr(const td_thrhandle_t* unused1, psaddr_t unused2, size_t unused3, psaddr_t* unused4) {
  return TD_NOAPLIC; // TODO: fix this if/when we support ELF TLS.
}

//
// Thread events.
//

// Thread events are no longer used by gdb >= 7.0.
// Because we link gdbserver statically, though, we need dummy definitions.
td_err_e td_ta_set_event(td_thragent_t const* agent, td_thr_events_t* events) {
    abort();
}
td_err_e td_ta_event_getmsg(td_thragent_t const* agent, td_event_msg_t* event) {
    abort();
}
td_err_e td_thr_event_enable(const td_thrhandle_t* handle, int event) {
    abort();
}
td_err_e td_ta_clear_event(const td_thragent_t* ta_arg, td_thr_events_t* event) {
    abort();
}
td_err_e td_ta_event_addr(td_thragent_t const* agent, td_event_e event, td_notify_t* notify_out) {
    abort();
}
