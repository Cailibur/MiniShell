#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include "job.h"
#include "shell.h"


typedef struct job{
    pid_t pid;
    char command[MAX_LINE];
    jobstatus status;
}job;

int tot_job_id = 0;
job jobs[MAX_JOBS];

int add_job(pid_t pid, char* line){
    tot_job_id++;
    jobs[tot_job_id].pid = pid;
    strcpy(jobs[tot_job_id].command, line);
    jobs[tot_job_id].status = JOB_RUNNING;
    return tot_job_id;
}

void stop_job(int id){
    if(id > tot_job_id){
        fprintf(stderr, "jobs: invalid id out of bound");
        return;
    }
    jobs[id].status = JOB_STOPPED;
}

void end_job(int id){
    if(id > tot_job_id){
        fprintf(stderr, "jobs: invalid id out of bound");
        return;
    }
    jobs[id].status = JOB_DONE;
}

char* status_to_str(jobstatus status){
    switch (status)
    {
    case JOB_STOPPED:
        return "STOPPED";
    case JOB_RUNNING:
        return "RUNNING";
    case JOB_DONE:
        return "DONE";
    default:
        return "NULL";
    }
}

void job_query(){
    for(int i = 1; i <= tot_job_id; i++){
        fprintf(stdout, "%d \033[32m%d \033[0m%s \033[1m\033[31m%s\033[0m\n", i, jobs[i].pid, jobs[i].command, status_to_str(jobs[i].status));
    }
    //printf("%s %s %s\n", status_to_str(JOB_STOPPED), status_to_str(JOB_RUNNING), status_to_str(JOB_DONE));
}

void job_query_on_status(jobstatus status){
    for(int i = 1; i <= tot_job_id; i++){
        if(status == jobs[i].status) fprintf(stdout, "%d \033[32m%d \033[0m%s \033[1m\033[31m%s\033[0m\n", i, jobs[i].pid, jobs[i].command, status_to_str(jobs[i].status));
    }
}

void check_job(){
    int status;
    //detect every son process when jobs query on
    for(int i = 1; i <= tot_job_id; i++){
        if(jobs[i].status == JOB_RUNNING){
            if(waitpid(jobs[i].pid, &status, WNOHANG | WUNTRACED) > 0){
                if(WIFEXITED(status) || WIFSIGNALED(status)){
                    end_job(i);
                }
                else if(WIFSTOPPED(status)){
                    stop_job(i);
                }
            }
        }
    }
}