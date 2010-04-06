
#include <math.h>
#include <stdlib.h>

#include "sfl.h"

int SFLItemCmp(const void *a, const void *b) {
  return ((SFLItem*)a)->coefficient < ((SFLItem*)b)->coefficient;
}

SFLItem *performSFL(ProgramData *programData, int spectrumIndex, int coefficient) {
  _Spectrum *s = getSpectrum(programData, spectrumIndex);
  int n = s->nComponents;
  SFLItem *result = malloc(n * sizeof(SFLItem));
  int a00=0; /* # passed runs, comp not involved */
  int a01=0; /* # failed runs, comp not involved */
  int a10=0; /* # passed runs, comp involved */
  int a11=0; /* # failed runs, comp involved */
  int i, r;

  for(i=0; i<n; i++) {
    a00=a01=a10=a11=0;
    for(r=0; r<programData->nRuns; r++) {
      if(programData->passFail[r]) {
        if(spectrumData(s, r, i)) {
          a10++;
        } else {
          a00++;
        }
      } else {
        if(spectrumData(s, r, i)) {
          a11++;
        } else {
          a01++;
        }
      }
    }

    result[i].componentIndex = i;
    switch(coefficient) {
      case S_JACCARD:
        result[i].coefficient = ((float) a11) / ((float)(a11+a01+a10));
        break;
      case S_TARANTULA:
        result[i].coefficient = (((float) a11) / ((float)(a11+a01))) / ((((float) a11) / ((float)(a11+a01))) + (((float) a10) / ((float)(a10+a00))));
        if(isnan(result[i].coefficient)) { /* simple protection against NaN */
          result[i].coefficient = 0;
        }
        break;
      case S_OCHIAI:
        /* catch the case where the component has not been executed at all */
        if(a11 + a10 == 0) {
          result[i].coefficient = 0;
        } else {
          result[i].coefficient = ((float) a11) / (float) sqrt((a11+a01)*(a11+a10));
        }
        break;
      default:
        break;
    }
  }

  qsort(result, n, sizeof(SFLItem), SFLItemCmp);

  return result;
}

