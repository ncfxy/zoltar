
#ifndef _INVARIANT_H
#define _INVARIANT_H

#include <stdio.h>
#include "config.h"

/* data types associated with invariant */
#define DATA_TYPE_UNSET   0
#define DATA_TYPE_UINT    1
#define DATA_TYPE_INT     2
#define DATA_TYPE_DOUBLE  3
#define DATA_TYPE_PTR     4

/* screener types that can be activated per invariant */
#define SCRN_RANGE        (1<<0)
#define SCRN_BITMASK      (1<<1)
#define SCRN_BLOOM        (1<<2)

typedef struct _strInvariant {
  unsigned char datatype;
  unsigned int usage;
  unsigned char activatedScreener;
  unsigned char timerInvariantError;
  unsigned int nErrors;
  unsigned int runAtPrevError;
  union {
    struct { /* unsigned integer range */
      unsigned int val;
      unsigned int min;
      unsigned int max;
    } u;
    struct { /* signed integer range */
      signed int val;
      signed int min;
      signed int max;
    } i;
    struct { /* double precision range */
      double val;
      double min;
      double max;
    } d;
    struct { /* pointer range */
      void* val;
      void* min;
      void* max;
    } p;
  } range;
  struct {
    unsigned int first;
    unsigned int mask;
  } bitmask;
} _Invariant;

typedef struct _strInvariantType {
  unsigned int nInvariants;     /* number of invariants of invariant type */
  _Invariant *data;             /* array of size nInvariants containing invariant data */
  char *name;                   /* name of invariant type */
  char isTimerUpdated;          /* indicates if this invariant type is updated by a timer */
} _InvariantType;

void _fprintInvariantTypeHeader(FILE *f, _InvariantType *invT);
void _fprintInvariantTypeData(FILE *f, _InvariantType *invT);
void _fprintInvariant(FILE *f, _Invariant *inv);

void _fscanInvariantTypeHeader(FILE *f, _InvariantType *invT);
void _fscanInvariantTypeData(FILE *f, _InvariantType *invT);
void _fscanInvariant(FILE *f, _Invariant *inv, int i);



#endif

