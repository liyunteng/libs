include ../../build/def.mk

PROGBIN = backtrace_test
INC_PATH += -I../
INC_PATH += -I../../include
INC_PATH += -I../include
LD_LIBS_PATH += -L../.libs
LD_LIBS += -lbacktrace -lpthread
CFLAGS += -rdynamic

include ../../build/bin.mk
