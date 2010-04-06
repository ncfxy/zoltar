##===- tools/opt/Makefile ------------------------------*- Makefile -*-===##
# 
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../..
TOOLNAME = zoltar
REQUIRES_EH := 1

#LDFLAGS += ../../Debug/lib/Delft.o
#LDFLAGS += -lanalyzer
USEDLIBS = common sfl

LDFLAGS += ../../Release/lib/libinstrument.a -lmenu -lncursesw
CFLAGS += -g

include $(LEVEL)/Makefile.common
