
#include <stdlib.h>
#include "programdata.h"

ProgramData *newProgramData() {
  ProgramData *result = (ProgramData*)calloc(1, sizeof(ProgramData));
  return result;
}

void freeProgramData(ProgramData *programData) {
  unsigned int i;

  for(i=0; i<MAX_SPECTRA; i++) {
    if(programData->spectrum[i].data) {
      free(programData->spectrum[i].data);
    }
  }

  for(i=0; i<MAX_INVARIANTTYPES; i++) {
    if(programData->invariantType[i].data) {
      free(programData->invariantType[i].data);
    }
  }

  free(programData->passFail);

  free(programData);
}


_Spectrum *getSpectrum(ProgramData *programData, int index) {
  int i,cnt;
  if(index<0 || index>=programData->nSpectra) {
    return NULL;
  }
  cnt=0;
  for(i=0; i<MAX_SPECTRA; i++) {
    if(programData->spectrum[i].nComponents > 0) {
      if(cnt==index) {
        return &programData->spectrum[i];
      } else {
        cnt++;
      }
    }
  }
  return NULL;
}

_InvariantType *getInvariantType(ProgramData *programData, int index) {
  int i,cnt;
  if(index<0 || index>=programData->nInvariantTypes) {
    return NULL;
  }
  cnt=0;
  for(i=0; i<MAX_INVARIANTTYPES; i++) {
    if(programData->invariantType[i].nInvariants > 0) {
      if(cnt==index) {
        return &programData->invariantType[i];
      } else {
        cnt++;
      }
    }
  }
  return NULL;
}

