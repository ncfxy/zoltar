
#ifndef __SFL_H
#define __SFL_H

#include "programdata.h"

#define S_JACCARD   0
#define S_TARANTULA 1
#define S_OCHIAI    2

typedef struct strSFLItem {
  int componentIndex;
  float coefficient;
} SFLItem;

SFLItem *performSFL(ProgramData *programData, int spectrumIndex, int coefficient);

#endif


