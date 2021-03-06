/*!
 * \file CMRTMain.cpp
 * \brief MRT Main Class Source File
 */

// Include Standard Header
#include "CMRTMain.hpp"
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

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
 * \class CMRTMain
 * \brief CMRT Main Class
 */


//! Constructor
/*!
 * \brief Constructor For CMRTMain Class
 * \details Init Variables 
 * \param a_strLogPath is Log file path if NULL, default stdout
 */
CMRTMain::CMRTMain(char *a_strLogPath)
{
	m_pLogPath = NULL;

	//Copy Log path
	memset(m_strLogPath, 0x00, sizeof(m_strLogPath));

	//a_strLogPath Default is NULL
	if(a_strLogPath)
	{
		snprintf(m_strLogPath, sizeof(m_strLogPath), "%s",  a_strLogPath);
		m_pLogPath = m_strLogPath;
		
	}

	//Init Variables
	m_pstRingInfo = NULL;
	m_pstMemPool = NULL;

}

//! Destructor
/*!
 * \brief Destructor For CMRTMain Class
 * \details Init Variables and Delete Memory
 */
CMRTMain::~CMRTMain()
{


}


//! Initialize
/*!
 * \brief Init Variables and DPDK Library
 * \return Succ 0, Fail -1
 */
int CMRTMain::Initialize()
{
	int ret = 0;
	
	//Set Process Type (primary)
	rte_eal_set_proc_type(RTE_PROC_PRIMARY);
	//Init to DPDK Library	
	ret = rte_eal_init(m_pLogPath);

	//Failed
	if(ret < 0)
	{
		RTE_LOG (ERR, EAL, "Cannot init MRT\n");
		return -1;
	}

	//Ring Create For To Communicate to Client
	/* [Parameters]
	 * name, Ring Count, Memory Socket ID, Flags
	 */
	m_pstRingInfo = rte_ring_create((char*)DEF_RING_INFO_NAME, DEF_RING_SIZE, SOCKET_ID_ANY, 0);	
	if(m_pstRingInfo == NULL)
	{
		RTE_LOG (ERR, RING, "Cannot Create ring\n");
		return -1;
	}

	/* [Parameters]
	 * name, elem Count, elem Size, cache Size, private Data Size,
	 * Memory Pool Init Func, Memory Pool Init func Argument
	 * element Init Func, element Init Func Argument
	 * Memory Socket ID, Flags
	 */
	m_pstMemPool = rte_mempool_create(DEF_BASE_MEMORY_POOL_NAME, DEF_MBUF_COUNT,
										DEF_MBUF_SIZE, DEF_MBUF_CACHE_SIZE,
										sizeof(struct rte_pktmbuf_pool_private), rte_pktmbuf_pool_init,
										NULL, rte_pktmbuf_init, NULL, SOCKET_ID_ANY, 0); 
	if(m_pstMemPool == NULL)
	{
		RTE_LOG (ERR, MEMPOOL, "Cannot Create Memory Pool\n");
		return -1;
	}

	return 0;
}

//! Create the Ring
/*! 
 * \brief Create the Ring for enqeueing elements
 * \param a_strName is name for ring
 * \param a_Size is count of Ring Elements
 * \return Succ 0, Fail -1
 */
int CMRTMain::CreateRing(char *a_strName, int a_nSize)
{
	struct rte_ring *tmpRing = NULL;

	//Call to Ring Create Function in DPDK Lib
	tmpRing = rte_ring_create(a_strName, a_nSize, SOCKET_ID_ANY, 0);

	if(tmpRing == NULL)
	{
		//Ring exist
		if(rte_errno == EEXIST)
		{
			RTE_LOG(ERR, RING, "Ring %s is exist\n", a_strName);
			return 0;
		}

		return -1;
	}


	RTE_LOG(INFO, RING, "ring (%s/%x) is %p\n", a_strName, a_nSize, tmpRing);

	return 0;
}


//! Create Mempool 
/*!
 * \brief Create Mempool For Ring info to use in Application
 * \return Succ 0, Fail -1
 */
int CMRTMain::CreateMempool()
{


	return 0;
}

//! Run Process
/*!
 * \brief Run Process for CMRT Process
 * \return Succ 0, Fail -1
 */
int CMRTMain::Run()
{
	//Message Object
	void *pMsgs = NULL;
	//Memory Buffer Pointer in Ring
	struct rte_mbuf *m = NULL;
	//Ring Info Pointer
	RING_INFO *pRingInfo = NULL;

	while(1)
	{
		while(unlikely(rte_ring_dequeue(m_pstRingInfo, &pMsgs) != 0))
		{
			usleep(3000);
		}

		//Assign Memory Buffer
		m = (struct rte_mbuf*)pMsgs;

		//Move to Data Position in Memory Buffer
		pRingInfo = rte_pktmbuf_mtod(m, RING_INFO*);

		if( CreateRing(pRingInfo->strName, pRingInfo->nSize) < 0 )
		{
			RTE_LOG(ERR, RING, "Create (%s) Ring Failed\n", pRingInfo->strName);
			return -1;
		}
	}

	return 0;
}

//! Main Function
/*!
 * \brief main Function For MRT Process
 * \param argc is Arguments count
 * \param args is String Array of Arguments
 * \return Succ 0, Fail -1
 */
int main(int argc, char *args[])
{
	CMRTMain *m_pclsCMRT = NULL;

	m_pclsCMRT = new CMRTMain();

	//Failed
	if(m_pclsCMRT == NULL)
		return -1;


	m_pclsCMRT->Initialize();
	m_pclsCMRT->Run();

	delete m_pclsCMRT;

	return 0;
}

