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
  u32 first_execution;
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
  int clock = 0;
  int current_process_quantum_time = quantum_length;
  int total_time = data[0].arrival_time;
  //getting the total amount of time the process should run
  for(int i = 0; i < size; i++) {
    total_time += data[i].burst_time;
  }

  //adding dummy node to queue
  struct process * dummy = malloc(sizeof(struct process));
  TAILQ_INSERT_HEAD(&list, dummy, pointers);

  //fix this while statement
  while(clock < total_time) {
    //removing the dummy node
    if(TAILQ_FIRST(&list)->pid == 0 && clock == data[0].arrival_time) {
      TAILQ_REMOVE(&list, TAILQ_FIRST(&list), pointers);
    }
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
      //switch processes if finished with current one
      if (p->time_left == 0) {
        TAILQ_REMOVE(&list, p, pointers);
        free(p);
        struct process * head = TAILQ_FIRST(&list);
        printf("Clock: %d - REMOVE: %d, %d, %d, %d\n", clock, head->pid, head->arrival_time, head->burst_time, head->time_left);

        head->time_left--;
        current_process_quantum_time = quantum_length;
        current_process_quantum_time--;
      }
      else {
        printf("Clock: %d - process: %d, %d, %d, %d\n", clock, p->pid, p->arrival_time, p->burst_time, p->time_left);
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
      r->first_execution = head->first_execution;
      r->pid = head->pid;
      r->time_left = head->time_left;
      TAILQ_INSERT_TAIL(&list, r, pointers);
      TAILQ_REMOVE(&list, head, pointers);
      free(head);

      struct process * temp = TAILQ_FIRST(&list);
      printf("Clock: %d - NEXT: %d, %d, %d, %d\n", clock, temp->pid, temp->arrival_time, temp->burst_time, temp->time_left);

      TAILQ_FIRST(&list)->time_left--;
      current_process_quantum_time = quantum_length;
      current_process_quantum_time--;
    }


    //print debug - DELETE LATER
    // struct process * p = TAILQ_FIRST(&list);
    // printf("Clock: %d - process: %d, %d, %d\n", clock, p->pid, p->arrival_time, p->burst_time);
    clock++;
  }

  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}
