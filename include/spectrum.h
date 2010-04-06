#ifndef _SPECTRUM_H
#define _SPECTRUM_H

#include <stdio.h>

#include "config.h"

#define spectrumData(spectrum, run, comp) (spectrum->data[run*spectrum->nComponents + comp])

typedef struct _strSpectrum {
  unsigned int nComponents;     /* number of components of spectrum */
  unsigned int *data;           /* array of size nComponents containing spectrum data */
  char *name;                   /* name for spectrum */
} _Spectrum;

void _fprintSpectrumHeader(FILE *f, _Spectrum *s);
void _fprintSpectrumData(FILE *f, _Spectrum *s, int run);
void _fscanSpectrumHeader(FILE *f, _Spectrum *s);
void _fscanSpectrumData(FILE *f, _Spectrum *s, int run);

#endif

