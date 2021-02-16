#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>

#define inputSize 1024
#define jobsSize 1024
#define true 1
#define false 0

typedef struct job {
    pid_t jobPid;
    char input[inputSize];
    int isEmpty;
}job;

//functions declaration
void cd(char* args[],char prevPath[inputSize]);
void splitToArgs(char *args[inputSize], char input[inputSize] , int *runInBack, char inputCopy[inputSize]);
void systemCall(char *args[inputSize],const int runInBack,job jobsArr[jobsSize],char inputForJobs[inputSize]);
int processEnded(pid_t process_id);
void printJobs(job jobsArr[jobsSize]);
void killJobs(job jobsArr[jobsSize]);

/**
 * main function
 */
int main() {

    char input[inputSize];
    int runInBack=false;
    char *args[inputSize];
    job jobsArr[jobsSize];
    char inputCopy[inputSize];
    char prevPath[inputSize];

    //initialize the array of structs.
    int i=0;
    for(i;i<jobsSize;i++){
        job job1;
        job1.isEmpty=true;
        jobsArr[i]=job1;
    }

    while(true){
        printf("prompt>");
        fgets(input, inputSize, stdin);
        input[strlen(input) - 1] = '\0';
        strcpy(inputCopy, input);

        if(strcmp(input, "exit")==0){
            printf("%d \n", getpid());
            killJobs(jobsArr);
            break;
        }
        else if (strcmp(input, "jobs")==0){
            printJobs(jobsArr);
        }
        else if (strcmp(input, "")!=0){
            //breaking the string into an array of arguments
            splitToArgs(args, input, &runInBack,inputCopy);
            if(strcmp(args[0], "cd")==0){
                printf("%d \n", getpid());
                cd(args,prevPath);
            }
            else{
                systemCall(args,runInBack,jobsArr,inputCopy);
            }
        }
    }

   return 0;
}

/******
 * splits the array into string arguments and checks if the command needs to work in the background
 * @param args the array of strings to fill
 * @param input the command string to split
 * @param runInBack a flag to raise if the command needs to run in the background
 */
void splitToArgs(char *args[inputSize], char input[inputSize] , int *runInBack, char inputCopy[inputSize]) {
    char *token;
    const char delim[2]=" ";
    int i=0;
    token=strtok(input,delim);
    while(token!=NULL){
        args[i]=token;
        i++;
        token=strtok(NULL,delim);
    }
    if(strcmp(args[i-1],"&")==0){
        *runInBack=true;
        args[i-1]=NULL;
        inputCopy[strlen(inputCopy)-1]='\0';
    }
    else{
        args[i]=NULL;
    }
}

/**
 * the implementation of cd
 * @param args
 * @param prevPath the previous path for cd -
 */
void cd(char* args[], char prevPath[inputSize]){
    //just cd or cd ~ goes to "home" library
    if ((args[1] == NULL)||(strcmp(args[1],"~")==0)){
        getcwd(prevPath, inputSize); //save the previous path
        if(chdir(getenv("HOME")) == -1){ //change current path and check error
            fprintf(stderr, "Error in system call\n");
            return;
        }

        //cd - returns to previous path
    }else if (strcmp(args[1],"-")==0){
        char temp[inputSize];
        getcwd(temp, inputSize);//read current path into temp
        if (chdir(prevPath) == -1) {//change current path to previous one and check for error
            fprintf(stderr, "Error in system call\n");
            return;
        }
        printf("%s\n", prevPath ); //printing current path after change
        strcpy(prevPath, temp); // inserting the previous path to prevPath

        // cd .. - move to uper library
    } else if (strcmp(args[1],"..")==0){
        char path[inputSize];
        getcwd(path, inputSize);//get the current path
        while (strcmp((char[2]){path[strlen(path)-1],'\0'},"/")!=0){ //i need to compare the last char with '/' so i turn it into a string
            path[strlen(path)-1]='\0';
        }
        if (chdir(path) == -1) {//change current path to previous one and check for error
            fprintf(stderr, "Error in system call\n");
            return;
        }
        //path given
    }else {
        getcwd(prevPath, inputSize); //save the previous path
        if (chdir(args[1]) == -1) { //change current path and check for error
            fprintf(stderr, "Error in system call\n");
            return;
        }
    }
}

/**
 * the exec calls , wait if needed or if in background insert to jobs array
 * @param args
 * @param runInBack
 * @param jobsArr
 * @param inputForJobs
 */
void systemCall(char *args[inputSize], const int runInBack,job jobsArr[jobsSize], char inputForJobs[inputSize]){
    int value, status;
    pid_t pid=fork();
    if(pid == 0){ //in child
        value=execvp(args[0],&args[0]);
        if(value == -1) {
            fprintf(stderr, "Error in system call\n");
            exit(-1);
        }
    }
    else{ //in parent
        printf("%d \n", pid);
        if(runInBack == false){
            wait(&status);
        }
        else{ //insert to jobsArr
            int i=0;
            for(i;i<jobsSize;i++){
                if((jobsArr[i].isEmpty)||(!jobsArr[i].isEmpty && processEnded(pid))){//cell is empty or process there finished
                    jobsArr[i].jobPid=pid;
                    strcpy(jobsArr[i].input,inputForJobs);
                    jobsArr[i].isEmpty=false;
                    break;
                }
            }
        }
    }
}

/********
 *  checks if the process ended and returns true or false
 * @param process_id the id of the process we are checking
 * @return true or false
 */
int processEnded(pid_t process_id){
    int status;
    pid_t  return_pid=waitpid(process_id,&status,WNOHANG);
    if (return_pid == -1) {
        fprintf(stderr, "Error in process\n");
    } else if(return_pid == 0){
        return false; //child is still running
    } else if(return_pid == process_id){
        return true;//child has finished
    }
}

/**********
 * prints the jobs running in the background, while checking also sets the flags in the array
 * @param jobsArr the array of jobs
 */
void printJobs(job jobsArr[jobsSize]){
    int i=0;
    for(i;i<jobsSize;i++){
        if(jobsArr[i].isEmpty==false && processEnded(jobsArr[i].jobPid)){ //the process ended
            jobsArr[i].isEmpty=true; //we set the flag to empty
        }
        else if(jobsArr[i].isEmpty==false && !processEnded(jobsArr[i].jobPid)){//the process is in progress
            printf("%d %s\n",jobsArr[i].jobPid, jobsArr[i].input);
        }
    }
}

/**
 * kills the processes still running in the background
 * @param jobsArr the array of jobs
 */
void killJobs(job jobsArr[jobsSize]){
    int i=0;
    for(i;i<jobsSize;i++){
        if(jobsArr[i].isEmpty==false && !processEnded(jobsArr[i].jobPid)){//the process is in progress
            kill(jobsArr[i].jobPid, SIGKILL);
            job job1;
            job1.isEmpty=true;
            jobsArr[i]=job1;
        }
    }
}
