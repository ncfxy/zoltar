
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ncurses.h>
#include <menu.h>

#include "gui.h"

#include "version.h"

WINDOW *dataWin, 
       *dataWinBox, 
       *sflResultWin,
       *mainMenuWin, 
       *opmodeMenuWin, 
       *spectraMenuWin, 
       *spectraOpMenuWin,
       *sflCoeffMenuWin, 
       *invariantTypesMenuWin,
       *invariantsMenuWinScr,
       *invariantsMenuWinRng,
       *invariantsMenuWinBmsk,
       *textMin, *textMax, *textFirst, *textMask;
ITEM **mainMenuItems,  
     **opmodeMenuItems, 
     **spectraMenuItems,
     **spectraOpMenuItems,
     **sflCoeffMenuItems,
     **invariantTypesMenuItems,
     **invariantsMenuItemsScr = NULL,
     **invariantsMenuItemsRng = NULL,
     **invariantsMenuItemsBmsk = NULL;
MENU *mainMenu, 
     *opmodeMenu, 
     *spectraMenu,
     *sflCoeffMenu,
     *spectraOpMenu,
     *invariantTypesMenu,
     *invariantsMenuScr = NULL,
     *invariantsMenuRng = NULL,
     *invariantsMenuBmsk = NULL;


#define GUI_STATE_MAIN             0
#define GUI_STATE_OPMODE           1
#define GUI_STATE_SPECTRA          2
#define GUI_STATE_SPECTRA_OP       3
#define GUI_STATE_SPECTRUM_DATA    4
#define GUI_STATE_SFL_COEFF        5
#define GUI_STATE_SFL_RESULT       6
#define GUI_STATE_INVARIANTTYPES   7
#define GUI_STATE_INVARIANTS       8
#define GUI_STATE_INVARIANT_EDIT   9
#define GUI_STATE_TMP             99

#define GUI_EDIT_STATE_NONE        0
#define GUI_EDIT_STATE_MIN         1
#define GUI_EDIT_STATE_MAX         2
#define GUI_EDIT_STATE_FIRST       3
#define GUI_EDIT_STATE_MASK        4


#define MENU_ITEM_EXIT           0
#define MENU_ITEM_OPMODE         1
#define MENU_ITEM_SPECTRA        2
#define MENU_ITEM_INVARIANTS     3
#define MENU_ITEM_TRAINING       4
#define MENU_ITEM_TESTING        5
#define MENU_ITEM_DATA           6
#define MENU_ITEM_SFL            7


#define SFL_VISIBLE_RESULTS     10

#define INV_MODE_RANGE    0
#define INV_MODE_BITMASK  1
#define INV_MODE_SCREENER 2


int guiDataChanged;

int guiSelectedSpectrum;
int guiSelectedSFLCoeff;
int guiSelectedSFLResultOffset;
int guiSelectedInvariantType;
int guiSelectedInvariant;
int guiSelectedInvariantMode;
_Invariant guiInv;
int guiState;
int guiEditState;
int guiSelectedEditState;
ProgramData *guiProgramData;
Context*** guiContext;
SFLItem *guiSFL;


void createWindows();
void createMenus();

void initGui(ProgramData *programData, Context*** context) {
  guiDataChanged = 0;

  guiProgramData = programData;
  guiContext = context;

  cbreak();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);

  createWindows();
  createMenus();

  refresh();
  touchwin(mainMenuWin);
  touchwin(dataWinBox);
  touchwin(dataWin);
  touchwin(sflResultWin);
  touchwin(opmodeMenuWin);
  touchwin(spectraMenuWin);
  touchwin(spectraOpMenuWin);
  touchwin(sflCoeffMenuWin);
  touchwin(invariantTypesMenuWin);
  touchwin(invariantsMenuWinScr);
  touchwin(invariantsMenuWinRng);
  touchwin(invariantsMenuWinBmsk);
  touchwin(textMin);
  touchwin(textMax);
  touchwin(textFirst);
  touchwin(textMask);
  wrefresh(mainMenuWin);
  wrefresh(dataWinBox);
  wrefresh(dataWin);
  wrefresh(sflResultWin);
  wrefresh(opmodeMenuWin);
  wrefresh(spectraMenuWin);
  wrefresh(spectraOpMenuWin);
  wrefresh(sflCoeffMenuWin);
  wrefresh(invariantTypesMenuWin);
  wrefresh(invariantsMenuWinScr);
  wrefresh(invariantsMenuWinRng);
  wrefresh(invariantsMenuWinBmsk);

  mvprintw(0,0, "                         Software Analyzer v%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION);
  mvprintw(1,0, "                      Delft University of Technology");

  guiState = GUI_STATE_MAIN;
}

void destroyGui() {
  int i;

  i=0; while(item_name(mainMenuItems[i]) != NULL)            { free_item(mainMenuItems[i]); i++; }
  i=0; while(item_name(opmodeMenuItems[i]) != NULL)          { free_item(opmodeMenuItems[i]); i++; }
  i=0; while(item_name(spectraMenuItems[i]) != NULL)         { free_item(spectraMenuItems[i]); i++; }
  i=0; while(item_name(spectraOpMenuItems[i]) != NULL)       { free_item(spectraOpMenuItems[i]); i++; }
  i=0; while(item_name(sflCoeffMenuItems[i]) != NULL)        { free_item(sflCoeffMenuItems[i]); i++; }
  i=0; while(item_name(invariantTypesMenuItems[i]) != NULL)  { free_item(invariantTypesMenuItems[i]); i++; }

  free_menu(mainMenu);
  free_menu(opmodeMenu);
  free_menu(spectraMenu);
  free_menu(sflCoeffMenu);
  free_menu(spectraOpMenu);
  free_menu(invariantTypesMenu);
}

int dataChanged() {
  return guiDataChanged;
}

/* print functions */
void guiPrintInfo();
void guiPrintOpmode();
void guiPrintSpectra();
void guiPrintSpectraOp();
void guiPrintSFLCoeff();
void guiPrintSFLResult();
void guiPrintSpectrumData();
void guiPrintInvariantTypes();
void guiPrintInvariants();
void guiPrintInvariantEdit();
void guiPrintDefault();
int invariantToString(char* str, _Invariant *inv, int mode);
void editValue(WINDOW* editWindow, void *valPtr, int datatype);
void refreshInvariantsList();


/* update function for the gui components */
void updateGui() {
  box(dataWinBox, 0,0);
  werase(dataWin); /* clear data window */
  wrefresh(mainMenuWin);
  switch(guiState) {
    case GUI_STATE_MAIN:
      guiPrintInfo();
      wrefresh(dataWinBox);
      wrefresh(dataWin);
      break;
    case GUI_STATE_OPMODE:
      guiPrintOpmode();
      wrefresh(dataWinBox);
      wrefresh(dataWin);
      touchwin(opmodeMenuWin);
      wrefresh(opmodeMenuWin);
      break;
    case GUI_STATE_SPECTRA:
      guiPrintSpectra();
      wrefresh(dataWinBox);
      wrefresh(dataWin);
      touchwin(spectraMenuWin);
      wrefresh(spectraMenuWin);
      break;
    case GUI_STATE_SPECTRA_OP:
      guiPrintSpectraOp();
      wrefresh(dataWinBox);
      wrefresh(dataWin);
      touchwin(spectraOpMenuWin);
      wrefresh(spectraOpMenuWin);
      break;
    case GUI_STATE_SFL_COEFF:
      guiPrintSFLCoeff();
      wrefresh(dataWinBox);
      wrefresh(dataWin);
      touchwin(sflCoeffMenuWin);
      wrefresh(sflCoeffMenuWin);
      break;
    case GUI_STATE_SFL_RESULT:
      guiPrintSFLResult();
      wrefresh(dataWinBox);
      wrefresh(dataWin);
      wrefresh(sflResultWin);
      break;
    case GUI_STATE_SPECTRUM_DATA:
      guiPrintSpectrumData();
      wrefresh(dataWinBox);
      wrefresh(dataWin);
      wrefresh(sflResultWin);
      break;
    case GUI_STATE_INVARIANTTYPES:
      guiPrintInvariantTypes();
      wrefresh(dataWinBox);
      wrefresh(dataWin);
      touchwin(invariantTypesMenuWin);
      wrefresh(invariantTypesMenuWin);
      break;
    case GUI_STATE_INVARIANTS:
      guiPrintInvariants();
      wrefresh(dataWinBox);
      wrefresh(dataWin);
      switch(guiSelectedInvariantMode) {
        case INV_MODE_SCREENER:
          touchwin(invariantsMenuWinScr);
          wrefresh(invariantsMenuWinScr);
          break;
        case INV_MODE_RANGE:
          touchwin(invariantsMenuWinRng);
          wrefresh(invariantsMenuWinRng);
          break;
        case INV_MODE_BITMASK:
          touchwin(invariantsMenuWinBmsk);
          wrefresh(invariantsMenuWinBmsk);
          break;
      }
      break;
    case GUI_STATE_INVARIANT_EDIT:
      guiPrintInvariantEdit();
      wrefresh(dataWinBox);
      wrefresh(dataWin);
      wrefresh(textMin);
      wrefresh(textMax);
      wrefresh(textFirst);
      wrefresh(textMask);
      break;
    case GUI_STATE_TMP:
    default:
      guiPrintDefault();
      wrefresh(dataWinBox);
      wrefresh(dataWin);
      break;
  }
}


/* main gui loop function, can be called repeatedly, until returning 0 */
int loopGui() {
  int c;

  updateGui(guiProgramData, guiContext);

  c = getch();
  
  if(c=='q') {
    return 0;
  }

  switch(guiState) {
    case GUI_STATE_MAIN:
      switch(c) {
        case KEY_DOWN:
          menu_driver(mainMenu, REQ_DOWN_ITEM);
          break;
        case KEY_UP:
          menu_driver(mainMenu, REQ_UP_ITEM);
          break;
        case 10: /* enter */
          {
            switch((int)item_userptr(current_item(mainMenu))) {
              case MENU_ITEM_EXIT:
                return 0;
              case MENU_ITEM_OPMODE:
                set_menu_fore(mainMenu, A_NORMAL);
                guiState = GUI_STATE_OPMODE;
                break;
              case MENU_ITEM_SPECTRA:
                set_menu_fore(mainMenu, A_NORMAL);
                guiState = GUI_STATE_SPECTRA;
                break;
              case MENU_ITEM_INVARIANTS:
                set_menu_fore(mainMenu, A_NORMAL);
                guiState = GUI_STATE_INVARIANTTYPES;
                break;
              default:
                set_menu_fore(mainMenu, A_NORMAL);
                guiState = GUI_STATE_TMP;
                break;
            }
          }
          break;
        default:
          break;
      }
      break;
    case GUI_STATE_OPMODE:
      switch(c) {
        case KEY_DOWN:
          menu_driver(opmodeMenu, REQ_DOWN_ITEM);
          break;
        case KEY_UP:
          menu_driver(opmodeMenu, REQ_UP_ITEM);
          break;
        case 10: /* enter */
          {
            switch((int)item_userptr(current_item(opmodeMenu))) {
              case MENU_ITEM_TRAINING:
                guiProgramData->opMode = 0; /* TODO: define this */
                guiDataChanged = 1;
                break;
              case MENU_ITEM_TESTING:
                guiProgramData->opMode = 1; /* TODO: define this */
                guiDataChanged = 1;
                break;
              default:
                break;
            }
            set_menu_fore(mainMenu, A_STANDOUT);
            guiState = GUI_STATE_MAIN;
          }
          break;
        case KEY_BACKSPACE:
          set_menu_fore(mainMenu, A_STANDOUT);
          guiState = GUI_STATE_MAIN;
          break;
      }
      break;
    case GUI_STATE_SPECTRA:
      switch(c) {
        case KEY_DOWN:
          menu_driver(spectraMenu, REQ_DOWN_ITEM);
          break;
        case KEY_UP:
          menu_driver(spectraMenu, REQ_UP_ITEM);
          break;
        case 10: /* enter */
          guiSelectedSpectrum = item_index(current_item(spectraMenu));
          if(guiSelectedSpectrum >= 0) {
            guiState = GUI_STATE_SPECTRA_OP;
          }
          break;
        case KEY_BACKSPACE:
          set_menu_fore(mainMenu, A_STANDOUT);
          guiState = GUI_STATE_MAIN;
          break;
      }
      break;
    case GUI_STATE_SPECTRA_OP:
      switch(c) {
        case KEY_DOWN:
          menu_driver(spectraOpMenu, REQ_DOWN_ITEM);
          break;
        case KEY_UP:
          menu_driver(spectraOpMenu, REQ_UP_ITEM);
          break;
        case 10: /* enter */
          switch((int)item_userptr(current_item(spectraOpMenu))) {
            case MENU_ITEM_DATA:
              guiSelectedSFLResultOffset = 0;
              guiState = GUI_STATE_SPECTRUM_DATA;
              break;
            case MENU_ITEM_SFL:
              guiState = GUI_STATE_SFL_COEFF;
              break;
            default:
              guiState = GUI_STATE_SPECTRA;
              break;
          }
          break;
        case KEY_BACKSPACE:
          guiState = GUI_STATE_SPECTRA;
          break;
      }
      break;
    case GUI_STATE_SFL_COEFF:
      switch(c) {
        case KEY_DOWN:
          menu_driver(sflCoeffMenu, REQ_DOWN_ITEM);
          break;
        case KEY_UP:
          menu_driver(sflCoeffMenu, REQ_UP_ITEM);
          break;
        case 10: /* enter */
          switch((int)item_userptr(current_item(sflCoeffMenu))) {
            case S_OCHIAI:
            case S_JACCARD:
            case S_TARANTULA:
              guiSelectedSFLCoeff = (int)item_userptr(current_item(sflCoeffMenu));
              guiSFL = performSFL(guiProgramData, guiSelectedSpectrum, guiSelectedSFLCoeff);
              guiSelectedSFLResultOffset = 0;
              guiState = GUI_STATE_SFL_RESULT;
              break;
            default:
              guiState = GUI_STATE_SPECTRA_OP;
              break;
          }
          break;
        case KEY_BACKSPACE:
          guiState = GUI_STATE_SPECTRA_OP;
          break;
      }
      break;
    case GUI_STATE_SFL_RESULT:
      switch(c) {
        case KEY_DOWN:
          if(guiSelectedSFLResultOffset < (int)getSpectrum(guiProgramData, guiSelectedSpectrum)->nComponents - SFL_VISIBLE_RESULTS) {
            guiSelectedSFLResultOffset += 1;
          }
          break;
        case KEY_UP:
          if(guiSelectedSFLResultOffset > 0) {
            guiSelectedSFLResultOffset -= 1;
          }
          break;
        case KEY_NPAGE:
          guiSelectedSFLResultOffset += SFL_VISIBLE_RESULTS;
          if(guiSelectedSFLResultOffset > (int)getSpectrum(guiProgramData, guiSelectedSpectrum)->nComponents - SFL_VISIBLE_RESULTS) {
            guiSelectedSFLResultOffset = (int)getSpectrum(guiProgramData, guiSelectedSpectrum)->nComponents - SFL_VISIBLE_RESULTS;
          }
          if(guiSelectedSFLResultOffset < 0) {
            guiSelectedSFLResultOffset = 0;
          }
          break;
        case KEY_PPAGE:
          guiSelectedSFLResultOffset -= SFL_VISIBLE_RESULTS;
          if(guiSelectedSFLResultOffset < 0) {
            guiSelectedSFLResultOffset = 0;
          }
          break;
        case KEY_BACKSPACE:
          guiState = GUI_STATE_SFL_COEFF;
          break;
      }
      break;
    case GUI_STATE_SPECTRUM_DATA:
      switch(c) {
        case KEY_DOWN:
          if(guiSelectedSFLResultOffset < (int)getSpectrum(guiProgramData, guiSelectedSpectrum)->nComponents - SFL_VISIBLE_RESULTS) {
            guiSelectedSFLResultOffset += 1;
          }
          break;
        case KEY_UP:
          if(guiSelectedSFLResultOffset > 0) {
            guiSelectedSFLResultOffset -= 1;
          }
          break;
        case KEY_NPAGE:
          guiSelectedSFLResultOffset += SFL_VISIBLE_RESULTS;
          if(guiSelectedSFLResultOffset > (int)getSpectrum(guiProgramData, guiSelectedSpectrum)->nComponents - SFL_VISIBLE_RESULTS) {
            guiSelectedSFLResultOffset = (int)getSpectrum(guiProgramData, guiSelectedSpectrum)->nComponents - SFL_VISIBLE_RESULTS;
          }
          if(guiSelectedSFLResultOffset < 0) {
            guiSelectedSFLResultOffset = 0;
          }
          break;
        case KEY_PPAGE:
          guiSelectedSFLResultOffset -= SFL_VISIBLE_RESULTS;
          if(guiSelectedSFLResultOffset < 0) {
            guiSelectedSFLResultOffset = 0;
          }
          break;
        case KEY_BACKSPACE:
          guiState = GUI_STATE_SPECTRA_OP;
          break;
      }
      break;
    case GUI_STATE_INVARIANTTYPES:
      switch(c) {
        case KEY_DOWN:
          menu_driver(invariantTypesMenu, REQ_DOWN_ITEM);
          break;
        case KEY_UP:
          menu_driver(invariantTypesMenu, REQ_UP_ITEM);
          break;
        case 10: /* enter */
          guiSelectedInvariantType = item_index(current_item(invariantTypesMenu));
          if(guiSelectedInvariantType >= 0) {
            refreshInvariantsList();
            guiState = GUI_STATE_INVARIANTS;
            guiSelectedInvariantMode = INV_MODE_RANGE;
          }
          break;
        case KEY_BACKSPACE:
          set_menu_fore(mainMenu, A_STANDOUT);
          guiState = GUI_STATE_MAIN;
          break;
      }
      break;
    case GUI_STATE_INVARIANTS:
      switch(c) {
        case KEY_DOWN:
          menu_driver(invariantsMenuScr, REQ_DOWN_ITEM);
          menu_driver(invariantsMenuRng, REQ_DOWN_ITEM);
          menu_driver(invariantsMenuBmsk, REQ_DOWN_ITEM);
          break;
        case KEY_UP:
          menu_driver(invariantsMenuScr, REQ_UP_ITEM);
          menu_driver(invariantsMenuRng, REQ_UP_ITEM);
          menu_driver(invariantsMenuBmsk, REQ_UP_ITEM);
          break;
        case KEY_RIGHT:
          guiSelectedInvariantMode = (guiSelectedInvariantMode + 1 + 3)%3;
          break;
        case KEY_LEFT:
          guiSelectedInvariantMode = (guiSelectedInvariantMode - 1 + 3)%3;
          break;
        case KEY_NPAGE:
          {
            int i;
            for(i=0; i<10; i++) {
              menu_driver(invariantsMenuScr, REQ_DOWN_ITEM);
              menu_driver(invariantsMenuRng, REQ_DOWN_ITEM);
              menu_driver(invariantsMenuBmsk, REQ_DOWN_ITEM);
            }
          }
          break;
        case KEY_PPAGE:
          {
            int i;
            for(i=0; i<10; i++) {
              menu_driver(invariantsMenuScr, REQ_UP_ITEM);
              menu_driver(invariantsMenuRng, REQ_UP_ITEM);
              menu_driver(invariantsMenuBmsk, REQ_UP_ITEM);
            }
          }
          break;
        case 10: /* enter */
          guiSelectedInvariant = item_index(current_item(invariantsMenuScr));
          if(guiSelectedInvariant >= 0) {
            guiInv = getInvariantType(guiProgramData, guiSelectedInvariantType)->data[guiSelectedInvariant];
            guiState = GUI_STATE_INVARIANT_EDIT;
            guiEditState = GUI_EDIT_STATE_NONE;
            guiSelectedEditState = 0;
          }
          break;
        case KEY_BACKSPACE:
          guiState = GUI_STATE_INVARIANTTYPES;
          break;
      }
      break;
    case GUI_STATE_INVARIANT_EDIT:
      if(guiEditState == GUI_EDIT_STATE_NONE) {
        switch(c) {
          case KEY_UP:
            guiSelectedEditState = (guiSelectedEditState - 1 + 4)%4;
            break;
          case KEY_DOWN:
            guiSelectedEditState = (guiSelectedEditState + 1 + 4)%4;
            break;
          case 10: /* enter */
            guiEditState = guiSelectedEditState+1;
            guiSelectedInvariant = item_index(current_item(invariantsMenuScr));
            break;
          case KEY_BACKSPACE:
            refreshInvariantsList();
            guiState = GUI_STATE_INVARIANTS;
            break;
        }
      }
      if(guiEditState != GUI_EDIT_STATE_NONE) {
        if(guiEditState == GUI_EDIT_STATE_MIN) {
          switch(guiInv.datatype) {
            case DATA_TYPE_INT:
              editValue(textMin, &guiInv.range.i.min, DATA_TYPE_INT);
              break;
            case DATA_TYPE_UINT:
            case DATA_TYPE_UNSET:
              editValue(textMin, &guiInv.range.u.min, DATA_TYPE_UINT);
              break;
            case DATA_TYPE_DOUBLE:
              editValue(textMin, &guiInv.range.d.min, DATA_TYPE_DOUBLE);
              break;
            case DATA_TYPE_PTR:
              editValue(textMin, &guiInv.range.p.min, DATA_TYPE_PTR);
              break;
          }
        } else if(guiEditState == GUI_EDIT_STATE_MAX) {
          switch(guiInv.datatype) {
            case DATA_TYPE_INT:
              editValue(textMax, &guiInv.range.i.max, DATA_TYPE_INT);
              break;
            case DATA_TYPE_UINT:
            case DATA_TYPE_UNSET:
              editValue(textMax, &guiInv.range.u.max, DATA_TYPE_UINT);
              break;
            case DATA_TYPE_DOUBLE:
              editValue(textMax, &guiInv.range.d.max, DATA_TYPE_DOUBLE);
              break;
            case DATA_TYPE_PTR:
              editValue(textMax, &guiInv.range.p.max, DATA_TYPE_PTR);
              break;
          }
        } else if(guiEditState == GUI_EDIT_STATE_FIRST) {
          editValue(textFirst, &guiInv.bitmask.first, DATA_TYPE_PTR);
        } else if(guiEditState == GUI_EDIT_STATE_MASK) {
          editValue(textMask, &guiInv.bitmask.mask, DATA_TYPE_PTR);
        }
        guiEditState = GUI_EDIT_STATE_NONE;
        getInvariantType(guiProgramData, guiSelectedInvariantType)->data[guiSelectedInvariant] = guiInv;
        guiDataChanged = 1;
      }
      break;
    case GUI_STATE_TMP:
    default:
      switch(c) {
        case KEY_BACKSPACE:
          set_menu_fore(mainMenu, A_STANDOUT);
          guiState = GUI_STATE_MAIN;
          break;
      }
      break;
  }
  return 1;
}


void createWindows() {
  /* create and position windows */
  dataWinBox              = newwin(22, 60,  2, 19);
  dataWin                 = newwin(20, 58,  3, 20);
  sflResultWin            = newwin(SFL_VISIBLE_RESULTS+2, 56, 11, 21);
  mainMenuWin             = newwin(12, 18,  2,  1);
  opmodeMenuWin           = newwin( 4, 10,  8, 40);
  spectraMenuWin          = newwin(12, 40,  8, 30);
  spectraOpMenuWin        = newwin( 4, 25, 13, 40);
  sflCoeffMenuWin         = newwin( 4, 10, 13, 40);
  invariantTypesMenuWin   = newwin(12, 40,  8, 30);
  invariantsMenuWinScr    = newwin(12, 56, 11, 21);
  invariantsMenuWinRng    = newwin(12, 56, 11, 21);
  invariantsMenuWinBmsk   = newwin(12, 56, 11, 21);
  textMin                 = newwin( 1, 20, 11, 40);
  textMax                 = newwin( 1, 20, 12, 40);
  textFirst               = newwin( 1, 20, 14, 40);
  textMask                = newwin( 1, 20, 15, 40);

  /* make sure the function keys can be cought */
  keypad(mainMenuWin, TRUE);
  keypad(opmodeMenuWin, TRUE);
  keypad(spectraMenuWin, TRUE);
  keypad(spectraOpMenuWin, TRUE);
  keypad(sflCoeffMenuWin, TRUE);
  keypad(invariantTypesMenuWin, TRUE);
  keypad(invariantsMenuWinScr, TRUE);
  keypad(invariantsMenuWinRng, TRUE);
  keypad(invariantsMenuWinBmsk, TRUE);

  box(dataWinBox, 0, 0);
}

void createMenus() {
  int i;

  /* create menu items */
  mainMenuItems = (ITEM**)calloc(5, sizeof(ITEM*));
  mainMenuItems[0] = new_item("Operating Mode", "");
  set_item_userptr(mainMenuItems[0], (void*)MENU_ITEM_OPMODE);
  mainMenuItems[1] = new_item("Spectra",        "");
  set_item_userptr(mainMenuItems[1], (void*)MENU_ITEM_SPECTRA);
  mainMenuItems[2] = new_item("Invariants",     "");
  set_item_userptr(mainMenuItems[2], (void*)MENU_ITEM_INVARIANTS);
  mainMenuItems[3] = new_item("Exit",           "");
  set_item_userptr(mainMenuItems[3], (void*)MENU_ITEM_EXIT);
  mainMenuItems[4] = new_item(NULL,             NULL);

  opmodeMenuItems = (ITEM**)calloc(3, sizeof(ITEM*));
  opmodeMenuItems[0] = new_item("Training",     "");
  set_item_userptr(opmodeMenuItems[0], (void*)MENU_ITEM_TRAINING);
  opmodeMenuItems[1] = new_item("Testing",      "");
  set_item_userptr(opmodeMenuItems[1], (void*)MENU_ITEM_TESTING);
  opmodeMenuItems[2] = new_item(NULL,           NULL);

  spectraMenuItems = (ITEM**)calloc(guiProgramData->nSpectra+1, sizeof(ITEM*));
  for(i=0; i<guiProgramData->nSpectra; i++) {
    char *spectrumInfo = (char*)malloc(64);
    sprintf(spectrumInfo, "%-20s | %5d", 
      getSpectrum(guiProgramData, i)->name, 
      getSpectrum(guiProgramData, i)->nComponents);
    spectraMenuItems[i] = new_item(spectrumInfo, "");
  }

  spectraOpMenuItems = (ITEM**)calloc(3, sizeof(ITEM*));
  spectraOpMenuItems[0] = new_item("Inspect data",         "");
  set_item_userptr(spectraOpMenuItems[0], (void*)MENU_ITEM_DATA);
  spectraOpMenuItems[1] = new_item("Perform SFL analysis", "");
  set_item_userptr(spectraOpMenuItems[1], (void*)MENU_ITEM_SFL);
  spectraOpMenuItems[2] = new_item(NULL,                   NULL);
  
  sflCoeffMenuItems = (ITEM**)calloc(3, sizeof(ITEM*));
  sflCoeffMenuItems[0] = new_item("Ochiai",         "");
  set_item_userptr(sflCoeffMenuItems[0], (void*)S_OCHIAI);
  sflCoeffMenuItems[1] = new_item("Jaccard",        "");
  set_item_userptr(sflCoeffMenuItems[1], (void*)S_JACCARD);
  sflCoeffMenuItems[2] = new_item("Tarantula",      "");
  set_item_userptr(sflCoeffMenuItems[1], (void*)S_TARANTULA);
  sflCoeffMenuItems[3] = new_item(NULL,             NULL);
  
  invariantTypesMenuItems = (ITEM**)calloc(guiProgramData->nInvariantTypes+1, sizeof(ITEM*));
  for(i=0; i<guiProgramData->nInvariantTypes; i++) {
    char *invariantTypeInfo = (char*)malloc(64);
    sprintf(invariantTypeInfo, "%-20s | %5d", 
      getInvariantType(guiProgramData, i)->name, 
      getInvariantType(guiProgramData, i)->nInvariants);
    invariantTypesMenuItems[i] = new_item(invariantTypeInfo, "");
  }

  /*create actual menus*/
  mainMenu = new_menu((ITEM**)mainMenuItems);
  set_menu_win(mainMenu, mainMenuWin);
  set_menu_sub(mainMenu, derwin(mainMenuWin, 10,16, 1,1));
  set_menu_format(mainMenu, 10,1);
  box(mainMenuWin, 0, 0);
  post_menu(mainMenu);

  opmodeMenu = new_menu((ITEM**)opmodeMenuItems);
  set_menu_win(opmodeMenu, opmodeMenuWin);
  set_menu_format(opmodeMenu, 10,1);
  post_menu(opmodeMenu);

  spectraMenu = new_menu((ITEM**)spectraMenuItems);
  set_menu_win(spectraMenu, spectraMenuWin);
  set_menu_format(spectraMenu, 10,1);
  post_menu(spectraMenu);

  spectraOpMenu = new_menu((ITEM**)spectraOpMenuItems);
  set_menu_win(spectraOpMenu, spectraOpMenuWin);
  set_menu_format(spectraOpMenu, 10,1);
  post_menu(spectraOpMenu);

  sflCoeffMenu = new_menu((ITEM**)sflCoeffMenuItems);
  set_menu_win(sflCoeffMenu, sflCoeffMenuWin);
  set_menu_format(sflCoeffMenu, 10,1);
  post_menu(sflCoeffMenu);

  invariantTypesMenu = new_menu((ITEM**)invariantTypesMenuItems);
  set_menu_win(invariantTypesMenu, invariantTypesMenuWin);
  set_menu_format(invariantTypesMenu, 10,1);
  post_menu(invariantTypesMenu);
}

#define GUI_OPT_UP_DOWN    (1 << 1)
#define GUI_OPT_PG_UP_DOWN (1 << 2)
#define GUI_OPT_LEFT_RIGHT (1 << 3)
#define GUI_OPT_ENTER      (1 << 4)
#define GUI_OPT_BACKSPACE  (1 << 5)
#define GUI_OPT_INT        (1 << 6)
#define GUI_OPT_UINT       (1 << 7)
#define GUI_OPT_HEX        (1 << 8)
#define GUI_OPT_DOUBLE     (1 << 9)

void guiPrintOpt(int opt) {
  /* 0        10        20        30        40        50        60
     -[-]?[0-9]*.[0-9]+--pgup,pgdn---^v---<>---enter---backspace--
  */
  if(opt & GUI_OPT_UP_DOWN) 
    mvwprintw(dataWinBox, 21,32, "^v");
  if(opt & GUI_OPT_PG_UP_DOWN) 
    mvwprintw(dataWinBox, 21,20, "pgup,pgdn");
  if(opt & GUI_OPT_LEFT_RIGHT) 
    mvwprintw(dataWinBox, 21,37, "<>");
  if(opt & GUI_OPT_ENTER) 
    mvwprintw(dataWinBox, 21,42, "enter");
  if(opt & GUI_OPT_BACKSPACE) 
    mvwprintw(dataWinBox, 21,50, "backspace");
  if(opt & GUI_OPT_INT) 
    mvwprintw(dataWinBox, 21,1, "[-]?[0-9]+");
  else if(opt & GUI_OPT_UINT) 
    mvwprintw(dataWinBox, 21,1, "[0-9]+");
  if(opt & GUI_OPT_DOUBLE) 
    mvwprintw(dataWinBox, 21,1, "[-]?[0-9]*.[0-9]+");
  else if(opt & GUI_OPT_HEX) 
    mvwprintw(dataWinBox, 21,1, "[0-9,a-z,A-Z]+");
  wredrawln(dataWinBox, 21, 1);
  wrefresh(dataWinBox);
}

void guiPrintInfo() {
  mvwprintw(dataWin, 0,1, "Instrumentation Information");
  mvwprintw(dataWin, 1,1, "---------------------------");
  mvwprintw(dataWin, 3,3, "Instrumentation Version     %lu", guiProgramData->version);
  mvwprintw(dataWin, 5,3, "Number of Runs              %d", guiProgramData->nRuns);
  mvwprintw(dataWin, 7,3, "Operating Mode              %s", (guiProgramData->opMode==0)?"training":"testing");
  mvwprintw(dataWin, 9,3, "Number of Spectra           %d", guiProgramData->nSpectra);
  mvwprintw(dataWin,11,3, "Number of Invariant Types   %d", guiProgramData->nInvariantTypes);
  guiPrintOpt(GUI_OPT_UP_DOWN | GUI_OPT_ENTER);
}
void guiPrintOpmode() {
  mvwprintw(dataWin, 0, 1, "Operation mode setting");
  mvwprintw(dataWin, 1, 1, "----------------------");
  mvwprintw(dataWin, 3, 3, "Select operating mode");
  guiPrintOpt(GUI_OPT_UP_DOWN | GUI_OPT_ENTER | GUI_OPT_BACKSPACE);
}
void guiPrintSpectra() {
  mvwprintw(dataWin, 0, 1, "Spectrum information");
  mvwprintw(dataWin, 1, 1, "--------------------");
  mvwprintw(dataWin, 3, 3, "Select spectrum");
  guiPrintOpt(GUI_OPT_UP_DOWN | GUI_OPT_ENTER | GUI_OPT_BACKSPACE);
}
void guiPrintSpectraOp() {
  mvwprintw(dataWin, 0, 1, "Spectrum information");
  mvwprintw(dataWin, 1, 1, "--------------------");
  mvwprintw(dataWin, 3, 3, "spectrum name              %s", 
    getSpectrum(guiProgramData, guiSelectedSpectrum)->name);
  mvwprintw(dataWin, 5, 3, "number of components       %d", 
    getSpectrum(guiProgramData, guiSelectedSpectrum)->nComponents);
  mvwprintw(dataWin, 9, 3, "select analysis");
  guiPrintOpt(GUI_OPT_UP_DOWN | GUI_OPT_ENTER | GUI_OPT_BACKSPACE);
}
void guiPrintSFLCoeff() {
  mvwprintw(dataWin, 0, 1, "SFL analysis");
  mvwprintw(dataWin, 1, 1, "------------");
  mvwprintw(dataWin, 3, 3, "spectrum name              %s", 
    getSpectrum(guiProgramData, guiSelectedSpectrum)->name);
  mvwprintw(dataWin, 4, 3, "number of components       %d", 
    getSpectrum(guiProgramData, guiSelectedSpectrum)->nComponents);
  mvwprintw(dataWin, 9, 3, "select SFL coefficient");
  guiPrintOpt(GUI_OPT_UP_DOWN | GUI_OPT_ENTER | GUI_OPT_BACKSPACE);
}
void guiPrintSFLResult() {
  unsigned int max;
  unsigned int maxItems = SFL_VISIBLE_RESULTS;
  unsigned int sflIndex;

  mvwprintw(dataWin, 0, 1, "SFL analysis");
  mvwprintw(dataWin, 1, 1, "------------");
  mvwprintw(dataWin, 3, 3, "spectrum name              %s", 
    getSpectrum(guiProgramData, guiSelectedSpectrum)->name);
  mvwprintw(dataWin, 4, 3, "number of components       %d", 
    getSpectrum(guiProgramData, guiSelectedSpectrum)->nComponents);
  mvwprintw(dataWin, 5, 3, "SFL coefficient            %s", 
    guiSelectedSFLCoeff==S_OCHIAI?"Ochiai":
    (guiSelectedSFLCoeff==S_JACCARD?"Jaccard":
    (guiSelectedSFLCoeff==S_TARANTULA?"Tarantula":"unknown")));

  werase(sflResultWin);
  box(sflResultWin, 0, 0);
  mvwprintw(sflResultWin, 0, 2, "rank");
  mvwprintw(sflResultWin, 0, 8, "score");
  mvwprintw(sflResultWin, 0, 18, "component info");
  max = getSpectrum(guiProgramData, guiSelectedSpectrum)->nComponents;
  if(max > guiSelectedSFLResultOffset+maxItems) {
    max = guiSelectedSFLResultOffset+maxItems;
  }
  for(sflIndex=guiSelectedSFLResultOffset; sflIndex<max; sflIndex++) {
    char tmp[1024];
    sprintf(tmp, "%s:%d %s", 
      guiContext[0][guiSelectedSpectrum][guiSFL[sflIndex].componentIndex].filename,
      guiContext[0][guiSelectedSpectrum][guiSFL[sflIndex].componentIndex].line,
      guiContext[0][guiSelectedSpectrum][guiSFL[sflIndex].componentIndex].name);
    mvwprintw(sflResultWin, 1+sflIndex-guiSelectedSFLResultOffset, 1, 
      "%4d  %6f  %-38.38s",
      sflIndex,
      guiSFL[sflIndex].coefficient,
      tmp);
  }
  guiPrintOpt(GUI_OPT_PG_UP_DOWN | GUI_OPT_UP_DOWN | GUI_OPT_BACKSPACE);
}
void guiPrintSpectrumData() {
  unsigned int max;
  unsigned int maxItems = SFL_VISIBLE_RESULTS;
  unsigned int sflIndex;

  mvwprintw(dataWin, 0, 1, "Spectrum data inspection");
  mvwprintw(dataWin, 1, 1, "------------------------");
  mvwprintw(dataWin, 3, 3, "spectrum name              %s", 
    getSpectrum(guiProgramData, guiSelectedSpectrum)->name);
  mvwprintw(dataWin, 4, 3, "number of components       %d", 
    getSpectrum(guiProgramData, guiSelectedSpectrum)->nComponents);

  werase(sflResultWin);
  box(sflResultWin, 0, 0);
  mvwprintw(sflResultWin, 0, 2, "index");
  mvwprintw(sflResultWin, 0, 12, "hits");
  mvwprintw(sflResultWin, 0, 20, "component info");
  max = getSpectrum(guiProgramData, guiSelectedSpectrum)->nComponents;
  if(max > guiSelectedSFLResultOffset+maxItems) {
    max = guiSelectedSFLResultOffset+maxItems;
  }
  for(sflIndex=guiSelectedSFLResultOffset; sflIndex<max; sflIndex++) {
    char tmp[1024];
    sprintf(tmp, "%s:%d %s", 
      guiContext[0][guiSelectedSpectrum][sflIndex].filename,
      guiContext[0][guiSelectedSpectrum][sflIndex].line,
      guiContext[0][guiSelectedSpectrum][sflIndex].name);
    mvwprintw(sflResultWin, 1+sflIndex-guiSelectedSFLResultOffset, 1, 
      "%4d  %10u  %-34.34s",
      sflIndex,
      getSpectrum(guiProgramData, guiSelectedSpectrum)->data[sflIndex],
      tmp);
  }
  guiPrintOpt(GUI_OPT_UP_DOWN | GUI_OPT_BACKSPACE);
}
void guiPrintInvariantTypes() {
  mvwprintw(dataWin, 0, 1, "Invariant types information");
  mvwprintw(dataWin, 1, 1, "---------------------------");
  mvwprintw(dataWin, 3, 3, "Select invariant type");
  guiPrintOpt(GUI_OPT_UP_DOWN | GUI_OPT_ENTER | GUI_OPT_BACKSPACE);
}
void guiPrintInvariants() {
  mvwprintw(dataWin, 0, 1, "Invariants");
  mvwprintw(dataWin, 1, 1, "----------");
  mvwprintw(dataWin, 3, 3, "invariant type name        %s", 
    getInvariantType(guiProgramData, guiSelectedInvariantType)->name);
  mvwprintw(dataWin, 4, 3, "number of invariants       %d", 
    getInvariantType(guiProgramData, guiSelectedInvariantType)->nInvariants);
  mvwprintw(dataWin, 5, 3, "is updated by timer        %s", 
    getInvariantType(guiProgramData, guiSelectedInvariantType)->isTimerUpdated?"yes":"no");
  mvwprintw(dataWin, 7, 3, "show data: ");
  if(guiSelectedInvariantMode==INV_MODE_RANGE)
    wattron(dataWin, A_BOLD);
  mvwprintw(dataWin, 7, 20, "range");
  wattroff(dataWin, A_BOLD);
  if(guiSelectedInvariantMode==INV_MODE_BITMASK)
    wattron(dataWin, A_BOLD);
  mvwprintw(dataWin, 7, 27, "bitmask");
  wattroff(dataWin, A_BOLD);
  if(guiSelectedInvariantMode==INV_MODE_SCREENER)
    wattron(dataWin, A_BOLD);
  mvwprintw(dataWin, 7, 36, "screener");
  wattroff(dataWin, A_BOLD);
  if(guiSelectedInvariantMode==INV_MODE_BITMASK) {
    box(invariantsMenuWinBmsk, 0,0);
    mvwprintw(invariantsMenuWinBmsk, 0, 1, "index");
    mvwprintw(invariantsMenuWinBmsk, 0, 11, "type");
    mvwprintw(invariantsMenuWinBmsk, 0, 21, "first");
    mvwprintw(invariantsMenuWinBmsk, 0, 33, "mask");
  } else if(guiSelectedInvariantMode==INV_MODE_RANGE) {
    box(invariantsMenuWinRng, 0,0);
    mvwprintw(invariantsMenuWinRng, 0, 1, "index");
    mvwprintw(invariantsMenuWinRng, 0, 11, "type");
    mvwprintw(invariantsMenuWinRng, 0, 23, "min");
    mvwprintw(invariantsMenuWinRng, 0, 34, "max");
  } else if(guiSelectedInvariantMode==INV_MODE_SCREENER) {
    box(invariantsMenuWinScr, 0,0);
    mvwprintw(invariantsMenuWinScr, 0, 1, "index");
    mvwprintw(invariantsMenuWinScr, 0, 11, "type");
    mvwprintw(invariantsMenuWinScr, 0, 21, "range");
    mvwprintw(invariantsMenuWinScr, 0, 31, "bitmsk");
    mvwprintw(invariantsMenuWinScr, 0, 42, "bloomf");
  }
  guiPrintOpt(GUI_OPT_LEFT_RIGHT | GUI_OPT_PG_UP_DOWN | GUI_OPT_UP_DOWN | GUI_OPT_ENTER | GUI_OPT_BACKSPACE);
}
void guiPrintInvariantEdit() {
  mvwprintw(dataWin, 0, 1, "Invariant");
  mvwprintw(dataWin, 1, 1, "---------");
  mvwprintw(dataWin, 3, 3, "index:       %d", guiSelectedInvariant);
  mvwprintw(dataWin, 4, 3, "type:  %7s", guiInv.datatype==DATA_TYPE_INT?"INT":
                                         (guiInv.datatype==DATA_TYPE_UINT?"UINT":
                                         (guiInv.datatype==DATA_TYPE_PTR?"PTR":
                                         (guiInv.datatype==DATA_TYPE_DOUBLE?"DOUBLE":
                                         (guiInv.datatype==DATA_TYPE_UNSET?"UNSET":"none")))));
  mvwprintw(dataWin, 7, 3, "edit invariant data:");
  mvwprintw(dataWin, 8, 3, "%s Min:", guiSelectedEditState+1==GUI_EDIT_STATE_MIN?"->":"  ");
  mvwprintw(dataWin, 9, 3, "%s Max:", guiSelectedEditState+1==GUI_EDIT_STATE_MAX?"->":"  ");
  werase(textMin);
  werase(textMax);
  werase(textFirst);
  werase(textMask);
  switch(guiInv.datatype) {
    case DATA_TYPE_INT:
      mvwprintw(textMin, 0,0, "%d", guiInv.range.i.min);
      mvwprintw(textMax, 0,0, "%d", guiInv.range.i.max);
      break;
    case DATA_TYPE_UINT:
      mvwprintw(textMin, 0,0, "%u", guiInv.range.u.min);
      mvwprintw(textMax, 0,0, "%u", guiInv.range.u.max);
      break;
    case DATA_TYPE_DOUBLE:
      mvwprintw(textMin, 0,0, "%f", guiInv.range.d.min);
      mvwprintw(textMax, 0,0, "%f", guiInv.range.d.max);
      break;
    case DATA_TYPE_PTR:
      mvwprintw(textMin, 0,0, "%p", guiInv.range.p.min);
      mvwprintw(textMax, 0,0, "%p", guiInv.range.p.max);
      break;
    case DATA_TYPE_UNSET:
      mvwprintw(textMin, 0,0, "%d", guiInv.range.i.min);
      mvwprintw(textMax, 0,0, "%d", guiInv.range.i.max);
      break;
  }
  mvwprintw(dataWin, 11, 3, "%s First:", guiSelectedEditState+1==GUI_EDIT_STATE_FIRST?"->":"  ");
  mvwprintw(dataWin, 12, 3, "%s Mask:", guiSelectedEditState+1==GUI_EDIT_STATE_MASK?"->":"  ");
  mvwprintw(textFirst, 0,0, "%08X", guiInv.bitmask.first);
  mvwprintw(textMask, 0,0, "%08X", guiInv.bitmask.mask);
  guiPrintOpt(GUI_OPT_UP_DOWN | GUI_OPT_ENTER | GUI_OPT_BACKSPACE);
}
void guiPrintDefault() {
  mvwprintw(dataWin, 5,20, "NOT AVAILABLE");
  guiPrintOpt(GUI_OPT_BACKSPACE);
}

int invariantToString(char* str, _Invariant *inv, int mode) {
  int len = 0;
  switch(inv->datatype) {
    case DATA_TYPE_UINT:
      len += sprintf(str+len, "%7s ", "UINT");
      break;
    case DATA_TYPE_INT:
      len += sprintf(str+len, "%7s ", "INT");
      break;
    case DATA_TYPE_DOUBLE:
      len += sprintf(str+len, "%7s ", "DOUBLE");
      break;
    case DATA_TYPE_PTR:
      len += sprintf(str+len, "%7s ", "PTR");
      break;
    case DATA_TYPE_UNSET:
      len += sprintf(str+len, "%7s ", "UNSET");
      break;
  }
  switch(mode) {
    case INV_MODE_SCREENER:
      len += sprintf(str+len, "%10s %10s %10s ", (inv->activatedScreener & SCRN_RANGE)?"1":"0", 
                              (inv->activatedScreener & SCRN_BITMASK)?"1":"0", 
                              (inv->activatedScreener & SCRN_BLOOM)?"1":"0");
      break;
    case INV_MODE_RANGE:
      switch(inv->datatype) {
        case DATA_TYPE_UINT:
        case DATA_TYPE_UNSET:
          len += sprintf(str+len, "%10u %10u ", inv->range.u.min, inv->range.u.max);
          break;
        case DATA_TYPE_INT:
          len += sprintf(str+len, "%10d %10d ", inv->range.i.min, inv->range.i.max);
          break;
        case DATA_TYPE_DOUBLE:
          len += sprintf(str+len, "%10f %10f ", inv->range.d.min, inv->range.d.max);
          break;
        case DATA_TYPE_PTR:
          len += sprintf(str+len, "%10p %10p ", inv->range.p.min, inv->range.p.max);
          break;
      }
      break;
    case INV_MODE_BITMASK:
      len += sprintf(str+len, "  %08X   %08X ", inv->bitmask.first, inv->bitmask.mask);
      break;
  }
  return len;
}

void editValue(WINDOW* editWindow, void *valPtr, int datatype) {
  char tmp[20];
  int i;
  int c;
  switch(datatype) {
    case DATA_TYPE_INT:
      guiPrintOpt(GUI_OPT_INT | GUI_OPT_ENTER | GUI_OPT_BACKSPACE);
      sprintf(tmp, "%d", *(int*)valPtr);
      break;
    case DATA_TYPE_UINT:
    case DATA_TYPE_UNSET:
      guiPrintOpt(GUI_OPT_UINT | GUI_OPT_ENTER | GUI_OPT_BACKSPACE);
      sprintf(tmp, "%u", *(unsigned int*)valPtr);
      break;
    case DATA_TYPE_DOUBLE:
      guiPrintOpt(GUI_OPT_DOUBLE | GUI_OPT_ENTER | GUI_OPT_BACKSPACE);
      sprintf(tmp, "%f", *(double*)valPtr);
      break;
    case DATA_TYPE_PTR:
      guiPrintOpt(GUI_OPT_HEX | GUI_OPT_ENTER | GUI_OPT_BACKSPACE);
      sprintf(tmp, "%X", (unsigned int)*(void**)valPtr);
      break;
    default:
      break;
  }
  i = strlen(tmp);
  wattron(editWindow, A_STANDOUT);
  werase(editWindow);
  wprintw(editWindow, "%s", tmp);
  wrefresh(editWindow);
  while((c=getch())!=10) {
    if(i>0 && c==KEY_BACKSPACE) {
      i--;
      tmp[i] = 0;
    } else if(i<15) {
      switch(datatype) {
        case DATA_TYPE_INT:
          if(isdigit(c) || (i==0 && c=='-')) {
            tmp[i] = c;
            i++;
            tmp[i] = 0;
          }
          break;
        case DATA_TYPE_UINT:
        case DATA_TYPE_UNSET:
          if(isdigit(c)) {
            tmp[i] = c;
            i++;
            tmp[i] = 0;
          }
          break;
        case DATA_TYPE_DOUBLE:
          if(isdigit(c) || (i==0 && c=='-') || c=='.') {
            tmp[i] = c;
            i++;
            tmp[i] = 0;
          }
          break;
        case DATA_TYPE_PTR:
          if(isxdigit(c)) {
            tmp[i] = c;
            i++;
            tmp[i] = 0;
          }
          break;
        default:
          break;
      }
    }
    werase(editWindow);
    wprintw(editWindow, "%s", tmp);
    wrefresh(editWindow);
  }
  wattroff(editWindow, A_STANDOUT);
  wrefresh(editWindow);
  switch(datatype) {
    case DATA_TYPE_INT:
      sscanf(tmp, "%d", (int*)valPtr);
      break;
    case DATA_TYPE_UINT:
      sscanf(tmp, "%u", (unsigned int*)valPtr);
      break;
    case DATA_TYPE_DOUBLE:
      sscanf(tmp, "%lf", (double*)valPtr);
      break;
    case DATA_TYPE_PTR:
      sscanf(tmp, "%x", (unsigned int*)valPtr);
      break;
  }
}

void refreshInvariantsList() {
  if(guiSelectedInvariantType >= 0) {
    int i;
    _InvariantType *it;

    /* free existing menus */
    if(invariantsMenuItemsScr) {
      i=0; while(item_name(invariantsMenuItemsScr[i]) != NULL)  { free_item(invariantsMenuItemsScr[i]); i++; }
      free(invariantsMenuItemsScr);
    }
    if(invariantsMenuScr) {
      free_menu(invariantsMenuScr);
    }
    if(invariantsMenuItemsRng) {
      i=0; while(item_name(invariantsMenuItemsRng[i]) != NULL)  { free_item(invariantsMenuItemsRng[i]); i++; }
      free(invariantsMenuItemsRng);
    }
    if(invariantsMenuRng) {
      free_menu(invariantsMenuRng);
    }
    if(invariantsMenuItemsBmsk) {
      i=0; while(item_name(invariantsMenuItemsBmsk[i]) != NULL)  { free_item(invariantsMenuItemsBmsk[i]); i++; }
      free(invariantsMenuItemsBmsk);
    }
    if(invariantsMenuBmsk) {
      free_menu(invariantsMenuBmsk);
    }

    /* create new menus */
    it = getInvariantType(guiProgramData, guiSelectedInvariantType);
    invariantsMenuItemsScr = (ITEM**)calloc(it->nInvariants+1, sizeof(ITEM*));
    invariantsMenuItemsRng = (ITEM**)calloc(it->nInvariants+1, sizeof(ITEM*));
    invariantsMenuItemsBmsk = (ITEM**)calloc(it->nInvariants+1, sizeof(ITEM*));
    for(i=0; i<it->nInvariants; i++) {
      char *invariantInfoScr = (char*)malloc(512);
      char *invariantInfoRng = (char*)malloc(512);
      char *invariantInfoBmsk = (char*)malloc(512);
      sprintf(invariantInfoScr, "%5d  ", i);
      sprintf(invariantInfoRng, "%5d  ", i);
      sprintf(invariantInfoBmsk, "%5d  ", i);
      invariantToString(&invariantInfoScr[7], &it->data[i], INV_MODE_SCREENER);
      invariantToString(&invariantInfoRng[7], &it->data[i], INV_MODE_RANGE);
      invariantToString(&invariantInfoBmsk[7], &it->data[i], INV_MODE_BITMASK);
      invariantsMenuItemsScr[i] = new_item(invariantInfoScr, "");
      invariantsMenuItemsRng[i] = new_item(invariantInfoRng, "");
      invariantsMenuItemsBmsk[i] = new_item(invariantInfoBmsk, "");
    }
    invariantsMenuItemsScr[it->nInvariants] = new_item(NULL, NULL);
    invariantsMenuItemsRng[it->nInvariants] = new_item(NULL, NULL);
    invariantsMenuItemsBmsk[it->nInvariants] = new_item(NULL, NULL);
    invariantsMenuScr = new_menu((ITEM**)invariantsMenuItemsScr);
    invariantsMenuRng = new_menu((ITEM**)invariantsMenuItemsRng);
    invariantsMenuBmsk = new_menu((ITEM**)invariantsMenuItemsBmsk);
    set_menu_win(invariantsMenuScr, derwin(invariantsMenuWinScr, 10,54,1,1));
    set_menu_win(invariantsMenuRng, derwin(invariantsMenuWinRng, 10,54,1,1));
    set_menu_win(invariantsMenuBmsk, derwin(invariantsMenuWinBmsk, 10,54,1,1));
    set_menu_format(invariantsMenuScr, 10,1);
    set_menu_format(invariantsMenuRng, 10,1);
    set_menu_format(invariantsMenuBmsk, 10,1);
    post_menu(invariantsMenuScr);
    post_menu(invariantsMenuRng);
    post_menu(invariantsMenuBmsk);
  }
}

