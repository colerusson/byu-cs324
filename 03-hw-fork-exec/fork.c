#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<string.h>

int main(int argc, char *argv[]) {
	int pid;
	FILE *output_file;
	int pipefd[2];
	char buffer[100];

	if (pipe(pipefd) == -1) {
		perror("Pipe failed");
		exit(1);
	}

	output_file = fopen("fork-output.txt", "w");

	if (output_file == NULL) {
		perror("Error opening file");
		exit(1);
	}

	fprintf(output_file, "BEFORE FORK (%d)\n", fileno(output_file));

	fflush(output_file);

	printf("Starting program; process has pid %d\n", getpid());

	if ((pid = fork()) < 0) {
		fprintf(stderr, "Could not fork()");
		exit(1);
	}

	/* BEGIN SECTION A */

	printf("Section A;  pid %d\n", getpid());
	//sleep(5);

	/* END SECTION A */
	if (pid == 0) {
		/* BEGIN SECTION B */

		printf("Section B\n");
		//sleep(30);
		//sleep(30);
		//printf("Section B done sleeping\n");

		sleep(10);
		close(pipefd[0]);
		const char *message = "hello from Section B\n";
		write(pipefd[1], message, strlen(message));
		sleep(10);
		close(pipefd[1]);

		sleep(5);
		fprintf(output_file, "SECTION B (%d)\n", fileno(output_file));
		fflush(output_file);

		char *newenviron[] = { NULL };

		printf("Program \"%s\" has pid %d. Sleeping.\n", argv[0], getpid());
		//sleep(30);

		if (argc <= 1) {
			printf("No program to exec.  Exiting...\n");
			exit(0);
		}

		printf("Running exec of \"%s\"\n", argv[1]);
		int fd_output = fileno(output_file);
		if (dup2(fd_output, STDOUT_FILENO) == -1) {
    			perror("dup2 failed");
    			exit(1);
		}

		close(fd_output);
		execve(argv[1], &argv[1], newenviron);
		printf("End of program \"%s\".\n", argv[0]);

		exit(0);

		/* END SECTION B */
	} else {
		/* BEGIN SECTION C */

		printf("Section C\n");
		//wait(NULL);
		//sleep(30);
		//printf("Section C done sleeping\n");

		close(pipefd[1]);
		ssize_t bytes_read = read(pipefd[0], buffer, sizeof(buffer));
		if (bytes_read == -1) {
			perror("Read failed");
			exit(1);
		}

		buffer[bytes_read] = '\0';

		printf("Number of bytes received from pipe: %zd\n", bytes_read);
	        printf("Received string from pipe: %s", buffer);
		ssize_t second_read = read(pipefd[0], buffer, sizeof(buffer));

        	if (second_read == -1) {
            		perror("Second read failed");
            		exit(1);
        	}

        	printf("Number of bytes received from pipe (second read): %zd\n", second_read);
        	printf("Received string from pipe (second read): %s", buffer);

		fprintf(output_file, "SECTION C (%d)\n", fileno(output_file));
		fflush(output_file);
		fclose(output_file);

		sleep(5);

		exit(0);

		/* END SECTION C */
	}
	/* BEGIN SECTION D */

	printf("Section D\n");
	//sleep(30);

	/* END SECTION D */
}

