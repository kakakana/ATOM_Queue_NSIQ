include ../include.mk

default : all

OBJS = 	CLQManager.cpp

.PHONY: all clean distclean

all :: $(LQ_API_LIB)

$(LQ_API_LIB) : $(OBJS)
	$(AR) cur $@ $^
	$(RANLIB) $@

clean ::
	$(RM) -f $(LQ_API_LIB) *.o
