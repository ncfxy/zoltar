##===- docs/tutorial/Makefile ------------------------------*- Makefile -*-===##
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
##===----------------------------------------------------------------------===##

LEVEL := ../..
include $(LEVEL)/Makefile.common

LATEX = pdflatex -shell-escape -interaction=batchmode

%.pdf: %.tex
	$(Echo) Compiling $@
	$(Verb) $(LATEX) $< > /dev/null
	$(Verb) $(LATEX) $< > /dev/null


.PHONY: install-pdf

PDF = tutorial.pdf
EXTRA_DIRS = sources test


all-local:: $(PDF)

install-local:: install-pdf install-src

install-pdf: tutorial.pdf
	$(Echo) Installing Documentation
	$(Verb) $(MKDIR) $(PROJ_docsdir)/tutorial
	$(Verb) $(DataInstall) $(PDF) $(PROJ_docsdir)/tutorial

install-src:
	$(Echo) Installing Example
	$(Verb) $(MKDIR) $(PROJ_docsdir)/tutorial
	$(Verb) $(MKDIR) $(PROJ_docsdir)/tutorial/sources
	$(Verb) $(DataInstall) sources/* $(PROJ_docsdir)/tutorial/sources
	$(Verb) $(MKDIR) $(PROJ_docsdir)/tutorial/test
	$(Verb) $(MKDIR) $(PROJ_docsdir)/tutorial/test/textVal
	$(Verb) $(DataInstall) test/textVal/* $(PROJ_docsdir)/tutorial/test/textVal

uninstall-local::
	$(Echo) Uninstalling Documentation
	$(Verb) $(RM) -rf $(PROJ_docsdir)

clean-local:: clean-pdf

clean-pdf:
	$(Echo) Removing PDF tutorial and intermediate files
	$(Verb) $(RM) -f $(PDF) *.log *.toc *.aux 
