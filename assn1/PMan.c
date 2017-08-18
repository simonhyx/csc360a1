#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h> // why do we need history?
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#define PROMPT "PMan: > "
#define MAX_BG_PROCS 40 // max # of child processes

void welcome();
void insert(pid_t , char *, char *);
void list();
int run_bg(char *);
int run_bgkill(char *);
int run_bgstop(char *);
int run_bgstart(char *);
int run_pstat(char *);
int run_exit();

struct proc_elem {
	pid_t pid;
	char path[1024];
	char name[128];
	int status; // 0 being paused 1 being running
};
struct proc_elem procArray[MAX_BG_PROCS];

int numProc = 0;

int main() {
	welcome();

	for (;;) {
		char *input = NULL ;

		input = readline(PROMPT);

		if ( *input == 0 ) {
			continue;
		}
		else if ( strncmp(input, "bg ", 3) == 0 || strcmp(input, "bg") == 0) {
			run_bg(input);
		}
		else if ( strncmp(input, "bglist ", 7) == 0 || strcmp(input, "bglist") == 0) {
			list();
		}
		else if ( strncmp(input, "bgkill ", 7) == 0 || strcmp(input, "bgkill") == 0) {
			run_bgkill(input);
		}
		else if ( strncmp(input, "bgstop ", 7) == 0  || strcmp(input, "bgstop") == 0) {
			run_bgstop(input);
		}
		else if ( strncmp(input, "bgstart ", 8) == 0 || strcmp(input, "bgstart") == 0) {
			run_bgstart(input);
		}
		else if ( strncmp(input, "pstat ", 6) == 0 || strcmp(input, "pstat") == 0) {
			run_pstat(input);
		}
		else if ( strcmp(input, "exit") == 0 || strcmp(input, "exit") == 0 ) {
			exit(run_exit());
		}
		else {
			printf("%s: command not found\n", input);
		}
	}

	return 0;
}

void welcome() {
	printf("\n\tCSC360\tASSN 1\n\tPMan: a simple proc manager\n\tWilliam (xiaozhou) Liu\n\tV00764353\n\n");
	printf("\tUseage: bg $EXE, bglist , bgkill $PID, bgstop $PID,\n\t\tbgstart $PID, pstat $PID, exit\n\n");
}

int run_bg(char *input) {
	input += 2;

	char *argv[10]; // 10 arguments are enough
	char *p = strtok(input, " ");

	if (!p) {
		printf("ERROR: no argument given after bg\n");
		return -1;
	}

	int i;

	for (i = 0; i < sizeof(argv) / sizeof(argv[0]); i++) {
		argv[i] = p;
		p = strtok(NULL, " ");
		if (!p) {
			argv[i + 1] = 0;
			break;
		}
	}

	pid_t pid = fork();


	if ( pid < 0 ) { // child creation fails
		printf("ERROR: forking child process failed\n");
		return -1;
	}
	else if ( pid == 0 ) { // child
		if ( execvp(*argv, argv) == -1 ) { // child command execution fails
			printf("ERROR: \"%s\" command failed\n%s", *argv, PROMPT);
			exit(errno);
		}
	}
	else { // parent
		int status;
		int retVal = waitpid(pid, &status, WNOHANG | WUNTRACED | WCONTINUED);

		if ( retVal == -1 ) { // waitpid fails
			printf("ERROR: waitpid failed\n");
			return -1;
		}

		char cwd[256];
		getcwd(cwd, sizeof(cwd));

		insert(pid, cwd, argv[0]);

	}

	return 0;
}

void insert(pid_t pid, char *cwd, char *name) {
	if (numProc < MAX_BG_PROCS) {
		const char c[2] = "/";
		char *token, *s;
		token = strtok(name, c);

		while (token != NULL) { // just to get rid of "./" of "./xxx.o"; must be a better way.
			s = token;
			token = strtok(NULL, c);
		}

		procArray[numProc].pid = pid;
		strcpy(procArray[numProc].path, cwd);
		strcpy(procArray[numProc].name, s);
		procArray[numProc].status = 1;

		numProc++;
	}
	else {
		printf("ERROR: max # of background processes reached\n");
	}
}

void list() {
	int i = 0;

	printf("\n====\trunning processes\t====\n");
	for ( i = 0; i < MAX_BG_PROCS; i++ ) {
		struct proc_elem x = procArray[i];

		if (x.pid != 0 && x.status == 1) {
			printf("%d:\t%s/%s\n", x.pid, x.path, x.name);
		}
	}

	printf("====\tpaused processes\t====\n");
	for ( i = 0; i < MAX_BG_PROCS; i++ ) {
		struct proc_elem x = procArray[i];

		if (x.pid != 0 && x.status == 0) {
			printf("%d:\t%s/%s\n", x.pid, x.path, x.name);
		}
	}
	printf("\nTotal background jobs: %d\n", numProc);
}

int run_bgkill(char *input) {
	input += 6;

	char *p = strtok(input, " ");

	if (!p) {
		printf("ERROR: no pid given after bgkill\n");
		return -1;
	}

	pid_t pid = atoi(p);
	int killed = 0;
	int i;

	for (i = 0; i < MAX_BG_PROCS; i++) {
		struct proc_elem x = procArray[i];

		if (x.pid == pid && pid != 0) {
			int k = kill(pid, SIGTERM);

			if (k == 0) {
				procArray[i].pid = 0;
				procArray[i].path[0] = 0;
				procArray[i].name[0] = 0;
				procArray[i].status = 0;
				killed = 1;
				numProc--;
				printf("Process %d is terminated\n", pid);
				break;
			}
			else {
				printf("ERROR: process %d cannot be terminated\n", pid);
				return -1;
			}
		}
	}

	if (killed != 1) {
		printf("ERROR: process %s does not exist\n", p);
		return -1;
	}

	return 0;
}

int run_bgstop (char *input) {
	input += 6;

	char *p = strtok(input, " ");

	if (!p) {
		printf("ERROR: no pid given after bgstop\n");
		return -1;
	}

	pid_t pid = atoi(p);
	int stopped = 0;
	int i;

	for (i = 0; i < MAX_BG_PROCS; i++) {
		struct proc_elem x = procArray[i];

		if (x.pid == pid && pid != 0) {
			int k = kill(pid, SIGSTOP);

			if (k == 0) {
				if (procArray[i].status == 0) {
					printf("ERROR: process %d is already paused\n", pid);
					return -1;
				}
				procArray[i].status = 0;
				stopped = 1;
				printf("Process %d is paused\n", pid);
				break;
			}
			else {
				printf("ERROR: process %d cannot be paused\n", pid);
				return -1;
			}
		}
	}

	if (stopped != 1) {
		printf("ERROR: process %s does not exist\n", p);
		return -1;
	}

	return 0;
}

int run_bgstart(char *input) {
	input += 7;

	char *p = strtok(input, " ");

	if (!p) {
		printf("ERROR: no pid given after bgstart\n");
		return -1;
	}

	pid_t pid = atoi(p);
	int started = 0;
	int i;

	for (i = 0; i < MAX_BG_PROCS; i++) {
		struct proc_elem x = procArray[i];

		if (x.pid == pid && pid != 0) {
			int k = kill(pid, SIGCONT);

			if (k == 0) {
				if (procArray[i].status == 1) {
					printf("ERROR: process %d is already running\n", pid);
					return -1;
				}
				procArray[i].status = 1;
				started = 1;
				printf("Process %d is resumed\n", pid);
				break;
			}
			else {
				printf("ERROR: process %d cannot be resumed\n", pid);
				return -1;
			}
		}
	}

	if (started != 1) {
		printf("ERROR: process %s does not exist\n", p);
		return -1;
	}

	return 0;
}

int run_pstat(char *input) {
	input += 6;

	char *p = strtok(input, " ");

	if (!p) {
		printf("ERROR: no pid given after bgstart\n");
		return -1;
	}

	pid_t pid = atoi(p);
	int i;
	int found = 0;

	for (i = 0; i < MAX_BG_PROCS; i++) {
		struct proc_elem x = procArray[i];

		if (x.pid == pid && x.pid != 0) {


			char *s1 = "/proc/";
			char *s2 = p;
			char *s3 = "/stat";
			char *path = (char *)malloc(1 + strlen(s1) + strlen(s2) + strlen(s3));
			char *path2 = (char *)malloc(3 + strlen(s1) + strlen(s2) + strlen(s3));
			strcpy(path, s1);
			strcat(path, s2);
			strcat(path, s3);
			strcpy(path2, path);
			strcat(path2, "us");

			printf("pstat of process %d:\n", pid);

			FILE* f;
			f = fopen(path, "r");
			char ret = 0;
			char buffer[100];

			int j = 1;
			do {
				ret = fscanf(f, "%s", buffer);
				if (j == 2) printf("comm: %s, ", buffer);
				if (j == 3) printf("state: %s, ", buffer);
				char *ptr;
				if (j == 14) printf("utime: %lu, ", strtoul(buffer, &ptr, 10));
				if (j == 15) printf("stime: %lu, ", strtoul(buffer, &ptr, 10));
				if (j == 24) printf("rss: %ld\n", strtoul(buffer, &ptr, 10));
				j++;
			} while (ret != EOF);

			fclose(f);
			free(path);

			FILE* f2;
			f2 = fopen(path2, "r");
			char line[100];
			while (fgets(line, sizeof(line), f2)) {
				char *c = strtok(line, "\t");
				if (strcmp(c, "voluntary_ctxt_switches:") == 0) {
					printf("voluntary_ctxt_switches: %s, ", strtok(strtok(NULL, "\t"), "\n"));
				}
				else if (strcmp(c, "nonvoluntary_ctxt_switches:") == 0) {
					printf("nonvoluntary_ctxt_switches: %s", strtok(strtok(NULL, "\t"), "\n"));
				}
			}

			printf("\n");
			fclose(f2);
			free(path2);

			found = 1;
			break;
		}
	}

	if (!found) {
		printf("ERROR: process %s does not exist\n", p);
		return -1;
	}

	return 0;
}

int run_exit() {
	int i = 0;

	for (i = 0; i < MAX_BG_PROCS; i++) {
		struct proc_elem x = procArray[i];
		if (x.pid != 0) kill(x.pid, SIGKILL);
	}

	printf("%sBye...\n", PROMPT);

	return 0;
}
