include ../include.mk

default : all

QUEUE_LIB = libatomqueue.a


#Import Dpdk
#CFLAGS += -I../COMMON/include
#CFLAGS += -L../COMMON
CFLAGS += -latomqueue_common

OBJS = 	CLQManager.o DbQuery.o
OBJS = 	CLQManager.o

.PHONY: all clean distclean

all :: $(QUEUE_LIB)

$(QUEUE_LIB) : $(OBJS)
	$(AR) cur $@ $^
	$(RANLIB) $@

clean ::
	$(RM) -f $(QUEUE_LIB) *.o

install ::
	cp $(QUEUE_LIB) ../../
	mkdir -p ../../../INCLUDE/QUEUE -p
	cp ./*.h* ../../../INCLUDE/QUEUE -r
