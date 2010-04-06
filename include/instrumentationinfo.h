
/*
 * --- instrumentationinfo ---
 *
 * Implements the way instrumentation info is kept in memory.
 * Contains functions to add spectra and invariants to the instrumentation info
 * and macros to access these.
 * Also, it contains a macro to check if a memory span does not overlap the complete
 * instrumentation info.
 *
 * The complete instrumentation info, containing multiple spectra and invariant data
 * for multiple invariant types, is located in a single memory block to limit a memory
 * storage check to a minimum, while still being able to ensure that all instrumentation 
 * information will not be corrupted.
 */

#ifndef _INSTRUMENTATIONINFO_H
#define _INSTRUMENTATIONINFO_H

#include "spectrum.h"
#include "invariant.h"
#include "config.h"
#ifdef THREAD_SAFE
  #include <pthread.h>
  #include <unistd.h>
#endif

#define MAX_SPECTRA         10
#define MAX_INVARIANTTYPES  10
#define MAX_RUNS           100

#define MAX_BITMASK_ERRORS    10
#define MAX_RANGE_ERRORS      10
#define MAX_INVARIANT_ERRORS  10

typedef struct _strInstrumentationInfo {
  unsigned int id;   /* unique instrumentation ID */
  void *end;         /* pointer to last alloc'ed byte */

  char training;     /* determines training mode */
  unsigned int run;  /* current run */
  unsigned int nBitmaskErrors;  /* number of bitmask errors during this run */
  unsigned int nRangeErrors;    /* number of range errors during this run */

  char passFail[MAX_RUNS];

  #ifdef THREAD_SAFE
    pthread_mutex_t spectrumMutex;
    pthread_mutex_t invariantMutex;
    pthread_mutex_t libMutex;
  #endif

  unsigned int nSpectra;          /* number of instrumented spectra */
  _Spectrum spectrum[MAX_SPECTRA];

  unsigned int nInvariantTypes;   /* number of instrumented invariant types */
  _InvariantType invariantType[MAX_INVARIANTTYPES];
} _InstrumentationInfo;

extern _InstrumentationInfo *_instrumentationInfo;
extern void *_instrumentationInfoEnd;

void _initInstrumentationInfo(unsigned int timestamp);
void _addSpectrum(unsigned int timestamp, unsigned int index, unsigned int nComponents, char *name);
void _addInvariantType(unsigned int timestamp, unsigned int index, unsigned int nInvariants, char *name, char isTimerUpdated);
void _destroyInstrumentationInfo();

/* macro for getting the training mode */
#define TRAINING  _instrumentationInfo->training

/* macro for getting the correct spectrum */
#define SPECTRUM(index)       _instrumentationInfo->spectrum[index]
#define SPECTRUMDATA(index)   (&(_instrumentationInfo->spectrum[index].data[_instrumentationInfo->run * _instrumentationInfo->spectrum[index].nComponents]))
/* macro for getting the correct invariantdata */
#define INVARIANTTYPE(index)  _instrumentationInfo->invariantType[index]

/* macro for checking if a range is outside the protected data block containing instrumentation data */
#define OUTSIDE_PROTECTION(startptr, endptr) (((char*)(endptr) < (char*)_instrumentationInfo) || ((char*)(startptr) > (char*)_instrumentationInfo->end))
#define CHECK_MEM_RANGE(startptr, size) OUTSIDE_PROTECTION((startptr), (((char*)startptr)+size-1))

#endif

