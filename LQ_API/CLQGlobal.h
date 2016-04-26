/*!
 * \addtogroup LQ-API
 * \author 이현재(presentlee@ntels.com)
 * \date 2016.03.18
 * \description
 * LQ API 는 기존 DPDK Library 에서 Ring 의 구조만을 떼어내어서
 * 프로세스간 데이터 혹은 명령 송수신에 도움을 주기 위하여 작성
 */
#ifndef _LQ_GLOBAL_H_
#define _LQ_GLOBAL_H_

//################ Define Error Code ################//
//! Config Error
#define E_Q_NO_CONFIG	E_RTE_NO_CONFIG		//1002
//! Secondary Process Can't Create Memory Pool 
#define E_Q_SECONDARY	E_RTE_SECONDARY		//1001
//! The process does not have permission to send the RTS
#define E_Q_PERM		EPERM  //1
//! Required Entry, File Open Error 
#define E_Q_NOENT		ENOENT  //2
//! No such Process
#define E_Q_SRCH		ESRCH	//3
//! Try again
#define E_Q_AGAIN		EAGAIN //11
//! Out of Memory 
#define E_Q_NOMEM		ENOMEM	//12
//! Exist Queue 
#define E_Q_EXIST		EEXIST	//17
//! Invalid Arguments 
#define E_Q_INVAL		EINVAL	//22
//! There is no space for data 
#define E_Q_NOSPC		ENOSPC	//28
//! DB Connection Error
#define E_Q_DB_FAIL		2000
//! Mmap Function Error
#define E_Q_MMAP		2001
//! File Locking Error
#define E_Q_FLOCK		2002
//! File Truncate Error
#define E_Q_TRUN		2003
//! Msync Function Error
#define E_Q_MSYNC		2004


//###################################################//

//! Define Memory Buffer 32 byte
#define DEF_MEM_BUF_32		32

//! Define Memory Buffer 64 byte
#define DEF_MEM_BUF_64		64

//! Define Memory Buffer 128 byte
#define DEF_MEM_BUF_128		128

//! Define Memory Buffer 256 byte
#define DEF_MEM_BUF_256		256

//! Define Memory Buffer 512 byte
#define DEF_MEM_BUF_512		512

//! Define Memory Buffer 1024 byte
#define DEF_MEM_BUF_1024	1024

//! Define Memory Buffer 2048 byte
#define DEF_MEM_BUF_2048	2048

//! Define Memory Buffer 1 Mega byte
#define DEF_MEM_BUF_1M (DEF_MEM_BUF_1024 * DEF_MEM_BUF_1024)

//! Define Jumbo Memory Buffer Size
#define DEF_MEM_JUMBO DEF_MEM_BUF_1M

//! Define Max Attach Ring Count
#define DEF_MAX_RING	10

//! Define Default Ring Element Count
#define DEF_DEFAULT_RING_COUNT	0x40000

//! Define Memory Pool Name
/*!
 * Memory Pool Name For to Use in Application
 */
#define DEF_BASE_MEMORY_POOL_NAME    "BASE_MEMORY_POOL"

//! Define Command Memory Pool Name
/*!
 * Memory Pool Name For to Use in Application
 */
#define DEF_CMD_MEMORY_POOL_NAME    "COMMAND_MEMORY_POOL"

//! Define timeout Count
#define DEF_TIME_OUT_COUNT	20

//! Define usleep Time
#define DEF_USLEEP_TIME	 300000

//! Define Write Ring Type 
#define DEF_RING_TYPE_READ	0

//! Define Read Ring Type
#define DEF_RING_TYPE_WRITE 1

//! Define Command Type Util
#define DEF_CMD_TYPE_UTIL 2

//! Define Command Type Sender
#define DEF_CMD_TYPE_SEND 1

//! Define Command Type Reciever
#define DEF_CMD_TYPE_RECV 0

//! Define Data Signal
#define DEF_SIG_DATA 0

//! Define Command Signal
#define DEF_SIG_COMMAND 1

//! Define Max Bulk
#define DEF_MAX_BULK	32

//! Define Max Jumbo Msg Count
#define DEF_MAX_JUMBO	32

//! Define Memory Buffer Data Size
#define DEF_MBUF_DATA_SIZE 2560

//! Define Max Fail Count
/*!
 * 만약 데이터 전송시 다음의 Count 만큼 실패 할 경우
 * Queue Full 이라고 판단하여서 Ring 에 연결된 모든 Consumer 에게 Signal 전달
 */
#define DEF_MAX_FAIL_CNT	10000

//! Define String Format For Q Name
#define DEF_STR_FORMAT_Q_NAME "%s_%s"

//! Define Read Multi Queue Type
#define DEF_MULTI_TYPE_READ	'R'

//! Define Write Multi Queue Type
#define DEF_MULTI_TYPE_WRITE 'W'

//! Define Multi Queue Prefix
#define DEF_STR_MULTI_PREFIX "MULTI"

//! Define Default Backup Folder
#define DEF_STR_DEFAULT_BK_PATH "/NMD/USER/lhj/LQ_LIB/BACKUP"

//! Define Default Backup File Mask
#define DEF_DEFAULT_BK_MASK 0x7FF

//! Define Name of Command Ring
#define DEF_STR_COMMAND_RING "COMMAND_RING"
 
//! Define Ring Create Request Structure
/*!
 * \struct _create_req_
 * \brief Structure for Ring Create Request
 * to Use in Application For to Create Ring
 */
typedef struct _create_req
{
	char    strName[DEF_MEM_BUF_128];   //!< Name of Ring
	int     nSize;          //!< Size of Ring
} CREATE_REQ;

using namespace std;

//! Define Ring Info Structure
/*!
 * \struct _ring_info 
 * \brief Structure for Store to Ring Info
 */
typedef struct _ring_info
{
	struct rte_ring	*pstRing;		//!< Ring
	uint32_t		unFailCnt;		//!< Count of Send Error
	uint32_t		unMMapSize;		//!< Size of MMap
	uint32_t		unMask;			//!< Mask of The Backup File
	int				nIdx;			//!< Index of Consumer Info Array in the ring 
	int				fd;				//!< Backup File Descriptor
	void			*pBackup;				//!< mmap Pointer of Backup File
	char			szName[DEF_MEM_BUF_64];	 //!< Name of this Ring
	vector<string>	vecRelProc;		//!< Name of Relation Process
}RING_INFO;

//! Define Consumer Info Structure For Backup
/*!
 * \struct _cons_info
 * \brief Structure for Consumer Process 
 */
typedef struct _cons_info
{
	char		strName[40];
	uint32_t	unTail;
}CONS_INFO;

//! Define Producer Info Structure For Backup
/*!
 * \struct _prod_info
 * \brief Structure for Producer Process
 */
typedef struct _prod_info
{
	char		strName[40];
	uint32_t	unTail;
}PROD_INFO;

//! Define Data Buffer Structure
/*!
 * \struct _data_buffer
 * \brief Data Buffer Structure for Data Backup
 */
typedef struct _data_buffer
{
	uint32_t	unBuffLen;
	bool		bNext;
	char		szBuff[DEF_MBUF_DATA_SIZE];
}DATA_BUFFER;

//! Define Backup File Info Structure
/*!
 * \struct _backup_info
 * \brief Structure for Backup File 
 */
typedef struct _backup_info
{
	uint16_t	unConsCnt;
	uint16_t	unProdCnt;
	uint32_t	unMask;
	CONS_INFO	stConsInfo[RTE_RING_MAX_CONS_COUNT]; //20
	PROD_INFO	stProdInfo[RTE_RING_MAX_PROD_COUNT]; //20
}BACKUP_INFO;

//! Define Hash Function Pointer
/*!
 * Get to Write Ring Idx
 */
typedef int p_function_hash(void *a_pArgs);

#endif
