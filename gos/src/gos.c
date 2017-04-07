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
#include <cf.h>

#include "gos_base.h"
#include "gos_config.h"
#include "gos_connection.h"
#include "gos_control.h"
#include "gos_device.h"
#include "gos_rule.h"
#include "gos_server.h"

#define GOS_WORKING_DIRECTORY	"/"

#ifndef _MSC_VER
void 
gos_daemonize ()
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
	//chdir(GOS_WORKING_DIRECTORY);

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
			gos_finalize ();
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

void ToHexFile( int no , const char *pBuf , int size )
{
#ifdef _MSC_VER
	char hex[5] ;
	unsigned char toN ;
	int i , len ;
	FILE *pFile  ;
	if ( no == 0 ) {
		pFile = fopen("e:\\readbuf.txt","a+") ;
	}
	else if ( no == 1 ) {
		pFile = fopen("e:\\parsebuf.txt","a+") ;
	}

	for ( i = 0 ; i < size ; i++ ) {
		toN = pBuf[i] ;
		len = sprintf( hex , "%02X " , toN) ;
		fwrite( hex , len , 1 , pFile ) ;
	}
	fwrite( "\n" , 1 , 1 , pFile ) ;
	fflush( pFile ) ;
	fclose( pFile ) ;
#else
#endif
}


void
gos_execute () {
	gos_timer_start (gos_get_server (), gos_get_config ());
	gos_ttaserver_start (gos_get_server (), gos_get_config ());
	CF_VERBOSE (CF_VERBOSE_MID, "GOS Timer & TTA P3 server started.");
	
	uv_run (uv_default_loop (), UV_RUN_DEFAULT);
	uv_loop_close (uv_default_loop ());
}

int
#ifdef _MSC_VER
main (int argc, char **argv)
//gos_win_main (int argc, char **argv)
#else
main (int argc, char **argv)
#endif
{
	int dflag = 0;
	char *conffile = NULL;
	int c;

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
				cf_set_verbose (atoi(optarg));
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
		gos_daemonize ();
#else
		SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE ) ;
#endif

//	do {
		CF_ERR_RETURN (gos_initialize (conffile), "gos initialization failed.");
		CF_VERBOSE (CF_VERBOSE_LOW, "GOS Initialized");
		gos_execute ();
		gos_finalize ();
#ifdef _MSC_VER
		Sleep(10) ;
#else
		sleep(10) ;
#endif
//	} while (gos_get_restart (gos_get_server()));
//	} while (0);

	return 0;
}
