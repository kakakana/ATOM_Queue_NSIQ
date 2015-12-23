/*!
 * \file CLQManager.hpp
 * \brief CLQManager Class Header File
 */

#ifndef _LQ_MGR_H_
#define _LQ_MGR_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//! Define Memory Buffer 128 byte
#define DEF_MEM_BUF_128		128

//! Define Memory Buffer 256 byte
#define DEF_MEM_BUF_256		256

//! Define Max Attach Ring Count
#define DEF_MAX_RING	10

//! Define Default Ring Element Count
#define DEF_DEFAULT_RING_COUNT	0x40000

//! Define Ring Name
/*!
 * Ring Name For to Communicate to Application
 */
#define DEF_RING_INFO_NAME  "RING_INFO" 

//! Define Memory Pool Name
/*!
 * Memory Pool Name For to Use in Application
 */
#define DEF_BASE_MEMORY_POOL_NAME    "BASE_MEMORY_POOL"

//! Define timeout Count
#define DEF_TIME_OUT_COUNT	20

//! Define usleep Time
#define DEF_USLEEP_TIME	 300000

//! Define Write Ring Type 
#define DEF_RING_TYPE_READ	0

//! Define Read Ring Type
#define DEF_RING_TYPE_WRITE 1

//! Define Ring Info Structure
/*!
 * \struct _ring_info
 * \brief Structure for Ring Info
 * to Use in Application For to Create Ring
 */
typedef struct _ring_info
{
	char    strName[DEF_MEM_BUF_128];   //!< Name of Ring
	int     nSize;          //!< Size of Ring
} RING_INFO;


/*!
 * \class CLQManager
 * \brief CLQManager Class For LQ API
 */
class CLQManager
{
	public:
		//! Constructor
		CLQManager(char *a_strLogPath=NULL);
		//! Destructor
		~CLQManager();
		//! Initialize
		int Initialize();
		//! Attach Ring
		int AttachRing(char *a_strName, int a_nType);	
	private:
		//! Log Path
		char m_strLogPath[DEF_MEM_BUF_256];
		//! Log Path Pointer
		char *m_pLogPath;
		//! Read Ring Current Count
		uint8_t	m_unReadRingCount;
		//! Write Ring Curret Count
		uint8_t m_unWriteRingCount;
		//! Read Ring
		struct rte_ring *m_pstReadRing[DEF_MAX_RING];
		//! Write Ring
		struct rte_ring *m_pstWriteRing[DEF_MAX_RING];

		//! Mempool For To Use to Communicate with MRT
		struct rte_mempool *m_pstMemPool;
		//! Ring For To Use to Communicate with MRT
		struct rte_ring *m_pstRingInfo;


		//! Send Ring Info to MRT
		int SendRingInfo(char *a_strName, struct rte_ring **a_stRing);	
};

#endif
