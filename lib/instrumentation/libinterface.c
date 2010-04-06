
#include <stdlib.h>
#include <stdio.h>

#include "invariant.h"
#include "spectrum.h"
#include "instrumentationinfo.h"
#include "config.h"
#ifdef THREAD_SAFE
  #include <pthread.h>
  #include <unistd.h>
#endif

#define DBL_E (0.000001)

void _aborthandler();

void _handleInvariantBitmaskError(_Invariant *inv) {
  fprintf(stderr, "handle invariant bitmask error:\n");
  fprintf(stderr, "  val       = %08x\n", inv->range.u.val);
  fprintf(stderr, "  first     = %08x\n", inv->bitmask.first);
  fprintf(stderr, "  mask      = %08x\n", inv->bitmask.mask);
  fprintf(stderr, "  first^val = %08x\n\n", (inv->bitmask.first ^ (unsigned int) inv->range.u.val));

  if(inv->runAtPrevError < _instrumentationInfo->run) {
    inv->nErrors = 0;
    inv->runAtPrevError = _instrumentationInfo->run;
  }
  inv->nErrors++;
  fprintf(stderr, "  errors    = %d\n\n", inv->nErrors);
  if(inv->nErrors > MAX_INVARIANT_ERRORS) {
    /* too many invariant errors, end program */
    _aborthandler();
  }
  /*_instrumentationInfo->nBitmaskErrors++;
  if(_instrumentationInfo->nBitmaskErrors > MAX_BITMASK_ERRORS) {
    fprintf(stderr, "Maximum number of bitmask errors reached: run failed\n");
    _instrumentationInfo->nBitmaskErrors = 0;
    _instrumentationInfo->nRangeErrors = 0;
    _aborthandler();
  }*/
}

void _handleInvariantRangeError(_Invariant *inv) {
  fprintf(stderr, "invariant range error:\n");
  switch(inv->datatype) {
    case DATA_TYPE_INT:
      fprintf(stderr, "  data type = INT\n");
      fprintf(stderr, "  val       = %d\n", inv->range.i.val);
      fprintf(stderr, "  range     = %d - %d\n\n", inv->range.i.min, inv->range.i.max);
      break;
    case DATA_TYPE_UINT:
      fprintf(stderr, "  data type = UINT\n");
      fprintf(stderr, "  val       = %u\n", inv->range.u.val);
      fprintf(stderr, "  range     = %u - %u\n\n", inv->range.u.min, inv->range.u.max);
      break;
    case DATA_TYPE_DOUBLE:
      fprintf(stderr, "  data type = DOUBLE\n");
      fprintf(stderr, "  val       = %f\n", inv->range.d.val);
      fprintf(stderr, "  range     = %f - %f\n\n", inv->range.d.min, inv->range.d.max);
      break;
    case DATA_TYPE_PTR:
      fprintf(stderr, "  data type = PTR\n");
      fprintf(stderr, "  val       = %p\n", inv->range.p.val);
      fprintf(stderr, "  range     = %p - %p\n\n", inv->range.p.min, inv->range.p.max);
      break;
    default:
      fprintf(stderr, "  invalid data type\n");
      break;
  }

  if(inv->runAtPrevError < _instrumentationInfo->run) {
    inv->nErrors = 0;
    inv->runAtPrevError = _instrumentationInfo->run;
  }
  inv->nErrors++;
  fprintf(stderr, "  errors    = %d\n\n", inv->nErrors);
  //fprintf(stderr, "  timer     = %d\n\n", inv->timerInvariantError);
  if(inv->nErrors > MAX_INVARIANT_ERRORS || inv->timerInvariantError) {
    /* too many invariant errors, end program */
    _aborthandler();
  }
  /*_instrumentationInfo->nRangeErrors++;
  if(_instrumentationInfo->nRangeErrors > MAX_RANGE_ERRORS) {
    fprintf(stderr, "Maximum number of range errors reached: run failed\n");
    _instrumentationInfo->nBitmaskErrors = 0;
    _instrumentationInfo->nRangeErrors = 0;

    _aborthandler();
  }*/
}

/* register functions */
void _registerInvariantType(unsigned int timestamp,
                           unsigned int invTypeIndex,
                           unsigned int nInvariants,
                           char *name,
                           unsigned int isTimerUpdated) {
  _addInvariantType(timestamp, invTypeIndex, nInvariants, name, (char)isTimerUpdated);
}

void _registerSpectrum(unsigned int timestamp,
                       unsigned int spectrumIndex,
                       unsigned int nComponents,
                       char *name) {
  _addSpectrum(timestamp, spectrumIndex, nComponents, name);
}


/* spectrum update function */
void _updateSpectrum(unsigned int spectrumIndex,
                     unsigned int componentIndex) {
  #ifdef THREAD_SAFE
    pthread_mutex_lock(&_instrumentationInfo->spectrumMutex);
  #endif

  SPECTRUMDATA(spectrumIndex)[componentIndex]++;

  #ifdef THREAD_SAFE
    pthread_mutex_unlock(&_instrumentationInfo->spectrumMutex);
  #endif
}


/* invariant change functions */
void _handleInvariantChangeDouble(unsigned int invTypeIndex, 
                                  unsigned int invIndex, 
                                  double val) {
  _Invariant *inv;

  #ifdef THREAD_SAFE
    pthread_mutex_lock(&_instrumentationInfo->invariantMutex);
  #endif

  inv = &(INVARIANTTYPE(invTypeIndex).data[invIndex]);
  if(inv->datatype == DATA_TYPE_UNSET) {
    /* first use, init invariant data */
    inv->usage = 1;
    inv->datatype = DATA_TYPE_DOUBLE;
    inv->activatedScreener = SCRN_RANGE | SCRN_BITMASK;
    inv->range.d.val = val;
    inv->range.d.min = val;
    inv->range.d.max = val;
    inv->bitmask.first = (unsigned int) val;
    inv->bitmask.mask = ~0x0;
  } else {
    inv->usage++;
    inv->range.d.val = val;
    if(TRAINING) {
      /* train ranges */
      if(val < inv->range.d.min) inv->range.d.min = val;
      if(val > inv->range.d.max) inv->range.d.max = val;
      /* train bitmask */
      inv->bitmask.mask = ~(inv->bitmask.first ^ (unsigned int) val) & inv->bitmask.mask;
    } else {
      if((inv->activatedScreener & SCRN_RANGE) && (val < inv->range.d.min - DBL_E || val > inv->range.d.max + DBL_E)) {
        fprintf(stderr, "invariant type index = %d\n", invTypeIndex);
        fprintf(stderr, "invariant index      = %d\n", invIndex);
        _handleInvariantRangeError(inv);
      }
      if((inv->activatedScreener & SCRN_BITMASK) && ((inv->bitmask.first ^ (unsigned int) val) & inv->bitmask.mask)) {
        fprintf(stderr, "invariant type index = %d\n", invTypeIndex);
        fprintf(stderr, "invariant index      = %d\n", invIndex);
        _handleInvariantBitmaskError(inv);
      }
    }
  }

  #ifdef THREAD_SAFE
    pthread_mutex_unlock(&_instrumentationInfo->invariantMutex);
  #endif
}

void _handleInvariantChangeInt(unsigned int invTypeIndex, 
                               unsigned int invIndex, 
                               int val) {
  _Invariant *inv;

  #ifdef THREAD_SAFE
    pthread_mutex_lock(&_instrumentationInfo->invariantMutex);
  #endif

  inv = &(INVARIANTTYPE(invTypeIndex).data[invIndex]);
  if(inv->datatype == DATA_TYPE_UNSET) {
    /* first use, init invariant data */
    inv->usage = 1;
    inv->datatype = DATA_TYPE_INT;
    inv->activatedScreener = SCRN_RANGE | SCRN_BITMASK;
    inv->range.i.val = val;
    inv->range.i.min = val;
    inv->range.i.max = val;
    inv->bitmask.first = (unsigned int) val;
    inv->bitmask.mask = ~0x0;
  } else {
    inv->usage++;
    inv->range.i.val = val;
    if(TRAINING) {
      /* train ranges */
      if(val < inv->range.i.min) inv->range.i.min = val;
      if(val > inv->range.i.max) inv->range.i.max = val;
      /* train bitmask */
      inv->bitmask.mask = ~(inv->bitmask.first ^ (unsigned int) val) & inv->bitmask.mask;
    } else {
      inv->timerInvariantError = 0;
      if((inv->activatedScreener & SCRN_RANGE) && (val < inv->range.i.min || val > inv->range.i.max)) {
        fprintf(stderr, "invariant type index = %d\n", invTypeIndex);
        fprintf(stderr, "invariant index      = %d\n", invIndex);
        _handleInvariantRangeError(inv);
      }
      if((inv->activatedScreener & SCRN_BITMASK) && ((inv->bitmask.first ^ (unsigned int) val) & inv->bitmask.mask)) {
        fprintf(stderr, "invariant type index = %d\n", invTypeIndex);
        fprintf(stderr, "invariant index      = %d\n", invIndex);
        _handleInvariantBitmaskError(inv);
      }
    }
  }

  #ifdef THREAD_SAFE
    pthread_mutex_unlock(&_instrumentationInfo->invariantMutex);
  #endif
}

void _handleInvariantChangeUInt(unsigned int invTypeIndex, 
                                unsigned int invIndex, 
                                unsigned int val) {
  _Invariant *inv;

  #ifdef THREAD_SAFE
    pthread_mutex_lock(&_instrumentationInfo->invariantMutex);
  #endif

  inv = &(INVARIANTTYPE(invTypeIndex).data[invIndex]);
  if(inv->datatype == DATA_TYPE_UNSET) {
    /* first use, init invariant data */
    inv->usage = 1;
    inv->datatype = DATA_TYPE_UINT;
    inv->activatedScreener = SCRN_RANGE | SCRN_BITMASK;
    inv->range.u.val = val;
    inv->range.u.min = val;
    inv->range.u.max = val;
    inv->bitmask.first = val;
    inv->bitmask.mask = ~0x0;
  } else {
    inv->usage++;
    inv->range.u.val = val;
    if(TRAINING) {
      /* train ranges */
      if(val < inv->range.u.min) inv->range.u.min = val;
      if(val > inv->range.u.max) inv->range.u.max = val;
      /* train bitmask */
      inv->bitmask.mask = ~(inv->bitmask.first ^ val) & inv->bitmask.mask;
    } else {
      inv->timerInvariantError = 0;
      if((inv->activatedScreener & SCRN_RANGE) && (val < inv->range.u.min || val > inv->range.u.max)) {
        fprintf(stderr, "invariant type index = %d\n", invTypeIndex);
        fprintf(stderr, "invariant index      = %d\n", invIndex);
        _handleInvariantRangeError(inv);
      }
      if((inv->activatedScreener & SCRN_BITMASK) && ((inv->bitmask.first ^ val) & inv->bitmask.mask)) {
        fprintf(stderr, "invariant type index = %d\n", invTypeIndex);
        fprintf(stderr, "invariant index      = %d\n", invIndex);
        _handleInvariantBitmaskError(inv);
      }
    }
  }

  #ifdef THREAD_SAFE
    pthread_mutex_unlock(&_instrumentationInfo->invariantMutex);
  #endif
}

void _handleInvariantChangePtr(unsigned int invTypeIndex, 
                               unsigned int invIndex, 
                               void *val) {
  _Invariant *inv;

  #ifdef THREAD_SAFE
    pthread_mutex_lock(&_instrumentationInfo->invariantMutex);
  #endif

  inv = &(INVARIANTTYPE(invTypeIndex).data[invIndex]);
  if(inv->datatype == DATA_TYPE_UNSET) {
    /* first use, init invariant data */
    inv->usage = 1;
    inv->datatype = DATA_TYPE_PTR;
    inv->activatedScreener = SCRN_RANGE | SCRN_BITMASK;
    inv->range.p.val = val;
    inv->range.p.min = val;
    inv->range.p.max = val;
    inv->bitmask.first = (unsigned int) val;
    inv->bitmask.mask = ~0x0;
  } else {
    inv->usage++;
    inv->range.p.val = val;
    if(TRAINING) {
      /* train ranges */
      if(val < inv->range.p.min) inv->range.p.min = val;
      if(val > inv->range.p.max) inv->range.p.max = val;
      /* train bitmask */
      inv->bitmask.mask = ~(inv->bitmask.first ^ (unsigned int) val) & inv->bitmask.mask;
    } else {
      if((inv->activatedScreener & SCRN_RANGE) && (val < inv->range.p.min || val > inv->range.p.max)) {
        fprintf(stderr, "invariant type index = %d\n", invTypeIndex);
        fprintf(stderr, "invariant index      = %d\n", invIndex);
        _handleInvariantRangeError(inv);
      }
      if((inv->activatedScreener & SCRN_BITMASK) && ((inv->bitmask.first ^ (unsigned int) val) & inv->bitmask.mask)) {
        fprintf(stderr, "invariant type index = %d\n", invTypeIndex);
        fprintf(stderr, "invariant index      = %d\n", invIndex);
        _handleInvariantBitmaskError(inv);
      }
    }
  }

  #ifdef THREAD_SAFE
    pthread_mutex_unlock(&_instrumentationInfo->invariantMutex);
  #endif
}

void _handleInvariantIncrement(unsigned int invTypeIndex, 
                               unsigned int invIndex) {
  _Invariant *inv;
  unsigned int val;

  #ifdef THREAD_SAFE
    pthread_mutex_lock(&_instrumentationInfo->invariantMutex);
  #endif

  inv = &(INVARIANTTYPE(invTypeIndex).data[invIndex]);

  if(inv->datatype != DATA_TYPE_UINT) {
    fprintf(stderr, "Instrumentation failed: counter was not initialized properly\n");
    abort();
  }

  inv->usage++;
  inv->range.u.val++;
  val = inv->range.u.val;
  if(TRAINING) {
    /* train ranges */
    if(val > inv->range.u.max) inv->range.u.max = val;
    /* train bitmask */
    inv->bitmask.mask = ~(inv->bitmask.first ^ val) & inv->bitmask.mask;
  } else {
    if((inv->activatedScreener & SCRN_RANGE) && (val < inv->range.u.min || val > inv->range.u.max)) {
        fprintf(stderr, "invariant type index = %d\n", invTypeIndex);
        fprintf(stderr, "invariant index      = %d\n", invIndex);
        _handleInvariantRangeError(inv);
    }
    if((inv->activatedScreener & SCRN_BITMASK) && ((inv->bitmask.first ^ val) & inv->bitmask.mask)) {
        fprintf(stderr, "invariant type index = %d\n", invTypeIndex);
        fprintf(stderr, "invariant index      = %d\n", invIndex);
        _handleInvariantBitmaskError(inv);
    }
  }

  #ifdef THREAD_SAFE
    pthread_mutex_unlock(&_instrumentationInfo->invariantMutex);
  #endif
}


/* memory protection functions */
void _handleStore(void *loc, 
                  unsigned int size) {
  #ifdef DEBUG
    fprintf(stderr, "checking store: loc=%p, size=%u\n", loc, size);
    fprintf(stderr, "protected area: start=%p\n", (void*)_instrumentationInfo);
    fprintf(stderr, " end=%p\n", _instrumentationInfo->end);
  #endif
  if(!CHECK_MEM_RANGE(loc, size)) {
    fprintf(stderr, "Cought illegal store operation!\n");
    abort();
  }
}

void _handleMemFail(void *loc, unsigned int size) {
  fprintf(stderr, "Cought illegal store operation!\n");
  abort();
}


