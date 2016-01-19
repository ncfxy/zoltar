##===- tools/opt/Makefile ------------------------------*- Makefile -*-===##
# 
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
CXXFLAGS += -fexceptions -g

LEVEL = ../..
TOOLNAME = instrument
#REQUIRES_EH := 1

LINK_COMPONENTS := bitreader bitwriter transformutils
USEDLIBS := passes.a mutation.a

include $(LEVEL)/Makefile.common

