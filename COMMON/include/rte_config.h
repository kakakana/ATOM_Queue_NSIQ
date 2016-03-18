#ifndef __RTE_CONFIG_H
#define __RTE_CONFIG_H
#undef RTE_EXEC_ENV
#define RTE_EXEC_ENV "linuxapp"
#undef RTE_EXEC_ENV_LINUXAPP
#define RTE_EXEC_ENV_LINUXAPP 1
#undef RTE_FORCE_INTRINSICS
#undef RTE_ARCH_STRICT_ALIGN
#undef RTE_BUILD_SHARED_LIB
#undef RTE_BUILD_COMBINE_LIBS
#undef RTE_NEXT_ABI
#define RTE_NEXT_ABI 1
#undef RTE_LIBRTE_EAL
#define RTE_LIBRTE_EAL 1
#undef RTE_MAX_LCORE
#define RTE_MAX_LCORE 128
#undef RTE_MAX_NUMA_NODES
#define RTE_MAX_NUMA_NODES 8
#undef RTE_MAX_MEMSEG
#define RTE_MAX_MEMSEG 256
#undef RTE_MAX_MEMZONE
#define RTE_MAX_MEMZONE 2560
#undef RTE_MAX_TAILQ
#define RTE_MAX_TAILQ 32
#undef RTE_LOG_LEVEL
#define RTE_LOG_LEVEL 8
#undef RTE_LOG_HISTORY
#define RTE_LOG_HISTORY 256
#undef RTE_LIBEAL_USE_HPET
#undef RTE_EAL_ALLOW_INV_SOCKET_ID
#undef RTE_EAL_ALWAYS_PANIC_ON_ERROR
#undef RTE_EAL_IGB_UIO
#define RTE_EAL_IGB_UIO 1
#undef RTE_EAL_VFIO
#define RTE_EAL_VFIO 1
#undef RTE_MALLOC_DEBUG
#undef RTE_PCI_CONFIG
#undef RTE_PCI_EXTENDED_TAG
#define RTE_PCI_EXTENDED_TAG ""
#undef RTE_PCI_MAX_READ_REQUEST_SIZE
#define RTE_PCI_MAX_READ_REQUEST_SIZE 0
#undef RTE_LIBRTE_EAL_LINUXAPP
#define RTE_LIBRTE_EAL_LINUXAPP 1
#undef RTE_LIBRTE_EAL_VMWARE_TSC_MAP_SUPPORT
#define RTE_LIBRTE_EAL_VMWARE_TSC_MAP_SUPPORT 1
#undef RTE_LIBRTE_KVARGS
#define RTE_LIBRTE_KVARGS 1
#undef RTE_LIBRTE_ETHER
#define RTE_LIBRTE_ETHER 1
#undef RTE_LIBRTE_ETHDEV_DEBUG
#undef RTE_MAX_ETHPORTS
#define RTE_MAX_ETHPORTS 32
#undef RTE_MAX_QUEUES_PER_PORT
#define RTE_MAX_QUEUES_PER_PORT 256
#undef RTE_LIBRTE_IEEE1588
#undef RTE_ETHDEV_QUEUE_STAT_CNTRS
#define RTE_ETHDEV_QUEUE_STAT_CNTRS 16
#undef RTE_ETHDEV_RXTX_CALLBACKS
#define RTE_ETHDEV_RXTX_CALLBACKS 1
#undef RTE_NIC_BYPASS
#undef RTE_LIBRTE_EM_PMD
#define RTE_LIBRTE_EM_PMD 1
#undef RTE_LIBRTE_IGB_PMD
#define RTE_LIBRTE_IGB_PMD 1
#undef RTE_LIBRTE_E1000_DEBUG_INIT
#undef RTE_LIBRTE_E1000_DEBUG_RX
#undef RTE_LIBRTE_E1000_DEBUG_TX
#undef RTE_LIBRTE_E1000_DEBUG_TX_FREE
#undef RTE_LIBRTE_E1000_DEBUG_DRIVER
#undef RTE_LIBRTE_E1000_PF_DISABLE_STRIP_CRC
#undef RTE_LIBRTE_IXGBE_PMD
#define RTE_LIBRTE_IXGBE_PMD 1
#undef RTE_LIBRTE_IXGBE_DEBUG_INIT
#undef RTE_LIBRTE_IXGBE_DEBUG_RX
#undef RTE_LIBRTE_IXGBE_DEBUG_TX
#undef RTE_LIBRTE_IXGBE_DEBUG_TX_FREE
#undef RTE_LIBRTE_IXGBE_DEBUG_DRIVER
#undef RTE_LIBRTE_IXGBE_PF_DISABLE_STRIP_CRC
#undef RTE_IXGBE_INC_VECTOR
#define RTE_IXGBE_INC_VECTOR 1
#undef RTE_IXGBE_RX_OLFLAGS_ENABLE
#define RTE_IXGBE_RX_OLFLAGS_ENABLE 1
#undef RTE_LIBRTE_I40E_PMD
#define RTE_LIBRTE_I40E_PMD 1
#undef RTE_LIBRTE_I40E_DEBUG_INIT
#undef RTE_LIBRTE_I40E_DEBUG_RX
#undef RTE_LIBRTE_I40E_DEBUG_TX
#undef RTE_LIBRTE_I40E_DEBUG_TX_FREE
#undef RTE_LIBRTE_I40E_DEBUG_DRIVER
#undef RTE_LIBRTE_I40E_RX_ALLOW_BULK_ALLOC
#define RTE_LIBRTE_I40E_RX_ALLOW_BULK_ALLOC 1
#undef RTE_LIBRTE_I40E_16BYTE_RX_DESC
#undef RTE_LIBRTE_I40E_QUEUE_NUM_PER_VF
#define RTE_LIBRTE_I40E_QUEUE_NUM_PER_VF 4
#undef RTE_LIBRTE_I40E_QUEUE_NUM_PER_VM
#define RTE_LIBRTE_I40E_QUEUE_NUM_PER_VM 4
#undef RTE_LIBRTE_I40E_ITR_INTERVAL
#define RTE_LIBRTE_I40E_ITR_INTERVAL -1
#undef RTE_LIBRTE_FM10K_PMD
#define RTE_LIBRTE_FM10K_PMD 1
#undef RTE_LIBRTE_FM10K_DEBUG_INIT
#undef RTE_LIBRTE_FM10K_DEBUG_RX
#undef RTE_LIBRTE_FM10K_DEBUG_TX
#undef RTE_LIBRTE_FM10K_DEBUG_TX_FREE
#undef RTE_LIBRTE_FM10K_DEBUG_DRIVER
#undef RTE_LIBRTE_FM10K_RX_OLFLAGS_ENABLE
#define RTE_LIBRTE_FM10K_RX_OLFLAGS_ENABLE 1
#undef RTE_LIBRTE_MLX4_PMD
#undef RTE_LIBRTE_MLX4_DEBUG
#undef RTE_LIBRTE_MLX4_SGE_WR_N
#define RTE_LIBRTE_MLX4_SGE_WR_N 4
#undef RTE_LIBRTE_MLX4_MAX_INLINE
#define RTE_LIBRTE_MLX4_MAX_INLINE 0
#undef RTE_LIBRTE_MLX4_TX_MP_CACHE
#define RTE_LIBRTE_MLX4_TX_MP_CACHE 8
#undef RTE_LIBRTE_MLX4_SOFT_COUNTERS
#define RTE_LIBRTE_MLX4_SOFT_COUNTERS 1
#undef RTE_LIBRTE_BNX2X_PMD
#undef RTE_LIBRTE_BNX2X_DEBUG
#undef RTE_LIBRTE_BNX2X_DEBUG_INIT
#undef RTE_LIBRTE_BNX2X_DEBUG_RX
#undef RTE_LIBRTE_BNX2X_DEBUG_TX
#undef RTE_LIBRTE_BNX2X_MF_SUPPORT
#undef RTE_LIBRTE_CXGBE_PMD
#define RTE_LIBRTE_CXGBE_PMD 1
#undef RTE_LIBRTE_CXGBE_DEBUG
#undef RTE_LIBRTE_CXGBE_DEBUG_REG
#undef RTE_LIBRTE_CXGBE_DEBUG_MBOX
#undef RTE_LIBRTE_CXGBE_DEBUG_TX
#undef RTE_LIBRTE_CXGBE_DEBUG_RX
#undef RTE_LIBRTE_ENIC_PMD
#define RTE_LIBRTE_ENIC_PMD 1
#undef RTE_LIBRTE_ENIC_DEBUG
#undef RTE_LIBRTE_VIRTIO_PMD
#define RTE_LIBRTE_VIRTIO_PMD 1
#undef RTE_LIBRTE_VIRTIO_DEBUG_INIT
#undef RTE_LIBRTE_VIRTIO_DEBUG_RX
#undef RTE_LIBRTE_VIRTIO_DEBUG_TX
#undef RTE_LIBRTE_VIRTIO_DEBUG_DRIVER
#undef RTE_LIBRTE_VIRTIO_DEBUG_DUMP
#undef RTE_LIBRTE_VMXNET3_PMD
#define RTE_LIBRTE_VMXNET3_PMD 1
#undef RTE_LIBRTE_VMXNET3_DEBUG_INIT
#undef RTE_LIBRTE_VMXNET3_DEBUG_RX
#undef RTE_LIBRTE_VMXNET3_DEBUG_TX
#undef RTE_LIBRTE_VMXNET3_DEBUG_TX_FREE
#undef RTE_LIBRTE_VMXNET3_DEBUG_DRIVER
#undef RTE_LIBRTE_PMD_RING
#define RTE_LIBRTE_PMD_RING 1
#undef RTE_PMD_RING_MAX_RX_RINGS
#define RTE_PMD_RING_MAX_RX_RINGS 16
#undef RTE_PMD_RING_MAX_TX_RINGS
#define RTE_PMD_RING_MAX_TX_RINGS 16
#undef RTE_LIBRTE_PMD_PCAP
#undef RTE_LIBRTE_PMD_BOND
#define RTE_LIBRTE_PMD_BOND 1
#undef RTE_LIBRTE_BOND_DEBUG_ALB
#undef RTE_LIBRTE_BOND_DEBUG_ALB_L1
#undef RTE_LIBRTE_PMD_AF_PACKET
#define RTE_LIBRTE_PMD_AF_PACKET 1
#undef RTE_LIBRTE_PMD_XENVIRT
#undef RTE_LIBRTE_PMD_NULL
#define RTE_LIBRTE_PMD_NULL 1
#undef RTE_PMD_PACKET_PREFETCH
#define RTE_PMD_PACKET_PREFETCH 1
#undef RTE_LIBRTE_RING
#define RTE_LIBRTE_RING 1
#undef RTE_LIBRTE_RING_DEBUG
#undef RTE_RING_SPLIT_PROD_CONS
#undef RTE_RING_PAUSE_REP_COUNT
#define RTE_RING_PAUSE_REP_COUNT 10000
#undef RTE_LIBRTE_MEMPOOL
#define RTE_LIBRTE_MEMPOOL 1
#undef RTE_MEMPOOL_CACHE_MAX_SIZE
#define RTE_MEMPOOL_CACHE_MAX_SIZE 512
#undef RTE_LIBRTE_MEMPOOL_DEBUG
#undef RTE_LIBRTE_MBUF
#define RTE_LIBRTE_MBUF 1
#undef RTE_LIBRTE_MBUF_DEBUG
#undef RTE_MBUF_REFCNT_ATOMIC
#define RTE_MBUF_REFCNT_ATOMIC 1
#undef RTE_PKTMBUF_HEADROOM
#define RTE_PKTMBUF_HEADROOM 128
#undef RTE_LIBRTE_TIMER
#define RTE_LIBRTE_TIMER 1
#undef RTE_LIBRTE_TIMER_DEBUG
#undef RTE_LIBRTE_CFGFILE
#define RTE_LIBRTE_CFGFILE 1
#undef RTE_LIBRTE_CMDLINE
#define RTE_LIBRTE_CMDLINE 1
#undef RTE_LIBRTE_CMDLINE_DEBUG
#undef RTE_LIBRTE_HASH
#define RTE_LIBRTE_HASH 1
#undef RTE_LIBRTE_HASH_DEBUG
#undef RTE_LIBRTE_JOBSTATS
#define RTE_LIBRTE_JOBSTATS 1
#undef RTE_LIBRTE_LPM
#define RTE_LIBRTE_LPM 1
#undef RTE_LIBRTE_LPM_DEBUG
#undef RTE_LIBRTE_ACL
#define RTE_LIBRTE_ACL 1
#undef RTE_LIBRTE_ACL_DEBUG
#undef RTE_LIBRTE_POWER
#define RTE_LIBRTE_POWER 1
#undef RTE_LIBRTE_POWER_DEBUG
#undef RTE_MAX_LCORE_FREQS
#define RTE_MAX_LCORE_FREQS 64
#undef RTE_LIBRTE_NET
#define RTE_LIBRTE_NET 1
#undef RTE_LIBRTE_IP_FRAG
#define RTE_LIBRTE_IP_FRAG 1
#undef RTE_LIBRTE_IP_FRAG_DEBUG
#undef RTE_LIBRTE_IP_FRAG_MAX_FRAG
#define RTE_LIBRTE_IP_FRAG_MAX_FRAG 4
#undef RTE_LIBRTE_IP_FRAG_TBL_STAT
#undef RTE_LIBRTE_METER
#define RTE_LIBRTE_METER 1
#undef RTE_LIBRTE_SCHED
#define RTE_LIBRTE_SCHED 1
#undef RTE_SCHED_RED
#undef RTE_SCHED_COLLECT_STATS
#undef RTE_SCHED_SUBPORT_TC_OV
#undef RTE_SCHED_PORT_N_GRINDERS
#define RTE_SCHED_PORT_N_GRINDERS 8
#undef RTE_LIBRTE_DISTRIBUTOR
#define RTE_LIBRTE_DISTRIBUTOR 1
#undef RTE_LIBRTE_REORDER
#define RTE_LIBRTE_REORDER 1
#undef RTE_LIBRTE_PORT
#define RTE_LIBRTE_PORT 1
#undef RTE_PORT_STATS_COLLECT
#undef RTE_LIBRTE_TABLE
#define RTE_LIBRTE_TABLE 1
#undef RTE_TABLE_STATS_COLLECT
#undef RTE_LIBRTE_PIPELINE
#define RTE_LIBRTE_PIPELINE 1
#undef RTE_PIPELINE_STATS_COLLECT
#undef RTE_LIBRTE_KNI
#define RTE_LIBRTE_KNI 1
#undef RTE_KNI_KMOD
#define RTE_KNI_KMOD 1
#undef RTE_KNI_PREEMPT_DEFAULT
#define RTE_KNI_PREEMPT_DEFAULT 1
#undef RTE_KNI_KO_DEBUG
#undef RTE_KNI_VHOST
#undef RTE_KNI_VHOST_MAX_CACHE_SIZE
#define RTE_KNI_VHOST_MAX_CACHE_SIZE 1024
#undef RTE_KNI_VHOST_VNET_HDR_EN
#undef RTE_KNI_VHOST_DEBUG_RX
#undef RTE_KNI_VHOST_DEBUG_TX
#undef RTE_LIBRTE_VHOST
#define RTE_LIBRTE_VHOST 1
#undef RTE_LIBRTE_VHOST_USER
#define RTE_LIBRTE_VHOST_USER 1
#undef RTE_LIBRTE_VHOST_NUMA
#undef RTE_LIBRTE_VHOST_DEBUG
#undef RTE_LIBRTE_XEN_DOM0
#undef RTE_INSECURE_FUNCTION_WARNING
#undef RTE_APP_TEST
#define RTE_APP_TEST 1
#undef RTE_TEST_PMD
#define RTE_TEST_PMD 1
#undef RTE_TEST_PMD_RECORD_CORE_CYCLES
#undef RTE_TEST_PMD_RECORD_BURST_STATS
#undef RTE_MACHINE
#define RTE_MACHINE "native"
#undef RTE_ARCH
#define RTE_ARCH "x86_64"
#undef RTE_ARCH_X86_64
#define RTE_ARCH_X86_64 1
#undef RTE_ARCH_64
#define RTE_ARCH_64 1
#undef RTE_TOOLCHAIN
#define RTE_TOOLCHAIN "gcc"
#undef RTE_TOOLCHAIN_GCC
#define RTE_TOOLCHAIN_GCC 1
#undef RTE_RING_MAX_CONS_COUNT 
#define RTE_RING_MAX_CONS_COUNT 20 /**< Ring 의 Consumer 의 최대 숫자  */
#undef RTE_RING_MAX_PROD_COUNT 
#define RTE_RING_MAX_PROD_COUNT 20 /**< Ring 의Producer 의 최대 숫자  */
#endif /* __RTE_CONFIG_H */
