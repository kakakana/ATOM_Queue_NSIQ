include ../include.mk

#CFLAGS += -DNCURSES_ON
default : all

EXES = TEST_MRT

OBJS = main.o

.PHONY: all clean distclean

all :: $(EXES)

$(EXES) : $(OBJS)
	$(CXX) -o $@ $^ -Wall -O2 $(CFLAGS) -L/usr/lib64 $(INCDIR) $(LDFLAGS)
