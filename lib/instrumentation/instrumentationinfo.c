
#include <stdlib.h>
#include <stdio.h>

#include "instrumentationinfo.h"
#include "config.h"
#ifdef THREAD_SAFE
  #include <pthread.h>
  #include <unistd.h>
#endif


/* init at zero */
_InstrumentationInfo *_instrumentationInfo = (void*)0;
void *_instrumentationInfoEnd = (void*)0;

void _initInstrumentationInfo(unsigned int timestamp) {
  int i;

  _instrumentationInfo = (_InstrumentationInfo*) calloc(1, sizeof(_InstrumentationInfo));
  if (!_instrumentationInfo) {
    fprintf(stderr, "Error locating memory, aborting...\n");
    abort();
  }

  _instrumentationInfo->id = timestamp;
  _instrumentationInfo->end = (char*)_instrumentationInfo + sizeof(_InstrumentationInfo) - 1;
  _instrumentationInfoEnd = _instrumentationInfo->end;

  _instrumentationInfo->training = 1;
  _instrumentationInfo->run = 0;

  for(i=0; i<MAX_RUNS; i++) {
    _instrumentationInfo->passFail[i] = 0;
  }

  for(i=0; i<MAX_SPECTRA; i++) {
    SPECTRUM(i).nComponents = 0;
    SPECTRUM(i).data = 0;
    SPECTRUM(i).name = (char*)"-";
  }
  for(i=0; i<MAX_INVARIANTTYPES; i++) {
    INVARIANTTYPE(i).nInvariants = 0;
    INVARIANTTYPE(i).isTimerUpdated = 0;
    INVARIANTTYPE(i).data = 0;
    INVARIANTTYPE(i).name = (char*)"-";
  }

  #ifdef THREAD_SAFE
    pthread_mutex_init(&_instrumentationInfo->spectrumMutex, NULL);
    pthread_mutex_init(&_instrumentationInfo->invariantMutex, NULL);
    pthread_mutex_init(&_instrumentationInfo->libMutex, NULL);
  #endif

  #ifdef DEBUG
    fprintf(stderr, "initInstrumentationInfo\n");
    fprintf(stderr, "  id:       %u\n", _instrumentationInfo->id);
    fprintf(stderr, "  memstart: %p\n", (void*)_instrumentationInfo);
    fprintf(stderr, "  memend:   %p\n", _instrumentationInfo->end);
  #endif
}

void _addSpectrum(unsigned int timestamp, unsigned int index, unsigned int nComponents, char *name) {
  unsigned int oldSize, addedSize, i;
  void *spectrumStart;
  void *oldInstrumentationInfo;

  if (index >= MAX_SPECTRA) {
    return;
  }

  /* create instrumentation info if not created yet */
  if (!_instrumentationInfo) {
    _initInstrumentationInfo(timestamp);
  } else if (_instrumentationInfo->id == 0) {
    _instrumentationInfo->id = timestamp;
  }

  spectrumStart = (char*)_instrumentationInfo->end + 1;
  oldSize = (char*)_instrumentationInfo->end - (char*)_instrumentationInfo + 1;
  addedSize = sizeof(unsigned int)*nComponents*MAX_RUNS;
  oldInstrumentationInfo = _instrumentationInfo;

  #ifdef DEBUG
    fprintf(stderr, "Reallocating %u bytes for new spectrum\n", addedSize);
  #endif

  /* expand memory block to include new spectrum data */
  _instrumentationInfo = (_InstrumentationInfo*) realloc(_instrumentationInfo, oldSize + addedSize);
  if (!_instrumentationInfo) {
    fprintf(stderr, "Error locating memory, aborting...\n");
    abort();
  }

  /* update instrumentation info */
  _instrumentationInfo->end = (char*)_instrumentationInfo + oldSize + addedSize - 1;
  _instrumentationInfoEnd = _instrumentationInfo->end;
  _instrumentationInfo->nSpectra++;
  SPECTRUM(index).nComponents = nComponents;
  SPECTRUM(index).data = spectrumStart;
  SPECTRUM(index).name = name;
  if (_instrumentationInfo != oldInstrumentationInfo) {
    /* reallocation to other block */
    long diff = (char*)_instrumentationInfo - (char*)oldInstrumentationInfo;
    for(i=0; i<MAX_SPECTRA; i++) {
      if(SPECTRUM(i).data != 0) {
        SPECTRUM(i).data = (void*)((char*)SPECTRUM(i).data + diff);
      }
    }
    for(i=0; i<MAX_INVARIANTTYPES; i++) {
      if(INVARIANTTYPE(i).data != 0) {
        INVARIANTTYPE(i).data = (void*)((char*)INVARIANTTYPE(i).data + diff);
      }
    }
  }

  /* init spectrum data */
  for(i=0; i<nComponents*MAX_RUNS; i++) {
    SPECTRUM(index).data[i] = 0;
  }

  #ifdef DEBUG
    fprintf(stderr, "addSpectrum:\n");
    fprintf(stderr, "  pmem start:  %p\n", (void*)_instrumentationInfo);
    fprintf(stderr, "  pmem end:    %p\n", _instrumentationInfo->end);
    fprintf(stderr, "  data start:  %p\n", (void*)(SPECTRUM(index).data));
    fprintf(stderr, "  unit size:   %u\n", sizeof(unsigned int));
    fprintf(stderr, "  nComponents: %u\n", SPECTRUM(index).nComponents);
    fprintf(stderr, "  nRuns:       %u\n", MAX_RUNS);
    fprintf(stderr, "  name:        %s\n", SPECTRUM(index).name);
  #endif
}

void _addInvariantType(unsigned int timestamp, unsigned int index, unsigned int nInvariants, char *name, char isTimerUpdated) {
  unsigned int oldSize, addedSize, i;
  void *invariantTypeStart;
  void *oldInstrumentationInfo;

  if (index >= MAX_INVARIANTTYPES) {
    return;
  }
 
  /* create instrumentation info if not created yet */
  if (!_instrumentationInfo) {
    _initInstrumentationInfo(timestamp);
  } else if (_instrumentationInfo->id == 0) {
    _instrumentationInfo->id = timestamp;
  }

  invariantTypeStart = (char*)_instrumentationInfo->end + 1;
  oldSize = (char*)_instrumentationInfo->end - (char*)_instrumentationInfo + 1;
  addedSize = sizeof(_Invariant)*nInvariants;
  oldInstrumentationInfo = _instrumentationInfo;

  #ifdef DEBUG
    fprintf(stderr, "Reallocating %u bytes for new invariant type\n", addedSize);
  #endif

  /* expand memory block to include new invariant type data */
  _instrumentationInfo = (_InstrumentationInfo*) realloc(_instrumentationInfo, oldSize + addedSize);
  if (!_instrumentationInfo) {
    fprintf(stderr, "Error locating memory, aborting...\n");
    abort();
  }

  /* update instrumentation info */
  _instrumentationInfo->end = (char*)_instrumentationInfo + oldSize + addedSize - 1;
  _instrumentationInfoEnd = _instrumentationInfo->end;
  _instrumentationInfo->nInvariantTypes++;
  INVARIANTTYPE(index).nInvariants = nInvariants;
  INVARIANTTYPE(index).isTimerUpdated = isTimerUpdated;
  INVARIANTTYPE(index).data = invariantTypeStart;
  INVARIANTTYPE(index).name = name;
  if (_instrumentationInfo != oldInstrumentationInfo) {
    /* reallocation to other block */
    long diff = (long)_instrumentationInfo - (long)oldInstrumentationInfo;
    for(i=0; i<MAX_SPECTRA; i++) {
      if(SPECTRUM(i).data != 0) {
        SPECTRUM(i).data = (void*)((char*)SPECTRUM(i).data + diff);
      }
    }
    for(i=0; i<MAX_INVARIANTTYPES; i++) {
      if(INVARIANTTYPE(i).data != 0) {
        INVARIANTTYPE(i).data = (void*)((char*)INVARIANTTYPE(i).data + diff);
      }
    }
  }

  /* init invariant data */
  for(i=0; i<nInvariants; i++) {
    INVARIANTTYPE(index).data[i].datatype = DATA_TYPE_UNSET;
    if (INVARIANTTYPE(index).isTimerUpdated) {
      INVARIANTTYPE(index).data[i].datatype = DATA_TYPE_INT;
      INVARIANTTYPE(index).data[i].usage = 0;
      INVARIANTTYPE(index).data[i].activatedScreener = SCRN_RANGE;
      INVARIANTTYPE(index).data[i].timerInvariantError = 0;
      INVARIANTTYPE(index).data[i].nErrors = 0;
      INVARIANTTYPE(index).data[i].runAtPrevError = 0;
      INVARIANTTYPE(index).data[i].range.i.val = -1;
      INVARIANTTYPE(index).data[i].range.i.min = -1;
      INVARIANTTYPE(index).data[i].range.i.max = 0;
    }
  }

  #ifdef DEBUG
    fprintf(stderr, "addInvariantType:\n");
    fprintf(stderr, "  pmem start:     %p\n", (void*)_instrumentationInfo);
    fprintf(stderr, "  pmem end:       %p\n", _instrumentationInfo->end);
    fprintf(stderr, "  data start:     %p\n", (void*)(INVARIANTTYPE(index).data));
    fprintf(stderr, "  unit size:      %u\n", sizeof(_Invariant));
    fprintf(stderr, "  nInvariants:    %u\n", INVARIANTTYPE(index).nInvariants);
    fprintf(stderr, "  isTimerUpdated: %d\n", INVARIANTTYPE(index).isTimerUpdated);
    fprintf(stderr, "  name:           %s\n", INVARIANTTYPE(index).name);
  #endif
}

void _destroyInstrumentationInfo() {
  if (_instrumentationInfo) {
    #ifdef THREAD_SAFE
      pthread_mutex_destroy(&_instrumentationInfo->spectrumMutex);
      pthread_mutex_destroy(&_instrumentationInfo->invariantMutex);
      pthread_mutex_destroy(&_instrumentationInfo->libMutex);
    #endif
    free(_instrumentationInfo);
    _instrumentationInfo = (void*)0;
    _instrumentationInfoEnd = (void*)0;
    return;
  }

  #ifdef DEBUG
    fprintf(stderr, "While destroying instrumentation info: was not yet allocated\n");
  #endif
}

