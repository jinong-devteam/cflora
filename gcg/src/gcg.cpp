/**
 * Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
 * \file gcg.cpp
 * \brief GCG 메인 관련 코드. 기존 코드를 수정했음.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/file.h>
#include <errno.h>

#include <uv.h>
#include <tp3.h>
#include <gnodeutil.h>
#include <gnode.h>
#include <gnodehelper.h>
#include <cf.h>

#include "gcg_base.h"
#include "gcg_config.h"
#include "gcg_connection.h"
#include "gcg_server.h"

#define GCG_WORKING_DIRECTORY	"/"

void 
gcg_daemonize ()
{
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
	//chdir(GCG_WORKING_DIRECTORY);

	/* Close all open file descriptors */
	for (x = sysconf(_SC_OPEN_MAX); x > 0; x--) {
	//	close (x);
	}

    int pidf = open("/var/run/gcgg.pid", O_CREAT | O_RDWR, 0666);
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
gcg_execute () {
	gcg_ttaclient_connect (gcg_get_server (), gcg_get_config ());
	gcg_timer_start (gcg_get_server (), gcg_get_config ());
	gcg_ttaserver_start (gcg_get_server (), gcg_get_config ());
	
	uv_run (uv_default_loop (), UV_RUN_DEFAULT);
	uv_loop_close (uv_default_loop ());
}

int
main (int argc, char **argv) {
	int dflag = 0;
	char *conffile = NULL;
	int c;
	int gcgid = GCG_DEFAULT_GCGID;

    FLAGS_log_dir = "/var/log/farmos";
    FLAGS_max_log_size = 10;
    FLAGS_logbufsecs = 0;
    google::InitGoogleLogging (argv[0]);

	opterr = 0;
	while ((c = getopt (argc, argv, "c:di:v:s:")) != -1) {
		switch (c) {
			case 'd':
				dflag = 1;
				break;
			case 's':
				conffile = optarg;
				break;
			case 'c':
				// not use
				break;
			case 'v':
				break;
			case 'i':
				gcgid = atoi(optarg);
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
		gcg_daemonize ();

	ERR_RETURN (gcg_initialize (conffile, gcgid), "gcg initialization failed.");
	gcg_execute ();
	gcg_finalize ();

	return 0;
}
