#ifndef _MRT_MAIN_H_
#define _MRT_MAIN_H_

//! MRT Main Class
/*!
 * Init MRT Main Class Header Files
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
