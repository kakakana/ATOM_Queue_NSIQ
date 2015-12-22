include ../include.mk

#CFLAGS += -DRTE_MACHINE_CPUFLAG_SSE 
#CFLAGS += -DRTE_MACHINE_CPUFLAG_SSE2
#CFLAGS += -DRTE_MACHINE_CPUFLAG_SSE3
#CFLAGS += -DRTE_MACHINE_CPUFLAG_SSSE3
#CFLAGS += -DRTE_MACHINE_CPUFLAG_SSE4_1
#CFLAGS += -DRTE_MACHINE_CPUFLAG_SSE4_2
#CFLAGS += -DRTE_MACHINE_CPUFLAG_AES
#CFLAGS += -DRTE_MACHINE_CPUFLAG_PCLMULQDQ 
#CFLAGS += -DRTE_MACHINE_CPUFLAG_AVX
#CFLAGS += -m64
#CFLAGS += -pthread
#CFLAGS += -march=native
#CFLAGS += -DRTE_COMPILE_TIME_CPUFLAGS=RTE_CPUFLAG_SSE,RTE_CPUFLAG_SSE2,RTE_CPUFLAG_SSE3,RTE_CPUFLAG_SSSE3,RTE_CPUFLAG_SSE4_1,RTE_CPUFLAG_SSE4_2,RTE_CPUFLAG_AES,RTE_CPUFLAG_PCLMULQDQ,RTE_CPUFLAG_AVX 
#CFLAGS += -W -Wall -Werror -Wstrict-prototypes 
#CFLAGS += -Wmissing-prototypes -Wmissing-declarations
#CFLAGS += -Wold-style-definition -Wpointer-arith 
#CFLAGS += -Wcast-align -Wnested-externs -Wcast-qual 
#CFLAGS += -Wformat-nonliteral -Wformat-security -Wundef -Wwrite-strings 
#CFLAGS += -Wno-missing-field-initializers -Wno-uninitialized 
#CFLAGS += -Wno-return-type 
#CFLAGS += -Wp,-MD
#CFLAGS += -lrt
default : all

OBJS = 	eal.o \
		eal_hugepage_info.o \
		eal_memory.o \
		eal_thread.o \
		eal_log.o \
		eal_pci.o \
		eal_pci_uio.o \
		eal_pci_vfio.o \
		eal_pci_vfio_mp_sync.o \
		eal_debug.o \
		eal_lcore.o \
		eal_timer.o \
		eal_interrupts.o \
		eal_alarm.o \
		eal_common_lcore.o \
		eal_common_timer.o \
		eal_common_memzone.o \
		eal_common_log.o \
		eal_common_launch.o \
		eal_common_pci.o \
		eal_common_pci_uio.o \
		eal_common_memory.o \
		eal_common_tailqs.o \
		eal_common_errno.o \
		eal_common_cpuflags.o \
		eal_common_string_fns.o \
		eal_common_hexdump.o \
		eal_common_devargs.o \
		eal_common_dev.o \
		eal_common_options.o \
		eal_common_thread.o \
		rte_malloc.o \
		malloc_elem.o \
		malloc_heap.o \
		rte_mbuf.o \
		rte_ring.o \
		rte_mempool.o

.PHONY: all clean distclean

all :: $(LQ_COMMON_LIB)

$(LQ_COMMON_LIB) : $(OBJS)
	$(AR) cur $@ $^
	$(RANLIB) $@

clean ::
	$(RM) -f $(LQ_COMMON_LIB) *.o
