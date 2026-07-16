#ifndef JOB
#define JOB

#define MAX_JOBS 4096

int add_job(pid_t pid, char* line);

typedef enum jobstatus{
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
}jobstatus;

void job_query();
void job_query_on_status(jobstatus status);
void check_job();
void stop_job(int id);
void end_job(int id);

#endif