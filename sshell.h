#include "get_path.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int pid;
int sshell( int argc, char **argv, char **envp);
void which(char *command, struct pathelement *pathlist);
void where(char *command, struct pathelement *pathlist);
void list ( char *dir );

void printenv(char **envp, char **args, int numArgs);
void read_command(char cmd[], char** par, int *num, char **argv);

#define PROMPTMAX 32
#define MAXARGS 10
