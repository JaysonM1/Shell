//Partners Mario Durso and Jayson Morgado

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "sshell.h"
#include <wordexp.h>
#include <utmpx.h>
#include <pthread.h>


pid_t pid;




void pipeFunction(char **para);




//takes in any string and sees if it can be expanded
//returns the closest expanded string of the wildcard sting if found
//else returns the given string
char *fixWildcard(char *str){
  char **w;
  wordexp_t p;
  if( strchr (str, '*') != NULL || strchr (str, '?') != NULL){
    if(wordexp(str, &p, 0) == 0 ){
     w = p.we_wordv;
     return w[0];
    }
  }
  else{
    return str;
  }
  wordfree(&p);
}

//wordexp is easy solution to wildcard as it finds all the possible extentions ofa
//   file that has wildcard chars and it will just return the input if no expantions
//   are found
//takes in a string and returns an array of all the extended files of the string name
char **fixSingleWildcardArg(char *str){
  wordexp_t p;

  if(wordexp(str, &p, 0) == 0 ){
    return p.we_wordv;
  }
}



//read_command takes in the user input and parses it into the command and parameters
void read_command(char cmd[], char** par, int *numParameters, char **args){
 char line[1024];
 int count = 0;
 int iter = 0;
 int iterWild =0;
 char *token;
 char **wildcard;
 char *parCopy[20];
 //Read line and return a single string of the entered characters
 //fgets (line, 1024, stdin);
 if(fgets (line, 1024, stdin) == NULL){
   cmd = "newline";
   numParameters = 0;
   args[0] == 0;
   printf("%s1" , cmd);
   return;
 }

 //Pars the line to get the command and parameters
 //Get the first word for the command
 token = strtok(line, " \n");
 if (token == NULL){
   strcpy(cmd,"newline");
   numParameters = 0;
   args[0] == 0;
   return;
 } 
 
 strcpy(cmd, fixWildcard(token));
 args[count] = fixWildcard(token);
 

 // walk through other tokens for parameters
 // cannot check for wildcards here due to
 // memmory access constaints
 while( token != NULL ) {
  token = strtok(NULL, " \n");
     par[count] = token;
     args[count+1] = token;
     count ++;
 }
 if( token == NULL){
  par[count] = NULL;
  args[count+1] = NULL;
 }

 //sets the total number of parameters
 count --;
 *numParameters = count;

 //make copy of intial args for wildcardfixing
 while(par[iter] != NULL){
   parCopy[iter] = par[iter];
   iter++;
 }


 parCopy[iter] = NULL;
 iter = 0;
 //loop to fix and wildcards in args
 if(count >= 1){
   count = 0;
   while(parCopy[iter] != NULL){
     if( strchr (parCopy[iter], '*') != NULL || strchr (parCopy[iter], '?') != NULL){
       wildcard = fixSingleWildcardArg(parCopy[iter]);
       iterWild =0;
       while(wildcard[iterWild] != NULL){
         par[count] = wildcard[iterWild];
         args[count+1] = wildcard[iterWild];
         count ++;
         iterWild ++;
       }
       iter ++;
     }
     else{
       count++;
       iter++;
     }
   }
   par[count] = NULL;
   args[count+1] = NULL;
 *numParameters = count;
 }
}


//absolute command runs executables that start with '.' or "/"
void absolute_command(char *command, char **argv, char **envp,
	       	pid_t pid, int numParameters ){
  if(access(command, X_OK) == 0){
	  pid = fork();
	  if(pid == 0){
		  printf("%d", execve(command, argv, envp));
	  }
	  else{
		  if(strcmp(argv[numParameters], "&") == 0){
			  waitpid(pid,NULL, WNOHANG);
		  }
		  else{	
			  waitpid(pid,NULL,0); 
		  }
	  }
	  printf("\n");
  } 
  else{
    perror("invalid command");
  }
}

//where finds all the possible paths to the given command
void where(char *command, struct pathelement *p){
  char cmd[128];
  while (p) {         // WHERE
    sprintf(cmd,"%s/%s", p->element,command);
    if (access(cmd, X_OK) == 0)
      printf("[%s]\n", cmd);
    p = p->next;
  }
}


//which finds the first path to the given command
void which(char *command, struct pathelement *p){
  char cmd[128];
  while (p) {         // WHICH
    sprintf(cmd,"%s/%s", p->element,command);
    if (access(cmd, X_OK) == 0) {
      printf("[%s]\n", cmd);
      break;
    }
    p = p->next;
  }
}

//cd changes the current directory
void cd(char **para, int numArgs, const char *homedir,char *prev){
  if(numArgs == 0){
    chdir(getenv("HOME"));
  }
  else
  if(strcmp(para[0], "-") == 0){
      chdir(prev);   
  }
  else{
    if(chdir(para[0]) == -1){
      perror("Error: Could not change current directory. To many args.\n");
    }
  }
}

//printls command finds all files in a directory and prints them
void printls(char *p){
	DIR *dp;
	struct dirent *dirp;
	if(opendir(p) == NULL){
		perror("");
	}
	else{
	  dp = opendir(p);
	  while((dirp = readdir(dp)) != NULL)
                printf("%s\n", dirp->d_name);
          closedir(dp);
	}

}


//kill command
//takes in the user args and then gets the signal if none is given
//or will use the given signal if the user passes it
void mykill(char **args, int argsLength){
  pid_t argPid;
  int arg1, arg2;
  const char *givenSignal;
  if (argsLength == 1){
    arg1 = atoi(args[0]);
    argPid = arg1;
    kill(arg1, SIGTERM);
  }
  else
  if (argsLength == 2){
    if(args[0][0] == '-'){
      givenSignal = &args[0][1];
      arg1 = atoi(givenSignal);
      arg2 = atoi(args[1]);
      argPid = arg2;
      kill(argPid, arg1);
    }
    else{
      perror("Invalid signal sent: must start with (-)");
    }
  }
  else{
    perror("Invalid number of args for kill");}
}

//printenv
void printenv(char **envp, char** args, int numArgs){
  int i= 0;
  if(numArgs == 0){
    while(envp[i] != NULL){
      printf("%s\n", envp[i]);
      i++;
    }
  }
  else
  if(numArgs == 1){
    printf("%s=%s\n", args[0], getenv(args[0]));
  }
  else{
    printf("invalid parameters");
  }
}
void sigintHandler(int sig_num)
{  
    signal(SIGINT, sigintHandler);
    printf("\n");
}

void sig_handlerSTP(int sig_num){
	signal(SIGTSTP,sig_handlerSTP);
	printf("\n");
}
 
void sig_handlerTERM(int sig_num){
	signal(SIGTERM,sig_handlerTERM);
	printf("\n");
}

void redirect_output(char* filename, char **envp, char* symbol){
        int fid;

        if(strcmp(getenv("noclobber"), "0") == 0){
                if(strcmp(symbol, ">") ==0 || strcmp(symbol, ">&") == 0){
                        fid = open(filename, O_WRONLY|O_TRUNC);
                        close(1);
                        dup(fid);
                        if(strcmp(symbol, ">&") == 0){
                                dup2(fid, STDERR_FILENO);
                        }
                        close(fid);
                }
                if(strcmp(symbol, ">>") == 0 || strcmp(symbol, ">>&") == 0){
                        fid = open(filename, O_WRONLY|O_APPEND);
                        close(1);
                        dup(fid);
                        if(strcmp(symbol, ">>&") == 0){
                                dup2(fid, STDERR_FILENO);
                        }
                        close(fid);
                        printf("\n");

                }
                if(strcmp(symbol, "<") == 0){
                        fid = open(filename, O_RDONLY|O_TRUNC);
                        close(1);
                        dup(fid);
                        dup2(fid, STDIN_FILENO);
                        close(fid);
                }
        }
        else{
                if(open(filename, O_RDONLY) < 0){
                        perror("File exists");
                }
                else{
                        perror("File does not exist");
                }
        }

}


void redirect_terminal(){
        int fid = open("/dev/tty", O_WRONLY);
        close(1);
        dup(fid);
        dup2(fid, STDERR_FILENO);
        //dup2(fid, STDIN_FILENO);
        close(fid);
}

void set_redirect(char* cmd, char** par, int *numParameters, char **args, char** envp){
        char symbols[7][5] = {">" , ">&", ">>", ">>&", "<", "|","|&"};
        int numPara = *numParameters;
        if(numPara > 1){
        for(int i = 0; i < 7; i++){
                //printf("%s\n", symbols[i]);
		if((strcmp("|",args[numPara-1]) == 0) || (strcmp("|&",args[numPara-1]) == 0)){
			pipeFunction(args);
				
		}
		else
           	if(strcmp(symbols[i], args[numPara-1]) == 0){
                        redirect_output(args[numPara], envp, args[numPara-1]);
                        for(int x = numPara-1; x <= numPara; x++){
                                args[x] = NULL;
                                par[x-1] = NULL;
                        }
                        *numParameters = numPara-2;
                        return;
                }
        }
        }
        return;

}


struct Node{
	struct Node *prev;
    	char* user;
    	struct Node *next;
};
struct Node* head = NULL;


struct Node* getNode(char *name){
	struct Node *temp = head;
	while(temp){
		if(strcmp(temp->user,name) == 0){
			return temp;
		}
	}
}


void removeFromLinkedList(struct Node *del){
	if (head == del){ 
        	head = head->next;
	}
	if(del->next != NULL)
		    del->next->prev = del->prev;

	if(del->prev != NULL)
    		del->prev->next = del->next;

 	free(del); //delete the node
}


void addToLinkedList(struct Node *insert){
	if(head == NULL){
		head = insert;
	}
	else{
		struct Node* end =head;
		insert->next = NULL;
		while (end->next != NULL) 
       	 		end = end->next;
	       end->next = insert;
  	       insert->prev = end; 	
	}
	
}


pthread_mutex_t lock;



int inLinkedList(char *name){
	struct Node* temp = head;
	while(temp){
		if(strcmp(temp->user,name) == 0){
			return 1;
		}
		temp = temp->next;
	}
	return 0;
}
void* runWatch(void *arg){
        while(1){
		struct utmpx *up;
		setutxent();			/* start at beginning */
	 	pthread_mutex_lock(&lock);
	
                //pthread_mutex_lock(&lock);
		while (up = getutxent()){	/* get an entry */
    			if ( up->ut_type == USER_PROCESS ){	/* only care about users */
    				if(inLinkedList(up->ut_user)){
					printf("Logged in: %s", up->ut_user);
				}
				else
					printf("here");	

    			}
  		}


		sleep(20);
        }
        pthread_mutex_unlock(&lock);
}

int runsThrough = 0;
pthread_t watchUse;
//watchuser
void watchuser(char* para,int off){
        if(off == 1){
                removeFromLinkedList(getNode(para));
                //turn off watch
        }
	if(runsThrough == 1){
		struct Node* use =(struct Node*) malloc(sizeof(struct Node));
		use->user = para;
		addToLinkedList(use);
	}
        else{
	     	//printf("here");
		runsThrough++;	
		struct Node* use =(struct Node*) malloc(sizeof(struct Node));
                use->user = para;	
		addToLinkedList(use);
		pthread_t tid[0];
                watchUse = pthread_create(&tid[0],NULL,runWatch,NULL);
                
		//pthread_join(watchUse,NULL);
                
        }
}



//function to handle ipc
void pipeFunction(char **para){
	   int pipefd[2], bytes;
           pid_t cpid;
           char buf;
           char *eString;
		char readBuffer[80];

	    int pfds[2];

    pipe(pfds);

    if (!fork()) {
        close(1);       
        dup(pfds[1]);   
        close(pfds[0]); 
        execlp("ls", "ls", NULL);
    } else {
        close(0);       
        dup(pfds[0]);   
        close(pfds[1]); 
      	execlp("wc", "wc", "-l", NULL);
    }    
}














//
//Start of main
//
//

int main(int argc, char **argv, char **envp){
//Shell variables:
 char *prompt = calloc(PROMPTMAX, sizeof(char));
 char *pwd;
 setenv("noclobber", "0", 1);
 //char *args[20];

 //directory variable 
 int uid, i, status, argsct, go = 1;
 struct passwd *password_entry;
 char *homedir;
 struct pathelement *pathlist;

 char *prev = getcwd(NULL,0);
 char *save = getcwd(NULL,0);;
 int cdaint=1;
 //command and parameter variables
 
 char **args = calloc(MAXARGS, sizeof(char*));
 
 //char *para[20];
 char **para = calloc(MAXARGS, sizeof(char*));
 
 
 //char command[100];
 char *command = calloc(PROMPTMAX, sizeof(char));
 int numParameters;

 //external command variables
 //char search[128];
 char *search = calloc(PROMPTMAX, sizeof(char));

 signal(SIGINT, sigintHandler); 
 signal(SIGTSTP, sig_handlerSTP);
 signal(SIGTERM, sig_handlerTERM);

//set pathlist
 pathlist = get_path();

//set home dir
  uid = getuid();
  password_entry = getpwuid(uid);               /* get passwd info */
  homedir = password_entry->pw_dir;
  
//shell loop for running comands
 while(1){

//update pathlist and current directory
  redirect_terminal();
  pathlist = get_path();
  pwd = getcwd(NULL, 0);
  //prompt msg to user
  printf("%s [%s]>", prompt, pwd);

  //read command that the user enters and parses the command
  read_command(command, para, &numParameters, args);
  set_redirect(command, para, &numParameters, args, envp);
  if(strcmp (command, "newline") == 0){ 
  }
  //checks and runs hello command
  
  else
  if(strcmp (command, "hello") == 0){
   printf("Executing built-in [hello]");
   printf("\nHELLO\n"); }
  
  //checks and runs the exit command
  else
  if(strcmp (command, "exit") == 0){
   printf("Executing built-in [exit]\n");
   break;
  }

  //checks and runs the which command
  else
  if( strcmp (command, "which") == 0){
   printf("Executing built-in [which]\n");
   which(para[0], pathlist);
  }

  //checks for then runs where command
  else
  if( strcmp(command, "where") == 0){
   printf("Executing built-in [where]\n");
   where(para[0], pathlist);
  }

  //checks for cd
  //runs cd
  else
  if( strcmp(command, "cd") == 0){
   printf("Executing built-in [cd]\n");
  
   if(cdaint % 2 == 1){
	prev = getcwd(NULL,0);   
   	cd(para, numParameters, homedir,save);
   }else{
	save = getcwd(NULL,0);
   	cd(para,numParameters,homedir,prev);
  }
   cdaint = cdaint + 1;
  }

  //checks and runs pwd
  else
  if( strcmp(command, "pwd") ==0){
   printf("Executing built-in [pwd]\n");
   printf("%s\n" , pwd);
  }

  //gets the first parameter of the passed command
  else
  if(strcmp(command, "para") ==0){
   printf("\n%s" , para[0]);
   printf("\n%d" , numParameters);
   printf("\n%s", command);
  }
  else
  if(strcmp(command, "args") ==0){
   int iterArgs =0; 
   while(args[iterArgs] != NULL){
     printf("%s\n" , args[iterArgs]);
     iterArgs ++;
   }
  }

  //checks for the ls command
  else
  if( strcmp(command, "list") == 0){
   printf("Executing built-in [ls]");
  	if(numParameters == 0){
		printls(pwd);}
	else
	if(numParameters == 1){
	  printls(para[0]);
	}
  }
  //check for kill command
  else
  if( strcmp(command, "kill") == 0){
    printf("Executing built-in [kill]");
    mykill(para, numParameters);
  }

  //checks for command pid
  //if command is pid will print the pid of the shell
  else
  if( strcmp(command, "pid") == 0){
    printf("Executing built-in [pid]");
    printf("%ld\n",(long)getpid());
  }

  //check and runs prompt command
  else
  if( strcmp(command, "prompt") == 0){
    if(numParameters == 0){
      printf("\tinput prompt prefix:");
      fgets (prompt, 64, stdin);
      prompt = strtok(prompt, " \n");
    }
    else
      strcpy(prompt,para[0]);
  }
 //printenv
  else
  if( strcmp(command, "printenv") == 0){
    printf("Executing built-in [printenv]\n");
    printenv(envp,para,numParameters);
  }
 

  //setenv
  else
  if( strcmp(command, "setenv") == 0){
	if(numParameters == 0){
		printenv(envp,para,numParameters);
	}
	else if(numParameters == 1){
		putenv(para[0]);
	}
	else if(numParameters == 2){
		setenv(para[0],para[1],1);
	}
	else
		printf("Error\n");

  }

  //watch user
  else
  if(strcmp(command, "watchuser") == 0){
	  if(numParameters > 2 || numParameters == 0){
		  printf("Incorrect arguemnts for watchuser");
	  }
          else{
		if(numParameters == 1){
			watchuser(para[0],0);
		}
		else
		if((numParameters == 2) && (strcmp(para[1],"off"))){
			watchuser(para[0],1);
	  	}
	  }
  }
  //checks if its an absolute command
  else
  if(command[0] == '.' || command[0] == '/'){
      absolute_command(command, args, envp, pid, numParameters);
      continue;
  }
  //search for external command 
  else {
    while (pathlist) {         // search for the command
      sprintf(search,"%s/%s", pathlist->element,command);
      //if command found will run the command
      if (access(search, X_OK) == 0) {
        printf("Executing [%s]" , search);
	
	//fork returns the parent id to the parent and for the child if the child is made
	pid = fork();
        if(pid == 0){
	//execvp will run command
	//note for later expand funtionality to include commands with no parameters
	 //set_redirect(command, para, &numParameters, args, envp);
	 execve(search, args, envp);
	}
	//waitpid with 0 as option so that the parent will
	//wait for the child to finish before any other
	//commands will be taken in
	else {                          /* parent */
          waitpid(pid, NULL, 0); 
	}
	printf("\n");
        break;
      }
      pathlist = pathlist->next;
    }
    //if the command is not found will return a message
    if(pathlist == NULL){
     printf("\n%s: Command not found.", command);
    }
  }
 
 
 
 }
 //free mem here
 free(command);
 free(para);
 free(args);
 free(prompt);
 free(search);
 pthread_join(watchUse,NULL);
 return 0;
}
