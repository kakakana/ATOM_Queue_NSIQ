/*-
 *   BSD LICENSE
 * 
 *   Copyright(c) 2010-2012 Intel Corporation. All rights reserved.
 *   All rights reserved.
 * 
 *   Redistribution and use in source and binary forms, with or without 
 *   modification, are permitted provided that the following conditions 
 *   are met:
 * 
 *     * Redistributions of source code must retain the above copyright 
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright 
 *       notice, this list of conditions and the following disclaimer in 
 *       the documentation and/or other materials provided with the 
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its 
 *       contributors may be used to endorse or promote products derived 
 *       from this software without specific prior written permission.
 * 
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/queue.h>

#if 0
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
#endif

#include <unistd.h>
#include <fcntl.h>

#include "CLQManager.hpp"
#include "CConfig.hpp"

#define CLIENT_QUEUE_RING_SIZE 0x40000  /* 262,144 */
#define CLIENT_RXQ_NAME "CAP_CLIENT_%u_RX"


typedef struct _test
{
    uint32_t	aa ;
}TEST;

uint32_t cnt = 0;
uint32_t sig_seq = 0;
uint32_t free_cnt = 0;
char g_szTimeStr[128];
CConfig *g_pclsConfig = NULL;

void print_help_msg()
{
	printf("\n\n\n");
	printf("[help] ================================================================\n");
	printf("   -i [Queue Name ] : Init Queue mode\n");
	printf("   -d [Queue Name ] : Delete Queue mode\n");
	printf("   -n [Queue Name ] : Name of Delete or Init Queue \n");
	printf("   -v [Queue Name ] : Queue Dump \n");
	printf("   -p [File Name  ] : Backup File Dump \n");
	printf("   -x [Start Index] : Start Index of Backup File Dump \n");
	printf("   -y [End Index  ] : End Index of Backup File Dump \n");
	printf("   -f [Log Path   ] : Log File Path\n");
	printf("   -m : Queue monitoring mode\n");
	printf("  ex)  ./TEST_APP -p FLC01 -c 1000 -s 512 -b 5 -k sync\n");
	printf("=======================================================================\n");
	printf("\n\n\n");
}

char *time2str(time_t *ptime)
{
    struct tm rt;
    strftime(g_szTimeStr, sizeof(g_szTimeStr), "%Y-%m-%d %H:%M:%S", localtime_r(ptime, &rt));
    return g_szTimeStr;
}

int dump_file(CLQManager *a_pclsCLQ, char *a_pszQName, uint32_t a_unStartIdx, uint32_t a_unEndIdx)
{
	char szFileName[1024];
	memset(szFileName, 0x00, sizeof(szFileName));
	sprintf(szFileName, "%s/%s.bak", g_pclsConfig->GetConfigValue("QUEUE", "BACKUP_PATH"), a_pszQName);

	int fd = 0;
	
	struct stat bk_file_stat;

	void *pMMap = NULL;
	BACKUP_INFO *pstBackup = NULL;
	DATA_BUFFER *pstData = NULL;

	uint32_t	unConsMax = 0;
	uint32_t	unProdMax = 0;
	uint32_t	unFileIdx = 0;

	uint32_t	unStartIdx = a_unStartIdx;
	uint32_t	unEndIdx = a_unEndIdx;

	fd = open(szFileName, O_RDWR, 0660);
	if(fd == -1)
	{
		printf("Backup File (%s) Open Error %d\n", szFileName, errno);
		return -1;
	}

	if(fstat(fd, &bk_file_stat) < 0)
	{
		printf("fstat function failed\n");
		return -1;
	}

	if(bk_file_stat.st_size == 0)
	{
		printf("File Size Error\n");
		return -1;
	}

	pMMap = mmap(NULL, bk_file_stat.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if(pMMap == MAP_FAILED)
	{
		printf("MMap function failed %d\n", errno);
		return -1;
	}

	pstBackup = (BACKUP_INFO*)pMMap;

	printf("cons Count %u\n", pstBackup->unConsCnt);
	printf("prod Count %u\n", pstBackup->unProdCnt);
	printf("mask       %x\n", pstBackup->unMask);

	for(int i = 0; i < pstBackup->unConsCnt; i++)
	{
		if(unConsMax < pstBackup->stConsInfo[i].unTail)
			unConsMax = pstBackup->stConsInfo[i].unTail;

		printf("Cons Name %s, Tail %u\n", pstBackup->stConsInfo[i].strName, pstBackup->stConsInfo[i].unTail);
	}

	for(int i = 0; i < pstBackup->unProdCnt; i++)
	{
		if(unProdMax < pstBackup->stProdInfo[i].unTail)
			unProdMax = pstBackup->stProdInfo[i].unTail;

		printf("Prod Name %s, Tail %u\n", pstBackup->stProdInfo[i].strName, pstBackup->stProdInfo[i].unTail);
	}


	printf("cons max %u\n", unConsMax);
	printf("prod max %u\n", unProdMax);
	pstData = (DATA_BUFFER*)(pstBackup + 1);	

	if(a_unStartIdx == 0)
		unStartIdx = unConsMax;
	if(a_unEndIdx == 0)
		unEndIdx = unProdMax;

	if(unEndIdx > unProdMax)
		unEndIdx = unProdMax;
			
	printf("start Idx %u\n", unStartIdx);
	printf("end Idx %u\n", unEndIdx);

	if(unEndIdx < unStartIdx)
	{
		printf("invalid start %u, end position %u\n", unEndIdx, unStartIdx);
		return -1;
	}

	for(uint32_t i = unStartIdx ; i < unEndIdx ; i++)
	{
		unFileIdx = i & pstBackup->unMask;	
		printf("idx : [%5u], data_len : [%5u], Data : %s\n"
					, unFileIdx, pstData[unFileIdx].unBuffLen, pstData[unFileIdx].szBuff);
	}


	if(munmap(pMMap, bk_file_stat.st_size) < 0)
	{
		printf("munmap failed %d\n", errno);
		return -1;
	}

	return 0;	
}

void dump_ring(CLQManager *a_pclsCLQ, char *a_pszQName, uint32_t a_unStartIdx, uint32_t a_unEndIdx)
{
	struct rte_ring *pstRing = NULL;
	struct rte_mbuf *m = NULL;
	int ret = 0;
	uint32_t unMask = 0;
	uint32_t unStartIdx = 0;
	uint32_t unEndIdx = 0;
	uint32_t unIdx = 0;

	ret = a_pclsCLQ->CreateRing(a_pszQName, &pstRing);
	if(ret != E_Q_EXIST)
	{
		printf("There is no Queue (%s) \n", a_pszQName);
		if(ret == 0)
		{
			a_pclsCLQ->DeleteQueue(a_pszQName);
		}
	}

	if(pstRing != NULL)
	{
		unMask = pstRing->prod.mask;
		unStartIdx = pstRing->cons.tail;
		unEndIdx = pstRing->prod.tail;

		if(a_unEndIdx > unEndIdx)
		{
			printf("Invalid End idx %u\n", a_unEndIdx);
			return ;
		}

		if(a_unEndIdx < a_unStartIdx)
		{
			printf("Invalid Start Idx %u, End Idx %u\n", a_unStartIdx, a_unEndIdx);
			return ;
		}

		if(a_unStartIdx)
			unStartIdx = a_unStartIdx;
		if(a_unEndIdx)
			unEndIdx = a_unEndIdx;

		printf("Start Idx %u, End Idx %u\n", unStartIdx, unEndIdx);

		for(uint32_t i = unStartIdx; i < unEndIdx ; i++)
		{
			unIdx = i & unMask ;
			m = (struct rte_mbuf*)pstRing->ring[unIdx];
			printf("idx : [%8u], total_len : [%5u], data_len : [%5u], seg_cnt : [%2u], Data : %s\n"
							, i
							, rte_pktmbuf_pkt_len(m)
							, rte_pktmbuf_data_len(m)
							, m->nb_segs
							, rte_pktmbuf_mtod(m, char*)
					);
		}
	}

	return ;
}

void queue_monitoring(CLQManager *a_pclsCLQ)
{
	float fUsage = 0;
	struct rte_ring *arrRing[RTE_MAX_MEMZONE];
	memset(arrRing, 0x00 , sizeof(arrRing));
	

	fprintf(stdout, "test test test\n");
	a_pclsCLQ->GetRingList(arrRing);

	for(int i = 1; i < RTE_MAX_MEMZONE ; i++)
	{
		if(arrRing[i] == NULL)
			break;

		if( strncmp(arrRing[i]->name, "MP", 2) == 0)
		{
			fUsage = rte_ring_free_count(arrRing[i]) / (float)arrRing[i]->prod.size * 100;
			printf("Memory Pool : %25s  /  Usage %3.2f\n", arrRing[i]->name, fUsage);
		}
		else
		{
			fUsage = rte_ring_count(arrRing[i]) / (float)arrRing[i]->prod.size * 100;
			printf("Queue       : %25s  /  Usage %3.2f\n", arrRing[i]->name, fUsage);
		}

	}

	return ;
}

int delete_queue(CLQManager *a_pclsCLQ, char *a_pszQName)
{
	int ret = 0;

	ret = a_pclsCLQ->DeleteQueue(a_pszQName);
	if(ret < 0)
	{
		printf("Delete Queue (%s) Failed ErrMsg(%s) \n", a_pszQName, a_pclsCLQ->GetErrorMsg());
		return -1;
	}

	return ret;
}

int init_queue(CLQManager *a_pclsCLQ, char *a_pszQName, struct rte_ring **a_pstRing)
{
	int ret = 0;

	ret = delete_queue(a_pclsCLQ, a_pszQName);

	ret = a_pclsCLQ->CreateRing(a_pszQName, a_pstRing, (uint32_t)ret);
	if(ret < 0)
	{
		printf("Init Queue (%s) Failed ErrMsg(%s) \n", a_pszQName, a_pclsCLQ->GetErrorMsg());
		return -1;
	}
	return 0;
}

int restore_backup(CLQManager *a_pclsCLQ, char *a_pszQName)
{
	int ret = 0;

	struct rte_ring *pstRing = NULL;

	char szFileName[1024];
	memset(szFileName, 0x00, sizeof(szFileName));
	sprintf(szFileName, "%s/%s.bak", g_pclsConfig->GetConfigValue("QUEUE", "BACKUP_PATH"), a_pszQName);

	int fd = 0;
	
	struct stat bk_file_stat;

	void *pMMap = NULL;
	BACKUP_INFO *pstBackup = NULL;
	DATA_BUFFER *pstData = NULL;

	uint32_t	unConsMax = 0;
	uint32_t	unProdMax = 0;
	uint32_t	unFileIdx = 0;

	fd = open(szFileName, O_RDWR, 0660);
	if(fd == -1)
	{
		printf("Backup File (%s) Open Error %d\n", szFileName, errno);
		return -1;
	}

	if(fstat(fd, &bk_file_stat) < 0)
	{
		printf("fstat function failed\n");
		return -1;
	}

	if(bk_file_stat.st_size == 0)
	{
		printf("File Size Error\n");
		return -1;
	}

	pMMap = mmap(NULL, bk_file_stat.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if(pMMap == MAP_FAILED)
	{
		printf("MMap function failed %d\n", errno);
		return -1;
	}

	pstBackup = (BACKUP_INFO*)pMMap;

	printf("cons Count %u\n", pstBackup->unConsCnt);
	printf("prod Count %u\n", pstBackup->unProdCnt);
	printf("mask       %x\n", pstBackup->unMask);

	for(int i = 0; i < pstBackup->unConsCnt; i++)
	{
		if(unConsMax < pstBackup->stConsInfo[i].unTail)
			unConsMax = pstBackup->stConsInfo[i].unTail;

		printf("Cons Name %s, Tail %u\n", pstBackup->stConsInfo[i].strName, pstBackup->stConsInfo[i].unTail);
	}

	for(int i = 0; i < pstBackup->unProdCnt; i++)
	{
		if(unProdMax < pstBackup->stProdInfo[i].unTail)
			unProdMax = pstBackup->stProdInfo[i].unTail;

		printf("Prod Name %s, Tail %u\n", pstBackup->stProdInfo[i].strName, pstBackup->stProdInfo[i].unTail);
	}

	if(init_queue(a_pclsCLQ, a_pszQName, &pstRing) < 0)
	{
		printf("Queue Init Failed [%s]\n", a_pclsCLQ->GetErrorMsg());
		return -1;
	}

	if(pstRing == NULL)
	{
		printf("Queue is NULL\n");
		return -1;
	}

	printf("cons max %u\n", unConsMax);
	printf("prod max %u\n", unProdMax);
	pstData = (DATA_BUFFER*)(pstBackup + 1);	
	for(uint32_t i = unConsMax ; i < unProdMax ; i++)
	{
		unFileIdx = i & pstBackup->unMask;	
	#if 1
		ret = a_pclsCLQ->WriteData(pstRing, pstData[unFileIdx].szBuff, pstData[unFileIdx].unBuffLen);
		if(ret < 0)
		{
			printf("1 errno : %d, errmsg : %s\n", ret, a_pclsCLQ->GetErrorMsg());
			return -1;
		}

	#endif
		printf("Data [%u], %s\n", unFileIdx, pstData[unFileIdx].szBuff);
	}


	if(munmap(pMMap, bk_file_stat.st_size) < 0)
	{
		printf("munmap failed %d\n", errno);
		return -1;
	}

	return 0;	
}


int main(int argc, char *args[])
{

	uint32_t ret;

	uint32_t unStart = 0;
	uint32_t unEnd = 0;

	int param_opt = 0;
	char *pszQName = NULL;
	char *pszLogPath = NULL;
	bool bDel = false;
	bool bInit = false;
	bool bMonitor = false;
	bool bRestore = false;
	bool bDumpFile = false;
	bool bDumpRing = false;
	
	if(argc < 2)
	{
		print_help_msg();
		return 0;
	}

	while( -1 != (param_opt = getopt(argc, args, "hmr:n:d:i:f:p:v:x:y:")))
	{
		switch(param_opt)
		{
			case 'h' :
				print_help_msg();
				return 0;
			case 'd' :
				bDel = true;
				pszQName = optarg;
				printf("We Will Delete (%s) Queue\n", pszQName);
				break;
			case 'i' :
				bInit = true;
				pszQName = optarg;
				printf("We Will Init (%s) Queue, Data Will be deleted\n", pszQName);
				break;
			case 'f' :
				pszLogPath = optarg;
				printf("File Log Path %s\n", pszLogPath);
				break;
			case 'm' :
				bMonitor = true;
				printf("Monitoring Mode on \n");
				break;
			case 'r' :
				bRestore = true;
				pszQName = optarg;
				printf("File %s Restore Start\n", pszQName);
			case 'p' :
				bDumpFile = true;
				pszQName = optarg;
				printf("File (%s) dump is Start\n", pszQName);
				break;
			case 'v' :
				bDumpRing = true;
				pszQName = optarg;
				printf("Queue (%s) dump is Start\n", pszQName);
				break;
			case 'x' :
				unStart = atoi(optarg);
				printf("Start Idx %u\n", unStart);
				break;
			case 'y' :
				unEnd = atoi(optarg);
				printf("End Idx %u\n", unEnd);
				break;
			default :
				break;

		}			
	}

	//Init CLQManager
	//Arguments : NODE ID, Process Name, Process Instance ID, Backup Flag, MSync Flag, Log Path(생략 가능)
	CLQManager	*m_pclsCLQ = new CLQManager("OFCS", "AP", NULL, 0, 0, 0 , pszLogPath);
	if(m_pclsCLQ->Initialize(DEF_CMD_TYPE_UTIL) < 0)
	{
		printf("CLQManager Init Failed ErrMsg[%s]\n", m_pclsCLQ->GetErrorMsg());
		return -1;
	}

	g_pclsConfig = new CConfig();
	if(g_pclsConfig->Initialize() < 0)
	{
		printf("Cannot Init CConfig\n");
		return -1;
	}

	if(bDel)
	{
		printf("\n\n================ Queue (%s) Delete Start ================\n", pszQName);
		ret = delete_queue(m_pclsCLQ, pszQName);
		if(ret <0)
		{
			return -1;
		}

		printf("%s Queue Deleted\n", pszQName);
		printf("=========================================================\n\n");

		return 0;
	}

	if(bInit)
	{
		printf("\n\n================ Queue (%s) Init Start ================\n", pszQName);
		struct rte_ring *pstRing;
		ret = init_queue(m_pclsCLQ, pszQName, &pstRing);
		if(ret < 0)
		{
			return -1;
		}
		printf("%s Queue Initialized\n", pszQName);
		printf("======================================================\n\n");
		return 0;
	}

	if(bMonitor)
	{
		printf("\n\n================ Monitoring Start ================\n");
		queue_monitoring(m_pclsCLQ);
		printf("==================================================\n\n");
		return 0;
	}

	if(bRestore)
	{
		printf("\n\n================ Queue (%s) Restore Start ================\n", pszQName);
		ret = restore_backup(m_pclsCLQ, pszQName);
		if(ret < 0)
		{
			return -1;
		}
		printf("==========================================================\n\n");
		return 0;
	}

	if(bDumpRing)
	{
		printf("\n\n================ Queue (%s) Dump Start ================\n", pszQName);
		dump_ring(m_pclsCLQ, pszQName, unStart, unEnd);
		
		printf("\n\n=======================================================\n\n");
	}

	if(bDumpFile)
	{
		printf("\n\n================ File (%s) Dump Start ================\n", pszQName);
		dump_file(m_pclsCLQ, pszQName, unStart, unEnd); 
		printf("\n\n=======================================================\n\n");
	}

	delete m_pclsCLQ;
	return 0;
}
