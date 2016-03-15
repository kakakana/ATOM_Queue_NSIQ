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


//! Default Hash Function
/*!
 * \brief Default Hash Function
 * \details Default Hash Function To get Write Ring Idx
 * \param a_pArgs is Arguments Object
 * \return 0
 */
int DefaultHashFunc(void *a_pArgs) { return 0; }


//! Constructor
/*!
 * \brief Constructor for CLQManager Class
 * Init Variables and Memory
 * \param a_szLogPath is Log Path to Use in DPDK Lib Init 
 */
CLQManager::CLQManager(int a_nNodeID, char *a_szProcName, char *a_szLogPath)
{
	//Init Backup Flag
	m_bBackup = false;

	//Init Buffer
	memset(m_szBuffer, 0x00, sizeof(m_szBuffer));

	memset(m_szLogPath, 0x00, sizeof(m_szLogPath));
	m_pLogPath = NULL;

	if(a_szLogPath)
	{
		snprintf(m_szLogPath, DEF_MEM_BUF_256, "%s", a_szLogPath);
		m_pLogPath = m_szLogPath;
	}

	m_pfuncHash = DefaultHashFunc;

	//Init Process Name
	memset(m_szProcName, 0x00, sizeof(m_szProcName));
	snprintf(m_szProcName, sizeof(m_szProcName), "%s", a_szProcName);	

	//Init Node ID
	m_unNodeID = a_nNodeID;

	m_pstMemPool = NULL;

	//Init Ring Info
	for(int i = 0; i < DEF_MAX_RING; i++)
	{
		m_stReadRingInfo[i].pRing	= NULL;
		m_stReadRingInfo[i].nIdx	= 0;
		m_stReadRingInfo[i].vecRelProc.clear();
		memset(m_stReadRingInfo[i].szName, 0x00, sizeof(m_stReadRingInfo[i].szName));

		m_stWriteRingInfo[i].pRing	= NULL;
		m_stWriteRingInfo[i].nIdx	= 0;
		m_stWriteRingInfo[i].vecRelProc.clear();
		memset(m_stWriteRingInfo[i].szName, 0x00, sizeof(m_stWriteRingInfo[i].szName));
	}

	//Init Backup File Header
	memset(&m_stBackupHead, 0x00, sizeof(BACKUP_INFO));

	//Set to RTS Blocking
	sigemptyset(&m_stSigSet);
	sigaddset(&m_stSigSet, SIGRTMIN);	
	sigaddset(&m_stSigSet, SIGRTMIN+1);	
	sigprocmask(SIG_BLOCK, &m_stSigSet, NULL);

	
	m_pclsDbConn = NULL;
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
int CLQManager::Initialize(p_function_hash a_pFunc)
{
	int ret = 0;
	char szQName[DEF_MEM_BUF_128];
	char *pszQuery = NULL;
	tuples_t	tTuple;

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

	//Set to Hash Function For to Select Write Ring Idx
	if(a_pFunc != NULL)
		m_pfuncHash = a_pFunc;

	m_pclsDbConn = new DbQuery( &ret, "127.0.0.1", "cgw", "cgw", "ATOM");

	if(ret < 0)
		RTE_LOG(ERR, EAL, "Cannot Init Mysql DB\n");

	pszQuery = GetQuery(DEF_QUERY_INIT_READ_Q, m_unNodeID, m_szProcName);
	m_pclsDbConn->OpenSQL(pszQuery, &tTuple);
	RTE_LOG(INFO, EAL, "QUERY %s\n", pszQuery);
	
	for(int i = 0; i < tTuple.size() ; i++)
	{
		RTE_LOG(INFO, EAL, "WRITE_PROC, %s, READ_PROC, %s, ELEM_CNT, %s, BI_DIR_YN, %c\n", 
				tTuple[i]["WRITE_PROC"].c_str(),
				tTuple[i]["READ_PROC"].c_str(),
				tTuple[i]["ELEM_CNT"].c_str(),
				tTuple[i]["BI_DIR_YN"].c_str()[0]
		);	


		AttachRing( tTuple[i]["WRITE_PROC"].c_str(), tTuple[i]["READ_PROC"].c_str()
					, DEF_RING_TYPE_READ, tTuple[i]["MULTI_TYPE"].c_str()[0]);

		if(tTuple[i]["BI_DIR_YN"].c_str()[0] == 'Y')
		{
			AttachRing( tTuple[i]["READ_PROC"].c_str(), tTuple[i]["WRITE_PROC"].c_str()
						, DEF_RING_TYPE_WRITE, NULL);
		}
	}

	tTuple.clear();

	pszQuery = GetQuery(DEF_QUERY_INIT_WRITE_Q, m_unNodeID, m_szProcName);
	m_pclsDbConn->OpenSQL(pszQuery, &tTuple);
	RTE_LOG(INFO, EAL, "QUERY %s\n", pszQuery);
	
	for(int i = 0; i < tTuple.size() ; i++)
	{
		RTE_LOG(INFO, EAL, "WRITE_PROC, %s, READ_PROC, %s, ELEM_CNT, %s, BI_DIR_YN, %c\n", 
				tTuple[i]["WRITE_PROC"].c_str(),
				tTuple[i]["READ_PROC"].c_str(),
				tTuple[i]["ELEM_CNT"].c_str(),
				tTuple[i]["BI_DIR_YN"].c_str()[0]
		);	

		AttachRing( tTuple[i]["WRITE_PROC"].c_str(), tTuple[i]["READ_PROC"].c_str()
					, DEF_RING_TYPE_WRITE, tTuple[i]["MULTI_TYPE"].c_str()[0]);

		if(tTuple[i]["BI_DIR_YN"].c_str()[0] == 'Y')
		{
			AttachRing( tTuple[i]["READ_PROC"].c_str(), tTuple[i]["WRITE_PROC"].c_str()
						, DEF_RING_TYPE_READ, NULL);
		}
	}

	for(int i = 0; i < m_unReadRingCount; i++)
	{
		RTE_LOG(INFO, EAL, "Read, Ring, %p, Idx, %d, Name, %s\n",
							m_stReadRingInfo[i].pRing,
							m_stReadRingInfo[i].nIdx,
							m_stReadRingInfo[i].szName
						);
								
		for(int j = 0; j < m_stReadRingInfo[i].vecRelProc.size(); j++)
		{
			RTE_LOG(INFO, EAL, "    --------- RelProc, %s\n",
								m_stReadRingInfo[i].vecRelProc[j].c_str()
								);
		}
	}
	for(int i = 0; i < m_unWriteRingCount; i++)
	{
		RTE_LOG(INFO, EAL, "Write, Ring, %p, Idx, %d, Name, %s\n",
							m_stWriteRingInfo[i].pRing,
							m_stWriteRingInfo[i].nIdx,
							m_stWriteRingInfo[i].szName
						);
								
		for(int j = 0; j < m_stWriteRingInfo[i].vecRelProc.size(); j++)
		{
			RTE_LOG(INFO, EAL, "    --------- RelProc, %s\n",
								m_stWriteRingInfo[i].vecRelProc[j].c_str()
								);
		}

	}

	//Set to Backup File
//	if(InitBackupFile() < 0)
//		return -1;

	return 0;
}

/*!
 * \brief Insert Consumer Info
 * \details Insert Consumer Info in Reader Ring
 *          For RTS Event Handle
 * \param a_szName is Consumers Name
 * \param a_nPID is Consumers PID
 * \return Succ 0, Fail -1
 */
int CLQManager::InsertConsInfo(char *a_szName, pid_t a_stPID, struct rte_ring *a_pstRing)
{
	//Index of Consumer Array in the ring
	int idx = 0;

	int i = 0;
	//if Found consumers Info in Cons Ring then to change Status to true
	bool bFind = false;
	void *pData = NULL;
	struct rte_mbuf *m = NULL;

	//Lock to Ring
	rte_ring_rw_lock();
	
	for(i = 0 ; i < a_pstRing->cons.cons_count ; i++)	
	{
		//Found Name
		if( strncmp(a_pstRing->cons.cons_info[i].name, a_szName, strlen(a_szName) ) == 0 )
		{
			bFind = true;
			break;
		}
	}//End of while

	//Change My PID
	if(bFind)
	{
		a_pstRing->cons.cons_info[i].pid = a_stPID;
		idx = i;
	}
	//Insert Reader's INFO
	else
	{
		if(a_pstRing->cons.cons_count >= RTE_RING_MAX_CONS_COUNT )
		{
			RTE_LOG (ERR, RING, "Consumer is Full\n");
			return -1;
		}

		snprintf(a_pstRing->cons.cons_info[a_pstRing->cons.cons_count].name, RTE_RING_NAMESIZE, "%s", a_szName);
		a_pstRing->cons.cons_info[a_pstRing->cons.cons_count].pid = a_stPID;
		idx = a_pstRing->cons.cons_count;

		a_pstRing->cons.cons_count++;
	}

	//UnLock to Ring
	rte_ring_rw_unlock();


	return idx;
}

/*!
 * \brief Insert Producer Info
 * \details Insert Producer Info in Reader Ring
 *          For Backup Data of in the Ring
 * \param a_szName is Producer Name
 * \param a_nPID is Producer PID
 * \return Succ 0, Fail -1
 */
int CLQManager::InsertProdInfo(char *a_szName, pid_t a_stPID, struct rte_ring *a_pstRing)
{
	//Index of Consumer Array in the ring
	int idx = 0;

	int i = 0;
	//if Found Producer Info in the Ring then to change Status to true
	bool bFind = false;
	void *pData = NULL;
	struct rte_mbuf *m = NULL;

	//Lock to Ring
	rte_ring_rw_lock();
	
	for(i = 0 ; i < a_pstRing->prod.prod_count ; i++)	
	{
		//Found Name
		if( strncmp(a_pstRing->prod.prod_info[i].name, a_szName, strlen(a_szName) ) == 0 )
		{
			bFind = true;
			break;
		}
	}//End of while

	//Change My PID
	if(bFind)
	{
		a_pstRing->prod.prod_info[i].pid = a_stPID;
		idx = i;
	}
	//Insert Reader's INFO
	else
	{
		if(a_pstRing->prod.prod_count >= RTE_RING_MAX_PROD_COUNT )
		{
			RTE_LOG (ERR, RING, "Producer is Full\n");
			return -1;
		}

		snprintf(a_pstRing->prod.prod_info[a_pstRing->prod.prod_count].name, RTE_RING_NAMESIZE, "%s", a_szName);
		a_pstRing->prod.prod_info[a_pstRing->prod.prod_count].pid = a_stPID;
		idx = a_pstRing->prod.prod_count;

		a_pstRing->prod.prod_count++;
	}

	//UnLock to Ring
	rte_ring_rw_unlock();


	return idx;
}

/*!
 * \brief Generate Query
 * \param a_szFmt is Format of Query
 * \param ... is Aurgument For Query
 * \return Succ : A Pointer of Query
 */
char* CLQManager::GetQuery(const char *a_szFmt, ...)
{
	va_list	args;

	va_start(args, a_szFmt);
	vsprintf(m_szBuffer, a_szFmt, args);
	va_end(args);

	return m_szBuffer;
}


/*!
 * \brief Generate Q Name
 * \param a_szWrite is Name of Write Process
 * \param a_szRead is Name of Read Process
 * \param a_cMultiType is Multi Queue Type (Read:'R', Write : 'W', else : NULL)
 * \return Succ : A Pointer of Q Name
 *		   Fail : NULL
 */
char* CLQManager::GetQName(const char *a_szWrite, const char *a_szRead, char a_cMultiType)
{
	switch (a_cMultiType)
	{
		case DEF_MULTI_TYPE_READ :
			sprintf(m_szBuffer, DEF_STR_FORMAT_Q_NAME
					, DEF_STR_MULTI_PREFIX
					, a_szRead
			);
			break;
		case DEF_MULTI_TYPE_WRITE :
			sprintf(m_szBuffer, DEF_STR_FORMAT_Q_NAME
					, a_szWrite
					, DEF_STR_MULTI_PREFIX
			);
			break;
		default :
			sprintf(m_szBuffer, DEF_STR_FORMAT_Q_NAME
					, a_szWrite
					, a_szRead
			);
			break;
	}


	return m_szBuffer;
}

/*!
 * \brief Attach Ring
 * \details Attach Ring. but If Ring is NULL then Send Ring Info to MRT for To Create Ring.
 *          And If a_nType is Type of read, Attach Ring name of "[a_szName]_CONS" for To Insert Consumers PID
 * \param a_szWrite is Name of Write Process
 * \param a_szRead is Name of Read Process
 * \param a_nType is Ring Type (Read:0/ Write:1)
 * \param a_cMultiType is Multi Queue Type (Read:'R', Write :'W', else : NULL)
 * \return Succ : 0
 *         Fail : -1
 */
int CLQManager::AttachRing(const char *a_szWrite, const char *a_szRead, int a_nType, char a_cMultiType)
{
	//SendRingInfo Function Result;
	int ret = 0;
	int i = 0;
	bool bFind = false;
	struct rte_ring *pRing = NULL;
	char *pszQName = GetQName(a_szWrite, a_szRead, a_cMultiType);

	if(pszQName == NULL)
	{
		RTE_LOG (ERR, RING, "Q Name is NULL\n");
		return -1;
	}
	
	switch(a_nType)
	{
		case DEF_RING_TYPE_READ :
			//Attach Ring
			ret =  CreateRing( pszQName, &(pRing) );
			//If Ring is NULL to Send Creating Request
			if(pRing != NULL)
			{
				//Insert Consumers Info to Cons Ring
				ret = InsertConsInfo(m_szProcName, getpid(), pRing);
				if(ret < 0)
				{
					RTE_LOG (ERR, RING, "Insert Reader's Info Failed\n");
					break;
				}


				for(i = 0; i < m_unReadRingCount ; i ++)
				{
					if(strcmp(pszQName, m_stReadRingInfo[i].szName) == 0)
					{
						bFind = true;
						break;
					}
				}

				//Found Queue Name
				if(bFind)
				{
					bFind = false;

					for(int j = 0; j < m_stReadRingInfo[i].vecRelProc.size() ; j++)
					{
						if( strcmp(a_szWrite, m_stReadRingInfo[i].vecRelProc[j].c_str()) == 0 )
						{
							bFind = true;
							break;
						}
					}

					//not found Process Name in Queue
					//And Insert Process Name to Queue
					if(bFind == false)
					{
						m_stReadRingInfo[i].vecRelProc.push_back(a_szWrite);
					}
				}
				//Insert QUeue Info
				else
				{
					m_stReadRingInfo[m_unReadRingCount].pRing = pRing;
					m_stReadRingInfo[m_unReadRingCount].nIdx = ret;
					sprintf(m_stReadRingInfo[m_unReadRingCount].szName,
							"%s", pszQName);
					m_stReadRingInfo[m_unReadRingCount].vecRelProc.push_back(a_szWrite);
					m_unReadRingCount++;
				}
				
//				printf("ddddddddddd idx %d\n", ret);

				//Increase Current Ring Count

				SetSleepFlag(pRing);
				return 0;
			}


		case DEF_RING_TYPE_WRITE :
			//Attach Ring
			CreateRing( pszQName, &(pRing) );
			//If Ring is NULL to Send Creating Request
			if(pRing != NULL)
			{
				//Insert Producer Info to Prod Ring
				ret = InsertProdInfo(m_szProcName, getpid(), pRing);
				if(ret < 0)
				{
					RTE_LOG (ERR, RING, "Insert Producer's Info Failed\n");
					break;
				}

				for(i = 0; i < m_unWriteRingCount ; i ++)
				{
					if(strcmp(pszQName, m_stWriteRingInfo[i].szName) == 0)
					{
						bFind = true;
						break;
					}
				}

				//Found Queue Name
				if(bFind)
				{
					bFind = false;

					for(int j = 0; j < m_stWriteRingInfo[i].vecRelProc.size() ; j++)
					{
						if( strcmp(a_szRead, m_stWriteRingInfo[i].vecRelProc[j].c_str()) == 0 )
						{
							bFind = true;
							break;
						}
					}

					//not found Process Name in Queue
					//so, Insert Process Name to Queue
					if(bFind == false)
					{
						m_stWriteRingInfo[i].vecRelProc.push_back(a_szRead);
					}
				}
				//Insert QUeue Info
				else
				{
					m_stWriteRingInfo[m_unWriteRingCount].pRing = pRing;
					m_stWriteRingInfo[m_unWriteRingCount].nIdx = ret;
					sprintf(m_stWriteRingInfo[m_unWriteRingCount].szName,
							"%s", pszQName);
					m_stWriteRingInfo[m_unWriteRingCount].vecRelProc.push_back(a_szRead);
					m_unWriteRingCount++;
				}

				return 0;
			}

			break;
		default :
			break;
	}

	return -1;
}

/*!
 * \brief Create Ring 
 * \details First Attach Ring, and If Ring is NULL,
 *          Create Ring
 * \param a_szName is Attach Ring Name
 * \param a_stRing is Ring Pointer
 * \return Succ 0, Fail -1
 */
int CLQManager::CreateRing(char *a_szName, struct rte_ring **a_stRing)
{
	struct rte_ring *r = NULL;
	struct rte_mbuf *m = NULL;
	CREATE_REQ *p_stCreateReq= NULL;

	r = rte_ring_lookup(a_szName);
	if(r)
	{
		*a_stRing = r;
		return 0;
	}
	else
	{
		r = rte_ring_create(a_szName, DEF_DEFAULT_RING_COUNT, SOCKET_ID_ANY, 0);
		if(r)
		{
			RTE_LOG(INFO, RING, "ring (%s/%x) is %p\n", a_szName, DEF_DEFAULT_RING_COUNT, r);
			*a_stRing = r;
			return 0;
		}
		else
		{
			if(rte_errno == EEXIST)
			{
				RTE_LOG(ERR, RING, "Ring %s is exist\n", a_szName);
				return 0;
			}
			else
			{
				return -1;
			}
		}

	}
	return 0; 
}

/*!
 * \brief Wait Data
 * \details Consumer Calls this Function, Wait Data From Producer 
 * \return 0 : Received Data Signal
 *         1 : Received Command Signal
 *         -1 :  Fail
 */
int CLQManager::ReadWait( struct rte_ring **a_pRing )
{
	int ret = 0;
	//Set to Timeout Sec
	struct timespec tWait;
	tWait.tv_sec = 1;
	tWait.tv_nsec = 0;

	//Wait RTS
	ret = sigtimedwait(&m_stSigSet, &m_stSigInfo, &tWait);

	//No Signal
	if(ret < 0)
		return -1;

	if(m_stSigInfo.si_code == SI_QUEUE)
	{
		*a_pRing = (struct rte_ring *)(m_stSigInfo.si_value.sival_ptr);

		//Received Signal For Process Data
		if(m_stSigInfo.si_signo == SIGRTMIN)
		{
				return DEF_SIG_DATA;
		}
		//received Signal For Process Command
		else if(m_stSigInfo.si_signo == (SIGRTMIN+1) )
		{
				return DEF_SIG_COMMAND;
		}
	}

	return 0;
}

/*!
 * \brief Set Sleep Flag by Consumer Process
 * \details 
 * \param a_pstRing is Ring Pointer
 * \return Succ : 0
 */
int CLQManager::SetSleepFlag( struct rte_ring *a_pstRing )
{
	for(int i = 0; i < m_unReadRingCount ; i++)
	{
//		printf("m_stReadRingInfo %p, a_pstRing %p, idx %d \n", m_stReadRingInfo[i].pRing, a_pstRing, m_stReadRingInfo[i].nIdx);
		if(m_stReadRingInfo[i].pRing == a_pstRing)
		{
			a_pstRing->cons.cons_info[m_stReadRingInfo[i].nIdx].sleep = 1;
			break;
		}
	}
	
	return 0;
}

/*!
 * \brief Read a Data From a Ring
 * \details This function calls the consumer
 * \param  a_pstRing is Ring 
 *         a_pBuff is Buffer to Store Data Pointer Array
 * \return Succ : 0 
 *         Fail : -1
 */
int CLQManager::ReadData( struct rte_ring *a_pstRing, void **a_pBuff )
{
	//Result
	int ret = 0;

	ret = rte_ring_dequeue(a_pstRing, a_pBuff);

	if(ret != 0)
		return -1;
	
	return 0;
}

/*!
 * \brief Read several Data from a ring
 * \details This function calls the consumer
 * \param a_pBuff is Buffer to Store Data Pointer Array
 *        a_nCount is element count of a_pBuff Array
 * \return Succ : 0 
 *         Fail : -1
 */
int CLQManager::ReadBulkData( struct rte_ring *a_pstRing, void **a_pBuff, int a_nCount )
{
	//Result
	int ret = 0;

	ret = rte_ring_dequeue_bulk(a_pstRing, a_pBuff, a_nCount);
	
	if(ret != 0)
		return -1;

	return 0;
}


/*!
 * \brief Get Data Buffer
 * \details Get Data Buffer From Memory Pool
 * \param a_nCount is Buffer Count
 *        a_pBuff is Buffer Pointer
 * \return Buffer Count
 */
int CLQManager::GetDataBuffer(int a_nCount, void **a_pBuff)
{
	int i = 0;
	for(i = 0 ; i < a_nCount ; i++)
	{
		//Memory buffer Alloc
		a_pBuff[i] = (void*)rte_pktmbuf_alloc(m_pstMemPool);

		//Alloc Failed
		if(a_pBuff[i] == NULL)
			break;
	}

	return i;
}

/*!
 * \brief Enqueue Data
 * \details Enqueue Data and Send RTS to Consumer 
 * \param a_pstData is Data Object
 *        a_nCount is Total Count of Data
 *        a_pArgs is Arguments for Hash Function
 * \return 0 : Succ
 *         -1 :  Fail
 */
int CLQManager::WriteData( void **a_pstData, int a_nCount, void *a_pArgs )
{
	int i = 0;
	//Data Buffer For Store Reader's PID
	void *pData = NULL;
	//Memory Buffer in rte Ring
	struct rte_mbuf *m = NULL;
	//Signal Value
	union sigval sv;
	//Idx of Write Ring
	int idx = m_pfuncHash(a_pArgs);
	//Ring Pointer
	struct rte_ring *pRing = NULL;

	//Check Idx Error
	if(idx < 0)
		return -1;
	
	if(idx > m_unWriteRingCount)
		return -1;

	pRing = m_stWriteRingInfo[idx].pRing;

	//Enqueue Data in Ring
	if(rte_ring_enqueue_bulk( pRing, a_pstData, a_nCount) != 0)
	{
		//Enqueue Failed
		for(i = 0; i < a_nCount; i++)
			rte_pktmbuf_free((struct rte_mbuf*)a_pstData[i]);

		return -1;
	}

	for(i = 0; i < pRing->cons.cons_count ; i++)
	{
//		printf("name, %s, pid, %d, sleep, %d\n", pRing->name, pRing->cons.cons_info[i].pid, pRing->cons.cons_info[i].sleep);
		if(pRing->cons.cons_info[i].sleep)
		{
			pRing->cons.cons_info[i].sleep = 0;
			sv.sival_ptr = pRing;
			sigqueue(pRing->cons.cons_info[i].pid, SIGRTMIN, sv);
		}

	}

	return 0;
}

#if 0
/*!
 * \brief Backup Data
 * \details Backup Enqueue Data in File
 * \param a_fp is Pointer of File
 * \param a_pstData is Data Object 
 * \param a_unCount is Total Count of Enqueue Data
 * \return 0 : Succ
 *         -1 : Fail
 */
int CLQManager::BackupWriteData( struct rte_ring *a_pstRing, FILE *a_fp, uint32_t a_unCount, void **a_pstData )
{
	uint32_t seek_position = 0;
	uint32_t startIdx = (a_pstRing->prod.tail - a_unCount) & DEF_MASK;

	seek_position = startIdx * sizeof(TEST_DATA);

	fseek(a_fp, 0, SEEK_SET);
	fwrite(&m_stHeader, sizeof(TEST_HEADER), 1, a_fp);
	fseek(a_fp, seek_position, SEEK_CUR);

	for(int i = 0; i < a_unCount; i++)
	{
		if( (startIdx & DEF_MASK) == 0 )
		{
			fseek(a_fp, sizeof(TEST_HEADER), SEEK_SET);
		}

		fwrite(&g_stData, sizeof(TEST_DATA), 1, a_fp);

		startIdx++;
		endIdx++;
	}

	m_stHeader.start_position = startIdx;
	g_stHeader.end_position = endIdx;

	int ret = fseek(a_fp, 0, SEEK_SET);
	fwrite(&m_stHeader, sizeof(TEST_HEADER), 1, a_fp);

	fflush(a_fp);

	return 0;
}
#endif
