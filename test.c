#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#define POP_SIZE 10

int main(int argc, char *argv[]){
    pid_t firstFork;
    int *status;
    int numStudents = 0;
    pid_t managerChild, managerParent;
    pid_t students[POP_SIZE];
    int studentStatus[POP_SIZE];


    switch(firstFork = fork()){
        case -1:
            perror("Something wrong with fork()\n");
            break;
        case 0:
            managerChild = getpid();
            printf("Manager Child Process %d started\n", managerChild);
            printf("I have to create %d Student Processes\n", POP_SIZE);
            for(int i = 0; i < POP_SIZE; i++){
                switch(students[i] = fork()){
                    case -1:
                        perror("Something wrong with FORK in Manager Child Process\n");
                        break;
                    case 0:
                        printf("Created first Student Process PID: %d\n", getpid());
                        numStudents++;
                        break;
                    default:
                        printf("Haven't created all Student Processes\n");
                        waitpid(managerChild, status, WUNTRACED | WNOHANG);
                        printf("%d Student Processes succesfully created\n", numStudents);
                        break;
                }
            }
            break;
        default:
            for(int i = 0; i < POP_SIZE; i++)
                wait(NULL);
    }   
}