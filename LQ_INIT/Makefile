include ../include.mk

CFLAGS += -latomqueue_common
CFLAGS += -latomconfig
default : all

EXES = LQ_INIT

OBJS = CLQInit.o

.PHONY: all clean distclean

all :: $(EXES)

$(EXES) : $(OBJS)
	$(CXX) -o $@ $^ -Wall -O2 $(CFLAGS) -L/usr/lib64 $(INCDIR) $(LDFLAGS)
