
#include "config.h"

#ifdef THREADED
  #include <pthread.h>
  #include <unistd.h>
#endif
#ifdef THREAD_SAFE
  #include <pthread.h>
  #include <unistd.h>
#endif

#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "signalhandling.h"
#include "instrumentationinfo.h"
#include "invariant.h"
#include "spectrum.h"
#include "programdata.h"
#include "fileio.h"
#include "string.h"
//#include "unistd.h"


struct timeval _programStart, _programEnd;
char *cwd=NULL;
char *datfname;

void _startTiming() {
  gettimeofday(&_programStart, NULL);
}

void _stopTiming() {
  gettimeofday(&_programEnd, NULL);
}

void _fprintTiming(FILE* f) {
  struct timeval span;
  span.tv_sec = _programEnd.tv_sec - _programStart.tv_sec;
  span.tv_usec = _programEnd.tv_usec - _programStart.tv_usec;
  if(span.tv_usec<0) {
    span.tv_sec -= 1;
    span.tv_usec += 1000000;
  }
  fprintf(f, "execution time: %u.%06u\n", (unsigned int)span.tv_sec, (unsigned int)span.tv_usec);
}

void _initialReadDataFile() {
  //char *datfname = (char*)"datafile.dat";
  ProgramData *programData = newProgramData();
  int i;

  if(!readDataFile(datfname, programData, 0)) {
    if(programData->version != _instrumentationInfo->id) {
      fprintf(stderr, "Data corruption reading file:\n");
      fprintf(stderr, " Invalid instrumentation version of existing datafile.\n");
      fprintf(stderr, " Remove or rename datafile and try again.\n");
      abort();
    }

    if(programData->opMode==0) {
      TRAINING=1; /* set training mode */
    } else if(programData->opMode==1) {
      TRAINING=0; /* set testing mode */
    }

    for(i=0; i<MAX_INVARIANTTYPES; i++) {
      unsigned int ii;

      for(ii=0; ii<INVARIANTTYPE(i).nInvariants; ii++) {
        _Invariant *source, *target;
        source = &programData->invariantType[i].data[ii];
        target = &INVARIANTTYPE(i).data[ii];
        if(source->datatype == DATA_TYPE_UNSET) {
          continue;
        }
        target->datatype = source->datatype;
        target->usage = source->usage;
        target->activatedScreener = source->activatedScreener;
        switch(target->datatype) {
          case DATA_TYPE_UINT:
            target->range.u.min = source->range.u.min;
            target->range.u.max = source->range.u.max;
            break;
          case DATA_TYPE_INT:
            target->range.i.min = source->range.i.min;
            target->range.i.max = source->range.i.max;
            break;
          case DATA_TYPE_DOUBLE:
            target->range.d.min = source->range.d.min;
            target->range.d.max = source->range.d.max;
            break;
          case DATA_TYPE_PTR:
            target->range.p.min = source->range.p.min;
            target->range.p.max = source->range.p.max;
            break;
          default:
            fprintf(stderr, "unknown datatype in datafile!\n");
            abort();
        }
        target->bitmask.first = source->bitmask.first;
        target->bitmask.mask = source->bitmask.mask;
      }
    }
  }
  freeProgramData(programData);
}


void _updateDataFile() {
  //char *datfname = (char*)"datafile.dat";
  char *tmpdatfname = tmpfname(datfname);
  ProgramData *programData = newProgramData();
  int i, res;

  if(!readDataFile(datfname, programData, 1)) {
    /* datafile exists, update */
    if(programData->version != _instrumentationInfo->id) {
      fprintf(stderr, "Data corruption reading file:\n");
      fprintf(stderr, " Invalid instrumentation version of existing datafile.\n");
      fprintf(stderr, " Remove or rename datafile and try again.\n");
      abort();
    }

    programData->passFail = realloc(programData->passFail, (programData->nRuns+_instrumentationInfo->run+1) * sizeof(char));
    for(i=0; i<(int)_instrumentationInfo->run; i++) {
      programData->passFail[programData->nRuns+i] = _instrumentationInfo->passFail[i];
    }

    programData->nRuns+=_instrumentationInfo->run;
    programData->passFail[programData->nRuns] = 0;

    for(i=0; i<MAX_SPECTRA; i++) {
      int comps = programData->spectrum[i].nComponents;
      programData->spectrum[i].data = (unsigned int*)realloc(programData->spectrum[i].data, comps * programData->nRuns * sizeof(unsigned int));
      if(comps > 0) {
        int r;
        for(r=0; r<(int)_instrumentationInfo->run; r++) {
          int c;
          int pdRun = programData->nRuns - _instrumentationInfo->run + r;
          for(c=0; c<comps; c++) {
            programData->spectrum[i].data[pdRun*comps + c] = SPECTRUM(i).data[r*comps + c];
          }
        }
      }
    }
    for(i=0; i<MAX_INVARIANTTYPES; i++) {
      int invs = programData->invariantType[i].nInvariants;
      int inv;
      for(inv=0; inv<invs; inv++) {
        programData->invariantType[i].data[inv] = INVARIANTTYPE(i).data[inv];
      }
    }

    /* write data */
    res = writeDataFile(tmpdatfname, programData);
    if(res) {
      fprintf(stderr, "Error writing datafile: res=%d\n", res);
    } else {
      if(remove(datfname)) {
        fprintf(stderr, "Error removing outdated datafile '%s'\n", datfname);
        fprintf(stderr, "New datafile is stored in '%s'\n", tmpdatfname);
      } else if(rename(tmpdatfname, datfname)) {
        fprintf(stderr, "Error renaming temporary datafile '%s' to '%s'\n", tmpdatfname, datfname);
        fprintf(stderr, "New datafile is stored in '%s'\n", tmpdatfname);
      } else {
      }
    }
  } else {
    /* no datafile exists, create */
    programData->version = _instrumentationInfo->id;
    programData->opMode = 0;
    programData->nRuns = _instrumentationInfo->run;

    programData->passFail = calloc(_instrumentationInfo->run+1, sizeof(char));
    for(i=0; i<(int)_instrumentationInfo->run; i++) {
      programData->passFail[i] = _instrumentationInfo->passFail[i];
    }
    
    for(i=0; i<MAX_SPECTRA; i++) {
      int comps = SPECTRUM(i).nComponents;
      programData->spectrum[i] = SPECTRUM(i);
      programData->spectrum[i].data = (unsigned int*)calloc(comps * programData->nRuns, sizeof(unsigned int));
      if(comps > 0) {
        int r;
        for(r=0; r<(int)_instrumentationInfo->run; r++) {
          int c;
          int pdRun = programData->nRuns - _instrumentationInfo->run + r;
          for(c=0; c<comps; c++) {
            programData->spectrum[i].data[pdRun*comps + c] = SPECTRUM(i).data[r*comps + c];
          }
        }
      }
    }
    for(i=0; i<MAX_INVARIANTTYPES; i++) {
      int invs = INVARIANTTYPE(i).nInvariants;
      int inv;
      programData->invariantType[i] = INVARIANTTYPE(i);
      programData->invariantType[i].name = (char*)calloc(strlen(INVARIANTTYPE(i).name)+1, sizeof(char));
      strcpy(programData->invariantType[i].name, INVARIANTTYPE(i).name);
      programData->invariantType[i].data = (_Invariant*)calloc(invs, sizeof(_Invariant));
      for(inv=0; inv<invs; inv++) {
        programData->invariantType[i].data[inv] = INVARIANTTYPE(i).data[inv];
      }
    }

    res = writeDataFile(tmpdatfname, programData);
    if(res) {
      fprintf(stderr, "Error writing datafile: res=%d\n", res);
    } else {
      if(rename(tmpdatfname, datfname)) {
        fprintf(stderr, "Error renaming temporary datafile '%s' to '%s'\n", tmpdatfname, datfname);
        fprintf(stderr, "New datafile is stored in '%s'\n", tmpdatfname);
      } else {
      }
    }
  }

  free(tmpdatfname);
  freeProgramData(programData);
 
}


void _handleInvariantRangeError(_Invariant *inv);
void _registerAll();
int _main_original(int argc, char* argv[]);

#ifdef THREADED
  struct strMainArgs {
    int ret;
    int argc;
    char **argv;
  };
  void *programThread(void *t) {
    struct strMainArgs *mainArgs = (struct strMainArgs*)t;
    mainArgs->ret = _main_original(mainArgs->argc, mainArgs->argv);
    return 0;
  }
  struct strTimerArgs {
    int msInterval;
    void (*timerFunction)(void);
  };
  int _running;
  void *timerThread(void *t) {
    struct strTimerArgs *timerArgs = (struct strTimerArgs*)t;
    while(_running) {
      usleep(1000 * timerArgs->msInterval);
      #ifdef DEBUG
        fprintf(stderr, "timer event (%dms)\n", timerArgs->msInterval);
      #endif
      if(_running) {
        timerArgs->timerFunction();
      }
    }
    return 0;
  }
  void *runTimerThread(void *t) {
    int cnt = 0;
    while(_running) {
      usleep(10000); /* check every 10 ms */
      cnt += 10000;
      if(cnt < 1000000 * 60) { /* run duration of 60 seconds */
        continue;
      }
      #ifdef DEBUG
        fprintf(stderr, "new run initiated\n");
      #endif
      /* set values for new run, previous run is considered passed */
      _instrumentationInfo->nBitmaskErrors = 0;
      _instrumentationInfo->nRangeErrors = 0;
      _instrumentationInfo->passFail[_instrumentationInfo->run] = 1;
      _instrumentationInfo->run++;
      cnt = 0;
    }
    return 0;
  }
  void _handleInvariantTimerUpdate() {
    unsigned int i,j;
    for(i=0; i<MAX_INVARIANTTYPES; i++) {
      if(INVARIANTTYPE(i).isTimerUpdated) {
        #ifdef THREAD_SAFE
          pthread_mutex_lock(&_instrumentationInfo->invariantMutex);
        #endif
        for(j=0; j<INVARIANTTYPE(i).nInvariants; j++) {
          if(INVARIANTTYPE(i).data[j].range.i.val >= 0) {
            INVARIANTTYPE(i).data[j].usage++;
            INVARIANTTYPE(i).data[j].range.i.val += 1;
            if(INVARIANTTYPE(i).data[j].range.i.val > INVARIANTTYPE(i).data[j].range.i.max) {
              if(TRAINING) {
                INVARIANTTYPE(i).data[j].range.i.max = INVARIANTTYPE(i).data[j].range.i.val;
              } else {
                if(!INVARIANTTYPE(i).data[j].timerInvariantError) {
                  INVARIANTTYPE(i).data[j].timerInvariantError = 1;
                  _handleInvariantRangeError(&(INVARIANTTYPE(i).data[j]));
                }
              }
            }
          }
        }
        #ifdef THREAD_SAFE
          pthread_mutex_unlock(&_instrumentationInfo->invariantMutex);
        #endif
      }
    }
  }
#endif


pthread_t t_timer;
pthread_mutex_t libMutex;
void _finalize() {

    _running = 0;
  pthread_join(t_timer, NULL);

  #ifdef ENABLE_DATA_IO
    _updateDataFile();
  #endif

  _destroyInstrumentationInfo();

}


int _exitHandled=0;

void _exithandler(int status) {
  #ifdef THREAD_SAFE
    pthread_mutex_lock(&libMutex);
  #endif

  if(_exitHandled) {
    #ifdef THREAD_SAFE
      pthread_mutex_unlock(&libMutex);
    #endif
    return;
  }

  #ifdef THREADING
    _running = 0;
  #endif

  /* in training mode, do not save the failing run */
  /*if(status==0 || !_instrumentationInfo->training) {
    _instrumentationInfo->passFail[_instrumentationInfo->run] = 0;
    _instrumentationInfo->run++;
  }*/
  /* assume exit is meant to be, so no error */
    _instrumentationInfo->passFail[_instrumentationInfo->run] = 1;
    _instrumentationInfo->run++;

  _stopTiming();

  fprintf(stderr, "program exit with status %d.\n", status);
  _fprintTiming(stderr);

  _finalize();
  
  _exitHandled = 1;

  #ifdef THREAD_SAFE
    pthread_mutex_unlock(&libMutex);
  #endif

  exit(0);
}

void _aborthandler() {
  #ifdef THREAD_SAFE
    pthread_mutex_lock(&libMutex);
  #endif

  if(_exitHandled) {
    #ifdef THREAD_SAFE
      pthread_mutex_unlock(&libMutex);
    #endif
    return;
  }

  #ifdef THREADING
    _running = 0;
  #endif

  /* in training mode, do not save the failing run */
  /* TODO: what about the changed invariants, keep a tmp? */
  if(!_instrumentationInfo->training) {
    _instrumentationInfo->passFail[_instrumentationInfo->run] = 0;
    _instrumentationInfo->run++;
  }

  _stopTiming();

  fprintf(stderr, "program aborted.\n");
  _fprintTiming(stderr);

  _finalize();
  
  _exitHandled = 1;

  #ifdef THREAD_SAFE
    pthread_mutex_unlock(&libMutex);
  #endif

  #ifdef THREAD_SAFE
    pthread_mutex_destroy(&libMutex);
  #endif

  exit(0);
}

int main(int argc, char* argv[]) {
  int retvalue = 0, rlret;
  char *dataFileName, *dir, buf[1024];
  
  signal(SIGSEGV, _handleSignal);
  signal(SIGINT, _handleSignal);

  //sprintf(dataFileName, "%s.dat", argv[0]);

  rlret = readlink("/proc/self/exe", buf, 1023);
  if (rlret != -1) {
    buf[rlret] = 0;
    dir = dirname(buf);
  } else {
    dir = getcwd(NULL, 0);
  }
  fprintf(stderr, "Writing datafile in %s.\n", dir);
  dataFileName = (char*)"datafile.dat";
  datfname = malloc(strlen(dir) + strlen(dataFileName) + 2);
  sprintf(datfname, "%s/%s", dir, dataFileName);
  //fprintf(stderr, "%s\n", datfname);
  //exit(0);
  

  #ifdef THREADED
    #ifdef DEBUG
      fprintf(stderr, "Threaded mode enabled.\n");
    #endif
  #endif

  #ifdef DEBUG
    fprintf(stderr, "program start\n");
  #endif

  #ifdef THREAD_SAFE
    pthread_mutex_init(&libMutex, NULL);
  #endif

  _registerAll();

  if(!_instrumentationInfo) {
    _initInstrumentationInfo(0);
  }

  _initialReadDataFile();

  _startTiming();

  /* call original main function */
  #ifdef THREADED
    {
      struct strMainArgs mainArgs;
      struct strTimerArgs timerArgs;
      pthread_t t_program, t_runtimer;
      /* TODO: threaded?? 
         issue: stack space for thread */
      /*mainArgs.ret = 0;
      mainArgs.argc = argc;
      mainArgs.argv = argv;
      if(pthread_create(&t_program, NULL, programThread, (void*)&mainArgs)) {
        fprintf(stderr, "Error creating program thread.\n");
        abort();
      }*/
      _running = 1;
      timerArgs.msInterval = 1;
      timerArgs.timerFunction = &_handleInvariantTimerUpdate;
      if(pthread_create(&t_timer, NULL, timerThread, (void*)&timerArgs)) {
        fprintf(stderr, "Error creating timer thread.\n");
        abort();
      }
      if(pthread_create(&t_runtimer, NULL, runTimerThread, NULL)) {
        fprintf(stderr, "Error creating run-timer thread.\n");
        abort();
      }

      /* non-threaded main call */
      retvalue = _main_original(argc, argv);

      /*pthread_join(t_program, NULL);*/
      _running = 0;

      _instrumentationInfo->passFail[_instrumentationInfo->run] = 1;
      _instrumentationInfo->run++;

      /*pthread_join(t_timer, NULL);*/
      pthread_join(t_runtimer, NULL);
      /*retvalue = mainArgs.ret;*/
    }
  #else
    retvalue = _main_original(argc, argv);
  #endif

  #ifdef THREAD_SAFE
    pthread_mutex_lock(&libMutex);
  #endif

  _stopTiming();

  #ifdef DEBUG
    fprintf(stderr, "program returned with value %d\n", retvalue);
    _fprintTiming(stderr);
  #endif

  _finalize();

  _exitHandled = 1;

  #ifdef THREAD_SAFE
    pthread_mutex_unlock(&libMutex);
  #endif

  return 0;
}


