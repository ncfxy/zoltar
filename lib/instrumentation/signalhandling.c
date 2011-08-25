
#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "signalhandling.h"
#include "instrumentationinfo.h"

void _handleSignal(int sigNum) {
  switch(sigNum) {
    case SIGSEGV:
      #ifdef DEBUG
      fprintf(stderr, "!! caught signal SIGSEGV\n");
      #endif
      break;
    case SIGINT:
      #ifdef DEBUG
      fprintf(stderr, "!! caught signal SIGINT\n");
      #endif
      break;
    case SIGPROF:
      fprintf(stderr, "Timeout.\n");
      break;
    default:
      #ifdef DEBUG
      fprintf(stderr, "!! caught undefined signal\n");
      #endif
      break;
  }
  #ifdef DEBUG
  fprintf(stderr, "   premature end of execution\n");
  #endif
      
  /* TODO: handle end-of-run */
  
  #ifdef ENABLE_DATA_IO
    _instrumentationInfo->passFail[_instrumentationInfo->run] = 0;
    _instrumentationInfo->run++;
    _finalize();
  #endif
  
  if(sigNum==SIGINT) {
    return;
  }

  //_destroyInstrumentationInfo();

  exit(0);
}


