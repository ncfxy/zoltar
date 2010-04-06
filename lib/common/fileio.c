
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "fileio.h"




/* helper functions */
int readUL(FILE *f, unsigned long int *result) {
  char str[128];

  if(!fgets(str, 128, f)) {
    return 0;
  }

  *result = strtoul(str, NULL, 10);
  return 1;
}
int readInt(FILE *f, int *result) {
  char str[128];

  if(!fgets(str, 128, f)) {
    return 0;
  }

  *result = atoi(str);
  return 1;
}


int readDataFile(char *fName, ProgramData *programData, char saveSpectra) {
  FILE *datf;
  unsigned int i;

  datf = fopen(fName, "r");
  if(datf) {
    if(!readUL(datf, &programData->version) || /* read version */
       !readInt(datf, &programData->opMode) || /* read operation mode */
       !readInt(datf, &programData->nRuns)) {  /* read number of runs */
      fclose(datf);
      return -2; /* ERROR parsing header */
    }
  
    programData->passFail = calloc(programData->nRuns+1, sizeof(char));
    for(i=0; i<(unsigned int)programData->nRuns; i++) {
      int c = fgetc(datf);
      if(c=='1') programData->passFail[i] = 1;
      else if(c=='0') programData->passFail[i] = 0;
      else return -3;
    }
    if(fgetc(datf)!='\n') {
      return -3;
    }

    programData->nSpectra = 0;
    for(i=0; i<MAX_SPECTRA; i++) {
      int r;
      int comps;

      _fscanSpectrumHeader(datf, &programData->spectrum[i]);           /* read spectrum header */
      comps = programData->spectrum[i].nComponents;

      if(saveSpectra) {
        programData->spectrum[i].data = (unsigned int*)calloc(comps * programData->nRuns, sizeof(unsigned int));
        if(comps > 0) {
          programData->nSpectra++;
          for(r=0; r<programData->nRuns; r++) {
            _fscanSpectrumData(datf, &programData->spectrum[i], r);  /* read spectrum data for run r */
          }
        }
      } else {
        _Spectrum s;
        s = programData->spectrum[i];
        s.data = (unsigned int*)calloc(comps * programData->nRuns, sizeof(unsigned int));
        if(comps > 0) {
          programData->nSpectra++;
          for(r=0; r<programData->nRuns; r++) {
            _fscanSpectrumData(datf, &s, r);
          }
        }
        free(s.data);
      }
    }

    programData->nInvariantTypes = 0;
    for(i=0; i<MAX_INVARIANTTYPES; i++) {
      int invariants;
      _fscanInvariantTypeHeader(datf, &programData->invariantType[i]);            /* read invariant type header */
      invariants = programData->invariantType[i].nInvariants;

      if(invariants > 0) {
        programData->nInvariantTypes++;
      }
      programData->invariantType[i].data = (_Invariant*)calloc(invariants, sizeof(_Invariant));
      _fscanInvariantTypeData(datf, &programData->invariantType[i]);  /* read invariant data */
    }

    fclose(datf);
    return 0; /* success */
  } else {
    return -1; /* ERROR opening datafile */
  }
}

int writeDataFile(char *fName, ProgramData *programData) {
  FILE *datf;
  unsigned int i;

  datf = fopen(fName, "w");
  if(datf) {
    fprintf(datf, "%lu\n", programData->version); /* version */
    fprintf(datf, "%d\n", programData->opMode);  /* training mode */
    fprintf(datf, "%d\n", programData->nRuns);   /* number of runs */
    
    for(i=0; i<(unsigned int)programData->nRuns; i++) {
      fprintf(datf, "%c", programData->passFail[i]==0?'0':'1');
    }
    fprintf(datf, "\n");

    for(i=0; i<MAX_SPECTRA; i++) {
      int r;

      _fprintSpectrumHeader(datf, &programData->spectrum[i]);
      if(programData->spectrum[i].nComponents > 0) {
        for(r=0; r<programData->nRuns; r++) {
          _fprintSpectrumData(datf, &programData->spectrum[i], r);
        }
      }
    }

    for(i=0; i<MAX_INVARIANTTYPES; i++) {
      _fprintInvariantTypeHeader(datf, &programData->invariantType[i]);
      _fprintInvariantTypeData(datf, &programData->invariantType[i]);
    }

    return 0; /* success */
    fclose(datf);
  } else {
    return -1; /* ERROR opening datafile */
  }
}


int readPassFailFile(char *fName, ProgramData *programData) {
  int c;
  int nRuns = 0;
  FILE *datf;

  datf = fopen(fName, "r");
  if(datf) {
    while((c = fgetc(datf)) != EOF) {
      if(nRuns==programData->nRuns) 
        break;
      if(c=='1') {
        programData->passFail[nRuns] = 1; /* passed run */
        nRuns++;
      } else if(c=='0') {
        programData->passFail[nRuns] = 0; /* failed run */
        nRuns++;
      } else if(isspace(c)) {
        /* do nothing */
      } else {
        fclose(datf);
        return -3; /* ERROR: unexpected character */
      }
    }
    fclose(datf);
    if(nRuns == programData->nRuns) {
      return 0;
    } else {
      return -2; /* ERROR: number of runs mismatch */
    }
  }
  
  return -1; /* ERROR: opening pass/fail file */
}


char *tmpfname(char *fName) {
  int i;
  char *res = calloc(strlen(fName)+5, sizeof(char));
  for(i=0; i<1000; i++) {
    sprintf(res, "%s.%03d", fName, i);
    if(NULL==fopen(res, "r")) {
      return res;
    }
  }
  free(res);
  return NULL;
}


/* TODO: needs to go here 
int readContextFile(char *fName, Context *context) {
{
  FILE *cf;
  int i;
  char **dirs, **filenames;
  int nDirs, nFiles;
  context = (Context***) malloc(2*sizeof(Context**));
  context[0] = (Context**) calloc(programData->nSpectra, sizeof(Context*));
  context[1] = (Context**) calloc(programData->nInvariantTypes, sizeof(Context*));
  for(i=0; i<programData->nSpectra; i++) {
    context[0][i] = (Context*)calloc(getSpectrum(programData, i)->nComponents, sizeof(Context));
  }
  for(i=0; i<programData->nInvariantTypes; i++) {
    context[1][i] = (Context*)calloc(getInvariantType(programData, i)->nInvariants, sizeof(Context));
  }
  cf = fopen(contextfileName, "r");
  if(cf) {
    int ty, si, ci, l, di, fi;
    char name[1024];
    fscanf(cf, "%d\n", &nDirs);
    dirs = (char**)malloc(nDirs*sizeof(char*));
    for(i=0; i<nDirs; i++) {
      int dirIndex;
      fscanf(cf, "%d", &dirIndex);
      dirs[dirIndex] = (char*)malloc(1024);
      fscanf(cf, "%s\n", dirs[dirIndex]);
    }
    fscanf(cf, "%d\n", &nFiles);
    filenames = (char**)malloc(nFiles*sizeof(char*));
    for(i=0; i<nFiles; i++) {
      int fileIndex;
      fscanf(cf, "%d", &fileIndex);
      filenames[fileIndex] = (char*)malloc(1024);
      fscanf(cf, "%s\n", filenames[fileIndex]);
    }
    while(EOF != fscanf(cf, "%d %d %d %d %d %d %s\n", &ty, &si, &ci, &di, &fi, &l, name)) {
      context[ty][si][ci].dirIndex = di;
      context[ty][si][ci].dir = dirs[di];
      context[ty][si][ci].fileIndex = fi;
      context[ty][si][ci].filename = filenames[fi];
      context[ty][si][ci].line = l;
      context[ty][si][ci].name = malloc(strlen(name)+1);
      strncpy(context[ty][si][ci].name, name, strlen(name)+1);
    }
    fclose(cf);
  } else {
    fprintf(stderr, "Unable to load context file\n");
    abort();
  }
}
*/
