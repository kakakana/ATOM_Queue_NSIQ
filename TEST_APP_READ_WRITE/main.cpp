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

void print_help_msg()
{
	printf("\n\n\n");
	printf("[help] ================================================================\n");
	printf("   -p [ProcessName] : Input Process Name\n");
//	printf("   -c [count      ] : Input Send Count\n");
//	printf("   -s [size(byte) ] : Input Size of each data\n");
	printf("   -b [count      ] : Input Bulk Count (bulk mode on)\n");
	printf("   -k [sync/async ] : Select Sync mode(sync/async) (backup mode on)\n");
	printf("   -f [Log Path   ] : Log File Path\n");
	printf("   -i [Instance Id] : Instance ID of Process \n");
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

static void *
thread_start(void *arg)
{
    uint32_t old_cnt = 0;
	time_t	tCur;
    while(1)
    {
		time(&tCur);
		RTE_LOG(INFO, EAL, "%s Read Count, %u, Write Count %u\n", time2str(&tCur), cnt - old_cnt, free_cnt);
//        RTE_LOG(INFO, EAL, "read Count %u , free_cnt %u\n", cnt, free_cnt);
        old_cnt = cnt;
        sleep(1);
    }

    return arg;
}

//Process Bulk Data
int process_bulk_data (CLQManager *a_pclsCLQ, int a_nBulkCount)
{
	int ret = 0;
	TEST *test;
	while(1)
	{
		//Read Bulk Data
		ret = a_pclsCLQ->ReadBulkData(a_nBulkCount);
		if(ret < 0)
			return -1;

		while(a_pclsCLQ->GetNext((char**)&test) > 0)
		{
			cnt++;
			//Insert one Data (bypass)
			//InsertData( char* data, int size) <-- New Data Insert
			ret = a_pclsCLQ->InsertData();
			if(ret < 0)
			{
				if(ret == -E_Q_NOSPC)
					continue;

				printf("insert buffer is full\n");
				return -1;
			}
		}

		//Commit Data
		ret = a_pclsCLQ->CommitData();
		if(ret < 0)
		{
			if(ret == -E_Q_NOSPC)
				continue;

			printf("Commit Data is Failed ErrMsg[%s]\n", a_pclsCLQ->GetErrorMsg());
			return -1;
		}

		//Read Complete
		a_pclsCLQ->ReadComplete();

		if(ret == 0)
			free_cnt += a_nBulkCount;
		else
		{
			printf("Bulk Insert Failed ErrMsg[%s]\n", a_pclsCLQ->GetErrorMsg());
			return -1;	
		}

		//Memory 의 재사용을 위해서 Free 함수를 호출하지 않는다.

	}


}

//Process One Data
int process_data (CLQManager *a_pclsCLQ)
{
	int ret = 0;
	TEST *test = new TEST;


		
	while(1)
	{
		//Read one Data
		ret = a_pclsCLQ->ReadData( (char**)&test);
		if(ret < 0)
		{
			return -1;
		}

		cnt ++;
		//Write one Data (bypass)
		//WriteData(char *data, int size) <-- Write new Data
		ret = a_pclsCLQ->WriteData();
		if(ret < 0)
		{
			if(ret == -E_Q_NOSPC)
				continue;

			printf("WriteData Failed\n");
			return -1;
		}

		//Read Complete
		a_pclsCLQ->ReadComplete();

		//Memory 의 재사용을 위해서 Free 함수를 호출하지 않는다.
		free_cnt++;
	}

	return 0;
}

//Process Command
int process_command (CLQManager *a_pclsCLQ, char *a_pszProc, int a_nInstanceId)
{
	int ret = 0;
	char *test = NULL;
	char test_data[50];


	while(1)
	{
		//Read Command 
		ret = a_pclsCLQ->ReceiveCommand(&test);
		if(ret < 0)
		{
			return -1;
		}
		printf("Command %s\n", test);
		a_pclsCLQ->FreeReadData();

		memset(test_data, 0x00, sizeof(test_data));
		sprintf(test_data, "%s_%d %d Signal Result OK", a_pszProc, a_nInstanceId, sig_seq);

		sig_seq ++;
		ret = a_pclsCLQ->SendCommandResult(test_data, strlen(test_data));
		if(ret < 0)
		{
			printf("Send Command Result Failed (%s)\n", a_pclsCLQ->GetErrorMsg());
			return -1;
		}

	}
	
	return 0;
}

int main(int argc, char *args[])
{

	int ret;
	int param_opt = 0;
	int nInstanceId = 0;
	char *pszProcName = NULL;
	char *pszSyncMode = NULL;
	char *pszLogPath = NULL;
	bool bBulk = false;
	int nBulkCount = 0;
	bool bBackup = false;
	bool bSync = false;
	
	if(argc < 2)
	{
		print_help_msg();
		return 0;
	}

	while( -1 != (param_opt = getopt(argc, args, "hp:b:k:f:i:")))
	{
		switch(param_opt)
		{
			case 'h' :
				print_help_msg();
				return 0;
			case 'p' :
				pszProcName =  optarg ;
				printf("process Name %s\n", pszProcName);
				break;
//			case 'c' :
//				unSendCount = atoi(optarg);
//				printf("Send Count %u\n", unSendCount);
//				break;
			case 'b' :
				bBulk = true;
				nBulkCount = atoi(optarg);
				printf("Bulk Mode On, Count %d\n", nBulkCount);
				break;
			case 'k' :
				bBackup = true;
				pszSyncMode = optarg;
				if(strncmp (pszSyncMode, "sync", strlen("sync")) == 0)
				{
					bSync = true;
				}

				printf("Backup on Sync Mode[%s] On\n", pszSyncMode);

				break;
//			case 's' :
//				unDataSize = atoi(optarg);
//				printf("Send Data Size %u\n", unDataSize);
//				break;
			case 'f' :
				pszLogPath = optarg;
				printf("File Log Path %s\n", pszLogPath);
				break;
			case 'i' :
				nInstanceId = atoi(optarg);
				printf("Instance ID %d\n", nInstanceId);
				break;
															
			default :
				break;

		}			
	}

	//Init CLQManager
	//Arguments : NODE ID, Process Name, Process Instance ID, Backup Flag, MSync Flag, Log Path(삾]략 ꯾@능)
	CLQManager	*m_pclsCLQ = new CLQManager("OFCS", "AP", pszProcName, nInstanceId, bBackup,  bSync , pszLogPath);
	if(m_pclsCLQ->Initialize(DEF_CMD_TYPE_RECV) < 0)
	{
		printf("CLQManager Init Failed\n");
		return -1;
	}

	//Init Stat Thread
#if 1
	pthread_t thread_id;
	ret = pthread_create(&thread_id, NULL, &thread_start, NULL);

	if(ret != 0)
	{
		RTE_LOG(INFO, EAL, "Thread create failed\n");
		return -1;
	}
									
#endif

	while(1)
	{
		ret = m_pclsCLQ->ReadWait();
		switch(ret)
		{
			case DEF_SIG_DATA :
				if(bBulk)
				{
					process_bulk_data(m_pclsCLQ, nBulkCount);
				}
				else
				{
					process_data(m_pclsCLQ);
				}
				break;
			case DEF_SIG_COMMAND :
				process_command(m_pclsCLQ, pszProcName, nInstanceId);
				break;
			default:
				break;
			}

	}


	return 0;
}
