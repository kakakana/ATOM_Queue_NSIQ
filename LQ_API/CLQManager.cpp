/*!
 * \file CLQManager.cpp
 * \brief CLQManager class Source File
 * \author 이현재 (presentlee@ntels.com)
 * \date 2016.03.18
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

using namespace std;

/*!
 * \class CLQManager
 * \brief CLQManager Class For LQ API
 */


//! Default Hash Function
/*!
 * \brief Default Hash Function
 * \details 다수의 Write Queue 를 쓸 경우Write 할 Queue 를 지정해야 할 경우가 생김
 * 이 때에 적절한 Hash Function 을 등록해 놓게 되면 Hash Function 의 결과값에 의하여
 * 출력 되는 Queue 의 Index 번호를 지정할 수 있음.
 * Default Hash Function 은 언제나 '0' 번의 Index 를 Return
 * \param a_pArgs is Arguments Object
 * \return 0
 */
int DefaultHashFunc(void *a_pArgs) { return 0; }


//! Constructor
/*!
 * \brief Constructor for CLQManager Class Init Variables and Memory
 * \param a_nNodeID is System Node ID
 * \param a_szProcName is Process Name
 * \param a_szLogPath is Log Path to Use in DPDK Lib Init 
 */
CLQManager::CLQManager(int a_nNodeID, char *a_szProcName, char *a_szLogPath)
{
	//Init Backup Flag
	m_bBackup = false;

	//Init Buffer
	memset(m_szBuffer, 0x00, sizeof(m_szBuffer));
	//Init Jumbo Buffer
	memset(m_szJumboBuff, 0x00, sizeof(m_szJumboBuff));

	//Init Log Path
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

	if(a_szProcName != NULL)
		snprintf(m_szProcName, sizeof(m_szProcName), "%s", a_szProcName);	

	//Init Node ID
	m_unNodeID = a_nNodeID;

	//Init Base Mempool Pointer
	m_pstMemPool = NULL;

	//Init Ring Count
	m_unReadRingCount = 0;
	m_unWriteRingCount = 0;

	//Init Ring Info
	for(int i = 0; i < DEF_MAX_RING; i++)
	{
		m_stReadRingInfo[i].pstRing		= NULL;
		m_stReadRingInfo[i].unFailCnt	= 0;
		m_stReadRingInfo[i].nIdx		= 0;
		m_stReadRingInfo[i].vecRelProc.clear();
		memset(m_stReadRingInfo[i].szName, 0x00, sizeof(m_stReadRingInfo[i].szName));

		m_stWriteRingInfo[i].pstRing	= NULL;
		m_stWriteRingInfo[i].unFailCnt	= 0;
		m_stWriteRingInfo[i].nIdx		= 0;
		m_stWriteRingInfo[i].vecRelProc.clear();
		memset(m_stWriteRingInfo[i].szName, 0x00, sizeof(m_stWriteRingInfo[i].szName));
	}

	//Init Backup File Header
	memset(&m_stBackupHead, 0x00, sizeof(BACKUP_INFO));

	//Init Read Mbuf, Write Mbuf
	memset(m_pstReadMbuf, 0x00, sizeof(m_pstReadMbuf));
	memset(m_pstWriteMbuf, 0x00, sizeof(m_pstWriteMbuf));

	//Init Read Mbuf, Write Mbuf Index
	m_unCurReadMbufIdx = 0;
	m_unTotReadMbufIdx = 0;
	m_unCurWriteMbufIdx = 0;	

	//Set to RTS Blocking
	sigemptyset(&m_stSigSet);
	sigaddset(&m_stSigSet, SIGRTMIN);	

	for(int i = SIGRTMIN+1; i < SIGRTMIN+1+DEF_MAX_RING; i++)
	{
		sigaddset(&m_stSigSet, i);	
	}
	sigprocmask(SIG_BLOCK, &m_stSigSet, NULL);

	//Init Cur Read Ring
	m_pstReadRing = NULL;

	//Init Cur Read Ring Index 
	m_unReadIdx = 0;
	
	//Init Read Complete Flag
	m_bReadComplete = false;		

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
 * \details
 * 1. 기본 Memory Pool Attach
 * 2. Load Config from DB
 * 3. Init Backup Files 
 * \return Succ 0, Fail -1
 */
int CLQManager::Initialize(p_function_hash a_pFunc)
{
	int ret = 0;
	char szQName[DEF_MEM_BUF_128];
	char *pszQuery = NULL;
	tuples_t	tTuple;

	if(strlen(m_szProcName) == 0)
	{
		RTE_LOG (ERR, EAL, "Please Insert Process Name\n");
		return -1;
	}

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
							m_stReadRingInfo[i].pstRing,
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
							m_stWriteRingInfo[i].pstRing,
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
 * \details Attach 된 Ring 에 자신의 Process 정보를 기입한다. 목적은 다음과 같다
 * 1. Producer Process 가 데이터 입력 완료 후 Signal 을 전송하기 위해
 * 2. 프로세스 비정상 종료 후 Ring Position 의 복구를 위해
 * \param a_szName is Consumers Name
 * \param a_nPID is Consumers PID
 * \param a_pstRing is Pointer of Ring
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
 * \details Attach 된 Ring 에 자신의 Process 정보를 기입한다. 목적은 다음과 같다
 * 1. 프로세스 비정상 종료 후 Ring Position 의 복구를 위해
 * \param a_szName is Producer Name
 * \param a_nPID is Producer PID
 * \param a_pstRing is Pointer of Ring
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
		case DEF_MULTI_TYPE_WRITE :
			sprintf(m_szBuffer, DEF_STR_FORMAT_Q_NAME
					, DEF_STR_MULTI_PREFIX
					, a_szRead
			);
			break;
		case DEF_MULTI_TYPE_READ :
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
 * \details 입력 된 Write Process 와 Read Process 의 이름을 가지고 Queue 를 Attach 한다
 * 만약 Attach 할 Queue 가 존재하지 않을 시 Queue 를 생성한다.
 * 또한, Queue 가 존재할 시 Queue 에 저장 된 자신의 정보를 읽어들여 프로세스의 비정상 종료를 판단한다.
 * 이후 비정상 종료로 인한 Queue(Ring) 내부에 관리되는 Index 번호를 복구 한다.
 * Index 번호를 복구하지 않을 시 Multi Queue 상황에서 비정상 종료 된 프로세스 외의 프로세스에서
 * 무한 루프가 발생할 수 있다.
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
	//Ring 의 복구를 위해서 Consumer 와 Producer 의 Tail Position 을 판단하기 위한 변수
	vector<uint32_t> vecConsTail ;
	vector<uint32_t> vecProdTail ;

	uint32_t unConsMin = 0;
	uint32_t unProdMin = 0;


	uint32_t unStartIdx = 0;
	bool	 bRestoreMode = false;	

	bool bFind = false;
	struct rte_ring *pstRing = NULL;
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
			ret =  CreateRing( pszQName, &(pstRing) );
			//If Ring is NULL to Send Creating Request
			if(pstRing != NULL)
			{
				//Insert Consumers Info to Cons Ring
				ret = InsertConsInfo(m_szProcName, getpid(), pstRing);
				if(ret < 0)
				{
					RTE_LOG (ERR, RING, "Insert Reader's Info Failed\n");
					break;
				}

				RTE_LOG(ERR, RING, "ddddddddd cons ret %d, info head, %u, tail %u, ring head, %u, tail %u \n"
									,ret
									,pstRing->cons.cons_info[ret].head
									,pstRing->cons.cons_info[ret].tail
									,pstRing->cons.head
									,pstRing->cons.tail
									);
				//만약 비정상 종료 등의 이유로 Ring 의 consumer Head 랑 Tail 의 값이 차이가 날 경우
				//Ring 의 Consumer Head 위치를 Ring 에 기억 되어 있는Consumer 정보의 Head 값으로 강제 변경
				//변경 하지 않을 경우 연관되어 있는 다른 Process 들의 무한 루프 발생
				//(어쩔 수 없이 데이터의 유실이 발생할 수 있음)
				if( 
					pstRing->cons.tail == pstRing->cons.cons_info[ret].tail
				)
				{
					vecConsTail.clear();

					for(int i = 0 ; i < pstRing->cons.cons_count; i++)
					{
						if(ret == i)
						{
							continue;
						}
					
						vecConsTail.push_back ((uint32_t)pstRing->cons.cons_info[i].tail);
					}
					
					if(vecConsTail.size() > 0)
					{
						unConsMin = vecConsTail[0];

						for(int i = 1; i < vecConsTail.size(); i++)
						{
							if(unConsMin > vecConsTail[i])
							{
								unConsMin = vecConsTail[i];
							}
						}

						if(unConsMin > pstRing->cons.cons_info[ret].tail)
						{
							pstRing->cons.cons_info[ret].restore = 1;
							pstRing->cons.cons_info[ret].start_idx = unStartIdx;
							RTE_LOG (ERR, RING, "Consumer Info Invalid Head %u, Tail %u, StartIdx %u\n",
											pstRing->cons.cons_info[ret].head,
											pstRing->cons.cons_info[ret].tail,
											unStartIdx
									);
							
						}
					}
					else
					{
						if(pstRing->cons.head != 0)
						{
							pstRing->cons.head = pstRing->cons.cons_info[ret].tail;
								RTE_LOG (ERR, RING, "Consumer Info Invalid Head %u, Tail %u, StartIdx %u\n",
												pstRing->cons.cons_info[ret].head,
												pstRing->cons.cons_info[ret].tail,
												unStartIdx
										);
						}
					}

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
				//Insert Queue Info
				else
				{
					m_stReadRingInfo[m_unReadRingCount].pstRing = pstRing;
					m_stReadRingInfo[m_unReadRingCount].nIdx = ret;
					sprintf(m_stReadRingInfo[m_unReadRingCount].szName,
							"%s", pszQName);
					m_stReadRingInfo[m_unReadRingCount].vecRelProc.push_back(a_szWrite);

					m_unReadRingCount++;
				}
			
					
				//Increase Current Ring Count
				SetSleepFlag(pstRing);
				return 0;
			}


		case DEF_RING_TYPE_WRITE :
			//Attach Ring
			CreateRing( pszQName, &(pstRing) );
			//If Ring is NULL to Send Creating Request
			if(pstRing != NULL)
			{
				//Insert Producer Info to Prod Ring
				ret = InsertProdInfo(m_szProcName, getpid(), pstRing);
				if(ret < 0)
				{
					RTE_LOG (ERR, RING, "Insert Producer's Info Failed\n");
					break;
				}

				RTE_LOG(ERR, RING, "ddddddddd prod ret %d, info head, %u, tail %u, ring head, %u, tail %u \n"
									,ret
									,pstRing->prod.prod_info[ret].head
									,pstRing->prod.prod_info[ret].tail
									,pstRing->prod.head
									,pstRing->prod.tail
									);

				//만약 비정상 종료 등의 이유로 Ring 의 Producer Head 랑 Tail 의 값이 차이가 날 경우
				//Ring 의 Producer Head 위치를 Ring 에 기억 되어 있는Producer 정보의 Tail 값으로 강제 변경
				//변경 하지 않을 경우 연관되어 있는 다른 Process 들의 무한 루프 발생
				//(어쩔 수 없이 데이터의 유실이 발생할 수 있음)
				if(	pstRing->prod.tail == pstRing->prod.prod_info[ret].tail )
				{
					for(int i = 0 ; i < pstRing->prod.prod_count; i++)
					{
						if(ret == i)
						{
							continue;
						}
					
						vecProdTail.push_back((uint32_t)pstRing->prod.prod_info[i].tail);
					}

					if(vecProdTail.size() > 0)
					{
						unProdMin = vecProdTail[0];
						for(int i = 1; i < vecProdTail.size(); i++)
						{
							if(unProdMin > vecProdTail[i])
							{
								unProdMin = vecProdTail[i];
							}
						}

						if(unProdMin > pstRing->prod.prod_info[ret].tail)
						{
							unStartIdx = pstRing->prod.prod_info[ret].tail;

							pstRing->prod.prod_info[ret].restore = 1;
							pstRing->prod.prod_info[ret].start_idx = unStartIdx;

							RTE_LOG (ERR, RING, "Producer Info Invalid Head %u, Tail %u, StartIdx %u\n",
											pstRing->prod.prod_info[ret].head,
											pstRing->prod.prod_info[ret].tail,
											unStartIdx
									);
						}
					}
					else
					{
						if(pstRing->prod.head != 0)
						{
							pstRing->prod.head = pstRing->prod.prod_info[ret].tail;
							RTE_LOG (ERR, RING, "Producer Info Invalid Head %u, Tail %u, StartIdx %u\n",
											pstRing->prod.prod_info[ret].head,
											pstRing->prod.prod_info[ret].tail,
											unStartIdx
									);
						}
					}
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
				//Insert Queue Info
				else
				{
					m_stWriteRingInfo[m_unWriteRingCount].pstRing = pstRing;
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
 * \details 모든 Ring 의 이름은 [WriteProcess]_[ReadProcess] 형태로 구성
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
 * \details 초기 Class 생성 때 Mask 값으로 준 SIGRTMIN 의 Signal 을 기다린다.
 * SIGRTMIN+1 를 받는 다는 것은 해당하는 Queue 에서 Consumer Info 를 저장하는 배열의 
 * 자신의 Index 번호가 0이라는 뜻이다.
 * 즉, SIGRTMIN+1+[index] 번호이다.
 * \return 0 : Received Data Signal
 *         1 : Received Command Signal
 *         -1 :  Fail
 */
int CLQManager::ReadWait()
{
	int ret = 0;
	int nIdx = 0;
	//Set to Timeout Sec
	struct timespec tWait;
	tWait.tv_sec = 1;
	tWait.tv_nsec = 0;

	if(m_pstReadRing)
	{
		SetSleepFlag(m_pstReadRing);
	}

	//Wait RTS
	ret = sigtimedwait(&m_stSigSet, &m_stSigInfo, &tWait);

	//No Signal
	if(ret < 0)
		return -1;

	if(m_stSigInfo.si_code == SI_QUEUE)
	{
		m_pstReadRing = (struct rte_ring *)(m_stSigInfo.si_value.sival_ptr);

		//Received Signal For Process Data
		if(m_stSigInfo.si_signo == SIGRTMIN)
		{
			return DEF_SIG_COMMAND;
		}
		//received Signal For Process Command
		else
		{
			//Ring 내부에 저장된 Consumer 의 배열에서 자신의 Index 정보를 알아야
			//프로세스 비정상 종료시 Ring 의 Index 정보를 복구 가능
			//따라서, SIGRTMIN+1 부터 이후의 Signal 넘버로 Index 를 구분한다.
			m_unReadIdx = m_stSigInfo.si_signo - (SIGRTMIN+1);

			if(m_unReadIdx >= DEF_MAX_RING)
			{
				return -1;
			}
			return DEF_SIG_DATA;
		}
	}

	return 0;
}

/*!
 * \brief Set Sleep Flag by Consumer Process
 * \details Queue 로 부터 데이터를 완전히 읽어 들인 후 Sleep 상태를 체크한다.
 * \param a_pstRing is Ring Pointer
 * \return Succ : 0
 */
int CLQManager::SetSleepFlag( struct rte_ring *a_pstRing )
{
	for(int i = 0; i < m_unReadRingCount ; i++)
	{
//		printf("m_stReadRingInfo %p, a_pstRing %p, idx %d \n", m_stReadRingInfo[i].pstRing, a_pstRing, m_stReadRingInfo[i].nIdx);
		if(m_stReadRingInfo[i].pstRing == a_pstRing)
		{
			a_pstRing->cons.cons_info[m_stReadRingInfo[i].nIdx].sleep = 1;
			break;
		}
	}
	
	return 0;
}

/*!
 * \brief Read a Data From a Ring
 * \details Queue 로 부터 한개의 데이터를 읽어들여 입력된 포인터에 데이터의 주소값을 대입
 * \param  a_pszBuff is Buffer Pointer to Store Data
 * \return Succ : Data Size 
 *         Fail : -1
 */
int CLQManager::ReadData( char **a_pszBuff )
{
	//Result
	int ret = 0;
	char *p = m_szJumboBuff;
	struct rte_mbuf *pMbuf = NULL;

	ret = rte_ring_mc_dequeue_bulk_idx(m_pstReadRing, (void**)&m_pstCurMbuf, 1, RTE_RING_QUEUE_FIXED, m_unReadIdx);

	if(ret != 0)
	{
		return -1;
	}

	pMbuf = m_pstCurMbuf;

	//데이터의 크기가 Memory Buffer 한개의 사이즈를 초과한 경우에
	//여러개의 Memory Buffer 가 Linked list 형태로 연결 되어 있기 때문에
	//하나의 Memory Buffer 에 넣어서 포인터만 넘겨 줌
	if(pMbuf->nb_segs > 1)
	{
		if( rte_pktmbuf_pkt_len(pMbuf) > DEF_MEM_JUMBO )
		{
			RTE_LOG(ERR, MBUF, "Over Jumbo Memory Buffer Size %d, data_size : %d\n"
								, DEF_MEM_JUMBO, rte_pktmbuf_pkt_len(pMbuf));
			return -1;
		}
	
		for(int i = 0; i < pMbuf->nb_segs; i++)
		{
			memcpy(p, rte_pktmbuf_mtod(pMbuf, char *), rte_pktmbuf_data_len(pMbuf)); 
			p += rte_pktmbuf_data_len(pMbuf);
			pMbuf = pMbuf->next;
		}

		*a_pszBuff = m_szJumboBuff;
	}
	else
	{
		*a_pszBuff = rte_pktmbuf_mtod( pMbuf, char * );
	}

	m_bReadComplete = true;
	return rte_pktmbuf_pkt_len(pMbuf);
}

/*!
 * \brief Read several Data from a ring
 * \details Queue 로 부터 여러개의 데이터를 읽어들인다.(Bulk Mode 에서만 사용)
 * 읽어들인 데이터는 GetNext() 함수로만 데이터를 전달 받을 수 있다.
 * \param a_nCount is count of element 
 * \return Succ : 0 
 *         Fail : -1
 */
int CLQManager::ReadBulkData( int a_nCount )
{
	//Result
	int ret = 0;
	int nCount = a_nCount;

	if(nCount > DEF_MAX_BULK)
	{
		RTE_LOG(ERR, RING, "Bulk Count Over > %d\n", DEF_MAX_BULK);
		return -1;
	}


	ret = rte_ring_mc_dequeue_bulk_idx(m_pstReadRing, (void**)m_pstReadMbuf, nCount, RTE_RING_QUEUE_FIXED, m_unReadIdx);
	
	if(ret != 0)
	{
//		SetSleepFlag(m_pstReadRing);
		return -1;
	}

	m_unTotReadMbufIdx = nCount;
	m_unCurReadMbufIdx = 0;
	return 0;
}


/*!
 * \brief Get Next Data From m_pstMbuf
 * \details Bulk Mode 로 동작할 때 ReadBulkData 함수 호출 이후
 *          GetNext() 함수를 이용하여서 데이터를 하나씩 Read
 * \param  a_pszBuff is Buffer Pointer to Store Data
 * \return Succ : Data Size
 *         Fail : -1
 */
int CLQManager::GetNext(char **a_pszBuff)
{
	char *p = m_szJumboBuff;
	struct rte_mbuf *pMbuf = NULL;

	if(m_unCurReadMbufIdx >= m_unTotReadMbufIdx)
		return -1;

	m_pstCurMbuf = pMbuf = m_pstReadMbuf[m_unCurReadMbufIdx++];

	//데이터의 크기가 Memory Buffer 한개의 사이즈를 초과한 경우에
	//여러개의 Memory Buffer 가 Linked list 형태로 연결 되어 있기 때문에
	//하나의 Memory Buffer 에 넣어서 포인터만 넘겨 줌
	if(pMbuf->nb_segs > 1)
	{
		if( rte_pktmbuf_pkt_len(pMbuf) > DEF_MEM_JUMBO )
		{
			RTE_LOG(ERR, MBUF, "Over Jumbo Memory Buffer Size %d, data_size : %d\n"
								, DEF_MEM_JUMBO, rte_pktmbuf_pkt_len(pMbuf));
			return -1;
		}
	
		for(int i = 0; i < pMbuf->nb_segs; i++)
		{
			if(pMbuf)
			{
				memcpy(p, rte_pktmbuf_mtod(pMbuf, char *), rte_pktmbuf_data_len(pMbuf)); 
				p += rte_pktmbuf_data_len(pMbuf);
				pMbuf = pMbuf->next;
			}
			else
			{
				break;
			}
		}

		*a_pszBuff = m_szJumboBuff;
	}
	else
	{
		*a_pszBuff = rte_pktmbuf_mtod( pMbuf, char * );
	}

	if(m_unCurReadMbufIdx == m_unTotReadMbufIdx)
	{
		m_bReadComplete = true;
	}

	return rte_pktmbuf_pkt_len(pMbuf);
}

/*!
 * \brief Read Complete
 * \details 데이터 처리 완료 후 호출 된다.
 *          1. 현재 읽고 있는 Ring 의 Consumer Position 을 변경한다.
 *          2. 현재 읽고 있는 Memory Buffer 를 Memory Pool 로 돌려 준다.
 *          (* Bulk Mode 에서는 ReadCompleteBulk() 함수를 이용)
 * \param None
 * \return None
 */
void CLQManager::ReadComplete()
{

	if(m_bReadComplete)
	{
		rte_ring_read_complete(m_pstReadRing
							, m_pstReadRing->cons.cons_info[m_unReadIdx].tail
							, m_pstReadRing->cons.cons_info[m_unReadIdx].head
							);
		//Read Complete 함수 호출 뒤에 consumer Info 의 Tail 값을 Head 값으로 변경
		m_pstReadRing->cons.cons_info[m_unReadIdx].tail = m_pstReadRing->cons.cons_info[m_unReadIdx].head;
		m_bReadComplete = false;
	}
}

/*!
 * \brief Free Read Data
 * \details 처리가 완전히 완료된 Memory Buffer 를해제한다.
 * \param None
 * \return None
 */
void CLQManager::FreeReadData()
{
	rte_pktmbuf_free(m_pstCurMbuf);
}

/*!
 * \brief Insert Data To m_pstWriteMbuf
 * \details 만약 현재 읽은 Memory Buffer 를 그대로 다음 Queue 에 전달하고 싶을 때 사용
 * \param None
 * \return None
 */
void CLQManager::InsertData()
{
	m_pstWriteMbuf[m_unCurWriteMbufIdx++] = m_pstCurMbuf;
	
}

/*!
 * \brief Insert Data To m_pstWriteMbuf
 * \details Bulk Mode 에서 여러개의 Data 를 Buffer 에 넣기 위해서 사용
 * \param a_pszData is Data Pointer to Insert 
 * \param a_nSize is Size of Data
 * \return 0 : Succ
 *         -1 : Fail
 */
int CLQManager::InsertData( char *a_pszData, int a_nSize )
{
	int nSegCnt = 0;
	int nLastLen = 0;
	struct rte_mbuf *pHeadMbuf = NULL, *pMbuf = NULL, *pTmpMbuf = NULL;

	if(a_nSize > DEF_MEM_BUF_1M)
	{
		RTE_LOG(ERR, MBUF, "Data Length Over 1M , [Size:%d]\n", a_nSize);
		return -1;
	}

	pHeadMbuf = rte_pktmbuf_alloc(m_pstMemPool);
	if(pHeadMbuf == NULL)
	{
//		RTE_LOG(ERR, MBUF, "Mbuf Alloc Failed\n");
		return -1;
	}

	if(a_nSize <= pHeadMbuf->buf_len)
	{

		pHeadMbuf->data_len = a_nSize;
		pHeadMbuf->nb_segs = 1;
		pHeadMbuf->pkt_len = a_nSize;

		memcpy( rte_pktmbuf_mtod(pHeadMbuf, char*), a_pszData, a_nSize);

	}
	//만약 데이터의 Size 가 Memory Buffer 의 Size 를 초과한 경우
	//여러개의 Memory Buffer 를 Linked list 형태로 연결
	else
	{
		nSegCnt = a_nSize / pHeadMbuf->buf_len;
		nLastLen = a_nSize - (pHeadMbuf->buf_len * nSegCnt);

		for(int i = 0; i < (nSegCnt+1) ; i++)
		{

			if(pTmpMbuf != NULL)
			{
				pMbuf = rte_pktmbuf_alloc(m_pstMemPool);
				if(pMbuf == NULL)
				{
					rte_pktmbuf_free(pHeadMbuf);
					RTE_LOG(ERR, MBUF, "Mbuf Alloc Failed\n");
					return -1;
				}

				pTmpMbuf->next = pMbuf;
			}
			else
			{
				pMbuf = pHeadMbuf;
			}

			if(i < nSegCnt)
			{
				pMbuf->data_len = pMbuf->buf_len; 
			}
			else
			{
				pMbuf->data_len = nLastLen;
			}

			pMbuf->nb_segs = nSegCnt + 1;
			pMbuf->pkt_len = a_nSize;

			memcpy( rte_pktmbuf_mtod(pMbuf, char*), a_pszData, pMbuf->data_len);

			pTmpMbuf = pMbuf;

		}

	}

	m_pstWriteMbuf[m_unCurWriteMbufIdx++] = pHeadMbuf;
	return 0;
}

/*!
 * \brief Enqeue Data (Index)
 * \details 현재 읽고 있는 Memory Buffer 구조체 그대로 다음 Queue 로 전달
 *          데이터의 Length 의 변경 없이 Filed 의 변경만 있을 때 사용 가능
 *          혹은 데이터의 변경이 Memory Buffer 의 최대 Length 를 넘지 않을 경우 사용 가능
 * \param a_nIdx is Index of m_stWriteRingInfo Array (Default : 0)
 * \return Succ : 0
 *         Fail : -1
 */
int CLQManager::WriteData( int a_nIdx )
{
	//Ring Pointer
	struct rte_ring *pstRing = NULL;

	//Check Idx Error
	if(a_nIdx < 0)
		return -1;
	
	if(a_nIdx > m_unWriteRingCount)
		return -1;
	
	pstRing = m_stWriteRingInfo[a_nIdx].pstRing;

	//Fail 이 일정 Count 의 배수일 때
	//현재 Ring 이 Full 이고 Consumer Process 가 Signal 을 유실하였다고 판단하여
	//다시 한번 Signal 을 전송
	if(m_stWriteRingInfo[a_nIdx].unFailCnt % DEF_MAX_FAIL_CNT)
	{
		SendRTS(pstRing);
	}

	InsertData();

	//Enqueue Data in Ring
	if( EnqueueData(
				pstRing
				,1 
				,m_stWriteRingInfo[a_nIdx].nIdx
				,m_stWriteRingInfo[a_nIdx].bPositionRestore
				,m_stWriteRingInfo[a_nIdx].unStartIdx) < 0)
	{
		m_stWriteRingInfo[a_nIdx].unFailCnt++;
		return -1;
	}


	SendRTS(pstRing);

	return 0;
}

/*!
 * \brief Enqeue Data (Hash Function)
 * \details 현재 읽고 있는 Memory Buffer 구조체 그대로 다음 Queue 로 전달
 *          데이터의 Length 의 변경 없이 Filed 의 변경만 있을 때 사용 가능
 *          혹은 데이터의 변경이 Memory Buffer 의 최대 Length 를 넘지 않을 경우 사용 가능
 *          Hash 함수를 이용하여서 Write 할 Queue 를 선택
 * \param a_pArgs is Arguments for Hash Function
 * \return Succ : 0
 *         Fail : -1
 */
int CLQManager::WriteDataHash( void *a_pArgs )
{
	//Idx of Write Ring
	int idx = m_pfuncHash(a_pArgs);
	//Ring Pointer
	struct rte_ring *pstRing = NULL;

	//Check Idx Error
	if(idx < 0)
		return -1;
	
	if(idx > m_unWriteRingCount)
		return -1;
	
	pstRing = m_stWriteRingInfo[idx].pstRing;

	//Fail 이 일정 Count 의 배수일 때
	//현재 Ring 이 Full 이고 Consumer Process 가 Signal 을 유실하였다고 판단하여
	//다시 한번 Signal 을 전송
	if(m_stWriteRingInfo[idx].unFailCnt % DEF_MAX_FAIL_CNT)
	{
		SendRTS(pstRing);
	}


	InsertData();

	//Enqueue Data in Ring
	if( EnqueueData(
				pstRing
				,1 
				,m_stWriteRingInfo[idx].nIdx
				,m_stWriteRingInfo[idx].bPositionRestore
				,m_stWriteRingInfo[idx].unStartIdx) < 0)
	{
		m_stWriteRingInfo[idx].unFailCnt++;
	}
	m_stWriteRingInfo[idx].bPositionRestore = false;

	SendRTS(pstRing);

	return 0;
}

/*!
 * \brief Enqueue Data (Index)
 * \details Enqueue Data and Send RTS to Consumer 
 *          (Write Queue 선택 기준은 Write Queue 의 Index)
 * \param a_pszData is Data Pointer to Insert 
 * \param a_nSize is Size of Data
 * \param a_nIdx is Index of m_stWriteRingInfo Array(Default : 0)
 * \return Succ : 0
 *         Fail : -1
 */
int CLQManager::WriteData( char *a_pszData, int a_nSize, int a_nIdx )
{
	//Ring Pointer
	struct rte_ring *pstRing = NULL;

	//Check Idx Error
	if(a_nIdx < 0)
		return -1;
	
	if(a_nIdx > m_unWriteRingCount)
		return -1;

	pstRing = m_stWriteRingInfo[a_nIdx].pstRing;

	//Fail 이 일정 Count 의 배수일 때
	//현재 Ring 이 Full 이고 Consumer Process 가 Signal 을 유실하였다고 판단하여
	//다시 한번 Signal 을 전송
	if(m_stWriteRingInfo[a_nIdx].unFailCnt % DEF_MAX_FAIL_CNT)
	{
		SendRTS(pstRing);
	}

	if(InsertData(a_pszData, a_nSize) < 0)
	{
		m_stWriteRingInfo[a_nIdx].unFailCnt++;
		return -1;
	}

	//Enqueue Data in Ring
	if( EnqueueData(
				pstRing
				,1 
				,m_stWriteRingInfo[a_nIdx].nIdx
				,m_stWriteRingInfo[a_nIdx].bPositionRestore
				,m_stWriteRingInfo[a_nIdx].unStartIdx) < 0)
	{
		m_stWriteRingInfo[a_nIdx].unFailCnt++;
		return -1;
	}

	
	SendRTS(pstRing);

	return 0;
}


/*!
 * \brief Enqueue Data (HashFunction)
 * \details Enqueue Data and Send RTS to Consumer 
 *          Hash 함수를 이용하여서 Write 할 Queue 를 선택
 * \param a_pszData is Data Pointer to Insert 
 * \param a_nSize is Size of Data
 * \param a_pArgs is Arguments for Hash Function
 * \return 0 : Succ
 *         -1 :  Fail
 */
int CLQManager::WriteDataHash( char *a_pszData, int a_nSize, void *a_pArgs )
{
	//Idx of Write Ring
	int idx = m_pfuncHash(a_pArgs);
	//Ring Pointer
	struct rte_ring *pstRing = NULL;

	//Check Idx Error
	if(idx < 0)
		return -1;
	
	if(idx > m_unWriteRingCount)
		return -1;

	pstRing = m_stWriteRingInfo[idx].pstRing;


	//Fail 이 일정 Count 의 배수일 때
	//현재 Ring 이 Full 이고 Consumer Process 가 Signal 을 유실하였다고 판단하여
	//다시 한번 Signal 을 전송
	if(m_stWriteRingInfo[idx].unFailCnt % DEF_MAX_FAIL_CNT)
	{
		SendRTS(pstRing);
	}

	if(InsertData(a_pszData, a_nSize) < 0)
	{
		m_stWriteRingInfo[idx].unFailCnt++;
		return -1;
	}

	//Enqueue Data in Ring
	if( EnqueueData(
				pstRing
				,1 
				,m_stWriteRingInfo[idx].nIdx
				,m_stWriteRingInfo[idx].bPositionRestore
				,m_stWriteRingInfo[idx].unStartIdx) < 0)
	{
		m_stWriteRingInfo[idx].unFailCnt++;
		return -1;
	}

	
	SendRTS(pstRing);
	
	return 0;
}

/*!
 * \brief Commit Data to Queue
 * \details m_pstWriteMbuf 에 저장된 데이터를 Queue 에 Insert
 *          Hash Function 을 이용한 Queue 의 선택 기능은 지원하지 않음
 * \param a_nIdx is Index of m_stWriteRingInfo Array 
 * \return Succ : 0
 *         Fail : -1
 */
int CLQManager::CommitData(int a_nIdx)
{
	//Ring Pointer
	struct rte_ring *pstRing = NULL;
	
	//Check Idx Error
	if(a_nIdx < 0)
		return -1;
	
	if(a_nIdx > m_unWriteRingCount)
		return -1;

	//Fail 이 일정 Count 의 배수일 때
	//현재 Ring 이 Full 이고 Consumer Process 가 Signal 을 유실하였다고 판단하여
	//다시 한번 Signal 을 전송
	if(m_stWriteRingInfo[a_nIdx].unFailCnt % DEF_MAX_FAIL_CNT)
	{
		SendRTS(pstRing);
	}

	//Select Ring by Index 
	pstRing = m_stWriteRingInfo[a_nIdx].pstRing;

	//Enqueue Data in Ring
	if( EnqueueData(
				pstRing
				,m_unCurWriteMbufIdx
				,m_stWriteRingInfo[a_nIdx].nIdx
				,m_stWriteRingInfo[a_nIdx].bPositionRestore
				,m_stWriteRingInfo[a_nIdx].unStartIdx) < 0)
	{
		m_stWriteRingInfo[a_nIdx].unFailCnt++;
		return -1;
	}


	SendRTS(pstRing);

	return 0;

}

/*!
 * \brief Send RTS To Consumer Process
 * \details Ring 에서 현재 Sleep 상태인 Consumer Process 로 RTS 전송
 * \param a_pstRing is Ring Pointer
 * \return Succ : 0
 */
int CLQManager::SendRTS(struct rte_ring *a_pstRing)
{
	//Signal Value
	union sigval sv;

	//Find Sleep Consumer Process
	for(int i = 0; i < a_pstRing->cons.cons_count ; i++)
	{
//		printf("name, %s, pid, %d, sleep, %d\n", a_pstRing->name, a_pstRing->cons.cons_info[i].pid, a_pstRing->cons.cons_info[i].sleep);
		if(a_pstRing->cons.cons_info[i].sleep)
		{
			a_pstRing->cons.cons_info[i].sleep = 0;
			sv.sival_ptr = a_pstRing;
			sigqueue(a_pstRing->cons.cons_info[i].pid, SIGRTMIN + 1 + i, sv);
			break;
		}
	}

	return 0;
}


/*!
 * \brief enqueue Data
 * \details Data 를 Ring 에 입력
 * \param a_pstRing is Ring
 * \param a_nCnt is Enqueue Count
 * \param a_nIdx is Index of the Producer
 * \param a_bRestore is Ring Restore Flag
 * \param a_nStartIdx Resotre Mode 에서의 Start Position 정보 
 * \return 0 : Succ
 *         -1 : Fail
 */
int CLQManager::EnqueueData(struct rte_ring *a_pstRing, int a_nCnt, int a_nIdx, bool a_bRestore, int a_nStartIdx)
{
	int ret = 0;

	//Enqueue Data in Ring
	ret = rte_ring_mp_enqueue_bulk_idx(
			a_pstRing, (void**)&m_pstWriteMbuf, a_nCnt, RTE_RING_QUEUE_FIXED, a_nIdx);

	//내부 데이터 저장 Buffer 의 시작 Index 를 초기화
	m_unCurWriteMbufIdx = 0;

	//Enqueue Failed
	if( ret != 0)
	{
		for(int i = 0; i < a_nCnt ; i++)
		{
			//Data Buffer Free
			rte_pktmbuf_free(m_pstWriteMbuf[i]);
		}
		return -1;
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
