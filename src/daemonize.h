#ifndef __daemonize__h_
#define __daemonize__h_

void daemonize(char *rundir, char *pidfile);
void kill_daemon(int code);

#define SIG_TERM 0
#define SIG_HUP 1
extern void (*sig_funcs[2])();

#endif //__daemonize__h_
