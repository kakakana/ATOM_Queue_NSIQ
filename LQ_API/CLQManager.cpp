/*!
 * \file CLQManager.cpp
 * \brief CLQManager class Source File
 * \author 이현재 (presentlee@ntels.com)
 * \date 2016.03.18
 */

#include "CLQManager.hpp"
#include <string.h>
#include <unistd.h>


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
 * \param a_szPkgName is Pakage Name
 * \param a_szNodeType is Node Type
 * \param a_szProcName is Process Name
 * \param a_nInstanceID is Instance ID of Process
 * \param a_bBackup is Backup Flag (true : Backup Execute)
 * \param a_bMSync is MSync Function Flag (true : MSync Execute)
 * \param a_szLogPath is Log Path to Use in DPDK Lib Init 
 */
CLQManager::CLQManager(char *a_szPkgName, char *a_szNodeType, char *a_szProcName, int a_nInstanceID, bool a_bBackup, bool a_bMSync, char *a_szLogPath)
{
	//Init Backup Flag
	m_bBackup = a_bBackup;
	//Init MMap Sync Flag
	m_bMsync = a_bMSync;

	//Init Buffer
	memset(m_szBuffer, 0x00, sizeof(m_szBuffer));
	//Init Jumbo Buffer
	memset(m_szJumboBuff, 0x00, sizeof(m_szJumboBuff));
	//Init Log Path
	memset(m_szLogPath, 0x00, sizeof(m_szLogPath));
	//Init Log Path
	memset(m_szPkgName, 0x00, sizeof(m_szPkgName));
	//Init Log Path
	memset(m_szNodeType, 0x00, sizeof(m_szNodeType));
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

	if(a_szPkgName != NULL)
		snprintf(m_szPkgName, sizeof(m_szPkgName), "%s", a_szPkgName);	

	if(a_szNodeType != NULL)
		snprintf(m_szNodeType, sizeof(m_szNodeType), "%s", a_szNodeType);	

	//Init Instance ID
	m_nInstanceID = a_nInstanceID;

	//Init Base Mempool Pointer
	m_pstDataMemPool = NULL;

	//Init Command Ring
	m_pstCmdSndRing = NULL;
	m_pstCmdRcvRing = NULL;

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
	
	m_pclsConfig = NULL;
}


//! Destructor
/*!
 * \brief Destructor for CLQManager Class
 * Init Variables and Delete Memory and Objects
 */
CLQManager::~CLQManager()
{	
	for(int i = 0; i < m_unReadRingCount ; i++)
	{
		if( m_stReadRingInfo[i].pBackup != MAP_FAILED && m_stReadRingInfo[i].pBackup != NULL )
		{
			munmap(m_stReadRingInfo[i].pBackup, m_stReadRingInfo[i].unMMapSize);
		}

		if( m_stReadRingInfo[i].fd > 0 )
			close(m_stReadRingInfo[i].fd);
	}

	for(int i = 0; i < m_unWriteRingCount ; i++)
	{
		if( m_stWriteRingInfo[i].pBackup != MAP_FAILED && m_stWriteRingInfo[i].pBackup != NULL )
		{
			munmap(m_stWriteRingInfo[i].pBackup, m_stWriteRingInfo[i].unMMapSize);
		}

		if( m_stWriteRingInfo[i].fd > 0 )
			close(m_stWriteRingInfo[i].fd);
	}

	if(m_pclsConfig)
		delete m_pclsConfig;
}

//! Initialize
/*!
 * \brief Initialize Variable, DPDK Lib
 * \details
 * 1. 기본 Memory Pool Attach
 * 2. Load Config from DB
 * 3. Attach Read/Write Ring
 * 4. Command Ring Init
 * 5. Init Backup Files 
 * \param a_nCmdType is Command Type (Sender : 1 , Receiver : 0, Util : 3)
 * \param a_pFunc is Hash Function For Select Write Ring 
 * \return 
 *   - 0 on Success
 *   - -E_Q_INVAL Process Name 미입력; rte_eal_init 실패
 *   - -E_Q_NOENT required entry not available to return.
 *   - -E_Q_DB_FAIL DB Connection 실패
 *   - -E_Q_NO_CONFIG DB CONFIG 정보 없음, CConfig Class 생성 실패
 */
int CLQManager::Initialize(int a_nCmdType, p_function_hash a_pFunc)
{
	int ret = 0;
	char *pszQuery = NULL;


	//Set Process Type (secondary)
	rte_eal_set_proc_type(RTE_PROC_SECONDARY);
	//Init to DPDK Library
	ret = rte_eal_init(m_pLogPath, NULL);

	//Failed
	if(ret < 0)
	{
		SetErrorMsg("Cannot Init LQ Manager");
		RTE_LOG (ERR, EAL, "%s\n", GetErrorMsg());
		return -E_Q_INVAL;
	}

	//Attach Mempool, Using For Data Send
	m_pstDataMemPool = rte_mempool_lookup(DEF_BASE_MEMORY_POOL_NAME);
	if(m_pstDataMemPool == NULL)
	{
		SetErrorMsg("Cannot Attach Base Memory Pool errno : %d", rte_errno);
		RTE_LOG (ERR, MEMPOOL, "%s\n", GetErrorMsg());
		return -rte_errno;
	}

	//Attach Mempool, Using For Data Send
	m_pstCmdMemPool = rte_mempool_lookup(DEF_CMD_MEMORY_POOL_NAME);
	if(m_pstCmdMemPool == NULL)
	{
		SetErrorMsg("Cannot Attach Command Memory Pool errno : %d", rte_errno);
		RTE_LOG (ERR, MEMPOOL, "%s\n", GetErrorMsg());
		return -rte_errno;
	}

	m_pclsConfig = new CConfig();
	if(m_pclsConfig->Initialize() < 0)
	{
		SetErrorMsg("Fail to Init Config Class");
		delete m_pclsConfig;
		m_pclsConfig = NULL;
		return -E_Q_NO_CONFIG;
	}


	//Util 은 여기에서 Init 함수 종료	
	if(a_nCmdType == DEF_CMD_TYPE_UTIL)
		return 0;

	if(strlen(m_szProcName) == 0)
	{
		SetErrorMsg("Please Insert Process Name");
		RTE_LOG (ERR, EAL, "%s\n", GetErrorMsg());
		return -E_Q_INVAL;
	}

	//Set to Hash Function For to Select Write Ring Idx
	if(a_pFunc != NULL)
		m_pfuncHash = a_pFunc;

	m_pclsDbConn = new (std::nothrow) MariaDB();

	if(m_pclsDbConn == NULL)
	{
		SetErrorMsg("new operator Fail [%d:%s]", errno, strerror(errno));
		return -E_Q_DB_FAIL;
	}

	ret = m_pclsDbConn->Connect(
									m_pclsConfig->GetGlobalConfigValue("DB_HOST"), 
									atoi(m_pclsConfig->GetGlobalConfigValue("DB_PORT")), 
									m_pclsConfig->GetGlobalConfigValue("DB_USER"), 
									m_pclsConfig->GetGlobalConfigValue("DB_PASS"), 
									m_pclsConfig->GetGlobalConfigValue("DB_DATABASE")
								);

	if(ret != true)
	{
		SetErrorMsg("Cannot Init Mysql DB / host[%s], port[%s], user[%s], pw[%s], db[%s], [%d:%s]"
					,m_pclsConfig->GetGlobalConfigValue("DB_HOST") 
					,m_pclsConfig->GetGlobalConfigValue("DB_PORT") 
					,m_pclsConfig->GetGlobalConfigValue("DB_USER") 
					,m_pclsConfig->GetGlobalConfigValue("DB_PASS")
					,m_pclsConfig->GetGlobalConfigValue("DB_DATABASE")
					, m_pclsDbConn->GetError()
					, m_pclsDbConn->GetErrorMsg()
					);
		RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());

		delete m_pclsDbConn;
		return -E_Q_DB_FAIL;
	}

	pszQuery = GetQuery(DEF_QUERY_INIT_READ_Q, m_szPkgName, m_szNodeType, m_szProcName);
	ret = m_pclsDbConn->Query(pszQuery, strlen(pszQuery));
	if(ret < 0)
	{
		SetErrorMsg("Query Fail [%s] [%d:%s]", pszQuery, m_pclsDbConn->GetError(), m_pclsDbConn->GetErrorMsg());
		delete m_pclsDbConn;
		return -E_Q_DB_FAIL;
	}

	RTE_LOG(INFO, EAL, "QUERY %s\n", pszQuery);

	char szWriteProc[DEF_MEM_BUF_64];
	char szReadProc	[DEF_MEM_BUF_64];
	char szElemCnt	[DEF_MEM_BUF_64];
	char szBiDir	[DEF_MEM_BUF_64];
	char szMultiType[DEF_MEM_BUF_64];

	FetchMaria fdata;

	fdata.Clear();
	fdata.Set(szWriteProc	, sizeof(szWriteProc));	
	fdata.Set(szReadProc	, sizeof(szReadProc));	
	fdata.Set(szElemCnt		, sizeof(szElemCnt));	
	fdata.Set(szBiDir		, sizeof(szBiDir));	
	fdata.Set(szMultiType	, sizeof(szMultiType));	

	while(true)
	{
		if(fdata.Fetch(m_pclsDbConn) == false)
			break;

		RTE_LOG(INFO, EAL, "WRITE_PROC, %s, READ_PROC, %s, ELEM_CNT, %s, BI_DIR_YN, %c, Multi Type %c\n", 
				szWriteProc,
				szReadProc,
				szElemCnt,
				szBiDir[0],
				szMultiType[0]
		);	


		ret = AttachRing( szWriteProc, szReadProc, atoi(szElemCnt), DEF_RING_TYPE_READ, szMultiType[0]);
		if(ret < 0)
		{
			delete m_pclsDbConn;
			return ret;
		}

		if(szBiDir[0] == 'Y')
		{
			ret = AttachRing( szReadProc, szWriteProc, 
						atoi(szElemCnt), DEF_RING_TYPE_WRITE, 0);
			if(ret < 0)
			{
				delete m_pclsDbConn;
				return ret;
			}
		}
	}


	fdata.Clear();
	fdata.Set(szWriteProc	, sizeof(szWriteProc));	
	fdata.Set(szReadProc	, sizeof(szReadProc));	
	fdata.Set(szElemCnt		, sizeof(szElemCnt));	
	fdata.Set(szBiDir		, sizeof(szBiDir));	
	fdata.Set(szMultiType	, sizeof(szMultiType));	

	pszQuery = GetQuery(DEF_QUERY_INIT_WRITE_Q, m_szPkgName, m_szNodeType, m_szProcName);
	ret = m_pclsDbConn->Query(pszQuery, strlen(pszQuery));
	if(ret < 0)
	{
		SetErrorMsg("Query Fail [%s] [%d:%s]", pszQuery, m_pclsDbConn->GetError(), m_pclsDbConn->GetErrorMsg());
		delete m_pclsDbConn;
		return -E_Q_DB_FAIL;
	}

	RTE_LOG(INFO, EAL, "QUERY %s\n", pszQuery);
	
	while(true)
	{
		if(fdata.Fetch(m_pclsDbConn) == false)
			break;

		RTE_LOG(INFO, EAL, "WRITE_PROC, %s, READ_PROC, %s, ELEM_CNT, %s, BI_DIR_YN, %c, Multi Type %c\n", 
				szWriteProc,
				szReadProc,
				szElemCnt,
				szBiDir[0],
				szMultiType[0]
		);	

		ret = AttachRing( szWriteProc, szReadProc, atoi(szElemCnt), DEF_RING_TYPE_WRITE, szMultiType[0]);
		if(ret < 0)
		{
			delete m_pclsDbConn;
			return ret;
		}

		if(szBiDir[0] == 'Y')
		{
			ret = AttachRing( szReadProc, szWriteProc, atoi(szElemCnt), DEF_RING_TYPE_READ, 0);
			if(ret < 0)
			{
				delete m_pclsDbConn;
				return ret;
			}
		}
	}

	for(int i = 0; i < m_unReadRingCount; i++)
	{
		RTE_LOG(INFO, EAL, "Read, Ring, %p, Idx, %d, Name, %s\n",
							m_stReadRingInfo[i].pstRing,
							m_stReadRingInfo[i].nIdx,
							m_stReadRingInfo[i].szName
						);
								
		for(unsigned int j = 0; j < m_stReadRingInfo[i].vecRelProc.size(); j++)
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
								
		for(unsigned int j = 0; j < m_stWriteRingInfo[i].vecRelProc.size(); j++)
		{
			RTE_LOG(INFO, EAL, "    --------- RelProc, %s\n",
								m_stWriteRingInfo[i].vecRelProc[j].c_str()
								);
		}

	}

	if( ((m_unReadRingCount + m_unWriteRingCount) == 0) && (a_nCmdType != DEF_CMD_TYPE_SEND) )
	{
		SetErrorMsg("Not Found Queue Info for %s Process ", m_szProcName);
		RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
		delete m_pclsDbConn;
		return -E_Q_NO_CONFIG;
	}

	ret = InitCommandRing(m_szProcName, m_nInstanceID, a_nCmdType);
	if(ret < 0)
	{
		delete m_pclsDbConn;
		return ret;
	}
	
	delete m_pclsDbConn;
	return 0;
}

/*!
 * \brief Insert Consumer Info
 * \details Attach 된 Ring 에 자신의 Process 정보를 기입한다. 목적은 다음과 같다
 * 1. Producer Process 가 데이터 입력 완료 후 Signal 을 전송하기 위해
 * 2. 프로세스 비정상 종료 후 Ring Position 의 복구를 위해
 * \param a_szName is Consumers Name
 * \param a_nInstanceID is Instance ID of Consumer
 * \param a_nPID is Consumers PID
 * \param a_pstRing is Pointer of Ring
 * \return 
 *   - Index of The Process in the Ring
 *   - -E_Q_NOMEM Consumer Count Over 
 */
uint32_t CLQManager::InsertConsInfo(char *a_szName, int a_nInstanceID, pid_t a_stPID, struct rte_ring *a_pstRing)
{
	//Index of Consumer Array in the ring
	uint32_t idx = 0;

	uint32_t i = 0;
	//if Found consumers Info in Cons Ring then to change Status to true
	bool bFind = false;

	char szName[128];

	memset(szName, 0x00, sizeof(szName));
	sprintf(szName, "%s_%d", a_szName, a_nInstanceID);

	//Lock to Ring
	rte_ring_rw_lock();
	
	for(i = 0 ; i < a_pstRing->cons.cons_count ; i++)	
	{
		//Found Name
		if( strncmp(a_pstRing->cons.cons_info[i].name, szName, strlen(szName) ) == 0 )
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
			SetErrorMsg("Consumer is Full");
			RTE_LOG (ERR, RING, "%s\n", GetErrorMsg());
			return -E_Q_NOMEM;
		}

		snprintf(a_pstRing->cons.cons_info[a_pstRing->cons.cons_count].name, RTE_RING_NAMESIZE, "%s", szName);
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
 * \param a_nInstanceID is Instance ID of Producer
 * \param a_nPID is Producer PID
 * \param a_pstRing is Pointer of Ring
 *   - Index of The Process in the Ring
 *   - -E_Q_NOMEM Consumer Count Over 
 */
uint32_t CLQManager::InsertProdInfo(char *a_szName, int a_nInstanceID, pid_t a_stPID, struct rte_ring *a_pstRing)
{
	//Index of Consumer Array in the ring
	uint32_t idx = 0;

	uint32_t i = 0;
	//if Found Producer Info in the Ring then to change Status to true
	bool bFind = false;

	char szName[128];

	memset(szName, 0x00, sizeof(szName));
	sprintf(szName, "%s_%d", a_szName, a_nInstanceID);

	//Lock to Ring
	rte_ring_rw_lock();
	
	for(i = 0 ; i < a_pstRing->prod.prod_count ; i++)	
	{
		//Found Name
		if( strncmp(a_pstRing->prod.prod_info[i].name, szName, strlen(szName) ) == 0 )
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
			SetErrorMsg("Producer is Full");
			RTE_LOG (ERR, RING, "%s\n", GetErrorMsg());
			return -E_Q_NOMEM;
		}

		snprintf(a_pstRing->prod.prod_info[a_pstRing->prod.prod_count].name, RTE_RING_NAMESIZE, "%s", szName);
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
 * \return 
 *   - A Pointer of Query on Success
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
 * \return 
 *   - A Pointer of Q Name on Success
 *   - NULL on error
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
 * \param a_nElemCnt is Count of Ring Elements
 * \param a_nType is Ring Type (Read:0/ Write:1)
 * \param a_cMultiType is Multi Queue Type (Read:'R', Write :'W', else : NULL)
 * \return 
 *   - 0 on Success
 *   - -E_Q_NO_CONFIG- function could not get pointer to rte_config structure
 *   - -E_Q_SECONDARY - function was called from a secondary process instance
 *   - -E_Q_NOSPC - the maximum number of memzones has already been allocated
 *   - -E_Q_EXIST - a memzone with the same name already exists
 *   - -E_Q_INVAL - Ring count provided is not a power of 2 ;  Process Name Error
 *   - -E_Q_NOMEM - no appropriate memory area found in which to create memzone ; Consumer or Producer Proc Count Over
 *   - -E_Q_NOENT Backup File Open Error 
 *   - -E_Q_MMAP Backup File MMap Function Error 
 *   - -E_Q_TRUN Backup File Truncate Error 
 *   - -E_Q_FLOCK Backup File Locking Error 
 */
int CLQManager::AttachRing(const char *a_szWrite, const char *a_szRead, int a_nElemCnt, int a_nType, char a_cMultiType)
{
	//result of Function
	int ret = 0;
	//Index of Process in the ring
	uint32_t unRingIdx = 0;
	int i = 0;
	//Ring 의 복구를 위해서 Consumer 와 Producer 의 Tail Position 을 판단하기 위한 변수
	vector<uint32_t> vecConsTail ;
	vector<uint32_t> vecProdTail ;

	uint32_t unConsMin = 0;
	uint32_t unProdMin = 0;


	uint32_t unStartIdx = 0;

	struct rte_ring *pstRing = NULL;
	char *pszQName = GetQName(a_szWrite, a_szRead, a_cMultiType);

	bool bFind = false;

	if(pszQName == NULL)
	{
		SetErrorMsg("Q Name is NULL");
		RTE_LOG (ERR, RING, "%s\n", GetErrorMsg());
		return -E_Q_INVAL;
	}
	
	switch(a_nType)
	{
		case DEF_RING_TYPE_READ :
			//Attach Ring
			ret =  CreateRing( pszQName, &(pstRing) );
			if(ret< 0)
			{
				return ret;
			}
				
			//If Ring is NULL to Send Creating Request
			if(pstRing != NULL)
			{
				//Insert Consumers Info to Cons Ring
				unRingIdx = InsertConsInfo(m_szProcName, m_nInstanceID, getpid(), pstRing);
				if(unRingIdx < 0)
				{
					return unRingIdx;
				}

				RTE_LOG(ERR, RING, "ddddddddd cons RingIdx %d, info head, %u, tail %u, ring head, %u, tail %u \n"
									,unRingIdx
									,pstRing->cons.cons_info[unRingIdx].head
									,pstRing->cons.cons_info[unRingIdx].tail
									,pstRing->cons.head
									,pstRing->cons.tail
									);
				//만약 비정상 종료 등의 이유로 Ring 의 consumer Head 랑 Tail 의 값이 차이가 날 경우
				//Ring 의 Consumer Head 위치를 Ring 에 기억 되어 있는Consumer 정보의 Head 값으로 강제 변경
				//변경 하지 않을 경우 연관되어 있는 다른 Process 들의 무한 루프 발생
				//(어쩔 수 없이 데이터의 유실이 발생할 수 있음)
				if( 
					pstRing->cons.tail == pstRing->cons.cons_info[unRingIdx].tail
				)
				{
					vecConsTail.clear();

					for(uint32_t i = 0 ; i < pstRing->cons.cons_count; i++)
					{
						if(unRingIdx== i)
						{
							continue;
						}
					
						vecConsTail.push_back ((uint32_t)pstRing->cons.cons_info[i].tail);
					}
					
					if(vecConsTail.size() > 0)
					{
						unConsMin = vecConsTail[0];

						for(uint32_t i = 1; i < vecConsTail.size(); i++)
						{
							if(unConsMin > vecConsTail[i])
							{
								unConsMin = vecConsTail[i];
							}
						}

						if(unConsMin > pstRing->cons.cons_info[unRingIdx].tail)
						{
							pstRing->cons.cons_info[unRingIdx].restore = 1;
							pstRing->cons.cons_info[unRingIdx].start_idx = unStartIdx;
							RTE_LOG (ERR, RING, "Consumer Info Invalid Head %u, Tail %u, StartIdx %u\n",
											pstRing->cons.cons_info[unRingIdx].head,
											pstRing->cons.cons_info[unRingIdx].tail,
											unStartIdx
									);
							
						}
					}
					else
					{
						if(pstRing->cons.head != 0)
						{
							pstRing->cons.head = pstRing->cons.cons_info[unRingIdx].tail;
								RTE_LOG (ERR, RING, "Consumer Info Invalid Head %u, Tail %u\n",
												pstRing->cons.cons_info[unRingIdx].head,
												pstRing->cons.cons_info[unRingIdx].tail
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
					
					for(uint32_t j = 0; j < m_stReadRingInfo[i].vecRelProc.size() ; j++)
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
					m_stReadRingInfo[m_unReadRingCount].nIdx = unRingIdx;
					sprintf(m_stReadRingInfo[m_unReadRingCount].szName,
							"%s", pszQName);
					m_stReadRingInfo[m_unReadRingCount].vecRelProc.push_back(a_szWrite);

					
					if(m_bBackup)
					{
						ret = InitBackupFile( pstRing, pszQName, DEF_RING_TYPE_READ, unRingIdx);
						if( ret < 0)
						{
							return ret;
						}
					}
					m_unReadRingCount++;

				}
			
					
				//Increase Current Ring Count
				SetSleepFlag(pstRing);
				return 0;
			}


		case DEF_RING_TYPE_WRITE :
			//Attach Ring
			ret = CreateRing( pszQName, &(pstRing) );
			if(ret < 0)
			{
				return ret;
			}

			//If Ring is NULL to Send Creating Request
			if(pstRing != NULL)
			{
				//Insert Producer Info to Prod Ring
				unRingIdx = InsertProdInfo(m_szProcName, m_nInstanceID, getpid(), pstRing);
				if(unRingIdx < 0)
				{
					return unRingIdx;
				}

				RTE_LOG(ERR, RING, "ddddddddd prod unRingIdx %d, info head, %u, tail %u, ring head, %u, tail %u \n"
									,unRingIdx
									,pstRing->prod.prod_info[unRingIdx].head
									,pstRing->prod.prod_info[unRingIdx].tail
									,pstRing->prod.head
									,pstRing->prod.tail
									);

				//만약 비정상 종료 등의 이유로 Ring 의 Producer Head 랑 Tail 의 값이 차이가 날 경우
				//Ring 의 Producer Head 위치를 Ring 에 기억 되어 있는Producer 정보의 Tail 값으로 강제 변경
				//변경 하지 않을 경우 연관되어 있는 다른 Process 들의 무한 루프 발생
				//(어쩔 수 없이 데이터의 유실이 발생할 수 있음)
				if(	pstRing->prod.tail == pstRing->prod.prod_info[unRingIdx].tail )
				{
					for(uint32_t i = 0 ; i < pstRing->prod.prod_count; i++)
					{
						if(unRingIdx == i)
						{
							continue;
						}
					
						vecProdTail.push_back((uint32_t)pstRing->prod.prod_info[i].tail);
					}

					if(vecProdTail.size() > 0)
					{
						unProdMin = vecProdTail[0];
						for(uint32_t i = 1; i < vecProdTail.size(); i++)
						{
							if(unProdMin > vecProdTail[i])
							{
								unProdMin = vecProdTail[i];
							}
						}

						if(unProdMin > pstRing->prod.prod_info[unRingIdx].tail)
						{
							unStartIdx = pstRing->prod.prod_info[unRingIdx].tail;

							pstRing->prod.prod_info[unRingIdx].restore = 1;
							pstRing->prod.prod_info[unRingIdx].start_idx = unStartIdx;

							RTE_LOG (ERR, RING, "Producer Info Invalid Head %u, Tail %u, StartIdx %u\n",
											pstRing->prod.prod_info[unRingIdx].head,
											pstRing->prod.prod_info[unRingIdx].tail,
											unStartIdx
									);
						}
					}
					else
					{
						if(pstRing->prod.head != 0)
						{
							pstRing->prod.head = pstRing->prod.prod_info[unRingIdx].tail;
							RTE_LOG (ERR, RING, "Producer Info Invalid Head %u, Tail %u\n",
											pstRing->prod.prod_info[unRingIdx].head,
											pstRing->prod.prod_info[unRingIdx].tail
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

					for(uint32_t j = 0; j < m_stWriteRingInfo[i].vecRelProc.size() ; j++)
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
					m_stWriteRingInfo[m_unWriteRingCount].nIdx = unRingIdx;
					sprintf(m_stWriteRingInfo[m_unWriteRingCount].szName,
							"%s", pszQName);
					m_stWriteRingInfo[m_unWriteRingCount].vecRelProc.push_back(a_szRead);

					if(m_bBackup)
					{
						ret = InitBackupFile( pstRing, pszQName, DEF_RING_TYPE_WRITE, unRingIdx);
						if( ret < 0)
						{
							return ret;
						}
					}

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
 * \return 
 *   - 0 on Success
 *   - -E_Q_NO_CONFIG- function could not get pointer to rte_config structure
 *   - -E_Q_SECONDARY - function was called from a secondary process instance
 *   - -E_Q_INVAL - count provided is not a power of 2
 *   - -E_Q_NOSPC - the maximum number of memzones has already been allocated
 *   - -E_Q_EXIST - a memzone with the same name already exists
 *   - -E_Q_NOMEM - no appropriate memory area found in which to create memzone
 *
 */
int CLQManager::CreateRing(char *a_szName, struct rte_ring **a_stRing, int a_nSize)
{
	struct rte_ring *r = NULL;
	int nRingSize = a_nSize;

	r = rte_ring_lookup(a_szName);
	if(r)
	{
		*a_stRing = r;
		return E_Q_EXIST;
	}
	else
	{
		if(unlikely(nRingSize <= 0))
			nRingSize = DEF_DEFAULT_RING_COUNT; 

		r = rte_ring_create(a_szName, nRingSize, SOCKET_ID_ANY, 0);
		if(r)
		{
			SetErrorMsg("ring (%s/%x) is %p", a_szName, nRingSize, r);
			RTE_LOG(INFO, RING, "%s\n", GetErrorMsg());
			*a_stRing = r;
			return 0;
		}
		else
		{
			if(rte_errno == EEXIST)
			{
//				SetErrorMsg("Ring %s is exist", a_szName);
//				RTE_LOG(ERR, RING, "%s\n", GetErrorMsg());
				return 0;
			}
			else
			{
				SetErrorMsg("Create Ring %s/%u Failed", a_szName, a_nSize);
				RTE_LOG(INFO, RING, "%s\n", GetErrorMsg());
				return -rte_errno;
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
 * \return 
 *   - 0 on Recieved Signal 
 *   - -1 Timed out
 *   - -E_Q_NOMEM Ring Index Over
 */
int CLQManager::ReadWait()
{
	int ret = 0;
	//Set to Timeout Sec
	struct timespec tWait;
	tWait.tv_sec = 1;
	tWait.tv_nsec = 0;

	//ReadRing Init
	SetSleepFlag(m_pstReadRing);

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
				SetErrorMsg("Signal Number is Over");
				RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
				return -E_Q_NOMEM;
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
 * \return 
 *   - 0 on Success
 *   - -1 on Fail
 */
int CLQManager::SetSleepFlag( struct rte_ring *a_pstRing )
{
	if(a_pstRing == NULL)
		return -1;

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
 * \return 
 *   - size of data on Success
 *   - -E_Q_INVAL ring pointer invalid 
 *   - -E_Q_NOENT Not enough entries in the ring to dequeue; no object is dequeued.
 *   - -E_Q_NOMEM out of Memory
 */
int CLQManager::ReadData( char **a_pszBuff )
{
	//Result
	int ret = 0;
	int nSegs = 0;
	char *p = m_szJumboBuff;
	struct rte_mbuf *pMbuf = NULL;

	if(m_pstReadRing == NULL)
	{
		SetErrorMsg("m_pstReadRing is NULL");
		RTE_LOG(ERR, RING, "%s\n", GetErrorMsg());
		return -E_Q_INVAL;
	}

	ret = rte_ring_mc_dequeue_bulk_idx(m_pstReadRing, (void**)m_pstReadMbuf, 1, RTE_RING_QUEUE_FIXED, m_unReadIdx);

	if(unlikely(ret != 0))
	{
		return ret;
	}

	m_pstCurMbuf = pMbuf = m_pstReadMbuf[0];

	//데이터의 크기가 Memory Buffer 한개의 사이즈를 초과한 경우에
	//여러개의 Memory Buffer 가 Linked list 형태로 연결 되어 있기 때문에
	//하나의 Memory Buffer 에 넣어서 포인터만 넘겨 줌
	if(unlikely(pMbuf->nb_segs > 1))
	{
		nSegs = pMbuf->nb_segs;
		if( rte_pktmbuf_pkt_len(pMbuf) > DEF_MEM_JUMBO )
		{
			SetErrorMsg("Over Jumbo Memory Buffer Size %d, data_size : %d"
								, DEF_MEM_JUMBO, rte_pktmbuf_pkt_len(pMbuf));

			RTE_LOG(ERR, MBUF, "%s\n", GetErrorMsg());
			rte_pktmbuf_free(pMbuf);
			return -E_Q_NOMEM;
		}

		for(int i = 0; i < nSegs ; i++)
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

	m_bReadComplete = true;
	return rte_pktmbuf_pkt_len(m_pstCurMbuf);
}

/*!
 * \brief Read several Data from a ring
 * \details Queue 로 부터 여러개의 데이터를 읽어들인다.(Bulk Mode 에서만 사용)
 * 읽어들인 데이터는 GetNext() 함수로만 데이터를 전달 받을 수 있다.
 * \param a_nCount is count of element 
 * \return 
 *   - 0 on Success
 *   - -E_Q_INVAL bulk Count > Max Bulk Count; ring pointer invalid 
 *   - -E_Q_NOENT Not enough entries in the ring to dequeue; no object is dequeued.
 *   - -E_Q_NOMEM out of Memory
 *         Fail : -1
 */
int CLQManager::ReadBulkData( int a_nCount )
{
	//Result
	int ret = 0;
	int nCount = a_nCount;

	if(unlikely(nCount > DEF_MAX_BULK))
	{
		SetErrorMsg("Bulk Count Over > %d", DEF_MAX_BULK);
		RTE_LOG(ERR, RING, "%s\n", GetErrorMsg());
		return -E_Q_INVAL;
	}

	if(unlikely(m_pstReadRing == NULL))
	{
		SetErrorMsg("m_pstReadRing is NULL");
		RTE_LOG(ERR, RING, "%s\n", GetErrorMsg());
		return -E_Q_INVAL;
	}


	ret = rte_ring_mc_dequeue_bulk_idx(m_pstReadRing, (void**)m_pstReadMbuf, nCount, RTE_RING_QUEUE_FIXED, m_unReadIdx);
	
	if(unlikely(ret != 0))
	{
		m_unTotReadMbufIdx = 0;
		m_unCurReadMbufIdx = 0;
//		SetSleepFlag(m_pstReadRing);
		return ret;
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
 * \return 
 *   - Size of Data on Success
 *   - -E_Q_INVAL Read buffer Index Over;
 *   - -E_Q_NOMEM out of Memory;
 */
int CLQManager::GetNext(char **a_pszBuff)
{
	char *p = m_szJumboBuff;
	struct rte_mbuf *pMbuf = NULL;
	int nSegs = 0;

	if(unlikely(m_unCurReadMbufIdx >= m_unTotReadMbufIdx))
		return -E_Q_INVAL;

	m_pstCurMbuf = pMbuf = m_pstReadMbuf[m_unCurReadMbufIdx++];
//	RTE_LOG(ERR, MBUF, "pMbuf %p, m_pstCurMbuf, %p\n", pMbuf, m_pstCurMbuf);

	//데이터의 크기가 Memory Buffer 한개의 사이즈를 초과한 경우에
	//여러개의 Memory Buffer 가 Linked list 형태로 연결 되어 있기 때문에
	//하나의 Memory Buffer 에 넣어서 포인터만 넘겨 줌
	if(unlikely(pMbuf->nb_segs > 1))
	{
		nSegs = pMbuf->nb_segs;
			
		if( rte_pktmbuf_pkt_len(pMbuf) > DEF_MEM_JUMBO )
		{
			SetErrorMsg("Over Jumbo Memory Buffer Size %d, data_size : %d", DEF_MEM_JUMBO, rte_pktmbuf_pkt_len(pMbuf));
			RTE_LOG(ERR, MBUF, "%s\n"
								, GetErrorMsg());
			rte_pktmbuf_free(pMbuf);
			return -E_Q_NOMEM;
		}
	
		for(int i = 0; i < nSegs; i++)
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

	if(unlikely(m_unCurReadMbufIdx == m_unTotReadMbufIdx))
	{
		m_bReadComplete = true;
	}

	return rte_pktmbuf_pkt_len(m_pstCurMbuf);
}

/*!
 * \brief Read Complete
 * \details 데이터 처리 완료 후 호출 된다.
 *          1. 현재 읽고 있는 Ring 의 Consumer Position 을 변경한다.
 *          2. 현재 읽고 있는 Memory Buffer 를 Memory Pool 로 돌려 준다.
 * \param None
 * \return 
 *   - 0 on Success
 *   - -E_Q_MSYNC msync function error
 */
int CLQManager::ReadComplete()
{
	int ret = 0;

	if(likely(m_bReadComplete))
	{
		rte_ring_read_complete(m_pstReadRing
							, m_pstReadRing->cons.cons_info[m_unReadIdx].tail
							, m_pstReadRing->cons.cons_info[m_unReadIdx].head
							);
		//Read Complete 함수 호출 뒤에 consumer Info 의 Tail 값을 Head 값으로 변경
		m_pstReadRing->cons.cons_info[m_unReadIdx].tail = m_pstReadRing->cons.cons_info[m_unReadIdx].head;
		m_bReadComplete = false;

		if(m_bBackup)
		{
			ret = BackupReadData(m_pstReadRing, m_unReadIdx) ;
			if(ret < 0)
			{
				SetErrorMsg("Backup Read Data Failed");
				RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
				return ret;
			}
		}

	}

	return 0;
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
 * \brief Free Read Bulk Data
 * \details 처리가 완전히 완료된 Memory Buffer 를 해제한다(Bulk Mode)
 * \param None
 * \return None
 */
void CLQManager::FreeReadBulkData()
{
	for(int i = 0; i < m_unTotReadMbufIdx ; i++)
	{
		rte_pktmbuf_free(m_pstReadMbuf[i]);
	}
}

/*!
 * \brief Insert Data To m_pstWriteMbuf
 * \details 만약 현재 읽은 Memory Buffer 를 그대로 다음 Queue 에 전달하고 싶을 때 사용
 * \param None
 * \return 
 *   - 0 on Success
 *   - -E_Q_NOMEM out of memory
 */
int CLQManager::InsertData()
{
	if( unlikely( (m_unCurWriteMbufIdx + m_pstCurMbuf->nb_segs) >= (DEF_MEM_BUF_1024)) )
		return -E_Q_NOMEM;

	for(int i = 0; i < m_pstCurMbuf->nb_segs ; i++)
	{
		m_pstWriteMbuf[m_unCurWriteMbufIdx++] = m_pstCurMbuf;
		m_pstCurMbuf = m_pstCurMbuf->next;

		if( NULL == m_pstCurMbuf )
			break;
	}

	return 0;
	
}

/*!
 * \brief Insert Data To m_pstWriteMbuf
 * \details Bulk Mode 에서 여러개의 Data 를 Buffer 에 넣기 위해서 사용
 * \param a_pszData is Data Pointer to Insert 
 * \param a_nSize is Size of Data
 * \return 
 *   - 0 on Success
 *   - -E_Q_NOSPC there is no space for alloc;
 *   - -E_Q_NOMEM out of memory;
 */
int CLQManager::InsertData( char *a_pszData, int a_nSize )
{
	int nSegCnt = 0;
	int nLastLen = 0;
	uint16_t unBuffLen = 0;
	char *pCur = a_pszData;
	struct rte_mbuf *pHeadMbuf = NULL, *pMbuf = NULL, *pTmpMbuf = NULL;

	if(unlikely(m_unCurWriteMbufIdx >= DEF_MEM_BUF_1024))
		return -1;
	
	if(unlikely(a_nSize > DEF_MEM_BUF_1M))
	{
		SetErrorMsg("Data Length Over 1M , [Size:%d]", a_nSize);
		RTE_LOG(ERR, MBUF, "%s\n", GetErrorMsg());
		return -E_Q_NOMEM;
	}

	pHeadMbuf = rte_pktmbuf_alloc(m_pstDataMemPool);
	if(unlikely(pHeadMbuf == NULL))
	{
//		RTE_LOG(ERR, MBUF, "Mbuf Alloc Failed\n");
		return -E_Q_NOSPC;
	}

	unBuffLen = RTE_MBUF_DATA_SIZE;
	if(likely(a_nSize <= unBuffLen))
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
		nSegCnt = a_nSize / unBuffLen;
		nLastLen = a_nSize - (unBuffLen * nSegCnt);
		if(nLastLen == 0)
		{
			nLastLen = unBuffLen;
		}
		else
		{
			nSegCnt++;
		}

		for(int i = 1; i <= nSegCnt ; i++)
		{
			if(pTmpMbuf != NULL)
			{
				pMbuf = rte_pktmbuf_alloc(m_pstDataMemPool);
				if(pMbuf == NULL)
				{
					rte_pktmbuf_free(pHeadMbuf);
					SetErrorMsg("Mbuf Alloc Failed");
					RTE_LOG(ERR, MBUF, "%s\n", GetErrorMsg());
					return -E_Q_NOSPC;
				}

				pTmpMbuf->next = pMbuf;
			}
			else
			{
				pMbuf = pHeadMbuf;
			}

			if( i < nSegCnt )
			{
				pMbuf->data_len = unBuffLen;
			}
			else
			{
				pMbuf->data_len = nLastLen;
			}

			pMbuf->nb_segs = nSegCnt;
			pMbuf->pkt_len = a_nSize;
			memcpy( rte_pktmbuf_mtod(pMbuf, char*), pCur, pMbuf->data_len);

			pCur += pMbuf->data_len;
			pTmpMbuf = pMbuf;

		}

	}

	m_pstWriteMbuf[m_unCurWriteMbufIdx++] = pHeadMbuf;
	return 0;
}

/*!
 * \brief Insert Command Data To m_pstWriteMbuf
 * \details m_pstCmdPool 에서 Memory Buffer 를 가져와서 Command Data 를 입력
 * \param a_pszData is Data Pointer to Insert 
 * \param a_nSize is Size of Data
 * \return 
 *   - 0 on Success
 *   - -E_Q_NOSPC there is no space for alloc;
 *   - -E_Q_NOMEM out of memory;
 */
int CLQManager::InsertCommandData( char *a_pszData, int a_nSize )
{
	int nSegCnt = 0;
	int nLastLen = 0;
	uint16_t unBuffLen = 0;
	char *pCur = a_pszData;
	struct rte_mbuf *pHeadMbuf = NULL, *pMbuf = NULL, *pTmpMbuf = NULL;

	if(m_unCurWriteMbufIdx >= DEF_MEM_BUF_1024)
		return -1;
	
	if(a_nSize > DEF_MEM_BUF_1M)
	{
		SetErrorMsg("Data Length Over 1M , [Size:%d]", a_nSize);
		RTE_LOG(ERR, MBUF, "%s\n", GetErrorMsg());
		return -E_Q_NOMEM;
	}

	pHeadMbuf = rte_pktmbuf_alloc(m_pstCmdMemPool);
	if(pHeadMbuf == NULL)
	{
//		RTE_LOG(ERR, MBUF, "Mbuf Alloc Failed\n");
		return -E_Q_NOSPC;
	}

	unBuffLen = RTE_MBUF_DATA_SIZE;
	if(likely(a_nSize <= unBuffLen))
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
		nSegCnt = (a_nSize / unBuffLen);
		nLastLen = a_nSize - (unBuffLen * nSegCnt);
		if(nLastLen == 0)
		{
			nLastLen = unBuffLen;
		}
		else
		{
			nSegCnt++;
		}

		for(int i = 0; i < nSegCnt ; i++)
		{

			if(pTmpMbuf != NULL)
			{
				pMbuf = rte_pktmbuf_alloc(m_pstCmdMemPool);
				if(pMbuf == NULL)
				{
					rte_pktmbuf_free(pHeadMbuf);
					SetErrorMsg("Mbuf Alloc Failed");
					RTE_LOG(ERR, MBUF, "%s\n", GetErrorMsg());
					return -E_Q_NOSPC;
				}

				pTmpMbuf->next = pMbuf;
			}
			else
			{
				pMbuf = pHeadMbuf;
			}

			if( i < nSegCnt )
			{
				pMbuf->data_len = unBuffLen;
			}
			else
			{
				pMbuf->data_len = nLastLen;
			}

			pMbuf->nb_segs = nSegCnt + 1;
			pMbuf->pkt_len = a_nSize;

			memcpy( rte_pktmbuf_mtod(pMbuf, char*), pCur, pMbuf->data_len);

			pCur += pMbuf->data_len;
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
 * \return 
 *   - 0 on Success
 *   - -E_Q_INVAL Invalid Index; Invalid Ring Pointer;
 *   - -E_Q_NOSPC there is no space for alloc; there is no space for enqueue;
 *   - -E_Q_NOMEM out of memory;
 *   - -E_Q_MMAP MMap Function Error
 *   - -E_Q_TRUN File Truncate Error
 *   - -E_Q_FLOCK File Locking Error
 *   - -E_Q_MSYNC msync function error
 */
int CLQManager::WriteData( int a_nIdx )
{
	int ret = 0;
	//Ring Pointer
	struct rte_ring *pstRing = NULL;

	//Check Idx Error
	if(unlikely(a_nIdx < 0))
		return -E_Q_INVAL;
	
	if(unlikely(a_nIdx > m_unWriteRingCount))
		return -E_Q_INVAL;
	
	pstRing = m_stWriteRingInfo[a_nIdx].pstRing;
	if(unlikely(pstRing == NULL))
	{
		SetErrorMsg("Queue Idx[%d] is NULL", a_nIdx);
		RTE_LOG(ERR, RING, "%s\n", GetErrorMsg());
		return -E_Q_INVAL;
	}

	//Fail 이 일정 Count 의 배수일 때
	//현재 Ring 이 Full 이고 Consumer Process 가 Signal 을 유실하였다고 판단하여
	//다시 한번 Signal 을 전송
	if(unlikely((m_stWriteRingInfo[a_nIdx].unFailCnt % DEF_MAX_FAIL_CNT) == 0))
	{
		SendRTS(pstRing);
	}

	ret = InsertData();
	if(unlikely(ret < 0))
	{
		m_unCurWriteMbufIdx = 0;
		SetErrorMsg("Insert Data Failed");
		RTE_LOG(ERR, RING, "%s\n", GetErrorMsg());
		return ret;
	}

	//Enqueue Data in Ring
	ret = EnqueueData(
				pstRing
				,m_unCurWriteMbufIdx 
				,m_stWriteRingInfo[a_nIdx].nIdx	);

	if(unlikely(ret < 0))
	{
		m_unCurWriteMbufIdx = 0;
		m_stWriteRingInfo[a_nIdx].unFailCnt++;
		return ret;
	}

	//Backup Data
	if(m_bBackup)
	{
		ret = BackupWriteData( pstRing, m_stWriteRingInfo[a_nIdx].nIdx ) ;
		if(ret < 0)
		{
			SetErrorMsg("Write Data Backup Failed");
			RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
		}
	}

	//내부 데이터 저장 Buffer 의 시작 Index 를 초기화
	m_unCurWriteMbufIdx = 0;

	SendRTS(pstRing);

	return ret;
}

/*!
 * \brief Enqeue Data (Hash Function)
 * \details 현재 읽고 있는 Memory Buffer 구조체 그대로 다음 Queue 로 전달
 *          데이터의 Length 의 변경 없이 Filed 의 변경만 있을 때 사용 가능
 *          혹은 데이터의 변경이 Memory Buffer 의 최대 Length 를 넘지 않을 경우 사용 가능
 *          Hash 함수를 이용하여서 Write 할 Queue 를 선택
 * \param a_pArgs is Arguments for Hash Function
 * \return 
 *   - 0 on Success
 *   - -E_Q_INVAL Invalid Index; Invalid Ring Pointer;
 *   - -E_Q_NOSPC there is no space for alloc; there is no space for enqueue;
 *   - -E_Q_NOMEM out of memory;
 *   - -E_Q_MMAP MMap Function Error
 *   - -E_Q_TRUN File Truncate Error
 *   - -E_Q_FLOCK File Locking Error
 *   - -E_Q_MSYNC msync function error
 */
int CLQManager::WriteDataHash( void *a_pArgs )
{
	int ret = 0;
	//Idx of Write Ring
	int idx = m_pfuncHash(a_pArgs);
	//Ring Pointer
	struct rte_ring *pstRing = NULL;

	//Check Idx Error
	if(unlikely(idx < 0))
		return -E_Q_INVAL;
	
	if(unlikely(idx > m_unWriteRingCount))
		return -E_Q_INVAL;
	
	pstRing = m_stWriteRingInfo[idx].pstRing;

	if(unlikely(pstRing == NULL))
	{
		SetErrorMsg("Queue Idx[%d] is NULL", idx);
		RTE_LOG(ERR, RING, "%s\n", GetErrorMsg());
		return -E_Q_INVAL;
	}

	//Fail 이 일정 Count 의 배수일 때
	//현재 Ring 이 Full 이고 Consumer Process 가 Signal 을 유실하였다고 판단하여
	//다시 한번 Signal 을 전송
	if( unlikely((m_stWriteRingInfo[idx].unFailCnt % DEF_MAX_FAIL_CNT) == 0 ))
	{
		SendRTS(pstRing);
	}

	ret = InsertData();
	if(unlikely(ret < 0))
	{
		m_unCurWriteMbufIdx = 0;
		SetErrorMsg("Insert Data Failed");
		RTE_LOG(ERR, RING, "%s\n", GetErrorMsg());
		return ret;
	}

	//Enqueue Data in Ring
	ret = EnqueueData(
				pstRing
				,m_unCurWriteMbufIdx 
				,m_stWriteRingInfo[idx].nIdx ) ;

	if(unlikely(ret < 0))
	{
		m_unCurWriteMbufIdx = 0;
		m_stWriteRingInfo[idx].unFailCnt++;
		return ret;
	}

	//Backup Data
	if(m_bBackup)
	{
		ret = BackupWriteData( pstRing, m_stWriteRingInfo[idx].nIdx );
		if(ret < 0)
		{
			SetErrorMsg("Write Data Backup Failed");
			RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
		}
	}

	//내부 데이터 저장 Buffer 의 시작 Index 를 초기화
	m_unCurWriteMbufIdx = 0;

	SendRTS(pstRing);

	return ret;
}

/*!
 * \brief Enqueue Data (Index)
 * \details Enqueue Data and Send RTS to Consumer 
 *          (Write Queue 선택 기준은 Write Queue 의 Index)
 * \param a_pszData is Data Pointer to Insert 
 * \param a_nSize is Size of Data
 * \param a_nIdx is Index of m_stWriteRingInfo Array(Default : 0)
 * \return
 *   - 0 on Success
 *   - -E_Q_INVAL Invalid Index; Invalid Ring Pointer;
 *   - -E_Q_NOSPC there is no space for alloc; there is no space for enqueue;
 *   - -E_Q_NOMEM out of memory;
 *   - -E_Q_MMAP MMap Function Error
 *   - -E_Q_TRUN File Truncate Error
 *   - -E_Q_FLOCK File Locking Error
 *   - -E_Q_MSYNC msync function error
 */
int CLQManager::WriteData( char *a_pszData, int a_nSize, int a_nIdx )
{
	int ret = 0;
	//Ring Pointer
	struct rte_ring *pstRing = NULL;

	//Check Idx Error
	if(unlikely(a_nIdx < 0))
		return -E_Q_INVAL;
	
	if(unlikely(a_nIdx > m_unWriteRingCount))
		return -E_Q_INVAL;

	pstRing = m_stWriteRingInfo[a_nIdx].pstRing;

	if(unlikely(pstRing == NULL))
	{
		SetErrorMsg("Queue Idx[%d] is NULL", a_nIdx);
		RTE_LOG(ERR, RING, "%s\n", GetErrorMsg());
		return -E_Q_INVAL;
	}

	//Fail 이 일정 Count 의 배수일 때
	//현재 Ring 이 Full 이고 Consumer Process 가 Signal 을 유실하였다고 판단하여
	//다시 한번 Signal 을 전송
	if( unlikely((m_stWriteRingInfo[a_nIdx].unFailCnt % DEF_MAX_FAIL_CNT) == 0 ))
	{
		SendRTS(pstRing);
	}

	ret = InsertData(a_pszData, a_nSize) ;
	if(unlikely(ret < 0))
	{
		m_unCurWriteMbufIdx = 0;
		m_stWriteRingInfo[a_nIdx].unFailCnt++;
		return ret;
	}

	//Enqueue Data in Ring
	ret = EnqueueData(
				pstRing
				,m_unCurWriteMbufIdx
				,m_stWriteRingInfo[a_nIdx].nIdx ) ;

	if(unlikely(ret < 0))
	{
		//Free Memory Buffer
		rte_pktmbuf_free(m_pstWriteMbuf[0]);
		m_unCurWriteMbufIdx = 0;
		m_stWriteRingInfo[a_nIdx].unFailCnt++;
		return ret;
	}

	//Backup Data
	if(m_bBackup)
	{
		ret = BackupWriteData( pstRing, m_stWriteRingInfo[a_nIdx].nIdx );
		if(ret < 0)
		{
			SetErrorMsg("Write Data Backup Failed");
			RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
		}
	}

	//내부 데이터 저장 Buffer 의 시작 Index 를 초기화
	m_unCurWriteMbufIdx = 0;
	
	SendRTS(pstRing);

	return ret;
}


/*!
 * \brief Enqueue Data (HashFunction)
 * \details Enqueue Data and Send RTS to Consumer 
 *          Hash 함수를 이용하여서 Write 할 Queue 를 선택
 * \param a_pszData is Data Pointer to Insert 
 * \param a_nSize is Size of Data
 * \param a_pArgs is Arguments for Hash Function
 * \return 
 *   - 0 on Success
 *   - -E_Q_INVAL Invalid Index; Invalid Ring Pointer;
 *   - -E_Q_NOSPC there is no space for alloc; there is no space for enqueue;
 *   - -E_Q_NOMEM out of memory;
 *   - -E_Q_MMAP MMap Function Error
 *   - -E_Q_TRUN File Truncate Error
 *   - -E_Q_FLOCK File Locking Error
 *   - -E_Q_MSYNC msync function error
 */
int CLQManager::WriteDataHash( char *a_pszData, int a_nSize, void *a_pArgs )
{
	int ret = 0;
	//Idx of Write Ring
	int idx = m_pfuncHash(a_pArgs);
	//Ring Pointer
	struct rte_ring *pstRing = NULL;

	//Check Idx Error
	if(unlikely(idx < 0))
		return -E_Q_INVAL;
	
	if(unlikely(idx > m_unWriteRingCount))
		return -E_Q_INVAL;

	pstRing = m_stWriteRingInfo[idx].pstRing;

	if(unlikely(pstRing == NULL))
	{
		SetErrorMsg("Queue Idx[%d] is NULL", idx);
		RTE_LOG(ERR, RING, "%s\n", GetErrorMsg());
		return -E_Q_INVAL;
	}

	//Fail 이 일정 Count 의 배수일 때
	//현재 Ring 이 Full 이고 Consumer Process 가 Signal 을 유실하였다고 판단하여
	//다시 한번 Signal 을 전송
	if( unlikely((m_stWriteRingInfo[idx].unFailCnt % DEF_MAX_FAIL_CNT) == 0 ))
	{
		SendRTS(pstRing);
	}

	ret = InsertData(a_pszData, a_nSize);
	if(unlikely(ret < 0))
	{
		m_unCurWriteMbufIdx = 0;
		m_stWriteRingInfo[idx].unFailCnt++;
		return ret;
	}

	//Enqueue Data in Ring
	ret = EnqueueData(
				pstRing
				,m_unCurWriteMbufIdx
				,m_stWriteRingInfo[idx].nIdx ) ;

	if(unlikely(ret < 0))
	{
		//Free Memory Buffer
		rte_pktmbuf_free(m_pstWriteMbuf[0]);
		m_unCurWriteMbufIdx = 0;
		m_stWriteRingInfo[idx].unFailCnt++;
		return ret;
	}

	//Backup Data
	if(m_bBackup)
	{
		ret = BackupWriteData( pstRing, m_stWriteRingInfo[idx].nIdx ) ;
		if(ret < 0)
		{
			SetErrorMsg("Write Data Backup Failed");
			RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
		}
	}

	//내부 데이터 저장 Buffer 의 시작 Index 를 초기화
	m_unCurWriteMbufIdx = 0;
	
	SendRTS(pstRing);
	
	return ret;
}

/*!
 * \brief Write Data to Ring
 * \details Backup File 을 이용하여서 Queue 를 복구하는 기능을 위한 함수
 * \param a_pstRing is Ring Pointer
 * \param a_pszData is Data Pointer to Insert 
 * \param a_nSize is Size of Data
 * \return
 *   - 0 on Success
 *   - -E_Q_INVAL Invalid Index; Invalid Ring Pointer;
 *   - -E_Q_NOSPC there is no space for alloc; there is no space for enqueue;
 *   - -E_Q_NOMEM out of memory;
 *   - -E_Q_MMAP MMap Function Error
 *   - -E_Q_TRUN File Truncate Error
 *   - -E_Q_FLOCK File Locking Error
 *   - -E_Q_MSYNC msync function error
 */
int CLQManager::WriteData( struct rte_ring *a_pstRing, char *a_pszData, int a_nSize )
{
	int ret = 0;

	ret = InsertData(a_pszData, a_nSize) ;
	if(unlikely(ret < 0))
	{
		m_unCurWriteMbufIdx = 0;
		return ret;
	}

	//Enqueue Data in Ring
	ret = EnqueueData(
				a_pstRing
				,m_unCurWriteMbufIdx
				,0) ;

	if(unlikely(ret < 0))
	{
		//Free Memory Buffer
		rte_pktmbuf_free(m_pstWriteMbuf[0]);
		m_unCurWriteMbufIdx = 0;
		return ret;
	}

	//내부 데이터 저장 Buffer 의 시작 Index 를 초기화
	m_unCurWriteMbufIdx = 0;

	return ret;
}

/*!
 * \brief Commit Data to Queue
 * \details m_pstWriteMbuf 에 저장된 데이터를 Queue 에 Insert
 *          Hash Function 을 이용한 Queue 의 선택 기능은 지원하지 않음
 * \param a_nIdx is Index of m_stWriteRingInfo Array 
 * \return 
 *   - 0 on Success
 *   - -E_Q_INVAL Invalid Index; Invalid Ring Pointer;
 *   - -E_Q_NOSPC there is no space for alloc; there is no space for enqueue;
 *   - -E_Q_MMAP MMap Function Error
 *   - -E_Q_TRUN File Truncate Error
 *   - -E_Q_FLOCK File Locking Error
 *   - -E_Q_MSYNC msync function error
 */
int CLQManager::CommitData(int a_nIdx)
{
	int ret = 0;
	//Ring Pointer
	struct rte_ring *pstRing = NULL;
	
	//Check Idx Error
	if(unlikely(a_nIdx < 0))
	{
		return -E_Q_INVAL;
	}
	
	if(unlikely(a_nIdx > m_unWriteRingCount))
	{
		return -E_Q_INVAL;
	}

	//Select Ring by Index 
	pstRing = m_stWriteRingInfo[a_nIdx].pstRing;

	if(unlikely(pstRing == NULL))
	{
		SetErrorMsg("Queue Idx[%d] is NULL", a_nIdx);
		RTE_LOG(ERR, RING, "%s\n", GetErrorMsg());
		return -E_Q_INVAL;
	}

	//Fail 이 일정 Count 의 배수일 때
	//현재 Ring 이 Full 이고 Consumer Process 가 Signal 을 유실하였다고 판단하여
	//다시 한번 Signal 을 전송
	if( unlikely((m_stWriteRingInfo[a_nIdx].unFailCnt % DEF_MAX_FAIL_CNT) == 0))
	{
		SendRTS(pstRing);
	}

	//Enqueue Data in Ring
	ret = EnqueueData(
				pstRing
				,m_unCurWriteMbufIdx
				,m_stWriteRingInfo[a_nIdx].nIdx ) ;

	if(ret < 0)
	{
		//NO Space Error 시에는 재시도 하도록 변경. 따라서 m_unCurWriteMbufIdx 를 초기화 할 수 없음
//		m_unCurWriteMbufIdx = 0;
		m_stWriteRingInfo[a_nIdx].unFailCnt++;
		return ret; 
	}

	//Backup Data
	if(m_bBackup)
	{
		ret = BackupWriteData( pstRing, m_stWriteRingInfo[a_nIdx].nIdx );
		if(ret < 0)
		{
			SetErrorMsg("Write Data Backup Failed");
			RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
		}
	}

	//내부 데이터 저장 Buffer 의 시작 Index 를 초기화
	m_unCurWriteMbufIdx = 0;

	SendRTS(pstRing);

	return ret;

}

/*!
 * \brief Send RTS To Consumer Process
 * \details Ring 에서 현재 Sleep 상태인 Consumer Process 로 RTS 전송
 * \param a_pstRing is Ring Pointer
 * \return 0 on Success
 */
int CLQManager::SendRTS(struct rte_ring *a_pstRing)
{
	//Signal Value
	union sigval sv;

	//Find Sleep Consumer Process
	for(uint32_t i = 0; i < a_pstRing->cons.cons_count ; i++)
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
 * \breif Send RTS For Command Send
 * \details Ring 에 연결 된 Process 에게 RTS 전송
 * \param a_pstRing is Ring Pointer
 * \return 0 on Success
 */
int CLQManager::SendRTSCommand(struct rte_ring *a_pstRing)
{
	//Signal Value
	union sigval sv;
	int ret = 0;

	sv.sival_ptr = a_pstRing;
	ret = sigqueue(a_pstRing->cons.cons_info[0].pid, SIGRTMIN, sv);
	if(ret < 0)
		return -errno;

	return 0;
}

/*!
 * \brief enqueue Data
 * \details Data 를 Ring 에 입력
 * \param a_pstRing is Ring
 * \param a_nCnt is Enqueue Count
 * \param a_nIdx is Index of the Producer
 * \return 
 *   - 0 on Success
 *   - -E_Q_NOSPC there is no memory buffer
 */
int CLQManager::EnqueueData(struct rte_ring *a_pstRing, int a_nCnt, int a_nIdx)
{
	int ret = 0;

	//Enqueue Data in Ring
	ret = rte_ring_mp_enqueue_bulk_idx(
			a_pstRing, (void**)&m_pstWriteMbuf, a_nCnt, RTE_RING_QUEUE_FIXED, a_nIdx);


#if 0
	//Enqueue Failed
	if( ret != 0)
	{
		for(int i = 0; i < a_nCnt ; i++)
		{
			//Data Buffer Free
			rte_pktmbuf_free(m_pstWriteMbuf[i]);
		}
		return -E_Q_NOSPC;
	}
#endif
	return ret;
}

/*!
 * \brief Init Backup Files
 * \details Queue 의 Backup 을 위하여서 Backup File 을 초기화
 * \param a_pstRing is Ring 
 * \param a_szFileName is Name of Backup File
 * \param a_nType is Ring Type (Read:0/ Write:1)
 * \param a_nIdx is Index of the Process in the Queue
 * \return 
 *   - 0 on Success
 *   - -E_Q_NOENT File Open Error
 *   - -E_Q_MMAP MMap Function Error
 *   - -E_Q_TRUN File Truncate Error
 *   - -E_Q_FLOCK File Locking Error
 */
int CLQManager::InitBackupFile( struct rte_ring *a_pstRing, char *a_szFileName, int a_nType, int a_nIdx )
{
	char szFileName[DEF_MEM_BUF_1024];
	uint64_t unDefaultSize = sizeof(BACKUP_INFO) + (DEF_DEFAULT_BK_MASK * sizeof(DATA_BUFFER));
	struct stat bk_file_stat;
	//struct flock => { l_type, l_whence, l_start, l_len};
	//flock for Backup Info Update
	struct flock wr_info_lock = { F_WRLCK, SEEK_SET, 0, sizeof(BACKUP_INFO) };
	//funlock for Backup Info
	struct flock wr_info_un_lock = { F_UNLCK, SEEK_SET, 0, sizeof(BACKUP_INFO) };

	struct rte_ring_backup_info *pstRingBackup = NULL;
	BACKUP_INFO *pstBackup = NULL;

	//Init Backup File Info
	if(a_nType == DEF_RING_TYPE_READ)
		pstRingBackup = &(a_pstRing->cons.cons_info[a_nIdx].backup_info);
	else
		pstRingBackup = &(a_pstRing->prod.prod_info[a_nIdx].backup_info);

	memset(szFileName, 0x00, sizeof(szFileName));

	sprintf(szFileName, "%s/%s.bak", m_pclsConfig->GetConfigValue("QUEUE", "BACKUP_PATH"), a_szFileName);
	printf("szFile Name %s\n", szFileName);
	pstRingBackup->fd = open(szFileName, O_RDWR|O_CREAT, 0660);
	if(unlikely(pstRingBackup->fd == -1))
	{
		SetErrorMsg("Backup File Open Error");
		RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
		return -E_Q_NOENT;
	}

	if( unlikely(fstat(pstRingBackup->fd, &bk_file_stat) < 0) )
	{
		SetErrorMsg("Stat Function Error");
		RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
		close(pstRingBackup->fd);
		return -E_Q_NOENT;
	}
	
	if( bk_file_stat.st_size == 0 )
	{
		if(ftruncate(pstRingBackup->fd, unDefaultSize) < 0)
		{
			SetErrorMsg("ftruncate Function Error errno : %d", errno);
			RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
			close(pstRingBackup->fd);
			return -E_Q_TRUN;
		}

		//File Lock For Init
		if( fcntl(pstRingBackup->fd, F_SETLK, &wr_info_lock) >= 0 )
		{
			//mmap File 
			pstRingBackup->backup = mmap(NULL, unDefaultSize, PROT_READ|PROT_WRITE, MAP_SHARED, pstRingBackup->fd, 0);
			if(pstRingBackup->backup == MAP_FAILED)
			{
				SetErrorMsg("First mmap Failed errno : %d", errno);
				RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
				close(pstRingBackup->fd);
				return -E_Q_MMAP;
			}

			pstRingBackup->size = unDefaultSize;

			//Insert Process Info
			pstBackup = (BACKUP_INFO*)pstRingBackup->backup;
			memset(pstBackup, 0x00, sizeof(BACKUP_INFO));
			pstBackup->unMask = DEF_DEFAULT_BK_MASK;
		
			if(a_nType == DEF_RING_TYPE_READ)
			{
				pstBackup->unConsCnt = 1;
				sprintf(pstBackup->stConsInfo[a_nIdx].strName, "%s", m_szProcName);
			}
			else
			{
				pstBackup->unProdCnt = 1;
				sprintf(pstBackup->stProdInfo[a_nIdx].strName, "%s", m_szProcName);
			}

			pstRingBackup->mask = DEF_DEFAULT_BK_MASK;
				
//			if(fcntl(fd, F_SETLK, &wr_all_un_lock) < 0)
			if(fcntl(pstRingBackup->fd, F_SETLK, &wr_info_un_lock) < 0)
			{
				SetErrorMsg("File Unlock Failed");
				RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
				close(pstRingBackup->fd);
				return -E_Q_FLOCK;
			}

			//Sync mmap <-> File
			msync(pstBackup, sizeof(BACKUP_INFO), MS_SYNC);
			return 0;
		}

	}

	//mmap File 
	pstRingBackup->backup = mmap(NULL, unDefaultSize, PROT_READ|PROT_WRITE, MAP_SHARED, pstRingBackup->fd, 0);
	if(pstRingBackup->backup == MAP_FAILED)
	{
		SetErrorMsg("Sub mmap Failed errno : %d", errno);
		RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
		close(pstRingBackup->fd);
		return -E_Q_MMAP;
	}

	pstRingBackup->size = unDefaultSize;

	if( fcntl(pstRingBackup->fd, F_SETLKW, &wr_info_lock) >= 0 )
	{
		pstBackup = (BACKUP_INFO*)pstRingBackup->backup;

		if(a_nType == DEF_RING_TYPE_READ)
		{
			pstBackup->unConsCnt = a_pstRing->cons.cons_count;
			memset( &(pstBackup->stConsInfo[a_nIdx]), 0x00, sizeof(CONS_INFO));
			sprintf(pstBackup->stConsInfo[a_nIdx].strName, "%s_%d", m_szProcName, m_nInstanceID);
		}
		else
		{
			pstBackup->unProdCnt = a_pstRing->prod.prod_count;
			memset( &(pstBackup->stProdInfo[a_nIdx]), 0x00, sizeof(PROD_INFO));
			sprintf(pstBackup->stProdInfo[a_nIdx].strName, "%s_%d", m_szProcName, m_nInstanceID);

			//만약 백업 파일의 Mask 값이 변경 되었을 경우
			//mmap 을 바뀐 사이즈 만큼 다시 수행한다.	
			if(DEF_DEFAULT_BK_MASK != pstBackup->unMask)
			{
				unDefaultSize = sizeof(BACKUP_INFO) + (pstBackup->unMask * sizeof(DATA_BUFFER));
				if(munmap(pstRingBackup->backup, pstRingBackup->size) < 0)
				{
					SetErrorMsg("munmap Failed");
					RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
					return -E_Q_MMAP;
				}
				pstRingBackup->backup = mmap(NULL, unDefaultSize, PROT_READ|PROT_WRITE, MAP_SHARED, pstRingBackup->fd, 0);
				if(pstRingBackup->backup == MAP_FAILED)
				{
					SetErrorMsg("mremap Failed");
					RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
					return -E_Q_MMAP;
				}

				RTE_LOG(INFO, EAL, "remap Backup files (%p) oldsize(%u) -> newsize(%lu)\n"
						, pstRingBackup->backup
						, pstRingBackup->size, unDefaultSize);
				pstRingBackup->size = unDefaultSize;
			}
		}

		pstRingBackup->mask = pstBackup->unMask;

		if(fcntl(pstRingBackup->fd, F_SETLK, &wr_info_un_lock) < 0)
		{
			SetErrorMsg("File unlock Failed");
			RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
			close(pstRingBackup->fd);
			return -E_Q_FLOCK;
		}

		//Sync mmap <-> File
		msync(pstBackup, sizeof(BACKUP_INFO), MS_SYNC);

		return 0;
		
	}

	return 0;
}
 
/*!
 * \brief Backup Write Data
 * \details Backup Enqueue Data in the File
 * \param a_pstRing is Pointer of Ring
 * \param a_nIdx is Index Number of Producer Process of Ring
 * \return 
 *   - 0 on Success
 *   - -E_Q_MMAP MMap Function Error
 *   - -E_Q_TRUN File Truncate Error
 *   - -E_Q_FLOCK File Locking Error
 *   - -E_Q_MSYNC msync function error
 */
int CLQManager::BackupWriteData( struct rte_ring *a_pstRing, int a_nIdx )
{
	int ret = 0;
	//pstData 의 Buffer 와 m_pstWriteMbuf 의 Buffer Index 를 맞추기 위하여 nIdx 설정 
	int nIdx = 0;

	struct rte_ring_backup_info *pstRingBackup = &(a_pstRing->prod.prod_info[a_nIdx].backup_info);
	BACKUP_INFO *pstBackup = (BACKUP_INFO*)pstRingBackup->backup;
	DATA_BUFFER *pstData = (DATA_BUFFER*)(pstBackup + 1);

	uint32_t unEndIdx = 0;
	uint32_t unStartIdx = 0;
	uint32_t unEntry = 0;
	float fUsage = 0;
	uint64_t unNewSize = 0;
	uint32_t unNewMask = 0;

	//flock for Backup Info Update
	struct flock wr_info_lock = { F_WRLCK, SEEK_SET, 0, sizeof(BACKUP_INFO) };
	//funlock for Backup Info
	struct flock wr_info_un_lock = { F_UNLCK, SEEK_SET, 0, sizeof(BACKUP_INFO) };

	//calculate start, end Index Number	
	unEndIdx = a_pstRing->prod.prod_info[a_nIdx].tail & pstBackup->unMask;
	unStartIdx = (a_pstRing->prod.prod_info[a_nIdx].tail - m_unCurWriteMbufIdx) & pstBackup->unMask;
	unEntry = rte_ring_count(a_pstRing);

//	RTE_LOG(INFO, EAL, "unEntry %u, unMask %u, \n", unEntry, pstRingBackup->mask);
	fUsage = (unEntry / (float)pstRingBackup->mask) * 100;

//	RTE_LOG(INFO, EAL, "backup startidx %d, endidx %d mask %X, tail %u , m_unCurWriteMbufIdx %u, fUasge %.2f\n", unStartIdx, unEndIdx, pstBackup->unMask, a_pstRing->prod.prod_info[a_nIdx].tail, m_unCurWriteMbufIdx, fUsage);
	//만약 Index 의 사용률이 80% 를 넘을 경우
	//Max Index 를 변경 
	//Max Index 는 반드시 2n -1 이 되어야 함
	if( unlikely(fUsage > 80.0) )
	{
		unNewMask = pstBackup->unMask << 1;
		unNewMask = unNewMask | 1;	
		unNewSize = sizeof(BACKUP_INFO) + (unNewMask * sizeof(DATA_BUFFER));
//		RTE_LOG(INFO, EAL, "Usage Over File Size reset old : %u, new : %u\n", pstRingBackup->size, unNewSize);

		//File Lock
		if( fcntl(pstRingBackup->fd, F_SETLKW, &wr_info_lock) >= 0 )
		{
			if(ftruncate(pstRingBackup->fd, unNewSize) < 0)
			{
				SetErrorMsg("ftruncate Function Error");
				RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
				return -E_Q_TRUN;
			}

			//Remap 
			if(munmap(pstRingBackup->backup, pstRingBackup->size) < 0)
			{
				SetErrorMsg("munmap Failed errno: %d", errno);
				RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
				return -E_Q_MMAP;
			}

			pstRingBackup->backup = mmap(NULL, unNewSize, PROT_READ|PROT_WRITE, MAP_SHARED, pstRingBackup->fd, 0);
			if(pstRingBackup->backup == MAP_FAILED)
			{
				SetErrorMsg("mremap Failed %d", errno);
				RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
				return -E_Q_MMAP;
			}

			pstRingBackup->size = unNewSize;

			//Reset Backup Info Pointer
			pstBackup = (BACKUP_INFO*)pstRingBackup->backup;

			if(pstRingBackup->mask == pstBackup->unMask)
			{
				pstBackup->unMask = unNewMask;
				//Sync mmap <-> File
				msync(pstBackup, sizeof(BACKUP_INFO), MS_SYNC);
			}

			//Reset Data Pointer
			pstData = (DATA_BUFFER*)(pstBackup + 1);

			//Reset unMask 
			pstRingBackup->mask = unNewMask;

			//File Unlock 
			if(fcntl(pstRingBackup->fd, F_SETLK, &wr_info_un_lock) < 0)
			{
				SetErrorMsg("File unlock Failed");
				RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
				close(pstRingBackup->fd);
				return -E_Q_FLOCK;
			}
		}
	}


	for(uint32_t i = unStartIdx; i < unEndIdx; i++, nIdx++)
	{
		
		pstData[i].unBuffLen =  m_pstWriteMbuf[nIdx]->data_len;
		memcpy( pstData[i].szBuff, rte_pktmbuf_mtod(m_pstWriteMbuf[nIdx], char*), pstData[i].unBuffLen);
//		RTE_LOG(INFO, EAL, "Buffer[%d] len %u, %s\n", i, pstData[i].unBuffLen, pstData[i].szBuff);
	}

	pstBackup->stProdInfo[a_nIdx].unTail = a_pstRing->prod.prod_info[a_nIdx].tail;	
		
	//fflush 와 같은 효과
	//성능 저하가 발생할 수 있기 때문에 Sync Flag 가 설정 된 경우에만 실시
	if(m_bMsync)
	{
		ret = msync(pstRingBackup->backup, pstRingBackup->size, MS_SYNC);
		if(ret < 0)
		{
			SetErrorMsg("msync(sync) (%p) failed size : %u, errno : %d"
						, pstRingBackup->backup
						, pstRingBackup->size, errno);
			RTE_LOG(ERR, EAL, "%s\n"
						, GetErrorMsg());
			return -E_Q_MSYNC;
		}
	}
	else
	{
		ret = msync(pstRingBackup->backup, pstRingBackup->size, MS_ASYNC);
		if(ret < 0)
		{
			SetErrorMsg("msync(async) (%p) failed size : %u, errno : %d"
						, pstRingBackup->backup
						, pstRingBackup->size, errno);
						
			RTE_LOG(ERR, EAL, "%s\n"
						, GetErrorMsg());
			return -E_Q_MSYNC;
		}
	}
	
	return 0;
}

/*!
 * \brief Backup Read Data
 * \details Backup Dequeue Position in the File
 * \param a_pstRing is Pointer of Ring
 * \param a_nIdx is Index Number of Consumer Process of Ring
 * \return 
 *   - 0 on Success
 *   - -E_Q_MSYNC msync function error
 */
int CLQManager::BackupReadData( struct rte_ring *a_pstRing, int a_nIdx )
{
	int ret = 0;
	//pstData 의 Buffer 와 m_pstWriteMbuf 의 Buffer Index 를 맞추기 위하여 nIdx 설정 

	struct rte_ring_backup_info *pstRingBackup = &(a_pstRing->cons.cons_info[a_nIdx].backup_info);
	BACKUP_INFO *pstBackup = (BACKUP_INFO*)pstRingBackup->backup;

	pstBackup->stConsInfo[a_nIdx].unTail = a_pstRing->cons.cons_info[a_nIdx].tail;	

	//fflush 와 같은 효과
	//성능 저하가 발생할 수 있기 때문에 Sync Flag 가 설정 된 경우에만 실시
	if(m_bMsync)
	{
		ret = msync(pstRingBackup->backup, pstRingBackup->size, MS_SYNC);
		if(ret < 0)
		{
			SetErrorMsg("msync(sync) (%p) failed size : %u, errno : %d"
						, pstRingBackup->backup
						, pstRingBackup->size, errno
			);
			RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());

			return -E_Q_MSYNC;
		}
	}
	else
	{
		ret = msync(pstRingBackup->backup, pstRingBackup->size, MS_ASYNC);
		if(ret < 0)
		{
			SetErrorMsg("msync(sync) (%p) failed size : %u, errno : %d"
						, pstRingBackup->backup
						, pstRingBackup->size, errno
			);
			RTE_LOG(ERR, EAL, "%s\n", GetErrorMsg());
			return -E_Q_MSYNC;
		}
	}
	return 0;
}

/*!
 * \brief Get Write Queue Index
 * \details 
 * Process 의 Name 을 입력하여서 함수를 호출한 Process 에 연결되어 있는 모든 Write Queue 중
 * 입력한 Process Name 과 연결 된 Write Queue 의 Index 를 Return
 * \param a_szProc is Process Name
 * \return 
 *   - Index of Write Queue on Success
 *   - -1 on Error
 */
 int CLQManager::GetWriteQueueIndex(char *a_szProc)
 {
	for(int i = 0; i < m_unWriteRingCount ; i++)
	{
		for(uint32_t j = 0 ; j < m_stWriteRingInfo[i].vecRelProc.size(); j++)
		{
			if( strncmp(m_stWriteRingInfo[i].vecRelProc[j].c_str(), a_szProc, strlen(a_szProc)) == 0 )
			{
				return i;
			}
		}
	}

	return -1;	
 }


/*!
 * \brief Get Read Queue Index
 * \details 
 * Process 의 Name 을 입력하여서 함수를 호출한 Process 에 연결되어 있는 모든 Read Queue 중
 * 입력한 Process Name 과 연결 된 Read Queue 의 Index 를 Return
 * \param a_szProc is Process Name
 * \return 
 *   - Index of Read Queue on Success
 *   - -1 on Error
 */
 int CLQManager::GetReadQueueIndex(char *a_szProc)
 {
	for(int i = 0; i < m_unReadRingCount ; i++)
	{
		for(uint32_t j = 0 ; j < m_stReadRingInfo[i].vecRelProc.size(); j++)
		{
			if( strncmp(m_stReadRingInfo[i].vecRelProc[j].c_str(), a_szProc, strlen(a_szProc)) == 0 )
			{
				return i;
			}
		}
	}

	return -1;	
 }


/*!
 * \brief Set Error Msg if Error occur
 * \param a_szFmt is Format of Query
 * \param ... is Aurgument For Query
 * \return None
 */
void CLQManager::SetErrorMsg(const char *a_szFmt, ...)
{
	va_list	args;

	va_start(args, a_szFmt);
	vsprintf(m_szErrorMsg, a_szFmt, args);
	va_end(args);
}


/*!
 * \brief Get Error Msg if Error occur
 * \return m_szErrorMsg is Buffer of Error Msg
 */
char *CLQManager::GetErrorMsg() { return m_szErrorMsg; }

/*!
 * \brief Init Command Ring
 * \param a_szProcName is Name of Process
 * \param a_nInstanceID is Instance ID of Process
 * \param a_nType is Type of Caller (Sender : 1, Receiver : 0)
 * \return
 *   - -1 Fail 
 *   - -E_Q_NO_CONFIG- function could not get pointer to rte_config structure
 *   - -E_Q_SECONDARY - function was called from a secondary process instance
 *   - -E_Q_INVAL - count provided is not a power of 2
 *   - -E_Q_NOSPC - the maximum number of memzones has already been allocated
 *   - -E_Q_EXIST - a memzone with the same name already exists
 *   - -E_Q_NOMEM - no appropriate memory area found in which to create memzone; Consumer Count Over
 */
int CLQManager::InitCommandRing(char *a_szProcName, int a_nInstanceID, int a_nType)
{
	int ret = 0;
	char pszQName[128];
	struct rte_ring *pstSnd = NULL;
	struct rte_ring *pstRcv = NULL;

	memset(pszQName, 0x00, sizeof(pszQName));
	sprintf(pszQName, "%s", DEF_STR_COMMAND_RING);
	//Attach Ring
	ret = CreateRing( pszQName, &(pstSnd));
	if(ret < 0)
	{
		return ret;
	}

	if(a_nType == DEF_CMD_TYPE_SEND)
	{
		//Insert Consumers Info to Cons Ring
		ret = InsertConsInfo(a_szProcName, m_nInstanceID, getpid(), pstSnd);
		if(ret < 0)
		{
			SetErrorMsg("Insert Process info to Command Send Ring Failed (%d)", ret);
			RTE_LOG(ERR, RING, "%s\n", GetErrorMsg());
			m_pstCmdSndRing = NULL;
			return ret;
		}

		m_pstCmdRcvRing = pstSnd;
		return 0;
	}


	memset(pszQName, 0x00, sizeof(pszQName));
	sprintf(pszQName, "%s_%d", a_szProcName, a_nInstanceID);
	
	//Attach Ring
	ret =  CreateRing( pszQName, &(pstRcv) );
	if(ret < 0)
	{
		return ret;
	}

	//Insert Consumers Info to Cons Ring
	ret = InsertConsInfo(a_szProcName, m_nInstanceID, getpid(), pstRcv);
	if(ret < 0)
	{
		SetErrorMsg("Insert Process info to Command Recieve Ring Failed (%d)", ret);
		RTE_LOG(ERR, RING, "%s\n", GetErrorMsg());
		return ret;
	}

	m_pstCmdRcvRing = pstRcv;
	m_pstCmdSndRing = pstSnd;	

	return 0;
}

/*!
 * \brief Send Command to Process
 * \param a_szProcName 명령을 전달하고 싶은 Process 의 이름
 * \param a_nInstanceID is Instance ID of Process
 * \param a_pszData is Data Pointer to Insert 
 * \param a_nSize is Size of Data
 * \return 
 *   - 0 on Success
 *   - -E_Q_INVAL Queue Pointer is NULL; sig wa invalid;
 *   - -E_Q_NOSPC there is no space for alloc;
 *   - -E_Q_NOMEM out of memory;
 *   - -E_Q_AGAIN The limit of signals which may be queued has been reached
 *   - -E_Q_PERM The  process does not have permission to send the signal to the receiving process
 *   - -E_Q_SRCH No process has a PID matching pid
 */
int CLQManager::SendCommand(char *a_szProcName, int a_nInstanceID, char *a_pszData, int a_nSize)
{
	int ret = 0;
	struct rte_ring *pstRing = NULL;
	char pszQName[128];
	
	memset(pszQName, 0x00, sizeof(pszQName));
	sprintf(pszQName, "%s_%d", a_szProcName, a_nInstanceID);

	ret = CreateRing(pszQName, &pstRing);
	if(ret != E_Q_EXIST)
	{
		SetErrorMsg("Queue %s Is not Exist", pszQName);
		RTE_LOG(ERR, RING, "%s\n", GetErrorMsg());
		return -E_Q_INVAL;
	}

	ret = InsertCommandData(a_pszData, a_nSize);
	if(ret < 0)
	{
		m_unCurWriteMbufIdx = 0;
		return ret;
	}

	//Enqueue Data in Ring
	ret = rte_ring_enqueue_bulk(pstRing, (void**)&m_pstWriteMbuf, 1) ;
	if(ret < 0)
	{
		//Free Memory Buffer
		rte_pktmbuf_free(m_pstWriteMbuf[0]);
		m_unCurWriteMbufIdx = 0;
		return ret;
	}

	//내부 데이터 저장 Buffer 의 시작 Index 를 초기화
	m_unCurWriteMbufIdx = 0;

	ret = SendRTSCommand(pstRing);	
	if(ret <0)
		return ret;

	return 0;
}

/*!
 * \brief Receive Command 
 * \details 누군가로 부터 전송된 Command 를 수신 
 * \param  a_pszBuff is Buffer Pointer to Store Data
 * \return 
 *   - 0 on Success
 *   - -E_Q_INVAL Ring Pointer is NULL
 *   - -E_Q_NOENT required entry not available to return.
 *   - -E_Q_NOMEM Msg Size is Over Buffer size
 */
int CLQManager::ReceiveCommand(char **a_pszBuff)
{
	//Result
	int ret = 0;
	int nSegs = 0;
	char *p = m_szJumboBuff;
	struct rte_mbuf *pMbuf = NULL;
	
	if(m_pstCmdRcvRing == NULL)
	{
		SetErrorMsg("m_pstCmdRcvRing is NULL");
		RTE_LOG(ERR, RING, "%s\n", GetErrorMsg());
		return -E_Q_INVAL;
	}

	ret = rte_ring_dequeue_bulk(m_pstCmdRcvRing, (void**)&m_pstCurMbuf, 1);

	if(ret != 0)
	{
		return ret;
	}

	pMbuf = m_pstCurMbuf;

	//데이터의 크기가 Memory Buffer 한개의 사이즈를 초과한 경우에
	//여러개의 Memory Buffer 가 Linked list 형태로 연결 되어 있기 때문에
	//하나의 Memory Buffer 에 넣어서 포인터만 넘겨 줌
	if(pMbuf->nb_segs > 1)
	{
		nSegs = pMbuf->nb_segs;
		if( rte_pktmbuf_pkt_len(pMbuf) > DEF_MEM_JUMBO )
		{
			SetErrorMsg("Over Jumbo Memory Buffer Size %d, data_size : %d"
								, DEF_MEM_JUMBO, rte_pktmbuf_pkt_len(pMbuf));

			RTE_LOG(ERR, MBUF, "%s\n", GetErrorMsg());
			rte_pktmbuf_free(pMbuf);
			return -E_Q_NOMEM;
		}
	
		for(int i = 0; i < nSegs; i++)
		{
			memcpy(p, rte_pktmbuf_mtod(pMbuf, char *), rte_pktmbuf_data_len(pMbuf)); 
			p += rte_pktmbuf_data_len(pMbuf);
			pMbuf = pMbuf->next;
			
			if( NULL == pMbuf )
				break;
		}

		*a_pszBuff = m_szJumboBuff;
	}
	else
	{
		*a_pszBuff = rte_pktmbuf_mtod( pMbuf, char * );
	}

	return 0;
}

/*!
 * \brief Send Result of Command
 * \param a_pszData is Data Pointer to Insert
 * \param a_nSize is Size of Data
 * \return
 *   - 0 on Success
 *   - -1 on Fail
 *   - -E_Q_NOSPC there is no space for alloc;
 *   - -E_Q_NOMEM out of memory;
 *   - -E_Q_AGAIN The limit of signals which may be queued has been reached
 *   - -E_Q_INVAL sig was invalid
 *   - -E_Q_PERM The  process does not have permission to send the signal to the receiving process
 *   - -E_Q_SRCH No process has a PID matching pid
 */
int CLQManager::SendCommandResult(char *a_pszData, int a_nSize)
{
	int ret = 0;
	struct rte_ring *pstRing = m_pstCmdSndRing;
	char pszQName[128];
	
	memset(pszQName, 0x00, sizeof(pszQName));
	sprintf(pszQName, "%s", DEF_STR_COMMAND_RING);

	ret = CreateRing(pszQName, &pstRing);
	if(ret != E_Q_EXIST)
	{
		SetErrorMsg("Queue %s Is not Exist", pszQName);
		RTE_LOG(ERR, RING, "%s\n", GetErrorMsg());
		return -E_Q_INVAL;
	}

	ret = InsertCommandData(a_pszData, a_nSize);
	if(ret < 0)
	{
		m_unCurWriteMbufIdx = 0;
		return ret;
	}

	//Enqueue Data in Ring
	ret = rte_ring_enqueue_bulk(pstRing, (void**)&m_pstWriteMbuf, 1) ;
	if(ret < 0)
	{
		//Free Memory Buffer
		rte_pktmbuf_free(m_pstWriteMbuf[0]);
		m_unCurWriteMbufIdx = 0;
		return ret;
	}

	//내부 데이터 저장 Buffer 의 시작 Index 를 초기화
	m_unCurWriteMbufIdx = 0;
	
	ret = SendRTSCommand(pstRing);
	if(ret < 0)
		return ret;

	return 0;
}


/*!
 * \brief Delete Queue
 * \param a_pszQueue Name of Queue
 * \return
 *   - size of deleted Ring on Success
 *   - -E_Q_INVAL There is no Queue
 */
int CLQManager::DeleteQueue(char *a_pszQueue)
{
	int ret = 0;
	uint32_t nRingSize = 0;
	struct rte_ring *pstRing = NULL;	

	pstRing = rte_ring_lookup(a_pszQueue);
	//Exist Ring -> Start to Delete Ring
	if(pstRing)
	{
		//Set the Origin Ring Count
		nRingSize = pstRing->prod.watermark;
		
		//Delete Data in the Ring
		while(1)
		{
			ret = rte_ring_dequeue(pstRing, (void**)m_pstReadMbuf);
			if(ret < 0)
				break;

			rte_pktmbuf_free(m_pstReadMbuf[0]);
		}

		rte_ring_free(pstRing);	

		return nRingSize;
	}

	return -E_Q_INVAL;
}

/*!
 * \brief Get All Queue List For Monitoring
 * \param a_arrRing is Array of Ring Pointer
 * \return None
 */
void CLQManager::GetRingList(struct rte_ring **a_arrRing)
{
	rte_ring_list_dump(stdout);
	rte_ring_list_get(a_arrRing);	
}
