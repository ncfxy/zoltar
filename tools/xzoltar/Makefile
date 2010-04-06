##===- tools/opt/Makefile ------------------------------*- Makefile -*-===##
# 
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../..
TOOLNAME = xzoltar
REQUIRES_EH := 1

USEDLIBS = common.a sfl.a

LDFLAGS += `pkg-config --libs gtk+-2.0`
CFLAGS += -g `pkg-config --cflags gtk+-2.0`

include $(LEVEL)/Makefile.common
