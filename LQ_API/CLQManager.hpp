/*!
 * \file CLQManager.hpp
 * \brief CLQManager Class Header File
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
		int ReadWait ( struct rte_ring **a_pRing );	
		//! Read a Data From Ring
		int ReadData( struct rte_ring *a_pstRing, void **a_pBuff );
		//! Read several Data From Ring
		int ReadBulkData( struct rte_ring *a_pstRing, void **a_pBuff, int a_nCount );
		//! Write Data
		int WriteData(void **a_pstData, int a_nCount, void *a_pArgs = NULL);
		//! Get Memory Buffer From Memory Pool
		int GetDataBuffer(int a_nCount, void **a_pBuff);
		//! Set Sleep Status in the Ring
		int SetSleepFlag(struct rte_ring *a_pstRing);
	private:
		//! Db Connector
		DbQuery	*m_pclsDbConn;
		//! Buffer 
		char m_szBuffer[DEF_MEM_BUF_2048];
		//! Backup Flag 
		bool m_bBackup;
		//! Log Path
		char m_szLogPath[DEF_MEM_BUF_256];
		//! Process name
		char m_szProcName[DEF_MEM_BUF_128];
		//! Node ID
		int m_unNodeID;
		//! Backup File Header
		BACKUP_INFO m_stBackupHead;

		//! Log Path Pointer
		char *m_pLogPath;
		//! Read Ring Current Count
		uint8_t	m_unReadRingCount;
		//! Write Ring Curret Count
		uint8_t m_unWriteRingCount;
		//! Read Ring
		RING_INFO m_stReadRingInfo[DEF_MAX_RING];
		//! Read Ring Position
		uint32_t	m_unarrReadPosition[DEF_MAX_RING];
		//! Write Ring
		RING_INFO m_stWriteRingInfo[DEF_MAX_RING];
		//! Write Ring Position(Idx)
		uint32_t	m_unarrWritePosition[DEF_MAX_RING];
		//! Cons Ring For Insert Consumers Info ( Name, PID )
		struct rte_ring *m_pstConsRing;
		
		//! Mempool For To Use to Communicate with MRT
		struct rte_mempool *m_pstMemPool;
		//! Ring For To Use to Communicate with MRT
		struct rte_ring *m_pstRingInfo;

		//! Structure of Signal, For Process to Signal Event
		sigset_t	m_stSigSet;
		siginfo_t	m_stSigInfo;

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
};

#endif
