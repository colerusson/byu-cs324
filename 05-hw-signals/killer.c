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
	case '1': // Working!
        kill(pid, 12);
        sleep(1);
        kill(pid, SIGTERM);
        sleep(1);
        break;
	case '2': // Working!
        kill(pid, SIGHUP);
        sleep(1);
        kill(pid, 30);
        sleep(1);
        kill(pid, 12);
        sleep(1);
        kill(pid, SIGTERM);
        sleep(1);
        break;
	case '3': // Working!
        kill(pid, SIGINT);
        sleep(1);
        kill(pid, SIGINT);
        sleep(1);
        kill(pid, SIGUSR2);
        sleep(1);
        kill(pid, 12);
        sleep(1);
        kill(pid, SIGTERM);
        sleep(1);
        break;
	case '4': // Working!
        kill(pid, SIGINT);
        sleep(1);
        kill(pid, SIGHUP);
        sleep(1);
        kill(pid, 12);
        sleep(1);
        kill(pid, SIGTERM);
        sleep(1);
        break;
	case '5': // Working!
        kill(pid, SIGUSR2);
        sleep(1);
        kill(pid, SIGHUP);
        sleep(1);
        kill(pid, SIGTERM);
        sleep(2);
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
	case '8': // TODO: not working, prints 1 2 7 25 (should be 1 2 6)
        kill(pid, SIGINT);
        sleep(1);
        kill(pid, SIGUSR1);
        sleep(1);
        kill(pid, SIGUSR2);
        sleep(1);
        kill(pid, 30);
        sleep(1);
        break;
	case '9': // TODO: not working, prints 8 1 9 2 25 (should be 8 9 1 2, and can't use SIGHUP or SIGINT)
        kill(pid, SIGQUIT);
        sleep(3);
		break;
	}
	waitpid(pid, NULL, 0);
}
