/*!
 * \file CLQManager.hpp
 * \brief CLQManager Class Header File
 * \author 이현재 (presentlee@ntels.com)
 * \date 2016.03.18 
 */

#ifndef _LQ_MGR_H_
#define _LQ_MGR_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <vector>
#include <string>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
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

#include "CQuery.hpp"
#include "MariaDB.hpp"
#include "FetchMaria.hpp"

#include "CConfig.hpp"

#include "CLQGlobal.h"

/*!
 * \class CLQManager
 * \brief CLQManager Class For LQ API
 */
class CLQManager
{
	public:
		//! Constructor
		CLQManager(char *a_szPkgName, char *a_szNodeType, char *a_szProcName, int a_nInstanceID, bool a_bBackup, bool a_bMSync, char *a_szLogPath=NULL);
		//! Destructor
		~CLQManager();
		//! Initialize
		int Initialize(int a_nCmdType, p_function_hash *a_pFunc = NULL);
		//! Wait Data
		int ReadWait ();	
		//! Get Error Msg
		char *GetErrorMsg();
		//! Get Write Queue Index
		int GetWriteQueueIndex(char *a_szProc);
		//! Get Read Queue Index
		int GetReadQueueIndex(char *a_szProc);
		//! Delete Queue
		int DeleteQueue(char *a_pszQueue);
		//! Create Ring 
		int CreateRing(char *a_szName, struct rte_ring **a_stRing, int a_nSize = DEF_DEFAULT_RING_COUNT);	
		//! Get Queue List For Monitoring
		void GetRingList(struct rte_ring **a_arrRing);
		//! Write Data (For Util)
		int WriteData( struct rte_ring *a_pstRing, char *a_pszData, int a_nSize );
		
		//################ One Data Function ################//
		//! Read a Data From Ring
		int ReadData( char **a_pszBuff );
		//! Write Data (Hash Function)
		int WriteDataHash( void *a_pArgs = NULL );
		//! Write Data (Index)
		int WriteData( int a_nIdx = 0 );
		//! Write Data (Hash Function)
		int WriteData( char *a_pszData, int a_nSize, int a_nIdx = 0);
		//! Write Data (Index)
		int WriteDataHash( char *a_pszData, int a_nSize, void *a_pArgs = NULL);
		//! Read Complete
		int ReadComplete();
		//! Free Read Data
		void FreeReadData();
		//###################################################//

		
		//################ Bulk Mode Function ################//
		//! Read several Data From Ring
		int ReadBulkData( int a_nCount );
		//! Read Next Memory Buffer (Bulk Mode 에서만 사용)
		int GetNext( char **a_pszBuff );
		//! Insert Data (Bulk Mode 에서만 사용)
		int InsertData( char *a_pszData, int a_nSize);
		//! Insert Data (Bulk Mode 에서만 사용)
		int InsertData();
		//! Commit Data to Queue
		int CommitData(int a_nIdx=0);
		//! Free Read Bulk Data
		void FreeReadBulkData();
		//####################################################//


		//################ Command Function ################//
		//! Init Command Ring
		int InitCommandRing(char *a_szProcName, int a_nInstanceID, int a_nType);
		//! Send Command
		int SendCommand(char *a_szProcName, int a_nInstanceID, char *a_pszData, int a_nSize);
		//! Receive Command
		int ReceiveCommand(char **a_pszBuff);
		//! Send Result of Command
		int SendCommandResult(char *a_pstData, int a_nSize);
		//##################################################//

		
	private:
		//################ String Buffer ################//
		//! Queue Name 을 만들기 위한 Buffer 
		char m_szBuffer[DEF_MEM_BUF_128];
		//! Error Log Buffer
		char m_szErrorMsg[DEF_MEM_BUF_1M];
		//! 여러개의 Segment 로 이루어진 데이터 즉, 큰 Size 의 Data 를 임시 저장하기 위한 Buffer
		char m_szJumboBuff[DEF_MEM_JUMBO];
		//! Log Path
		char m_szLogPath[DEF_MEM_BUF_256];
		//! Process name
		char m_szProcName[DEF_MEM_BUF_128];
		//! Node Type
		char m_szNodeType[DEF_MEM_BUF_128];
		//! PKG Name
		char m_szPkgName[DEF_MEM_BUF_128];
		//! Log Path Pointer
		char *m_pLogPath;
		//###############################################//

		//################## Structure ##################//
		//! Backup File Header
		BACKUP_INFO m_stBackupHead;
		//! Read Ring
		RING_INFO m_stReadRingInfo[DEF_MAX_RING];
		//! Write Ring
		RING_INFO m_stWriteRingInfo[DEF_MAX_RING];
		//! Mempool For Data
		struct rte_mempool *m_pstDataMemPool;
		//! Mempool For Command
		struct rte_mempool *m_pstCmdMemPool;
		//! Command 를 수신하기 위한 Ring
		struct rte_ring *m_pstCmdRcvRing;
		//! Command 및 결과를 송신하기 위한 Ring
		struct rte_ring *m_pstCmdSndRing;
		//! Signal 을 수신하였을 때 인자값으로 넘어오는 Ring(Queue) 의 주소 값
		struct rte_ring *m_pstReadRing;
		//! Bulk Mode 가 아닐 때 사용되는 Memory Buffer Pointer
		struct rte_mbuf *m_pstCurMbuf; 
		//! Bulk Mode 에서 데이터를 읽어들이기 위한 Memory Buffer 
		struct rte_mbuf *m_pstReadMbuf[DEF_MEM_BUF_1024];
		//! Bulk Mode 에서 데이터를 쓰기 위한 Memory Buffer 
		struct rte_mbuf *m_pstWriteMbuf[DEF_MEM_BUF_1024];

		//###############################################//

		//################### Integer ###################//
		//! Signal 을 수신하였을 때 인자값으로 넘어오는 Ring(Queue) 에서 현재 프로세스의 Consumer Index 정보
		uint8_t m_unReadIdx;
		//! Bulk Mode 에서 사용되는  m_pstReadMBuf 배열에서 현재 Index 위치
		uint8_t m_unCurReadMbufIdx;
		//! Bulk Mode 에서 사용되는  m_pstReadMBuf 배열의 Total Size
		uint8_t m_unTotReadMbufIdx;
		//! Bulk Mode 에서 사용되는  m_pstWriteMBuf 배열에서 현재 Index 위치
		uint8_t m_unCurWriteMbufIdx;
		//! Read Ring Current Count
		uint8_t	m_unReadRingCount;
		//! Write Ring Curret Count
		uint8_t m_unWriteRingCount;
		//! Instance ID
		int m_nInstanceID;
		//! Backup Flag 
		bool m_bBackup;
		//! Read Complete Flag
		bool m_bReadComplete;
		//! mmap Sync Flag
		bool m_bMsync;
		//###############################################//

		//#################### Signal ###################//
		//! Structure of Signal, For Process to Signal Event
		sigset_t	m_stSigSet;
		siginfo_t	m_stSigInfo;
		//###############################################//

		//! Db Connector
		DB *m_pclsDbConn;
		//! Global Config Class
		CConfig *m_pclsConfig;
		//! Function For Hash
		p_function_hash *m_pfuncHash;

		//! Set Error Msg
		void SetErrorMsg(const char *a_szFmt, ...);
		//! Generate Query
		char *GetQuery(const char *a_szFmt, ...);
		//! Generate Q Name
		char *GetQName(const char *a_szWrite, const char *a_szRead, char a_cMultiType);
		//! Attach Ring
		int AttachRing(const char *a_szWrite, const char *a_szRead, int a_nElemCnt, int a_nType, char a_cMultiType);	
		//! Insert Consumers Info 
		uint32_t InsertConsInfo(char *a_szName, int a_nInstanceID, pid_t a_stPID, struct rte_ring *a_pstRing);
		//! Insert Producer Info 
		uint32_t InsertProdInfo(char *a_szName, int a_nInstanceID, pid_t a_stPID, struct rte_ring *a_pstRing);
		//! Send RTS to Consumer Process
		int SendRTS(struct rte_ring *a_pstRing);
		//! Send RTS for Command
		int SendRTSCommand(struct rte_ring *a_pstRing);
		//! Set Sleep Status in the Ring
		int SetSleepFlag(struct rte_ring *a_pstRing);
		//! Init Backup Files
		int InitBackupFile( struct rte_ring *a_pstRing, char *a_szFileName, int a_nType, int a_nIdx );
		//! Backup Write Data
		int BackupWriteData( struct rte_ring *a_pstRing, int a_nIdx );
		//! Backup Read Data
		int BackupReadData( struct rte_ring *a_pstRing, int a_nIdx );
		//! Insert Data (Bulk Mode 에서만 사용)
		int InsertCommandData( char *a_pszData, int a_nSize);
		//! Enqueue Data to Ring
		int EnqueueData(struct rte_ring *a_pstRing, int a_nCnt, int a_nIdx);
};

#endif
