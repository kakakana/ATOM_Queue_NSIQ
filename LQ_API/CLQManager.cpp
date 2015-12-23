/*!
 * \file CLQManager.cpp
 * \brief CLQManager class Source File
 */

#include "CLQManager.hpp"
#include <string.h>
#include <unistd.h>

// Include Definition For DPDK Envirionment Variables and Functions
#include <rte_config.h>
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_launch.h>
#include <rte_tailq.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_string_fns.h>
#include <rte_malloc.h>
#include <rte_ring.h>
#include <rte_mbuf.h>
#include <rte_errno.h>

/*!
 * \class CLQManager
 * \brief CLQManager Class For LQ API
 */


//! Constructor
/*!
 * \brief Constructor for CLQManager Class
 * Init Variables and Memory
 * \param a_strLogPath is Log Path to Use in DPDK Lib Init 
 */
CLQManager::CLQManager(char *a_strLogPath)
{
	//Init Variables
	memset(m_strLogPath, 0x00, sizeof(m_strLogPath));
	m_pLogPath = NULL;

	if(a_strLogPath)
	{
		snprintf(m_strLogPath, DEF_MEM_BUF_256, "%s", a_strLogPath);
		m_pLogPath = m_strLogPath;
	}

	m_pstMemPool = NULL;

}


//! Destructor
/*!
 * \brief Destructor for CLQManager Class
 * Init Variables and Delete Memory and Objects
 */
CLQManager::~CLQManager()
{



}

//! Initialize
/*!
 * \brief Initialize Variable, DPDK Lib
 * \return Succ 0, Fail -1
 */
int CLQManager::Initialize()
{
	int ret = 0;

	//Set Process Type (secondary)
	rte_eal_set_proc_type(RTE_PROC_SECONDARY);
	//Init to DPDK Library
	ret = rte_eal_init(m_pLogPath);

	//Failed
	if(ret < 0)
	{
		RTE_LOG (ERR, EAL, "Cannot Init LQ Manager\n");
		return -1;
	}

	//Attach Mempool, Using For to Communicate with MRT
	m_pstMemPool = rte_mempool_lookup(DEF_BASE_MEMORY_POOL_NAME);
	if(m_pstMemPool == NULL)
	{
		RTE_LOG (ERR, MEMPOOL, "Cannot Attach Base Memory Pool\n");
		return -1;
	}

	//Attach Ring, Using For to Communicate with MRT
	m_pstRingInfo = rte_ring_lookup(DEF_RING_INFO_NAME);
	if(m_pstRingInfo == NULL)
	{
		RTE_LOG (ERR, RING, "Cannot Attach Ring Info\n");
		return -1;
	}
	return 0;
}

/*!
 * \brief Attach Ring
 * \details Attach Ring If Ring is NULL, 
 *          Send Ring Info to MRT
 * \param a_strName is Attach Ring Name
 * \param a_nType is Ring Type (Read:0/ Write:1)
 * \return Succ : Ring Array Index
 *         Fail : -1
 */
int CLQManager::AttachRing(char *a_strName, int a_nType)
{
	//temporary variable For to store Current Ring Count
	int nTmpRingCount = 0;

	switch(a_nType)
	{
		case DEF_RING_TYPE_READ :
			nTmpRingCount = m_unReadRingCount;
			//Attach Ring
			SendRingInfo( a_strName, &(m_pstReadRing[m_unReadRingCount]) );
			//If Ring is NULL to Send Creating Request
			if(m_pstReadRing[m_unReadRingCount] != NULL)
			{
				//Increase Current Ring Count
				m_unReadRingCount++;
				return nTmpRingCount;
			}
		case DEF_RING_TYPE_WRITE :
			nTmpRingCount = m_unWriteRingCount;
			//Attach Ring
			SendRingInfo( a_strName, &(m_pstWriteRing[m_unWriteRingCount]) );
			//If Ring is NULL to Send Creating Request
			if(m_pstWriteRing[m_unWriteRingCount] != NULL)
			{
				//Increase Current Ring Count
				m_unWriteRingCount++;
				return nTmpRingCount;
			}
			break;
		default :
			break;
	}

	return -1;
}

/*!
 * \brief Send Ring Info to MRT
 * \details First Attach Ring, and If Ring is NULL,
 *          Send Ring Info to MRT
 * \param a_strName is Attach Ring Name
 * \param a_stRing is Ring Pointer
 * \return Succ 0, Fail -1
 */
int CLQManager::SendRingInfo(char *a_strName, struct rte_ring **a_stRing)
{
	struct rte_mbuf *m = NULL;
	RING_INFO	*p_stInfo= NULL;

	*a_stRing = rte_ring_lookup(a_strName);
	if( *a_stRing )
		return 0;

	m = rte_pktmbuf_alloc(m_pstMemPool);
	if(m == NULL)
	{
		RTE_LOG (ERR, MEMPOOL, "Cannot Alloc Mbuf\n");
		return -1;
	}

	//Move to Data Filed
	p_stInfo = rte_pktmbuf_mtod(m, RING_INFO*);

	//Set to Ring Info
	memset(p_stInfo, 0x00, sizeof(RING_INFO));
	
	snprintf(p_stInfo->strName, DEF_MEM_BUF_128, "%s", a_strName);		
	p_stInfo->nSize = DEF_DEFAULT_RING_COUNT;

	//Enqueue Request Msg
	if( rte_ring_enqueue(m_pstRingInfo, (void*)m) != 0)
	{
		RTE_LOG (ERR, RING, "rte_ring_enqueue failed in Attach Read Ring Function\n");
		return -1;
	}

	int nCnt = 0;
	while(1)
	{
		if( nCnt >= DEF_TIME_OUT_COUNT )
		{
			RTE_LOG (ERR, RING, "Ring Create Function Timeout\n");
			return -1;
		}

		//Waiting Ring
		*a_stRing = rte_ring_lookup(a_strName);

		//Created Ring
		if( *a_stRing )
			break;

		usleep(DEF_USLEEP_TIME);						
		nCnt++;
	}

	return 0; 
}


