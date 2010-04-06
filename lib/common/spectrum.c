
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spectrum.h"

void _fprintSpectrumHeader(FILE *f, _Spectrum *s) {
  fprintf(f, "%s\n%u\n", s->name, s->nComponents);
}

void _fprintSpectrumData(FILE *f, _Spectrum *s, int run) {
  unsigned int i;
  unsigned int start = run*s->nComponents;
  unsigned int end = start+s->nComponents;
  for(i=start; i<end; i++) {
    fprintf(f, "%10u ", s->data[i]);
  }
  fprintf(f, "\n");
}

void _fscanSpectrumHeader(FILE *f, _Spectrum *s) {
  char *str, *str2;

  #ifdef DEBUG 
    fprintf(stderr, "reading spectrum header\n");
  #endif

  str = (char*) malloc(128);
  if(!fgets(str, 128, f)) {
    fprintf(stderr, "Data corruption reading file (spectrum header).\n");
    abort();
  }
  str[127] = 0;
  str2 = str;
  while(*str2 != 0) {
    if(*str2 == '\n') {
      *str2 = 0;
      break;
    }
    str2++;
  }
  s->name = malloc(strlen(str)+1);
  strcpy(s->name, str);

  #ifdef DEBUG 
    fprintf(stderr, "  name = %s\n", s->name);
  #endif

  if(!fgets(str, 128, f)) {
    fprintf(stderr, "Data corruption reading file (spectrum header).\n");
    abort();
  }
  s->nComponents = atoi(str);
  
  free(str);

  #ifdef DEBUG 
    fprintf(stderr, "  nComponents = %d\n", s->nComponents);
  #endif
}

void _fscanSpectrumData(FILE *f, _Spectrum *s, int run) {
  unsigned int i;
  unsigned int start = run*s->nComponents;
  unsigned int end = start+s->nComponents;

  #ifdef DEBUG 
    fprintf(stderr, "reading spectrum data\n");
  #endif

  for(i=start; i<end; i++) {
    if(fscanf(f, "%u ", &s->data[i]) != 1) {
      fprintf(stderr, "Data corruption reading file (spectrum data).\n");
      abort();
    }
    #ifdef DEBUG 
      fprintf(stderr, " %u: %u\n", i, s->data[i]);
    #endif
  }
}

