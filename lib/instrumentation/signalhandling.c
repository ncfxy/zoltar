
#include <stdlib.h>
#include <stdio.h>

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
  
  if(sigNum==SIGINT) {
    return;
  }

  _destroyInstrumentationInfo();

  exit(0);
}


