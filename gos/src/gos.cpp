/**
 * Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
 * \file gos.cpp
 * \brief GOS 메인관련 소스 파일. 기존 코드를 수정했음.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <sys/file.h>
#include <errno.h>
#include <unistd.h>

#include <uv.h>
#include <tp3.h>
#include <cf.h>

#include "gos_base.h"
#include "gos_config.h"
#include "gos_connection.h"
#include "gos_device.h"
#include "gos_rule.h"
#include "gos_server.h"

#define GOS_WORKING_DIRECTORY    "/"

void 
gos_daemonize () {
    int x;
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS);

    umask(0);
    //chdir(GOS_WORKING_DIRECTORY);

    /* Close all open file descriptors */
    for (x = sysconf(_SC_OPEN_MAX); x > 0; x--) {
    //    close (x);
    }

    int pidf = open("/var/run/gos.pid", O_CREAT | O_RDWR, 0666);
    if (flock(pidf, LOCK_EX | LOCK_NB) == -1) {
        fprintf (stderr, "Another instance is running.");
        exit(EXIT_FAILURE);
    } else {
        char pids[10];
        int n = sprintf (pids, "%d\n", getpid ());
        write (pidf, pids, n);
    }
    close (pidf);
}

void
gos_execute () {
    gos_timer_start (gos_get_server (), gos_get_config ());
    gos_ttaserver_start (gos_get_server (), gos_get_config ());
    LOG(INFO) << "GOS Timer & TTA P3 server started.";
    
    uv_run (uv_default_loop (), UV_RUN_DEFAULT);
    uv_loop_close (uv_default_loop ());
}

int
main (int argc, char **argv) {
    int dflag = 0;
    char *conffile = NULL;
    int c;

    FLAGS_log_dir = "/var/log/farmos";
    FLAGS_max_log_size = 10;
    FLAGS_logbufsecs = 0;
    google::InitGoogleLogging (argv[0]);

    opterr = 0;
    while ((c = getopt (argc, argv, "c:dv:")) != -1) {
        switch (c) {
            case 'd':
                dflag = 1;
                break;
            case 'c':
                conffile = optarg;
                break;
            case 'v':
                break;
            case '?':
                if (optopt == 'c')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint (optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
                return 1;
            default:
                abort ();
        }
    }

    if (dflag)
        gos_daemonize ();

    do {
        if (gos_initialize (conffile)) {
            LOG(ERROR) << "gos initialization failed.";
            return 0;
        }

        LOG (INFO) << "GOS Initialized";
        gos_execute ();
        gos_finalize ();
        sleep(1) ;
    } while (gos_get_restart (gos_get_server()));

    return 0;
}
