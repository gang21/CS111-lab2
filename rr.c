#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint32_t u32;
typedef int32_t i32;

struct process
{
  u32 pid;
  u32 arrival_time;
  u32 burst_time;

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  u32 finish_time;
  u32 time_left;
  /* End of "Additional fields here" */
};

TAILQ_HEAD(process_list, process);

u32 next_int(const char **data, const char *data_end)
{
  u32 current = 0;
  bool started = false;
  while (*data != data_end)
  {
    char c = **data;

    if (c < 0x30 || c > 0x39)
    {
      if (started)
      {
        return current;
      }
    }
    else
    {
      if (!started)
      {
        current = (c - 0x30);
        started = true;
      }
      else
      {
        current *= 10;
        current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data)
{
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++]))
  {
    if (c < 0x30 || c > 0x39)
    {
      exit(EINVAL);
    }
    if (!started)
    {
      current = (c - 0x30);
      started = true;
    }
    else
    {
      current *= 10;
      current += (c - 0x30);
    }
  }
  return current;
}

void init_processes(const char *path,
                    struct process **process_data,
                    u32 *process_size)
{
  int fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1)
  {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED)
  {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL)
  {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i)
  {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
  }

  munmap((void *)data, size);
  close(fd);
}

  //switching process function
void switch_process(struct process_list list) {
  struct process * r = malloc(sizeof(struct process));
  struct process * head = TAILQ_FIRST(&list);
    r->arrival_time = head->arrival_time;
    r->burst_time = head->burst_time;
    r->finish_time = head->burst_time;
    r->pid = head->pid;
    r->time_left = head->time_left;
    TAILQ_INSERT_TAIL(&list, r, pointers);
    TAILQ_REMOVE(&list, head, pointers);
    free(head);
}
int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    return EINVAL;
  }
  struct process *data;
  u32 size;
  init_processes(argv[1], &data, &size);

  u32 quantum_length = next_int_from_c_str(argv[2]);

  struct process_list list;
  TAILQ_INIT(&list);

  u32 total_waiting_time = 0;
  u32 total_response_time = 0;

  /* Your code here */

  //creating global "clock"
  u32 clock = 0;
  u32 current_process_quantum_time = quantum_length;
  u32 num_processes_left = size;

  //fix this while statement
  while(num_processes_left > 0) {
    //adding to queue based on arrival time
    for(int i = 0; i < size; i++) {
      if (data[i].arrival_time == clock) {
        struct process * next = malloc(sizeof(struct process));
        TAILQ_INSERT_TAIL(&list, next, pointers);
        next->arrival_time = data[i].arrival_time;
        next->burst_time = data[i].burst_time;
        next->pid = data[i].pid;
        next->time_left = data[i].burst_time;
      }
    }

    if (current_process_quantum_time > 0) {
      //run the current process
      struct process * p = TAILQ_FIRST(&list);
      //response time update
      //switch processes if finished with current one
      if (p->time_left == 0) {
        num_processes_left--;
        p->finish_time = clock;
        u32 waiting_time = p->finish_time - p->arrival_time - p->burst_time;
        total_waiting_time += waiting_time;
        // printf("Process %d waiting time: %d\n", p->pid, waiting_time);
        TAILQ_REMOVE(&list, p, pointers);
        free(p);
        if (TAILQ_EMPTY(&list)) {
          continue;
        }
        //starting new process
        struct process * head = TAILQ_FIRST(&list);
        if (head->time_left == head->burst_time) {
          u32 first_execution = clock;
          u32 response_time = first_execution - head->arrival_time;
          total_response_time += response_time;
        }
        // printf("Clock: %d - REMOVE: %d, %d, %d, %d\n", clock, head->pid, head->arrival_time, head->burst_time, head->time_left);
        head->time_left--;

        // last response time
        current_process_quantum_time = quantum_length;
        current_process_quantum_time--;
      }
      else {
        //response time update
        if (p->time_left == p->burst_time) {
          u32 first_execution = clock;
          u32 response_time = first_execution - p->arrival_time;
          total_response_time += response_time;
        }
          // printf("Clock: %d - process: %d, %d, %d, %d\n", clock, p->pid, p->arrival_time, p->burst_time, p->time_left);
          current_process_quantum_time--;
          p->time_left--;
      }
    }
    //switching processes
    else if (current_process_quantum_time == 0){
      struct process * r = malloc(sizeof(struct process));
      struct process * head = TAILQ_FIRST(&list);
      r->arrival_time = head->arrival_time;
      r->burst_time = head->burst_time;
      r->finish_time = head->burst_time;
      r->pid = head->pid;
      r->time_left = head->time_left;
      if (r->time_left > 0) {
        TAILQ_INSERT_TAIL(&list, r, pointers);
      }
      else {
        num_processes_left--;
        r->finish_time = clock;
        u32 waiting_time = r->finish_time - r->arrival_time - r->burst_time;
        total_waiting_time += waiting_time;
        // printf("Process %d waiting time: %d\n", r->pid, waiting_time);
        free(r);
      }
      TAILQ_REMOVE(&list, head, pointers);
      free(head);

      //making sure queue is not empty
      if (TAILQ_EMPTY(&list)) {
        continue;
      }      
      struct process * next = TAILQ_FIRST(&list);
      //response time update
      if (next->time_left == next->burst_time) {
        u32 first_execution = clock;
        u32 response_time = first_execution - next->arrival_time;
        total_response_time += response_time;
      }
      // printf("Clock: %d - NEXT: %d, %d, %d, %d\n", clock, next->pid, next->arrival_time, next->burst_time, next->time_left);

      next->time_left--;

      current_process_quantum_time = quantum_length;
      current_process_quantum_time--;
    }
    clock++;
  }

  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}
