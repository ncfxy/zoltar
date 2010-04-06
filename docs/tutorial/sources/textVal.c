
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* shared data */
int val;

/* mutex */
pthread_mutex_t mutex;

/* function for updating val */
void updateVal(int d) {
  int i;
  int tmp = val;
  /* introduce some delay */
  for (i = 0; i < tmp + 100; i++);
  tmp += d;
  val = tmp;
}

/* read characters */
void *readLetters(void *buffer) {
  char *c = (char*)buffer;
  while (*c != 0) {
    if (isalpha(*c)) {
      pthread_mutex_lock(&mutex);
      updateVal(1);
      pthread_mutex_unlock(&mutex);
    }
    c++;
  }
  pthread_exit(NULL);
}

void *readDigits(void *buffer) {
  char *c = (char*)buffer;
  while (*c != 0) {
    if (isdigit(*c)) {
      /* BUG: no mutex locking! */
      updateVal(2);
    }
    c++;
  }
  pthread_exit(NULL);
}

void *readOther(void *buffer) {
  char *c = (char*)buffer;
  while (*c != 0) {
    if (!isalnum(*c) && !isspace(*c)) {
      pthread_mutex_lock(&mutex);
      updateVal(10);
      /* BUG: forgot to release the lock! */
    }
    c++;
  }
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  char buffer[1024];
  pthread_t letterThread;
  pthread_t digitThread;
  pthread_t otherThread;
  pthread_mutex_init(&mutex, NULL);
  val = 0;
  do {
    /* read the next 1023 characters and
       create three threads to process each character type */
    int num = fread(buffer, 1, 1023,  stdin);
    buffer[num] = 0;
    if(num == 0) {
      break;
    }
    if (pthread_create(&letterThread, 
                       NULL, 
                       readLetters, 
                       (void*) buffer)) {
      fprintf(stderr, "error creating thread\n");
      exit(-1);
    }
    if (pthread_create(&digitThread, 
                       NULL, 
                       readDigits, 
                       (void*) buffer)) {
      fprintf(stderr, "error creating thread\n");
      exit(-1);
    }
    if (pthread_create(&otherThread, 
                       NULL, 
                       readOther, 
                       (void*) buffer)) {
      fprintf(stderr, "error creating thread\n");
      exit(-1);
    }
    pthread_join(digitThread, NULL);
    pthread_join(letterThread, NULL);
    pthread_join(otherThread, NULL);
  } while(1);
  pthread_mutex_destroy(&mutex);
  printf("%d\n", val);
  return 0;
}


