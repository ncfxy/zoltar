#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <gtk/gtk.h>

#include "invariant.h"
#include "spectrum.h"
#include "instrumentationinfo.h"
#include "fileio.h"
#include "sfl.h"

#include "version.h"

typedef struct sCodeListing {
  GtkWidget *scrollWin;
  GtkWidget *list;
  GtkWidget *topWidget;
  unsigned int nLines;
  char sourceAvailable;
} CodeListing;

enum {
  RANK_COLUMN = 0,
  SCORE_COLUMN,
  LINE_COLUMN,
  COLOR_COLUMN,
  LINENR_COLUMN,
  N_COLUMNS
};

CodeListing *newCodeListing() {
  GtkCellRenderer *renderer[4];
  GtkTreeViewColumn *column[4];
  GtkListStore *store;

  CodeListing *result = (CodeListing*)malloc(sizeof(CodeListing));

  /* create a scrolled window to contain the complete listing */
  result->scrollWin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(result->scrollWin),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(result->scrollWin),
    GTK_SHADOW_IN);
  result->topWidget = result->scrollWin;

  /* create treeview and add it to scrollwin */
  result->list = gtk_tree_view_new();
  gtk_container_add(GTK_CONTAINER(result->scrollWin), result->list);

  /* disable selection */
  gtk_tree_selection_set_mode(
    gtk_tree_view_get_selection(GTK_TREE_VIEW(result->list)), 
    GTK_SELECTION_NONE);

  /* first column - rank */
  renderer[0] = gtk_cell_renderer_text_new();
  column[0] = gtk_tree_view_column_new_with_attributes(
    "Rank", renderer[0], "text", RANK_COLUMN, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(result->list), column[0]);

  /* second column - score */
  renderer[1] = gtk_cell_renderer_text_new();
  column[1] = gtk_tree_view_column_new_with_attributes(
    "Score", renderer[1], "text", SCORE_COLUMN, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(result->list), column[1]);

  /* third column - linenr */
  renderer[2] = gtk_cell_renderer_text_new();
  column[2] = gtk_tree_view_column_new_with_attributes(
    "Line", renderer[2], "text", LINENR_COLUMN, "background", COLOR_COLUMN, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(result->list), column[2]);

  /* fourth column - line */
  renderer[3] = gtk_cell_renderer_text_new();
  column[3] = gtk_tree_view_column_new_with_attributes(
    "Code", renderer[3], "text", LINE_COLUMN, "background", COLOR_COLUMN, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(result->list), column[3]);

  /* create store for binding (rank, score, linenr, line, color) */
  store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  gtk_tree_view_set_model(GTK_TREE_VIEW(result->list), GTK_TREE_MODEL(store));
  g_object_unref(store);

  result->nLines = 0;
  result->sourceAvailable = 0;

  return result;
}

void addCodeListingLine(CodeListing* listing, char *code, int rank, float score, float color, int linenr) {
  GtkListStore *store;
  GtkTreeIter iter;
  char *rankstr, *scorestr, *colorstr, *linenrstr;
  unsigned int r, g;

  /* update listing info */
  listing->nLines++;

  /* retrieve store */
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(listing->list)));

  /* add new iterator */
  gtk_list_store_append(store, &iter);

  /* set data values */
  linenrstr = malloc(16);
  if(linenr < 0) {
    linenrstr = (char*)"";
  } else {
    sprintf(linenrstr, "%d", linenr);
  }
  rankstr = malloc(16);
  if(rank < 0) {
    rankstr = (char*)"";
  } else {
    sprintf(rankstr, "%d", rank);
  }
  scorestr = malloc(16);
  if(score < 0) {
    scorestr = (char*)"";
  } else {
    sprintf(scorestr, "%.5f", score);
  }
  colorstr = malloc(16);
  if(color < 0) {
    colorstr = (char*)"#ffffffffffff";
  } else {
    if(color > 0.5) {
      r = 0xffff;
    } else {
      r = (unsigned int) (0xffff * (color*2));
    }
    if(r > 0xffff) r = 0xffff;
    if(color < 0.5) {
      g = 0xffff;
    } else {
      g = (unsigned int) (0xffff * (1 - (color-0.5)*2));
    }
    if(g > 0xffff) g = 0xffff;
    sprintf(colorstr, "#%04x%04x%04x", r, g, 0x0000);
  }
  gtk_list_store_set(store, &iter, 
    RANK_COLUMN, rankstr, 
    SCORE_COLUMN, scorestr, 
    LINENR_COLUMN, linenrstr,
    LINE_COLUMN, code, 
    COLOR_COLUMN, colorstr, -1);
}

void modifyCodeListingLine(CodeListing* listing, unsigned int index, int rank, float score, float color) {
  GtkListStore *store;
  GtkTreeIter iter;
  char *rankstr, *scorestr, *colorstr;
  unsigned int r, g;
  unsigned int cnt=0;

  /* retrieve store */
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(listing->list)));

  /* get iterator */
  gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
  while(cnt<index) {
    gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
    cnt++;
  }

  /* check existing data, modify only if new rank is higher */
  {
    char *currRank;
    gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, RANK_COLUMN, &currRank, -1);
    if(strcmp(currRank, "") && atoi(currRank)<rank) {
      return;
    }
  }
  
  /* set data values */
  rankstr = malloc(16);
  if(rank < 0) {
    rankstr = (char*)"";
  } else {
    sprintf(rankstr, "%d", rank);
  }
  scorestr = malloc(16);
  if(score < 0) {
    scorestr = (char*)"";
  } else {
    sprintf(scorestr, "%.5f", score);
  }
  colorstr = malloc(16);
  if(color < 0) {
    colorstr = (char*)"#ffffffffffff";
  } else {
    if(color > 0.5) {
      r = 0xffff;
    } else {
      r = (unsigned int) (0xffff * (color*2));
    }
    if(r > 0xffff) r = 0xffff;
    if(color < 0.5) {
      g = 0xffff;
    } else {
      g = (unsigned int) (0xffff * (1 - (color-0.5)*2));
    }
    if(g > 0xffff) g = 0xffff;
    sprintf(colorstr, "#%04x%04x%04x", r, g, 0x0000);
  }
  gtk_list_store_set(store, &iter, 
    RANK_COLUMN, rankstr, 
    SCORE_COLUMN, scorestr, 
    COLOR_COLUMN, colorstr, -1);
}



static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
  /*g_print("delete event occurred\n");
  return TRUE;*/
  gtk_main_quit();
  return TRUE;
}

static void destroy(GtkWidget *widget, gpointer data) {
  gtk_main_quit();
}


void printHelp() {
  printf(
    "Usage:\n"
    "  xbarinel [-fFILE] [-pFILE]\n"
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
    "      --sfl=COEFF          perform SFL and output an ordered list of components\n"
    "                           where COEFF is one of\n"
    "                             ochiai\n"
    "                             jaccard\n"
    "                             tarantula\n"
    "  -i, --spectrumindex=INT  perform analysis on the INTth spectrum\n"
    "                             defaults to 0\n"
  );
}

void printVersion() {
  printf(
    "xbarinel v%d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION);
}

int main(int argc, char *argv[]) {
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *notebook;
  CodeListing *codeListing;
  CodeListing **fileListing=NULL;

  int coeff;
  char c;
  int optionIndex;
  static struct option longOptions[] = {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'v'},
    {"datafile", 1, 0, 'f'},
    {"passfailfile", 1, 0, 'p'},
    {"contextfile", 1, 0, 'c'},
		{"resultfile", 1, 0, 'r'},
    {"sfl", 1, 0, 1},
    {"spectrumindex", 1, 0, 'i'},
    {0, 0, 0, 0}
  };
  char *datafileName;
  char *passfailfileName;
  char *contextfileName;
  char *resultfileName;
  ProgramData *programData;
  Context ***context;
  int externalPassFail = 0;
	int externalResultFile = 0;
  char *sflCoeff;
  int maxItems = -1;
  unsigned int spectrumIndex = 0;
  SFLItem *sfl;
  char **dirs, **filenames;
  int nDirs, nFiles;
  float threshold;
  float *maxCoeff;
  /* initialize gtk with optional gtk-options at command line */
  /* these gtk options will be removed after call */
  gtk_init(&argc, &argv);


  datafileName = (char*)"datafile.dat";
  passfailfileName = (char*)"-";
  contextfileName = (char*)"context.dat";
	resultfileName = (char*)"result.dat";
  sflCoeff = (char*)"ochiai";
  programData = newProgramData();

  /* parse arguments */
  while ((c = getopt_long(argc, argv, "hvf:p:c:r:i:", longOptions, &optionIndex)) != EOF) {
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
			case 'r':
				resultfileName = calloc(strlen(optarg)+1, sizeof(char));
				strncpy(resultfileName, optarg, strlen(optarg));
				externalResultFile = 1;
				break;
      case 'i':
        spectrumIndex = atoi(optarg);
        break;
      case 1:
        sflCoeff = calloc(strlen(optarg)+1, sizeof(char));
        strncpy(sflCoeff, optarg, strlen(optarg));
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
      int ty, si, ci, l, di, fi;
      char name[1024];
      fscanf(cf, "%d\n", &nDirs);
      dirs = (char**)malloc(nDirs*sizeof(char*));
      for(i=0; i<nDirs; i++) {
        int dirIndex;
        fscanf(cf, "%d ", &dirIndex);
        dirs[dirIndex] = (char*)malloc(1024);
        fscanf(cf, "%s\n", dirs[dirIndex]);
      }
      fscanf(cf, "%d\n", &nFiles);
      filenames = (char**)malloc(nFiles*sizeof(char*));
      for(i=0; i<nFiles; i++) {
        int fileIndex;
        fscanf(cf, "%d ", &fileIndex);
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

  if(!strcmp("ochiai", sflCoeff)) {
    coeff = S_OCHIAI;
  } else if(!strcmp("tarantula", sflCoeff)) {
    coeff = S_TARANTULA;
  } else if(!strcmp("jaccard", sflCoeff)) {
    coeff = S_JACCARD;
  } else {
    fprintf(stderr, "unrecognized SFL coefficient\n");
    abort();
  }

	if(externalResultFile) {
		int i;
		int n = getSpectrum(programData, spectrumIndex)->nComponents;
		FILE *resf;
		sfl = malloc(n * sizeof(SFLItem));
    resf = fopen(resultfileName, "r");
    if(resf) {
      for(i=0; i<n; i++) {
				int si;
				double pr;
				int res;
				res = fscanf(resf, "%d  %lf\n", &si, &pr);
				if(res==EOF) {
					maxItems = i;
					break;
				} else if(res!=2) {
					fprintf(stderr, "error parsing %dth line of result file\n", i);
					abort();
				}
				sfl[i].componentIndex = si-1;
				sfl[i].coefficient = pr;
			}
		} else {
			fprintf(stderr, "could not open result file\n");
			abort();
	  }
	} else {
    /* perform SFL */
    sfl = performSFL(programData, spectrumIndex, coeff);
  }


  /* do graphical stuff */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(window),600,400);

  g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(delete_event), NULL);
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);
  gtk_container_set_border_width(GTK_CONTAINER(window), 10);

  notebook = gtk_notebook_new();
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
  gtk_container_add(GTK_CONTAINER(window), notebook);

  {
    GtkWidget *rankedListLabel = gtk_label_new("Ranked List");
    vbox = gtk_vbox_new(FALSE, 2);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, rankedListLabel);
    /* preferably the ranked list tab is fixed at pos 0,
     * but requires checking if orderable tabs are dragged
     * to pos 0, moving ranked list tab to 1
     * so tmp solution is to make this tab orderable as well */
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(notebook), vbox, TRUE);
  }

  {
    int d,cnt=0;
    int sflIndex;
    int max = getSpectrum(programData, spectrumIndex)->nComponents;
    float maxscore;
    float minscore;
    int fileIndex;
		if(maxItems > 0 && max > maxItems) {
			max = maxItems;
		}
    maxscore = sfl[0].coefficient;
    minscore = sfl[max-1].coefficient;
    threshold = minscore + 0.50 * (maxscore - minscore);
    maxCoeff = calloc(nDirs, sizeof(float));
    /* determine max of each source file */
    for(sflIndex=0; sflIndex<max; sflIndex++) {
      fileIndex = context[0][spectrumIndex][sfl[sflIndex].componentIndex].dirIndex;
      if(sfl[sflIndex].coefficient > maxCoeff[fileIndex]) {
        maxCoeff[fileIndex] = sfl[sflIndex].coefficient;
      }
    }
    /* load source files and add tab for each */
    for(d=0; d<nDirs; d++) {
      int i;
          GtkWidget *lstVBox, *lstLabel;
          fileListing = (CodeListing**)realloc(fileListing, (cnt+1)*sizeof(CodeListing*));
          fileListing[cnt] = newCodeListing();
          /* do not load if max sfl score is below threshold */
          if(maxCoeff[cnt] < threshold) {
            cnt++;
            continue;
          }
          lstVBox = gtk_vbox_new(FALSE, 2);
          lstLabel = gtk_label_new(strrchr(dirs[d], '/')+1);
          gtk_notebook_append_page(GTK_NOTEBOOK(notebook), lstVBox, lstLabel);
          /* every source tab is reorderable */
          gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(notebook), lstVBox, TRUE);
          /* arrange tab so that tabs are in alphabetical order */
          for(i=1; i<gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook))-1; i++) {
            if(strcmp(
                gtk_notebook_get_tab_label_text(GTK_NOTEBOOK(notebook),
                  gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), i)),
                gtk_label_get_text(GTK_LABEL(lstLabel)))
              > 0) {
              gtk_notebook_reorder_child(
                GTK_NOTEBOOK(notebook),
                lstVBox,
                i);
              break;
            }
          }
          gtk_box_pack_start(GTK_BOX(lstVBox), fileListing[cnt]->topWidget, 1,1,0);
          {
            char line[2024];
            FILE *codef;
            int linenr=0;
            if(NULL != (codef = fopen(dirs[d], "r"))) {
              fileListing[cnt]->sourceAvailable = 1;
              while(NULL != fgets(line, 2023, codef)) {
                linenr++;
                line[strlen(line)-1] = 0;
                addCodeListingLine(fileListing[cnt], line, -1, -1, -1, linenr);
              }
              fclose(codef);
            } else {
              /* nice notification */
              fprintf(stderr, "%s:\nSource file not found. Is it moved?\n", dirs[d]);
              fileListing[cnt]->sourceAvailable = 0;
            }
          }
          cnt++;
    }
  }

  codeListing = newCodeListing();
  gtk_box_pack_start(GTK_BOX(vbox), codeListing->topWidget, 1,1,0);

	/*
  //button = gtk_button_new_with_label("Hello World");
  //g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(hello), NULL);
  //g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(gtk_widget_destroy), G_OBJECT(window));
  //gtk_box_pack_start(GTK_BOX(vbox), button, 0,0,0);
  */

  {
    int sflIndex;
    int max = getSpectrum(programData, spectrumIndex)->nComponents;
    float maxscore;
    float minscore;
    int fileIndex, lineIndex;
		if(maxItems > 0 && max > maxItems) {
			max = maxItems;
		}
    maxscore = sfl[0].coefficient;
    minscore = sfl[max-1].coefficient;
    for(sflIndex=0; sflIndex<max; sflIndex++) {
      char *line = malloc(1024);
      sprintf(line, "%s:%d %s", 
        context[0][spectrumIndex][sfl[sflIndex].componentIndex].filename,
        context[0][spectrumIndex][sfl[sflIndex].componentIndex].line,
        context[0][spectrumIndex][sfl[sflIndex].componentIndex].name);
      addCodeListingLine(codeListing, line, sflIndex, sfl[sflIndex].coefficient, (sfl[sflIndex].coefficient-minscore)/(maxscore-minscore), -1);
      fileIndex = context[0][spectrumIndex][sfl[sflIndex].componentIndex].dirIndex;
      lineIndex = context[0][spectrumIndex][sfl[sflIndex].componentIndex].line - 1;
      if(maxCoeff[fileIndex] >= threshold) {
        if(fileListing[fileIndex]->sourceAvailable) {
          modifyCodeListingLine(fileListing[fileIndex], lineIndex, sflIndex, sfl[sflIndex].coefficient, (sfl[sflIndex].coefficient-minscore)/(maxscore-minscore));
        } else {
          addCodeListingLine(fileListing[fileIndex], line, sflIndex, sfl[sflIndex].coefficient, (sfl[sflIndex].coefficient-minscore)/(maxscore-minscore), -1);
        }
      }
    }
  }
 
  gtk_widget_show_all(window);



  gtk_main();

  return 0;
}
