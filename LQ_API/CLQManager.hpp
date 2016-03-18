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

#include <rte_config.h>
#include "DbQuery.hpp"
#include "CQuery.hpp"

#include "global.h"

/*!
 * \class CLQManager
 * \brief CLQManager Class For LQ API
 */
class CLQManager
{
	public:
		//! Constructor
		CLQManager(int a_nNodeID, char *a_szProcName, char *a_szLogPath=NULL);
		//! Destructor
		~CLQManager();
		//! Initialize
		int Initialize(p_function_hash *a_pFunc = NULL);
		//! Wait Data
		int ReadWait ();	

		
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
		void ReadComplete();
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
		void InsertData();
		//! Commit Data to Queue
		int CommitData(int a_nIdx);
		//! Read Complete For Bulk Mode
		void ReadCompleteBulk();
		//####################################################//


	private:
		//################ String Buffer ################//
		//! Queue Name 을 만들기 위한 Buffer 
		char m_szBuffer[DEF_MEM_BUF_128];
		//! 여러개의 Segment 로 이루어진 데이터 즉, 큰 Size 의 Data 를 임시 저장하기 위한 Buffer
		char m_szJumboBuff[DEF_MEM_JUMBO];
		//! Log Path
		char m_szLogPath[DEF_MEM_BUF_256];
		//! Process name
		char m_szProcName[DEF_MEM_BUF_128];
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
		//! Mempool For To Use to Communicate with MRT
		struct rte_mempool *m_pstMemPool;
		//! Signal 을 수신하였을 때 인자값으로 넘어오는 Ring(Queue) 의 주소 값
		struct rte_ring *m_pstReadRing;
		//! Bulk Mode 가 아닐 때 사용되는 Memory Buffer Pointer
		struct rte_mbuf *m_pstCurMbuf; 
		//! Bulk Mode 에서 데이터를 읽어들이기 위한 Memory Buffer 
		struct rte_mbuf *m_pstReadMbuf[DEF_MAX_BULK];
		//! Bulk Mode 에서 데이터를 쓰기 위한 Memory Buffer 
		struct rte_mbuf *m_pstWriteMbuf[DEF_MAX_BULK];

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
		//! Read Ring Position
		uint32_t	m_unarrReadPosition[DEF_MAX_RING];
		//! Write Ring Position(Idx)
		uint32_t	m_unarrWritePosition[DEF_MAX_RING];
		//! Node ID
		int m_unNodeID;
		//! Backup Flag 
		bool m_bBackup;
		//! Read Complete Flag
		bool m_bReadComplete;
		//###############################################//

		//#################### Signal ###################//
		//! Structure of Signal, For Process to Signal Event
		sigset_t	m_stSigSet;
		siginfo_t	m_stSigInfo;
		//###############################################//

		//! Db Connector
		DbQuery	*m_pclsDbConn;

		//! Function For Hash
		p_function_hash *m_pfuncHash;

		//! Generate Query
		char *GetQuery(const char *a_szFmt, ...);
		//! Generate Q Name
		char *GetQName(const char *a_szWrite, const char *a_szRead, char a_cMultiType);
		//! Attach Ring
		int AttachRing(const char *a_szWrite, const char *a_szRead, int a_nType, char a_cMultiType);	
		//! Create Ring 
		int CreateRing(char *a_szName, struct rte_ring **a_stRing);	
		//! Insert Consumers Info 
		int InsertConsInfo(char *a_szName, pid_t a_stPID, struct rte_ring *a_pstRing);
		//! Insert Producer Info 
		int InsertProdInfo(char *a_szName, pid_t a_stPID, struct rte_ring *a_pstRing);
		//! Send RTS to Consumer Process
		int SendRTS(struct rte_ring *a_pstRing);
		//! Set Sleep Status in the Ring
		int SetSleepFlag(struct rte_ring *a_pstRing);
		//! Enqueue Data to Ring
		int EnqueueData(struct rte_ring *a_pstRing, int a_nCnt, int a_nIdx, bool a_bRestore, int a_nStartIdx);
};

#endif
