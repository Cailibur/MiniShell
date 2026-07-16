#ifndef JOB
#define JOB

#define MAX_JOBS 4096

void add_job(pid_t pid, char* line);

typedef enum jobstatus{
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
}jobstatus;

void job_query();
void job_query_on_status(jobstatus status);
void check_job();

#endif