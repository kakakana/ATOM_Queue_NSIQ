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
#include <time.h>
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

#include "data.h"
#include "CLQManager.hpp"

#define CLIENT_QUEUE_RING_SIZE 0x40000  /* 262,144 */
#define CLIENT_RXQ_NAME "CAP_CLIENT_%u_RX"

typedef struct _test
{
    char	aa[40] ;
}TEST;

uint32_t cnt = 0;
uint32_t free_cnt = 0;
char g_szTimeStr[128];
char g_szBuffer[1024*1024];

void print_help_msg()
{
    printf("\n\n\n");
    printf("[help] ================================================================\n");
    printf("   -p [ProcessName] : Cmd Receive Process Name\n");
    printf("   -i [Instance Id] : Instance ID of Cmd Receive Process \n");
    printf("   -c [count      ] : Input Send Count ( set to 0 is Loop )\n");
    printf("   -s [size(byte) ] : Input Size of each data\n");
    printf("   -f [Log Path   ] : Log File Path\n");
    printf("  ex)  ./TEST_APP -p FLC01 -c 1000 -s 512 -i 1\n");
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
		printf("%s Write Count, %u, total %u\n", time2str(&tCur), cnt - old_cnt, cnt);
//        RTE_LOG(INFO, EAL, "read Count %u , free_cnt %u\n", cnt, free_cnt);
        old_cnt = cnt;
        sleep(1);
    }

    return arg;
}

void get_data(uint32_t a_unDataSize, int a_nCount)
{
	switch(a_unDataSize)
	{
		case 128:
			memcpy(g_szBuffer, DEF_STR_128, a_unDataSize);
			break;
		case 256:
			memcpy(g_szBuffer, DEF_STR_256, a_unDataSize);
			break;
		case 512:
			memcpy(g_szBuffer, DEF_STR_512, a_unDataSize);
			break;
		case 1024:
			memcpy(g_szBuffer, DEF_STR_1024, a_unDataSize);
			break;
		case 2048:
			memcpy(g_szBuffer, DEF_STR_2048, a_unDataSize);
			break;
		default :
			return;

	}
	sprintf(g_szBuffer, "TEST_%u", a_nCount);
	g_szBuffer[strlen(g_szBuffer)] = 0x20;
}

//Process one Data
int process_data (CLQManager *a_pclsCLQ, char *a_pszProc, int a_nInstanceID, uint32_t a_unSendCount, uint32_t a_unDataSize)
{
	int ret = 0;

	while(1)
	{
		get_data(a_unDataSize, cnt++);
		//Write a Data
		//arguments : Data Buffer Pointer , Data Size, Write Queue Index (생략 가능 기본값 0)
		ret = a_pclsCLQ->SendCommand( a_pszProc, a_nInstanceID, (char*)g_szBuffer, a_unDataSize);
		if(ret != 0)
		{
			printf("fail\n");
			return -1;
			continue;
		}

		if( (a_unSendCount != 0)  && (cnt % a_unSendCount == 0) )
			return 0;

	}

	return 0;
}

//Process Command
int process_command (CLQManager *a_pclsCLQ)
{
	int ret = 0;
	char *test = NULL;
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
	}

	return 0;
}


int main(int argc, char *args[])
{


	int ret;
	int param_opt = 0;
	int nInstanceId = 0;
	char *pszProcName = NULL;
	char *pszLogPath = NULL;
	uint32_t unSendCount = 0;
	uint32_t unDataSize = 0;
	bool bBackup = false;
	bool bSync = false;

	memset(g_szBuffer, 0x00, sizeof(g_szBuffer));

	if(argc < 2)
	{
		print_help_msg();
		return 0;
	}
											

	while( -1 != (param_opt = getopt(argc, args, "hp:c:b:k:s:f:w:i:")))
    {
        switch(param_opt)
        {
            case 'h' :
                print_help_msg();
				return 0;
                break;
            case 'p' :
                pszProcName =  optarg ;
                printf("process Name %s\n", pszProcName);
                break;
            case 'c' :
                unSendCount = atoi(optarg);
                printf("Send Count %u\n", unSendCount);
                break;
            case 's' :
                unDataSize = atoi(optarg);
                printf("Send Data Size %u\n", unDataSize);
                break;
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
	//Arguments : NODE ID, Process Name, Process Instance ID, Backup Flag, MSync Flag, Log Path(생략 가능)
	CLQManager	*m_pclsCLQ = new CLQManager("OFCS", "AP", (char*)"TEST", nInstanceId, bBackup, bSync, pszLogPath);
	if(m_pclsCLQ->Initialize(DEF_CMD_TYPE_SEND) < 0)
	{
		printf("CLQManager Init Failed ErrMsg : %s\n", m_pclsCLQ->GetErrorMsg());
		return -1;
	}


	//Init Stat Thread
#if 1
	pthread_t thread_id;
	ret = pthread_create(&thread_id, NULL, &thread_start, NULL);

	if(ret != 0)
	{
		printf("Thread create failed\n");
		return -1;
	}
									
#endif

#if 1
	while(1)
	{
		//one data mode
		ret = process_data(m_pclsCLQ, pszProcName, nInstanceId, unSendCount, unDataSize);
		ret = m_pclsCLQ->ReadWait();
		switch(ret)
		{
			case DEF_SIG_COMMAND:
				process_command(m_pclsCLQ);
				sleep(1);
				break;
			default :
				break;
		}
	}

#endif
	RTE_LOG(INFO, EAL, "Total Send Count %u\n", cnt);

	delete m_pclsCLQ;

	return 0;
}
