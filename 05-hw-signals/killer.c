#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/wait.h>

void sigint_handler(int signum) {
	// send SIGKILL to all processes in group, so this process and children
	// will terminate.  Use SIGKILL because SIGTERM and SIGINT (among
	// others) are overridden in the child.
	kill(-getpgid(0), SIGKILL);
}

int main(int argc, char *argv[]) {
	char *scenario = argv[1];
	int pid = atoi(argv[2]);

	struct sigaction sigact;

	// Explicitly set flags
	sigact.sa_flags = SA_RESTART;
	sigact.sa_handler = sigint_handler;
	// Override SIGINT, so that interrupting this process sends SIGKILL to
	// this one and, more importantly, to the child.
	sigaction(SIGINT, &sigact, NULL);

	switch (scenario[0]) {
	case '0': // Working!
        kill(pid, SIGHUP);
        sleep(1);
		break;
	case '1': // TODO: not working
        // use the signals in signals.c to produce no output, as seen in the README
        kill(pid, SIGINT);
        sleep(1);
        kill(pid, SIGUSR1);
        sleep(1);
        kill(pid, SIGUSR2);
        sleep(1);
        kill(pid, SIGHUP);
        sleep(1);
        kill(pid, SIGTERM);
        sleep(1);
		break;
	case '2': // TODO: not working
        kill(pid, SIGHUP);
        sleep(5);
        kill(pid, 30);
        sleep(2);
        kill(pid, 16);
        sleep(1);
        kill(pid, SIGTERM);
        sleep(1);
        break;
	case '3': // TODO: not working
        kill(pid, SIGINT);
        sleep(1);
        kill(pid, SIGUSR1);
        sleep(1);
        kill(pid, SIGHUP);
        sleep(1);
        kill(pid, 30);
        sleep(1);
        kill(pid, SIGQUIT);
        sleep(1);
        break;
	case '4': // TODO: not working
        kill(pid, SIGINT);
        sleep(1);
        kill(pid, SIGHUP);
        sleep(1);
        kill(pid, 30);
        sleep(1);
        kill(pid, SIGTERM);
        sleep(1);
		break;
	case '5': // TODO: not working
		break;
	case '6': // Working!
        kill(pid, SIGINT);
        sleep(1);
        kill(pid, SIGUSR1);
        sleep(1);
        kill(pid, SIGUSR2);
        sleep(1);
        kill(pid, 30);
        sleep(1);
        kill(pid, 16);
        sleep(1);
        kill(pid, SIGTERM);
        sleep(1);
		break;
	case '7': // Working!
        kill(pid, SIGINT);
        sleep(1);
        kill(pid, SIGUSR1);
        sleep(1);
        kill(pid, SIGUSR2);
        sleep(1);
        kill(pid, SIGTERM);
        sleep(1);
		break;
	case '8': // TODO: not working
		break;
	case '9': // TODO: not working
		break;
    case 'a': // testing here
        kill(pid, SIGINT);
        sleep(1);
        kill(pid, SIGUSR1);
        sleep(1);
        kill(pid, SIGUSR2);
        sleep(1);
        kill(pid, SIGTERM);
        sleep(1);
        kill(pid, SIGQUIT);
        sleep(1);
        kill(pid, SIGTSTP);
        sleep(1);
        kill(pid, SIGCONT);
        sleep(1);
        kill(pid, SIGCHLD);
        sleep(1);
        kill(pid, SIGTTIN);
        sleep(1);
        kill(pid, SIGTTOU);
        sleep(1);
        kill(pid, SIGPIPE);
        sleep(1);
        kill(pid, SIGALRM);
        sleep(1);
        break;
	}
	waitpid(pid, NULL, 0);
}
