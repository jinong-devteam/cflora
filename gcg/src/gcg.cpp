#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#ifndef _MSC_VER
	#include <unistd.h>
#else
	#include "wingetopt.h"
#endif

#include <uv.h>
#include <tp3.h>
#include <tp12.h>
#include <cf.h>

#include "gcg_base.h"
#include "gcg_config.h"
#include "gcg_connection.h"
#include "gcg_server.h"

#define GCG_WORKING_DIRECTORY	"/"

#ifndef _MSC_VER
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
}
#else
BOOL CtrlHandler( DWORD fdwCtrlType ) 
{ 
	switch( fdwCtrlType ) 
	{ 
		// Handle the CTRL-C signal. 
		case CTRL_C_EVENT: 
			printf( "Ctrl-C event\n\n" );
			Beep( 750, 300 ); 
			gcg_finalize ();
			return( FALSE );

		// CTRL-CLOSE: confirm that the user wants to exit. 
		case CTRL_CLOSE_EVENT: 
			Beep( 600, 200 ); 
			printf( "Ctrl-Close event\n\n" );
			return( TRUE ); 

		// Pass other signals to the next handler. 
		case CTRL_BREAK_EVENT: 
			Beep( 900, 200 ); 
			printf( "Ctrl-Break event\n\n" );
			return FALSE; 

		case CTRL_LOGOFF_EVENT: 
			Beep( 1000, 200 ); 
			printf( "Ctrl-Logoff event\n\n" );
			return FALSE; 

		case CTRL_SHUTDOWN_EVENT: 
			Beep( 750, 500 ); 
			printf( "Ctrl-Shutdown event\n\n" );
			return FALSE; 

		default: 
			return FALSE; 
	} 
} 
#endif

void
gcg_execute () {
	gcg_ttaclient_connect (gcg_get_server (), gcg_get_config ());
	gcg_timer_start (gcg_get_server (), gcg_get_config ());
	gcg_ttaserver_start (gcg_get_server (), gcg_get_config ());
	
	uv_run (uv_default_loop (), UV_RUN_DEFAULT);
	uv_loop_close (uv_default_loop ());
}

int
main (int argc, char **argv)
{
	int dflag = 0;
	char *conffile = NULL;
	int c;
	int gcgid = GCG_DEFAULT_GCGID;

	opterr = 0;
	while ((c = getopt (argc, argv, "c:di:v:")) != -1) {
		switch (c) {
			case 'd':
				dflag = 1;
				break;
			case 'c':
				conffile = optarg;
				break;
			case 'v':
				cf_set_verbose ((cf_verbose_t)atoi(optarg));
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

#ifndef _MSC_VER
	if (dflag)
		gcg_daemonize ();
#else
		SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE ) ;
#endif

	CF_ERR_RETURN (gcg_initialize (conffile, gcgid), "gcg initialization failed.");
	gcg_execute ();
	gcg_finalize ();

	return 0;
}
