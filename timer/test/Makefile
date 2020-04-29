include ../../build/def.mk

PROGBIN = timer_test
INC_PATH += -I../
INC_PATH += -I../../include
INC_PATH += -I../include -I../../mpool/include
LD_LIBS_PATH += -L../../mpool/.libs -L../.libs
LD_LIBS += -ltimer -lmpool

include ../../build/bin.mk
