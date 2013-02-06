#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "daemonize.h"

#define DAEMON_NAME "guardian"

pid_t chld_pid;

/* kill children and then shutdown self */
void sig_term() {
  syslog(LOG_INFO, "Killing child_pid %d\n", chld_pid);
  kill(chld_pid, SIGTERM);
  kill_daemon(EXIT_SUCCESS);
}

/* restart child */
void sig_hup() {
  kill(chld_pid, SIGTERM);
}

void (*sig_funcs[2])() = {
  sig_term,
  sig_hup,
};

void start_service(int argc, char **argv) {
  // fork+exec+wait

  chld_pid = fork();

  if (chld_pid < 0) {
    exit(0);
  }

  if (chld_pid > 0) { // child created
    // parent is running in here

    syslog(LOG_INFO, "parent waiting...%d\n", getpid());
    int status;
    pid_t w;
    w = waitpid(chld_pid, &status, 0);

    if (w == -1) {
      syslog(LOG_DEBUG, "waitpid error'd");
    }

    if (WIFEXITED(status)) {
      syslog(LOG_INFO, "%d, exited, status=%d", chld_pid, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
      syslog(LOG_INFO, "%d, killed by signal %d", chld_pid, WTERMSIG(status));
    } else if (WIFSTOPPED(status)) {
      syslog(LOG_INFO, "%d, stopped by signal %d", chld_pid, WSTOPSIG(status));
    } else if (WIFCONTINUED(status)) {
      syslog(LOG_INFO, "continued");
    }

  } else {
    // child running here

    syslog(LOG_INFO, "child running... %d\n", getpid());

    const char *prgm = argv[0];

    syslog(LOG_INFO, "argc: %d\n", argc);

    if (execvp(prgm, argv) == -1) {
      perror("Service returned");
      syslog(LOG_DEBUG, "service returned when it shouldn't have");
      exit(-1);
    }
    syslog(LOG_DEBUG, "We shouldn't be here....");

  }
}

int main(int argc, char **argv)
{

  if (argc < 2) {
    printf("Need a command to run\n");
    exit(EXIT_FAILURE);
  }

  /* Debug logging
     setlogmask(LOG_UPTO(LOG_DEBUG));
     openlog(DAEMON_NAME, LOG_CONS, LOG_USER);
     */

  /* Logging */
  setlogmask(LOG_UPTO(LOG_INFO));
  openlog(DAEMON_NAME, LOG_CONS | LOG_PERROR, LOG_USER);

  syslog(LOG_INFO, "daemonzing...");

  /* Deamonize */
  daemonize("/tmp/", "/tmp/daemon.pid");

  char name[] = DAEMON_NAME;

  /* Change the name of the running executable */
  strncpy(argv[0], name , strlen(name));
  memset(&argv[0][strlen(name)], '\0', strlen(&argv[0][strlen(name)]));

  if (prctl(PR_SET_NAME, name, 0, 0, 0) == -1)
  {
    perror("Couldn't change daemon name");
    exit(EXIT_FAILURE);
  }

  syslog(LOG_INFO, "daemonized");

  argv += 1;
  argc--;

  while(1) {
    start_service(argc, argv);
    syslog(LOG_INFO, "Restarting service");
  }
}
