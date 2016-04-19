# make debug to build debug version
SUBARCH := $(shell uname -m)
ARCH			?= $(SUBARCH)
CROSS_COMPILE		?=
HOSTCC		= gcc
HOSTCXX		= g++
# HOSTCFLAGS	= -Wall -O2
# HOSTCXXFLAGS	= -O2
# if cc is clang
ifeq ($(shell $(HOSTCC) -v 2>&1 | grep -c "clang version"), 1)
HOSTCFLAGS +=
endif

AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CXX		= $(CROSS_COMPILE)g++
LIBTOOL		= $(CROSS_COMPILE)libtool
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
AWK		= awk
PERL		= perl
PYTHON		= python

CFLAGS		:=
CXXFLAGS	:= $(CFLAGS)
CPPFLAGS	:=
LDFLAGS		:=

export ARCH CROSS_COMPILE HOSTCC HOSTCXX HOSTCFLAGS HOSTCXXFLAGS
export AS LD CC CXX CPP AR NM STRIP OBJCOPY OBJDUMP AWK PERL PYTHON LIBTOOL
export CFLAGS CXXFLAGS CPPFLAGS LDFLAGS
################################################
BUILD_ENV	?=debug
ROOT		:= $(shell pwd)
INCDIR		:= $(ROOT)/include
LIBDIR		:= $(ROOT)/lib
BINDIR		:= $(ROOT)/bin
CFGDIR		:= $(ROOT)/cfg

INSTALL_BINDIR	:= /usr/local/lyt/bin/
INSTALL_LIBDIR  := /usr/local/lyt/lib/
INSTALL_INCDIR  := /usr/local/lyt/include/

SOURCE_C	:= $(wildcard *.c)
SOURCE_CXX	:= $(wildcard *.cpp)

OBJECT_C	:= $(SOURCE_C:.c=.o)
OBJECT_CXX	:= $(SOURCE_CXX:.cpp=.o)

INC_PATH	:= -I. -I${INCDIR} -I/usr/include
CXX_INC_PATH	+= $(INC_PATH)

# libraries to use in static link, xxx.a
LIBS		:=
LIBS_PATH	:=
# libraries to load in dynamic link, xxx.so
LD_LIBS		:=
LD_LIBS_PATH	:=

CLEAN_TARGETS	:= *.o *.lo *.loT *.la *.libs *.a *.so .libs
export INCDIR LIBDIR BINDIR CFGDIR
export INSTALL_BINDIR INSTALL_LIBDIR INSTALL_INCDIR
export SOURCE_C SOURCE_CXX OBJECT_C OBJECT_CXX
export INC_PATH CXX_INC_PATH
export LD_LIBS
export CLEAN_TARGETS
