
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ncurses.h>
#include <menu.h>

#include "invariant.h"
#include "spectrum.h"
#include "instrumentationinfo.h"
#include "fileio.h"
#include "sfl.h"
#include "gui.h"

#include "version.h"



void printHelp() {
  printf(
    "Usage:\n"
    "  analyze [-fFILE] [-pFILE]\n"
    "\n"
    "Perform analysis on program run information.\n"
  );
  printf(
    "Options:\n"
    "  -h, --help               show this help\n"
    "  -v, --version            show version\n"
    "  -f, --datafile=FILE      use FILE as datafile\n"
    "                           defaults to \"datafile.dat\"\n"
    "  -p, --passfailfile=FILE  use FILE as passed/failed run information\n"
    "  -c, --contextfile=FILE   use FILE as context information\n"
    "                           defaults to \"context.dat\"\n"
  );
  printf(
    "      --info               show instrumentation info\n"
    "      --sfl=COEFF          perform SFL and output an ordered list of components\n"
    "                           where COEFF is one of\n"
    "                             ochiai\n"
    "                             jaccard\n"
    "                             tarantula\n"
    "  -nNUM                    show a maximum of NUM items in SFL output\n"
    "      --settrain           set operation mode to 'training'\n"
    "      --settest            set operation mode to 'testing'\n"
  );
}

void printVersion() {
  printf(
    "analyze v%d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION);
}


void showSFL(ProgramData *programData, Context** context, int coeff, int maxItems) {
  int spectrumIndex;
  unsigned int sflIndex;
    
  printf("Performing Spectrum-based Fault Localization\n\n");
  printf("  coefficient = %s\n\n", 
    coeff==S_OCHIAI?"OCHIAI":
    (coeff==S_TARANTULA?"TARANTULA":
    (coeff==S_JACCARD?"JACCARD":"---")));
  for(spectrumIndex=0; spectrumIndex<programData->nSpectra; spectrumIndex++) {
    unsigned int max;
    SFLItem *sfl = performSFL(programData, spectrumIndex, coeff);
    printf("   ------------------\n");
    printf("  |     spectrum = %s\n", getSpectrum(programData, spectrumIndex)->name);
    printf("  | # components = %d\n", getSpectrum(programData, spectrumIndex)->nComponents);
    printf("  |------------------\n");
    max = getSpectrum(programData, spectrumIndex)->nComponents;
    if(maxItems > 0 && (unsigned int)maxItems < max) {
      max = (unsigned int)maxItems;
    }
    for(sflIndex=0; sflIndex<max; sflIndex++) {
      printf("  |  %6f   %s:%d %s\n", 
        sfl[sflIndex].coefficient,
        context[spectrumIndex][sfl[sflIndex].componentIndex].filename,
        context[spectrumIndex][sfl[sflIndex].componentIndex].line,
        context[spectrumIndex][sfl[sflIndex].componentIndex].name);
    }
    printf("\n");
    free(sfl);
  }
}

char *getInfo(ProgramData *programData) {
  char *result = (char*)malloc(2048);
  char *ptr = result;
  int i;
  ptr += sprintf(ptr, "Instrumentation Info and Run Data\n\n");
  ptr += sprintf(ptr, "  instrumentation version     = %lu\n", programData->version);
  ptr += sprintf(ptr, "  number of runs              = %d\n", programData->nRuns);
  ptr += sprintf(ptr, "  operating mode              = %s\n", (programData->opMode==0)?"training":"testing");
  ptr += sprintf(ptr, "\n");
  ptr += sprintf(ptr, "  program spectra:\n");
  ptr += sprintf(ptr, "    number of spectra         = %d\n", programData->nSpectra);
  ptr += sprintf(ptr, "      %-20s | %s\n", "name", "nComponents");
  ptr += sprintf(ptr, "      ---------------------|------------\n");
  for(i=0; i<programData->nSpectra; i++) {
    ptr += sprintf(ptr, "      %-20s | %5d\n", 
      getSpectrum(programData, i)->name, getSpectrum(programData, i)->nComponents);
  }
  ptr += sprintf(ptr, "\n");
  ptr += sprintf(ptr, "  invariant types:\n");
  ptr += sprintf(ptr, "    number of invariant types = %d\n", programData->nInvariantTypes);
  ptr += sprintf(ptr, "      %-20s | %s\n", "name", "nInvariants");
  ptr += sprintf(ptr, "      ---------------------|------------\n");
  for(i=0; i<programData->nInvariantTypes; i++) {
    ptr += sprintf(ptr, "      %-20s | %5d\n", 
      getInvariantType(programData, i)->name, getInvariantType(programData, i)->nInvariants);
  }
  ptr += sprintf(ptr, "\n");

  return result;
}

void showInfo(ProgramData *programData) {
  int i;
  printf("Instrumentation Info and Run Data\n\n");
  printf("  instrumentation version     = %lu\n", programData->version);
  printf("  number of runs              = %d\n", programData->nRuns);
  printf("  operating mode              = %s\n", (programData->opMode==0)?"training":"testing");
  printf("\n");
  printf("  program spectra:\n");
  printf("    number of spectra         = %d\n", programData->nSpectra);
  printf("      %-20s | %s\n", "name", "nComponents");
  printf("      ---------------------|------------\n");
  for(i=0; i<programData->nSpectra; i++) {
    printf("      %-20s | %d\n", 
      getSpectrum(programData, i)->name, getSpectrum(programData, i)->nComponents);
  }
  printf("\n");
  printf("  invariant types:\n");
  printf("    number of invariant types = %d\n", programData->nInvariantTypes);
  printf("      %-20s | %s\n", "name", "nInvariants");
  printf("      ---------------------|------------\n");
  for(i=0; i<programData->nInvariantTypes; i++) {
    printf("      %-20s | %d\n", 
      getInvariantType(programData, i)->name, getInvariantType(programData, i)->nInvariants);
  }
  printf("\n");
}




#define MODE_INTERACTIVE  0
#define MODE_SFL_ONLY     1
#define MODE_INFO_ONLY    2
#define MODE_SET_TRAIN    3
#define MODE_SET_TEST     4
int main(int argc, char *argv[]) {
  char c;
  int optionIndex;
  static struct option longOptions[] = {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'v'},
    {"datafile", 1, 0, 'f'},
    {"passfailfile", 1, 0, 'p'},
    {"contextfile", 1, 0, 'c'},
    {"sfl", 1, 0, 1},
    {"info", 0, 0, 2},
    {"settrain", 0, 0, 3},
    {"settest", 0, 0, 4},
    {0, 0, 0, 0}
  };
  char *datafileName;
  char *passfailfileName;
  char *contextfileName;
  ProgramData *programData;
  Context ***context;
  int externalPassFail = 0;
  char *sflCoeff;
  int maxItems = -1;
  int mode;

  mode = MODE_INTERACTIVE;

  datafileName = (char*)"datafile.dat";
  passfailfileName = (char*)"-";
  contextfileName = (char*)"context.dat";
  sflCoeff = (char*)"-";
  programData = newProgramData();

  /* parse arguments */
  while ((c = getopt_long(argc, argv, "hvf:p:c:n:", longOptions, &optionIndex)) != EOF) {
    switch (c) {
      case 'h':
        printHelp();
        return 0;
        break;
      case 'v':
        printVersion();
        return 0;
        break;
      case 'f':
        datafileName = calloc(strlen(optarg)+1, sizeof(char));
        strncpy(datafileName, optarg, strlen(optarg));
        break;
      case 'p':
        passfailfileName = calloc(strlen(optarg)+1, sizeof(char));
        strncpy(passfailfileName, optarg, strlen(optarg));
        externalPassFail = 1;
        break;
      case 'c':
        contextfileName = calloc(strlen(optarg)+1, sizeof(char));
        strncpy(contextfileName, optarg, strlen(optarg));
        break;
      case 'n':
        maxItems = atoi(optarg);
        break;
      case 1:
        mode = MODE_SFL_ONLY;
        sflCoeff = calloc(strlen(optarg)+1, sizeof(char));
        strncpy(sflCoeff, optarg, strlen(optarg));
        break;
      case 2:
        mode = MODE_INFO_ONLY;
        break;
      case 3:
        mode = MODE_SET_TRAIN;
        break;
      case 4:
        mode = MODE_SET_TEST;
        break;
      default:
        printHelp();
        break;
    }
  }


  /* read data file */
  { 
    int res;
    res = readDataFile(datafileName, programData, 1);
    if (res) {
      fprintf(stderr, "error reading datafile: res=%d\n", res);
      abort();
    }
  }


  /* read pass/fail file, if provided */
  if(externalPassFail) {
    int res;
    res = readPassFailFile(passfailfileName, programData);
    if (res) {
      fprintf(stderr, "error reading pass/fail file: res=%d\n", res);
      abort();
    }
  }


  /* read context file */
  {
    FILE *cf;
    int i;
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
      int ty, si, ci, l;
      char dir[1024], filename[1024], name[1024];
      while(EOF != fscanf(cf, "%d %d %d %s %s %d %s\n", &ty, &si, &ci, dir, filename, &l, name)) {
        context[ty][si][ci].dir = malloc(strlen(dir));
        strcpy(context[ty][si][ci].dir, dir);
        context[ty][si][ci].filename = malloc(strlen(filename));
        strcpy(context[ty][si][ci].filename, filename);
        context[ty][si][ci].line = l;
        context[ty][si][ci].name = malloc(strlen(name));
        strcpy(context[ty][si][ci].name, name);
      }
      fclose(cf);
    } else {
      printf("Unable to load context file\n");
    }
  }


  /* do stuff, depending on mode */
  switch(mode) {
    case MODE_SFL_ONLY:
      {
        int coeff;
        if(!strcmp("ochiai", sflCoeff)) {
          coeff = S_OCHIAI;
        } else if(!strcmp("tarantula", sflCoeff)) {
          coeff = S_TARANTULA;
        } else if(!strcmp("jaccard", sflCoeff)) {
          coeff = S_JACCARD;
        } else {
          printf("unrecognized SFL coefficient\n");
          abort();
        }
        showSFL(programData, context[0], coeff, maxItems);
      }
      break;
    case MODE_INFO_ONLY:
      showInfo(programData);
      break;
    case MODE_SET_TRAIN:
      programData->opMode = 0;
      {
        int res;
        char *tmp = tmpfname(datafileName);
        res = writeDataFile(tmp, programData);
        if(res) {
          fprintf(stderr, "Error writing datafile: res=%d\n", res);
        } else {
          if(remove(datafileName)) {
            fprintf(stderr, "Error removing outdated datafile '%s'\n", datafileName);
            fprintf(stderr, "New datafile is stored in '%s'\n", tmp);
          } else if(rename(tmp, datafileName)) {
            fprintf(stderr, "Error renaming temporary datafile '%s' to '%s'\n", tmp, datafileName);
            fprintf(stderr, "New datafile is stored in '%s'\n", tmp);
          } else {
            printf("Successfully changed operation mode to TRAINING\n");
          }
        }
        free(tmp);
      }
      break;
    case MODE_SET_TEST:
      programData->opMode = 1;
      {
        int res;
        char *tmp = tmpfname(datafileName);
        res = writeDataFile(tmp, programData);
        if(res) {
          fprintf(stderr, "Error writing datafile: res=%d\n", res);
        } else {
          if(remove(datafileName)) {
            fprintf(stderr, "Error removing outdated datafile '%s'\n", datafileName);
            fprintf(stderr, "New datafile is stored in '%s'\n", tmp);
          } else if(rename(tmp, datafileName)) {
            fprintf(stderr, "Error renaming temporary datafile '%s' to '%s'\n", tmp, datafileName);
            fprintf(stderr, "New datafile is stored in '%s'\n", tmp);
          } else {
            printf("Successfully changed operation mode to TESTING\n");
          }
        }
        free(tmp);
      }
      break;
    case MODE_INTERACTIVE:
    default:
      fprintf(stderr, "Entering interactive mode\n");
      initscr();
      
      initGui(programData, context);
      while(loopGui());
      destroyGui();

      endwin();

      if(dataChanged() || externalPassFail) {
        int res;
        fprintf(stderr, "data has changed, updating datafile \"%s\"\n", datafileName);
        res = writeDataFile(datafileName, programData);
        if (res) {
          fprintf(stderr, "error writing datafile: res=%d\n", res);
        }
      }
      break;
  }

  /*
   v create common fileio.h/c for reading and writing file data (all changes in 1 point)
   - run pass/fail information both in datafile
   v   and separate file (input)
   v perform SFL based on spectrum info and run info
   v add context to SFL output
   v toggle testing/training
   - console menu or graphical?
   - batch version execution option (calls instrumented prog)
   - combine datafiles for faster execution (needs run pass/fail info though)
  */

  return 0;
}

