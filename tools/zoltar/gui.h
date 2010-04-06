
#ifndef __GUI_H
#define __GUI_H

#include "programdata.h"
#include "sfl.h"

void initGui(ProgramData *programData, Context*** context);
int loopGui();
void destroyGui();
int dataChanged();

#endif

