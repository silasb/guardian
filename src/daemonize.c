#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "daemonize.h"

void signal_handler(int sig);

int pidFilehandle;

void signal_handler(int sig)
{
    switch(sig)
    {
        case SIGHUP:
            syslog(LOG_WARNING, "Received SIGHUP signal.");
            (*sig_funcs[SIG_HUP])();
            break;
        case SIGINT:
        case SIGTERM:
            syslog(LOG_INFO, "Daemon exiting");
            (*sig_funcs[SIG_TERM])();
            break;
        default:
            syslog(LOG_WARNING, "Unhandled signal %s", strsignal(sig));
            break;
    }
}

void kill_daemon(int code)
{
  close(pidFilehandle);
  exit(code);
}

void daemonize(char *rundir, char *pidfile)
{
    int pid, sid, i;
    char str[10];
    struct sigaction newSigAction;
    sigset_t newSigSet;

    /* Check if parent process id is set */
    if (getppid() == 1)
    {
        /* PPID exists, therefore we are already a daemon */
        return;
    }

    /* Set signal mask - signals we want to block */
    sigemptyset(&newSigSet);
    sigaddset(&newSigSet, SIGCHLD);  /* ignore child - i.e. we don't need to wait for it */
    sigaddset(&newSigSet, SIGTSTP);  /* ignore Tty stop signals */
    sigaddset(&newSigSet, SIGTTOU);  /* ignore Tty background writes */
    sigaddset(&newSigSet, SIGTTIN);  /* ignore Tty background reads */
    sigprocmask(SIG_BLOCK, &newSigSet, NULL);   /* Block the above specified signals */

    /* Set up a signal handler */
    newSigAction.sa_handler = signal_handler;
    sigemptyset(&newSigAction.sa_mask);
    newSigAction.sa_flags = SA_RESTART;// SA_RESETHAND; // maybe SA_RESTART

    /* Signals to handle */
    sigaction(SIGHUP, &newSigAction, NULL);     /* catch hangup signal */
    sigaction(SIGTERM, &newSigAction, NULL);    /* catch term signal */
    sigaction(SIGINT, &newSigAction, NULL);     /* catch interrupt signal */

    /* Fork*/
    pid = fork();

    if (pid < 0)
    {
        /* Could not fork */
        exit(EXIT_FAILURE);
    }

    if (pid > 0)
    {
        /* Child created ok, so exit parent process */
        exit(EXIT_SUCCESS);
    }

    /* Child continues */

    umask(027); /* Set file permissions 750 */

    /* Get a new process group */
    sid = setsid();

    if (sid < 0)
    {
        exit(EXIT_FAILURE);
    }

    /* close all descriptors */
    for (i = getdtablesize(); i >= 0; --i)
    {
        close(i);
    }

    /* Route I/O connections */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    chdir(rundir); /* change running directory */

    /* Ensure only one copy */
    pidFilehandle = open(pidfile, O_RDWR|O_CREAT, 0600);

    if (pidFilehandle == -1 )
    {
        /* Couldn't open lock file */
        syslog(LOG_INFO, "Could not open PID lock file %s, exiting", pidfile);
        exit(EXIT_FAILURE);
    }

    /* Try to lock file */
    if (lockf(pidFilehandle,F_TLOCK,0) == -1)
    {
        /* Couldn't get lock on lock file */
        syslog(LOG_INFO, "Could not lock PID lock file %s, exiting", pidfile);
        exit(EXIT_FAILURE);
    }

    /* Get and format PID */
    sprintf(str,"%d\n",getpid());

    /* write pid to lockfile */
    write(pidFilehandle, str, strlen(str));
}
