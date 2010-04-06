
#ifndef __FILEIO_H
#define __FILEIO_H

#include <stdio.h>

#include "programdata.h"
#include "instrumentationinfo.h"

/* datafile format:
 version/instrumentationID   - long unsigned int
 operation mode              - 0 for training, 1 for testing
 number of runs              - unsigned int
 for each spectrum:
   spectrum header
   for each run:
     spectrum data
 for each invariant type:
   invariant type header
   invariant data
*/


/* 
 -- readDataFile --

 fName       - name of datafile
 programData - pointer to an allocated struct to store data
 saveSpectra - toggle the saving of spectrum data for each run

 Reads the generated datafile, which contains run information of instrumented program points,
 according to the datafile format.
 If saveSpectra is 0, no memory is allocated to store the spectrum data for each run.
*/
int readDataFile(char *fName, ProgramData *programData, char saveSpectra);

/*
 -- writeDataFile --
*/
int writeDataFile(char *fName, ProgramData *programData);

/*
 -- updateDataFile --
*/
int updateDataFile(char *fName);

/* 
 -- readPassFailFile --
*/
int readPassFailFile(char *fName, ProgramData *programData);

/* 
 -- tmpfname --
 on success returns name for temporary file, based on 'fName'
 this is done by appending '.000' to '.999' until a non-existing file is found
 on failure NULL is returned
*/
char *tmpfname(char *fName);

#endif

