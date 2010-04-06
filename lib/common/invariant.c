
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "invariant.h"


void _fprintInvariantTypeHeader(FILE *f, _InvariantType *invT) {
  fprintf(f, "%s\n%u\n%d\n", invT->name, invT->nInvariants, invT->isTimerUpdated);
}

void _fprintInvariantTypeData(FILE *f, _InvariantType *invT) {
  unsigned int i;
  for(i=0; i<invT->nInvariants; i++) {
    fprintf(f, "%5d ", i);
    _fprintInvariant(f, &(invT->data[i]));
  }
}

void _fprintInvariant(FILE *f, _Invariant *inv) {
  switch(inv->datatype) {
    case DATA_TYPE_UINT:
      fprintf(f, "%7s ", "UINT");
      fprintf(f, "%10u ", inv->usage);
      fprintf(f, "%d %d %d ", (inv->activatedScreener & SCRN_RANGE)?1:0, 
                              (inv->activatedScreener & SCRN_BITMASK)?1:0, 
                              (inv->activatedScreener & SCRN_BLOOM)?1:0);
      fprintf(f, "%10u %10u ", inv->range.u.min, inv->range.u.max);
      fprintf(f, "%8X %8X ", inv->bitmask.first, inv->bitmask.mask);
      fprintf(f, "\n");
      break;
    case DATA_TYPE_INT:
      fprintf(f, "%7s ", "INT");
      fprintf(f, "%10u ", inv->usage);
      fprintf(f, "%d %d %d ", (inv->activatedScreener & SCRN_RANGE)?1:0, 
                              (inv->activatedScreener & SCRN_BITMASK)?1:0, 
                              (inv->activatedScreener & SCRN_BLOOM)?1:0);
      fprintf(f, "%10d %10d ", inv->range.i.min, inv->range.i.max);
      fprintf(f, "%8X %8X ", inv->bitmask.first, inv->bitmask.mask);
      fprintf(f, "\n");
      break;
    case DATA_TYPE_DOUBLE:
      fprintf(f, "%7s ", "DOUBLE");
      fprintf(f, "%10u ", inv->usage);
      fprintf(f, "%d %d %d ", (inv->activatedScreener & SCRN_RANGE)?1:0, 
                              (inv->activatedScreener & SCRN_BITMASK)?1:0, 
                              (inv->activatedScreener & SCRN_BLOOM)?1:0);
      fprintf(f, "%10f %10f ", inv->range.d.min, inv->range.d.max);
      fprintf(f, "%8X %8X ", inv->bitmask.first, inv->bitmask.mask);
      fprintf(f, "\n");
      break;
    case DATA_TYPE_PTR:
      fprintf(f, "%7s ", "PTR");
      fprintf(f, "%10u ", inv->usage);
      fprintf(f, "%d %d %d ", (inv->activatedScreener & SCRN_RANGE)?1:0, 
                              (inv->activatedScreener & SCRN_BITMASK)?1:0, 
                              (inv->activatedScreener & SCRN_BLOOM)?1:0);
      fprintf(f, "%10p %10p ", inv->range.p.min, inv->range.p.max);
      fprintf(f, "%8X %8X ", inv->bitmask.first, inv->bitmask.mask);
      fprintf(f, "\n");
      break;
    case DATA_TYPE_UNSET:
      fprintf(f, "%7s ", "UNSET");
      fprintf(f, "%10u ", inv->usage);
      fprintf(f, "%d %d %d ", (inv->activatedScreener & SCRN_RANGE)?1:0, 
                              (inv->activatedScreener & SCRN_BITMASK)?1:0, 
                              (inv->activatedScreener & SCRN_BLOOM)?1:0);
      fprintf(f, "%10u %10u ", inv->range.u.min, inv->range.u.max);
      fprintf(f, "%8X %8X ", inv->bitmask.first, inv->bitmask.mask);
      fprintf(f, "\n");
      break;
    default:
      fprintf(stderr, "Corruption in invariant data while printing (unsupported type)\n");
      break;
  }
  
}

void _fscanInvariantTypeHeader(FILE *f, _InvariantType *invT) {
  char str[128], *str2;

  #ifdef DEBUG 
    fprintf(stderr, "reading invariant type header\n");
  #endif

  if(!fgets(str, 128, f)) {
    fprintf(stderr, "Data corruption reading file (invariant type header).\n");
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
  invT->name = malloc(strlen(str)+1);
  strcpy(invT->name, str);
  /*tok = strtok(str, " \n\t\r");
  invT->name = malloc(strlen(tok)+1);
  strcpy(invT->name, tok);*/
  
  #ifdef DEBUG 
    fprintf(stderr, "  name = %s\n", invT->name);
  #endif

  if(!fgets(str, 128, f)) {
    fprintf(stderr, "Data corruption reading file (invariant type header).\n");
    abort();
  }
  invT->nInvariants = atoi(str);

  #ifdef DEBUG 
    fprintf(stderr, "  nInvariants = %d\n", invT->nInvariants);
  #endif

  if(!fgets(str, 128, f)) {
    fprintf(stderr, "Data corruption reading file (invariant type header).\n");
    abort();
  }
  invT->isTimerUpdated = (char)atoi(str);

  #ifdef DEBUG 
    fprintf(stderr, "  isTimerUpdated = %d\n", invT->isTimerUpdated);
  #endif
}

/*    fscanInvariantTypeData
 * Get invariant data from current point in FILE f.
 * InvariantType *invT should contain allocated data for
 * invT->nInvariants number of invariants
 */
void _fscanInvariantTypeData(FILE *f, _InvariantType *invT) {
  unsigned int i;
  #ifdef DEBUG 
    fprintf(stderr, "reading invariant type data\n");
  #endif
  for(i=0; i<invT->nInvariants; i++) {
    _fscanInvariant(f, &(invT->data[i]), i);
  }
}

void _fscanInvariant(FILE *f, _Invariant *inv, int index) {
  char *tokens[10], str[256];
  int i;

  #ifdef DEBUG 
    fprintf(stderr, "reading invariant data i=%d\n", index);
  #endif

  fgets(str, 256, f);
  
  tokens[0] = strtok(str, " \t");
  if(tokens[0]==NULL) {
    fprintf(stderr, "Data corruption reading file (invariant data) 1.\n");
    abort();
  }
  for(i=1; i<10; i++) {
    tokens[i] = strtok(NULL, " \t");
    if(tokens[i]==NULL) {
      fprintf(stderr, "Data corruption reading file (invariant data) 1.\n");
      abort();
    }
  }

  if(atoi(tokens[0])!=index) {
    fprintf(stderr, "Data corruption reading file (invariant data) 2.\n");
    abort();
  }

  #ifdef DEBUG 
    fprintf(stderr, "tokens[1] = '%s'\n", tokens[1]);
  #endif

  if(!strcmp(tokens[1], "UINT")) {
    char scrn = 0;
    inv->datatype = DATA_TYPE_UINT;
    inv->usage = (unsigned int) strtoul(tokens[2], NULL, 10);
    if(atoi(tokens[3])) scrn |= SCRN_RANGE;
    if(atoi(tokens[4])) scrn |= SCRN_BITMASK;
    if(atoi(tokens[5])) scrn |= SCRN_BLOOM;
    inv->activatedScreener = scrn;
    inv->range.u.min = (unsigned int) strtoul(tokens[6], NULL, 10);
    inv->range.u.max = (unsigned int) strtoul(tokens[7], NULL, 10);
    inv->bitmask.first = (unsigned int) strtoul(tokens[8], NULL, 16);
    inv->bitmask.mask = (unsigned int) strtoul(tokens[9], NULL, 16);
  } else if(!strcmp(tokens[1], "INT")) {
    char scrn = 0;
    inv->datatype = DATA_TYPE_INT;
    inv->usage = (unsigned int) strtoul(tokens[2], NULL, 10);
    if(atoi(tokens[3])) scrn |= SCRN_RANGE;
    if(atoi(tokens[4])) scrn |= SCRN_BITMASK;
    if(atoi(tokens[5])) scrn |= SCRN_BLOOM;
    inv->activatedScreener = scrn;
    inv->range.i.min = (signed int) strtol(tokens[6], NULL, 10);
    inv->range.i.max = (signed int) strtol(tokens[7], NULL, 10);
    inv->bitmask.first = (unsigned int) strtoul(tokens[8], NULL, 16);
    inv->bitmask.mask = (unsigned int) strtoul(tokens[9], NULL, 16);
  } else if(!strcmp(tokens[1], "DOUBLE")) {
    char scrn = 0;
    inv->datatype = DATA_TYPE_DOUBLE;
    inv->usage = (unsigned int) strtoul(tokens[2], NULL, 10);
    if(atoi(tokens[3])) scrn |= SCRN_RANGE;
    if(atoi(tokens[4])) scrn |= SCRN_BITMASK;
    if(atoi(tokens[5])) scrn |= SCRN_BLOOM;
    inv->activatedScreener = scrn;
    inv->range.d.min = strtod(tokens[6], NULL);
    inv->range.d.max = strtod(tokens[7], NULL);
    inv->bitmask.first = (unsigned int) strtoul(tokens[8], NULL, 16);
    inv->bitmask.mask = (unsigned int) strtoul(tokens[9], NULL, 16);
  } else if(!strcmp(tokens[1], "PTR")) {
    char scrn = 0;
    inv->datatype = DATA_TYPE_PTR;
    inv->usage = (unsigned int) strtoul(tokens[2], NULL, 10);
    if(atoi(tokens[3])) scrn |= SCRN_RANGE;
    if(atoi(tokens[4])) scrn |= SCRN_BITMASK;
    if(atoi(tokens[5])) scrn |= SCRN_BLOOM;
    inv->activatedScreener = scrn;
    sscanf(tokens[6], "%p", &(inv->range.p.min));
    sscanf(tokens[7], "%p", &(inv->range.p.max));
    inv->bitmask.first = (unsigned int) strtoul(tokens[8], NULL, 16);
    inv->bitmask.mask = (unsigned int) strtoul(tokens[9], NULL, 16);
  } else if(!strcmp(tokens[1], "UNSET")) {
    char scrn = 0;
    inv->datatype = DATA_TYPE_UNSET;
    inv->usage = (unsigned int) strtoul(tokens[2], NULL, 10);
    if(atoi(tokens[3])) scrn |= SCRN_RANGE;
    if(atoi(tokens[4])) scrn |= SCRN_BITMASK;
    if(atoi(tokens[5])) scrn |= SCRN_BLOOM;
    inv->activatedScreener = scrn;
    inv->range.u.min = (unsigned int) strtoul(tokens[6], NULL, 10);
    inv->range.u.max = (unsigned int) strtoul(tokens[7], NULL, 10);
    inv->bitmask.first = (unsigned int) strtoul(tokens[8], NULL, 16);
    inv->bitmask.mask = (unsigned int) strtoul(tokens[9], NULL, 16);
  } else {
    fprintf(stderr, "Data corruption reading file (invariant data) 3.\n");
    abort();
  }
}


