#	Makefile.inc

ARCH 		:= $(shell uname -m | sed -e s/i.86/i386/ )
OS			:= $(shell uname -s )

PREFIX		:= $(dir $(word $(words $(MAKEFILE_LIST)), $(MAKEFILE_LIST)))

LQ_COMMON_LIB	= liblq_comm.a
LQ_API_LIB		= liblq_api.a

INCDIR		= -I$(PREFIX)COMMON/include
INCDIR		= -I$(PREFIX)LQ_API
#INCDIR		+= -I$(PREFIX)RING/include
#INCDIR		+= -I$(PREFIX)MEMPOOL/include
#INCDIR		+= -I$(PREFIX)MBUF/include

LIBDIR		= -L$(PREFIX)COMMON
LIBDIR		+= -L$(PREFIX)LQ_API
#LIBDIR		+= -L$(PREFIX)MBUF
#LIBDIR		+= -L$(PREFIX)RING

LIBS		=  -lpthread -lz -lstdc++
LIBS		+= -ldl -lrt
#LIBS		+= -llq_mbuf -llq_comm -llq_ring -llq_mempool
LIBS		+= -llq_comm -llq_api
 

INSTALLDIR	= $(HOME)/BIN

#
# 2015.12.16 with DPDK compile option
# 
CFLAGS		= -B$(PREFIX) -fPIC
CFLAGS += -D_GNU_SOURCE
CFLAGS += -DRTE_MACHINE_CPUFLAG_SSE
CFLAGS += -DRTE_MACHINE_CPUFLAG_SSE2
CFLAGS += -DRTE_MACHINE_CPUFLAG_SSE3
CFLAGS += -DRTE_MACHINE_CPUFLAG_SSSE3
CFLAGS += -DRTE_MACHINE_CPUFLAG_SSE4_1
CFLAGS += -DRTE_MACHINE_CPUFLAG_SSE4_2
CFLAGS += -DRTE_MACHINE_CPUFLAG_AES
CFLAGS += -DRTE_MACHINE_CPUFLAG_PCLMULQDQ
CFLAGS += -DRTE_MACHINE_CPUFLAG_AVX
CFLAGS += -m64
CFLAGS += -pthread
CFLAGS += -march=native
CFLAGS += -DRTE_COMPILE_TIME_CPUFLAGS=RTE_CPUFLAG_SSE,RTE_CPUFLAG_SSE2,RTE_CPUFLAG_SSE3,RTE_CPUFLAG_SSSE3,RTE_CPUFLAG_SSE4_1,RTE_CPUFLAG_SSE4_2,RTE_CPUFLAG_AES,RTE_CPUFLAG_PCLMULQDQ,RTE_CPUFLAG_AVX
#CFLAGS += -W -Wall -Werror -Wstrict-prototypes
#CFLAGS += -Wmissing-prototypes -Wmissing-declarations
#CFLAGS += -Wold-style-definition -Wpointer-arith
#CFLAGS += -Wcast-align -Wnested-externs -Wcast-qual
#CFLAGS += -Wformat-nonliteral -Wformat-security -Wundef -Wwrite-strings
#CFLAGS += -Wno-missing-field-initializers -Wno-uninitialized
#CFLAGS += -Wno-return-type




LDFLAGS 	= $(LIBDIR) $(LIBS)

ifeq ("$(V)", "1")
	CC			= gcc
	CXX			= g++
	RM			= rm -f
	CP			= cp -f
	MV			= mv -f
	AR			= ar
	RANLIB		= ranlib
	PROC		= proc
else
	CC			= @echo "        CC      " $@; gcc
	CXX			= @echo "        CXX     " $@; g++
	RM			= @rm -f
	CP			= @cp -f
	MV			= @mv -f
	AR			= @echo "        AR      " $@; ar
	RANLIB		= @echo "        RANLIB  " $@; ranlib
	PROC		= @echo "		 PROC	 " $@; proc
endif

CHECKFLAGS		= -D__x86_64__ -m64

CFLAGS 			+= -O0
CPPFLAGS		+= -O0

#
# 2009.03.02, with debugging options
#
CFLAGS 			+= -ggdb 


#
# 2009.03.02, add directory information
#
CFLAGS			+= $(INCDIR) $(CHECKFLAGS)
CXXFLAGS		= $(CFLAGS)
MAKEFLAGS 		+= --no-print-directory


ifneq (,$(findstring UTIL,$(CURDIR)))
	INSTALLDIR = $(HOME)/UTIL
	NETINSTALL_DIR = UTIL
endif

#
# 2009.03.02, suffix
#
.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(LOCALFLAGS) -o $@ $<
.c.o:
	$(CC) -c $(CFLAGS) $(LOCALFLAGS) -o $@ $<
.S.o:
	$(AS) -c $(INCDIR) -o $@ $<
.s.o:
	$(AS) -c $(LIBDIR) -o $@ $<

.PHONY:	all clean install netinstall initial tags cscope distclean

all clean install netinstall initial tags cscope distclean ::
	@set -e;						\
	for i in $(SUBDIR); 			\
	do ( 							\
		if [ $$i"NULL" != "NULL" ];	\
		then 						\
			echo "    CD " $(CURDIR)/$$i;	\
			$(MAKE) -C $$i $@;		\
		fi 							\
	); 								\
	done;							\

clean ::
	@echo "        CLEAN   " $(CURDIR)
	$(RM) -f $(EXES) *.o *.gcov *.gcda *.gcno

tags ::
	ctags *.c *.h *.cpp *.hpp

cscope ::

distclean :: clean
	rm -f tags

install ::
	@set -e;									\
	if [ "NULL$(EXES)" != "NULL" ];				\
	then										\
		echo "        INSTALL $(EXES)";			\
		cp $(EXES) $(INSTALLDIR);				\
	fi											

