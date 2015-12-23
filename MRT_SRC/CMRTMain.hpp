/*!
 * \file CMRTMain.hpp
 * \brief MRT Main Class Header File
 */
#ifndef _MRT_MAIN_H_
#define _MRT_MAIN_H_

#include <stdio.h>
#include <stdlib.h>

//! Define Memory Buffer 128 byte
#define DEF_MEM_BUF_128		128

//! Define Memory Buffer 256 byte
#define DEF_MEM_BUF_256		256

//! Define Default Ring Size, (262,144)
/*!
 * Must be Power of 2
 */
#define DEF_RING_SIZE	0x40000  

//! Define Ring Name
/*!
 * Ring Name For to Communicate to Application
 */
#define DEF_RING_INFO_NAME	"RING_INFO"  

//! Define Memory Pool Name
/*!
 * Memory Pool Name For to Use in Application
 */
#define DEF_BASE_MEMORY_POOL_NAME	"BASE_MEMORY_POOL"  

//! Define Memory Buffer Elements Count
/*!
 * To Use in Memory Pool Create Function
 * Power of 2 - 1
 */
#define DEF_MBUF_COUNT	( 1 << 18 ) - 1  

//! Define Memory Buffer Cache Size
#define DEF_MBUF_CACHE_SIZE 512

//! Define Memory Buffer Over Head Size
#define DEF_MBUF_OVERHEAD (sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)

//! Define Memory Buffer Data Size
#define DEF_MBUF_DATA_SIZE 2048

//! Define Memory Buffer Elements Size
/*!
 * To Use in Memory Pool Create Function
 */
#define DEF_MBUF_SIZE ( DEF_MBUF_DATA_SIZE + DEF_MBUF_OVERHEAD )

//! Define Ring Info Structure
/*!
 * \struct _ring_info
 * \brief Structure for Ring Info
 * to Use in Application For to Create Ring
 */
typedef struct _ring_info
{
	char	strName[DEF_MEM_BUF_128];	//!< Name of Ring
	int		nSize;			//!< Size of Ring
} RING_INFO;

/*!
 * \class CMRTMain
 * \brief CMRT Main Class
 */
class CMRTMain
{
	public:
		//! Constructor.
		CMRTMain(char *a_strLogPath = NULL);
		//! Destructor.
		~CMRTMain();

		//! Initialize
		int Initialize();
		//! Run Process
		int Run();

	private:
		//! Log Path 
		char m_strLogPath[DEF_MEM_BUF_256];
		//! Log Path Pointer
		char *m_pLogPath;
		//! Ring For to Communicate to Application
		struct rte_ring *m_pstRingInfo;
		//! Memory Pool For using in Application 
		struct rte_mempool *m_pstMemPool;
		//! Create the Ring
		int CreateRing(char *a_strName, int a_nSize);
		//! Create Mempool
		int CreateMempool();

		
};

#endif
