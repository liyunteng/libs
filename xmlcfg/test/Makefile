include ../../build/def.mk

PROGBIN := xmlcfg_test
INC_PATH += -I../include
INC_PATH += -I../../include
LD_LIBS_PATH += -L../.libs
LD_LIBS += -lxmlcfg -lpthread
CFLAGS += `xml2-config --cflags --libs`

include ../../build/bin.mk
