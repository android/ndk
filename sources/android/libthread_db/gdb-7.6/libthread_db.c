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
#include <sys/ptrace.h>
#include <stdint.h>
#include <thread_db.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define DEBUG 0
#if DEBUG
#  include <string.h>  /* for strerror() */
#  define D(...)  fprintf(stderr, "libthread_db:%s: ", __FUNCTION__), fprintf(stderr, __VA_ARGS__)
#else
#  define D(...)  do{}while(0)
#endif


extern int ps_pglobal_lookup (void *, const char *obj, const char *name, void **sym_addr);
extern pid_t ps_getpid(struct ps_prochandle *ph);

/*
 * This is the list of "special" symbols we care about whose addresses are
 * cached by gdbserver from the host at init time.
 */
enum {
    SYM_TD_CREATE,
    SYM_THREAD_LIST,
    NUM_SYMS
};

static char const * gSymbols[] = {
    [SYM_TD_CREATE] = "_thread_created_hook",
    NULL
};


char const **
td_symbol_list(void)
{
    return gSymbols;
}


/* Extract the permitted capabilities of a given task */
static int
_get_task_permitted_caps(int pid, int tid, uint64_t *cap)
{
    char path[64];
    char buff[1024];
    int  len;
    int  fd;
    int  result = -1;
    char*  perm;
    char*  end;

    /* Open task status file */
    snprintf(path, sizeof path, "/proc/%d/task/%d/status", pid, tid);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        D("Could not open %s: %s\n", path, strerror(errno));
        return -1;
    }

    /* Read its content, up to sizeof buff-1, then zero-terminate */
    do {
        len = read(fd, buff, sizeof buff-1);
    } while (len < 0 && errno == EINTR);

    if (len < 0) {
        D("Could not read %s: %s\n", path, strerror(errno));
        goto EXIT;
    }

    buff[len] = 0;

    /* Look for "CapPrm: " in it */
    perm = strstr(buff, "CapPrm:");
    if (perm == NULL) {
        D("Could not find CapPrm in %s!\n---- cut here ----\n%.*s\n----- cut here -----\n",
          path, len, buff);
        errno = EINVAL;
        goto EXIT;
    }

    /* Now read the hexadecimal value after 'CapPrm: ' */
    errno = 0;
    *cap = (uint64_t) strtoull(perm+8, &end, 16);
    if (errno == 0) {
        D("Found CapPerm of %lld in %s\n", *cap, path);
        result = 0;
    } else {
        D("Cannot read CapPerm from %s: '%.*s'\n", path, 24, perm);
    }
EXIT:
    close(fd);
    return result;
}


td_err_e
td_ta_new(struct ps_prochandle * proc_handle, td_thragent_t ** agent_out)
{
    td_thragent_t * agent;

    /* Platforms before Android 2.3 contain a system bug that prevents
     * gdbserver to attach to all threads in a target process when
     * it is run as the same userID than the target (works fine if
     * run as root).
     *
     * Due to the way gdbserver is coded, this makes gdbserver exit()
     * immediately (see linux_attach_lwp in linux-low.c). Even if we
     * modify the source code to not exit(), then signals will not
     * be properly rerouted to gdbserver, preventing breakpoints from
     * working correctly.
     *
     * The following code is here to test for this problematic condition.
     * If it is detected, we return TD_NOLIBTHREAD to indicate that there
     * are no threads to attach to (gdbserver will attach to the main thread
     * though).
     */
    do {
        char  path[64];
        DIR*  dir;
        struct dirent *entry;
        pid_t     my_pid = getpid();
        int       target_pid = ps_getpid(proc_handle);
        uint64_t  my_caps, tid_caps;

        D("Probing system for platform bug.\n");

        /* nothing to do if we run as root */
        if (geteuid() == 0) {
            D("Running as root, nothing to do.\n");
            break;
        }

        /* First, get our own permitted capabilities */
        if (_get_task_permitted_caps(my_pid, my_pid, &my_caps) < 0) {
            /* something is really fishy here */
            D("Could not get gdbserver permitted caps!\n");
            return TD_NOLIBTHREAD;
        }

        /* Now, for each thread in the target process, compare the
         * permitted capabilities set to our own. If they differ,
         * the thread attach will fail. Booo...
         */
        snprintf(path, sizeof path, "/proc/%d/task", target_pid);
        dir = opendir(path);
        if (!dir) {
            D("Could not open %s: %s\n", path, strerror(errno));
            break;
        }
        while ((entry = readdir(dir)) != NULL) {
            int  tid;

            if (entry->d_name[0] == '.')   /* skip . and .. */
                continue;

            tid = atoi(entry->d_name);
            if (tid == 0)  /* should not happen - be safe */
                continue;

            if (_get_task_permitted_caps(target_pid, tid, &tid_caps) < 0) {
                /* again, something is fishy */
                D("Could not get permitted caps for thread %d\n", tid);
                closedir(dir);
                return TD_NOLIBTHREAD;
            }

            if (tid_caps != my_caps) {
                /* AAAARGH !! The permitted capabilities set differ. */
                D("AAAAAH, Can't debug threads!\n");
                fprintf(stderr, "Thread debugging is unsupported on this Android platform!\n");
                closedir(dir);
                return TD_NOLIBTHREAD;
            }
        }
        closedir(dir);
        D("Victory: We can debug threads!\n");
    } while (0);

    /* We now return to our regularly scheduled program */

    agent = (td_thragent_t *)malloc(sizeof(td_thragent_t));
    if (!agent) {
        return TD_MALLOC;
    }

    agent->pid = ps_getpid(proc_handle);
    agent->ph = proc_handle;
    *agent_out = agent;

    return TD_OK;
}


td_err_e
td_ta_delete(td_thragent_t * ta)
{
    free(ta);
    // FIXME: anything else to do?
    return TD_OK;
}


/* NOTE: not used by gdb 7.0 */

td_err_e
td_ta_set_event(td_thragent_t const * agent, td_thr_events_t * events)
{
    return TD_OK;
}


/* NOTE: not used by gdb 7.0 */
static td_thrhandle_t gEventMsgHandle;

/* NOTE: not used by gdb 7.0 */

static int
_event_getmsg_helper(td_thrhandle_t const * handle, void * bkpt_addr)
{
    void * pc;

    pc = (void *)ptrace(PTRACE_PEEKUSR, handle->tid, (void *)60 /* r15/pc */, NULL);

    if (pc == bkpt_addr) {
        // The hook function takes the id of the new thread as it's first param,
        // so grab it from r0.
        gEventMsgHandle.pid = ptrace(PTRACE_PEEKUSR, handle->tid, (void *)0 /* r0 */, NULL);
        gEventMsgHandle.tid = gEventMsgHandle.pid;
        return 0x42;
    }
    return 0;
}

/* NOTE: not used by gdb 7.0 */

td_err_e
td_ta_event_getmsg(td_thragent_t const * agent, td_event_msg_t * event)
{
    td_err_e err;
    void * bkpt_addr;

    err = ps_pglobal_lookup(NULL, NULL, gSymbols[SYM_TD_CREATE], &bkpt_addr);
    if (err) {
        return err;
    }

    err = td_ta_thr_iter(agent, _event_getmsg_helper, bkpt_addr, 0, 0, NULL, 0);
    if (err != 0x42) {
        return TD_NOMSG;
    }

    event->event = TD_CREATE;
    event->th_p = &gEventMsgHandle; // Nasty hack, but it's the only way!

    return TD_OK;
}


td_err_e
td_ta_map_lwp2thr(td_thragent_t const * agent, lwpid_t lwpid,
		  td_thrhandle_t *th)
{
    th->pid = ps_getpid(agent->ph);
    th->tid = lwpid;
    return TD_OK;
}


td_err_e
td_thr_get_info(td_thrhandle_t const * handle, td_thrinfo_t * info)
{
    info->ti_tid = handle->tid;
    info->ti_lid = handle->tid; // Our pthreads uses kernel ids for tids
    info->ti_state = TD_THR_SLEEP; /* XXX this needs to be read from /proc/<pid>/task/<tid>.
                                      This is only used to see if the thread is a zombie or not */
    return TD_OK;
}


/* NOTE: not used by gdb 7.0 */

td_err_e
td_thr_event_enable(td_thrhandle_t const * handle, td_event_e event)
{
    // I don't think we need to do anything here...
    return TD_OK;
}


/* NOTE: not used by gdb 7.0 */

td_err_e
td_ta_event_addr(td_thragent_t const * agent, td_event_e event, td_notify_t * notify_out)
{
    int32_t err;

    /*
     * This is nasty, ps_pglobal_lookup is implemented in gdbserver and looks up
     * the symbol from it's cache, which is populated at start time with the
     * symbols returned from td_symbol_list via calls back to the host.
     */

    switch (event) {
        case TD_CREATE:
            err = ps_pglobal_lookup(NULL, NULL, gSymbols[SYM_TD_CREATE], &notify_out->u.bptaddr);
            if (err) {
                return TD_NOEVENT;
            }
            return TD_OK;
    }
    return TD_NOEVENT;
}


td_err_e
td_ta_clear_event(const td_thragent_t * ta_arg, td_thr_events_t * event)
{
    /* Given that gdb 7.0 doesn't use thread events,
       there's nothing we need to do here.  */
    return TD_OK;
}


td_err_e
td_ta_thr_iter(td_thragent_t const * agent, td_thr_iter_f * func, void * cookie,
               td_thr_state_e state, int32_t prio, sigset_t * sigmask, uint32_t user_flags)
{
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

td_err_e
td_thr_tls_get_addr(const td_thrhandle_t * th,
		    psaddr_t map_address, size_t offset, psaddr_t * address)
{
    return TD_NOAPLIC; // FIXME: TODO
}
