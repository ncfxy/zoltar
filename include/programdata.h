
#ifndef __PROGRAMDATA_H
#define __PROGRAMDATA_H

#include "spectrum.h"
#include "invariant.h"
#include "instrumentationinfo.h"

typedef struct strContext {
  int dirIndex;
  int fileIndex;
  char *dir;
  char *filename;
  int line;
  char *name;
} Context;

typedef struct strContextItem {
  int dir;
  int file;
  int line;
  char *name; /* should be a table entry as well */
} ContextItem;

typedef struct strContextNew {
  char **path;
  char **filename; /* not necessary to keep this, can be obtained from path */
  ContextItem ***item;
} ContextNew;


typedef struct strProgramData {
  unsigned long int version;
  int opMode;
  int nRuns;
  int nSpectra;
  int nInvariantTypes;
  char *passFail;
  _Spectrum spectrum[MAX_SPECTRA];
  _InvariantType invariantType[MAX_INVARIANTTYPES];
} ProgramData;

ProgramData *newProgramData();
void freeProgramData(ProgramData*);
_Spectrum *getSpectrum(ProgramData *programData, int index);
_InvariantType *getInvariantType(ProgramData *programData, int index);

#endif

