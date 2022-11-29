#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h> // library for fcntl function

// library for fcntl function

#define SHMSZ 10
#define MAX_PATHNAME_LEN 1000
#define MAX_EVENTS 1024                           /* Maximum number of events to process*/
#define LEN_NAME 16                               /* Assuming that the length of the filename \
                             won't exceed 16 bytes*/
#define EVENT_SIZE (sizeof(struct inotify_event)) /*size of one event*/
#define BUF_LEN (MAX_EVENTS * (EVENT_SIZE + LEN_NAME))
/*buffer to store the data of events*/
int fd,
    wd;
void sig_handler(int sig)
{

  /* Step 5. Remove the watch descriptor and close the inotify instance*/
  inotify_rm_watch(fd, wd);
  close(fd);
  exit(0);
}

int directory_monitor(char *location)
{

  char *path_to_be_watched;
  signal(SIGINT, sig_handler);
  path_to_be_watched = location;
  /* Step 1. Initialize inotify */
  fd = inotify_init();

  if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) // error checking for fcntl
    exit(2);

  /* Step 2. Add Watch */
  wd = inotify_add_watch(fd, path_to_be_watched, IN_MODIFY | IN_CREATE | IN_DELETE);

  if (wd == -1)
  {
    printf("Could not watch : %s\n", path_to_be_watched);
  }
  else
  {
    printf("Watching : %s\n", path_to_be_watched);
  }

  while (1)
  {

    int i = 0, length;
    char buffer[BUF_LEN];

    /* Step 3. Read buffer*/
    length = read(fd, buffer, BUF_LEN);

    /* Step 4. Process the events which has occurred */
    while (i < length)
    {

      struct inotify_event *event = (struct inotify_event *)&buffer[i];
      if (event->len)
      {
        if (event->mask & IN_CREATE)
        {
          if (event->mask & IN_ISDIR)
          {
            printf("The directory %s was created.\n", event->name);
          }
          else
          {
            printf("The file %s was created.\n", event->name);
          }
          return 1;
        }
        else if (event->mask & IN_DELETE)
        {
          if (event->mask & IN_ISDIR)
          {
            printf("The directory %s was deleted.\n", event->name);
          }
          else
          {
            printf("The file %s was deleted.\n", event->name);
          }
          return 1;
        }
        else if (event->mask & IN_MODIFY)
        {
          if (event->mask & IN_ISDIR)
          {
            printf("The directory %s was modified.\n", event->name);
          }
          else
          {
            printf("The file %s was modified.\n", event->name);
          }
          return 1;
        }
      }
      i += EVENT_SIZE + event->len;
      return 0;
    }
  }
}

struct user
{
  char location[256];
  int token;
  int sleep;
};

struct user *shm;

int onlineUsers()
{
  int count = 0;
  for (int k = 1; shm[k].token != -1; k++)
  {
    count++;
  }
  return count;
}
int main()
{

  char c;
  int shmid;
  key_t key;
  char *s;

  key = 5680;
  if ((shmid = shmget(key, SHMSZ, IPC_CREAT | 0666)) < 0)
  {
    perror("shmget");
    exit(1);
  }
  if ((shm = shmat(shmid, NULL, 0)) == (char *)-1)
  {
    perror("shmat");
    exit(1);
  }
  char cwd[256];

  getcwd(cwd, sizeof(cwd));
  int curr_token; /*
  printf("%s",shm[0].location);
  printf("%d",strlen(shm[0].location));
  if (curr_token==0){

  }*/

  for (curr_token = 1; curr_token < SHMSZ; curr_token++)
  {
    if (strcmp(cwd, shm[curr_token].location) == 0)
    {
      break;
    }
    if (shm[curr_token].token != curr_token)
    {
      shm[curr_token].token = curr_token;
      strcpy(shm[curr_token].location, cwd);
      shm[curr_token].sleep = 1;
      shm[curr_token + 1].token = -1;
      break;
    }
  }

  printf("Online players:%d\t", onlineUsers());
  int status;
  printf("%d", curr_token);

  while (1)
  {
    shm[curr_token].sleep = 1;
    printf("\nSleeping...\n");
    while (status = directory_monitor(shm[curr_token].location) != 1)
    {
      sleep(2);
    }
    shm[curr_token].sleep = 0;
    for (int k = 1; k < onlineUsers()+1; k++)
    {
      while ((curr_token > k) && (shm[k].sleep == 0))
      {
        sleep(2);
      }
      if (curr_token != k)
      {
        printf("\nWriting from %s to %s", shm[curr_token].location, shm[k].location);
        char file[100][100];
        int i = 0, c = 0;
        DIR *dr;
        struct dirent *en;
        dr = opendir("."); // open all or present directory
        if (dr)
        {
          while ((en = readdir(dr)) != NULL)
          {
            int len = strlen(en->d_name);
            const char *last_four = &en->d_name[len - 4];
            if (strcmp(last_four, ".txt") == 0)
            {
              strcpy(file[c], en->d_name);

              c++;
            }
            i++;
          }
          closedir(dr); // close all directory
        }
        // CREATING FILE
        FILE *fp[c];
        char pathFile[MAX_PATHNAME_LEN];
        for (int i = 0; i < c; i++)
        {
          sprintf(pathFile, "%s/%s", shm[curr_token].location, file[i]);
          fp[i] = fopen(pathFile, "a+");
          fclose(fp[i]);
        }
        // READING FILE
        FILE *present_fp[c];
        FILE *other_fp[c];
        char str[1000];
        for (int i = 0; i < c; i++)
        {
          present_fp[i] = fopen(file[i], "r");
          fscanf(present_fp[i], "%[^\n]s", str);
          fclose(present_fp[i]);
          // WRITING TO OTHER FILE
          sprintf(pathFile, "%s/%s", shm[k].location, file[i]);
          other_fp[i] = fopen(pathFile, "w+");
          fprintf(other_fp[i], "%s", str);
          fclose(other_fp[i]);
        }
      }
    }
  }
  return 0;
}
