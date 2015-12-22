/*!
 * \file CMRTMain.hpp
 * \brief MRT Main Class Header File
 */
#ifndef _MRT_MAIN_H_
#define _MRT_MAIN_H_

/*!
 * \class CMRTMain
 * \brief CMRT Main Class
 */
class CMRTMain
{
	public:
		//! Constructor.
		CMRTMain();
		//! Destructor.
		~CMRTMain();

	private:
		//! Create the Ring
		/*!
		 * \param a_strName is name for ring
		 * \return Succ 0, Fail -1
		 */
		int CreateRing(char *a_strName);

};

#endif
