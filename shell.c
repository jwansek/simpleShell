#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include "shell.h"

#define MAX_CMD 1024
#define MAX_PATH 512

int main(int argc, char* argv[]) {
    // setup an array of the values in $PATH
    char* pathstr = getenv("PATH");
    const char* delim = ":";
    const char* cmddelim = " ";
    const int binlen = strcnt(pathstr, delim[0]) + 1;
    char* bins[binlen];
    char* path = strtok(pathstr, delim);
    int i = 0;
    while (path != NULL) {
        bins[i++] = path;
        path = strtok(NULL, delim);
    }

    while (true) {
        //prefix command, show user, host, cwd
        char hostbfr[256];
        gethostname(hostbfr, sizeof(hostbfr));
        char cwdbfr[MAX_PATH];
        getcwd(cwdbfr, sizeof(cwdbfr));
        printf(
            "[\033[1;32m%s\033[0m@\033[1;31m%s\033[0m]:\033[1;34m%s\033[1;31m$ \033[0m", getenv("USER"), 
            hostbfr, cwdbfr
        );

        // get the input string - keep accepting words until a newline
        char cmdstr[MAX_CMD];
        scanf("%[^\n]", cmdstr);
        int c;
        while ((c = getchar()) != '\n' && c != EOF);    //clear buffer

        // parse the input string into an array, splitting by spaces
        uint cmdlen = strcnt(cmdstr, cmddelim[0]) + 1;
        char* cmdarr[cmdlen + 1];
        char* arg = strtok(cmdstr, cmddelim);
        i = 0;
        while (arg != NULL) {
            cmdarr[i++] = arg;
            arg = strtok(NULL, cmddelim);
        }

        //work out if we need to change the cwd
        if (strcmp(cmdarr[0], "cd") == 0) {
            // printf("Change directory to '%s'\n", cmdarr[1]);
            if (chdir(cmdarr[1]) != 0) {
                printf("\033[0;31mERROR - Could not change cwd\n\033[0m");
            }
            continue;
        }

        //work out if we need to exit
        if (exitPrefix(cmdstr)) {
            printf("Exited.\n");
            break;
        }

        // try to find a binary
        strcpy(cmdstr, cmdarr[0]);
        bool binExists = false;
        char cmdpath[512];
        for (i = 0; i < binlen + 1; i++) {
            if (i >= binlen) {
                strcpy(cmdpath, cwdbfr);
            } else {
                strcpy(cmdpath, bins[i]);
            }
            strcat(cmdpath, "/");
            strcat(cmdpath, cmdstr);
            if (access(cmdpath, F_OK) != -1) {
                binExists = true;
                break;
            }
        }
        if (!(binExists)) {
            printf("Couldn't find binary - Looked in the following places:\n - %s\n", cwdbfr);
            for (i = 0; i < binlen; i++) {
                printf(" - %s\n", bins[i]);
            }
            continue;
        }
        // printf("Found binary at '%s'\n", cmdpath);
        cmdarr[cmdlen] = NULL;
        
        pid_t pid = fork();
        bool success = true;

        if (pid == 0) {
            // printf("Child Process: PID = %i, PPID = %i\n", getpid(), getppid());
            if (execvp(cmdpath, cmdarr) < 0) {
                perror("\033[0;31mexec error ");
                printf("\033[0m");
                success = false;
            }
            break;

        } else {
            // printf("Parent Process: PID = %i, PPID = %i\n", getpid(), getppid());
            int status;
            if (waitpid(pid, &status, 0) < 0) {
                perror("\033[0;31mwaitpid error ");
                printf("\033[0m");
                success = false;
            }
            if (success) {
                printStatus(status);
            }
        }
    }
    return EXIT_SUCCESS;
}

// prints out the status code of an exited process.
// prints in green if the status code is zero. anything
// else is red.
void printStatus(int status) {
    if (status == 0) {
        printf("\033[0;32m");
    } else {
        printf("\033[0;31m");
    }
    printf("[Process exited with status code %i]\n\033[0m", status);
}

// returns an integer of how many times a char was found in a char
// array
int strcnt(char* str, const char cnt) {
    int count = 0;
    for (int i = 0; i < INT_MAX; i++) {
        char c = str[i];
        if (c == '\0') {
            break;
        }
        if (c == cnt) {
            count++;
        }
    }
    return count;
}

// returns a boolean if a char array starts with the string "exit"
bool exitPrefix(const char* cmdstr) {
    char fourChars[5];
    strncpy(fourChars, cmdstr, 4);
    return strcmp(fourChars, "exit") == 0;
}