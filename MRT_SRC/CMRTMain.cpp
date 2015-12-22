/*!
 * \file CMRTMain.cpp
 * \brief MRT Main Class Source File
 */

// Include Standard Header
#include <CMRTMain.hpp>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

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
 * \class CMRTMain
 * \brief CMRT Main Class
 */


//! Constructor
/*!
 * \brief Constructor For CMRTMain Class
 *
 * Init Variables
 */
CMRTMain::CMRTMain()
{



}

//! Destructor
/*!
 * \brief Destructor For CMRTMain Class
 *
 * Init Variables and Delete Memory
 */
CMRTMain::~CMRTMain()
{


}


//! Create the Ring
/*! 
 * \param a_strName is name for ring
 * \return Succ 0, Fail -1
 */
int CreateRing(char *a_strName)
{

	return 0;
}
