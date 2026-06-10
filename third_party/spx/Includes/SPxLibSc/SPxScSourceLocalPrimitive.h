/*********************************************************************
*new  add 2020.12.25
**********************************************************************/

#ifndef _SPX_SC_SOURCE_LOCAL_PRIMITIVE_H
#define _SPX_SC_SOURCE_LOCAL_PRIMITIVE_H

/*
 * Other headers required.
 */

/* For SPxScanMode. */
#include "SPxLibData/SPxRib.h"

/* Our object is derived from the standard source class. */
#include "SPxLibSc/SPxScSource.h"
#include "./SPxLibSc/SPxScSourceLocal.h"

/*********************************************************************
*
*   Constants
*
**********************************************************************/


/*********************************************************************
*
*   Macros
*
**********************************************************************/


/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

/* Forward declarations. */
class SPxRunProcess;
struct SPxReturn_tag;
class SPxPacketDecoder;
class SPxViewControl;



/* Forward declare any classes we need in our own class in case
 * headers are included in the wrong order.
 */
class SPxPIM;
class SPxScDest;

/*
 * Define the class that actually does scan conversion and is therefore
 * a source of scan converted data!
 *
 * This is derived the SPxScSource class which is the base of all sources
 * of scan converted data (including network receipt), which in turn is
 * derived from the base SPxSc class.
 */
class SPxScSourceLocalPrimitive :public SPxScSourceLocal
{
public:
    /*
     * Public variables.
     */
    
    /*
     * Public functions.
     */
    /* Constructor and destructor. */
	SPxScSourceLocalPrimitive(SPxScDest *dest, int scType = SPX_SC_TYPE_PPI);
    virtual ~SPxScSourceLocalPrimitive();

    /* Input of data and information from the source. */
    int NotifyChange(SPxRunProcess *runProcess, UINT32 flags);
    int NewData(SPxRunProcess *runProcess, SPxReturn_tag *firstReturn,
			unsigned int firstAziIdx, unsigned int numAzis);



    /* Overrides for virtual functions in SPxObj base class. */
    int SetParameter(char *parameterName, char *parameterValue);
    int GetParameter(char *parameterName, char *valueBuf, int bufLen);
    
    /*
     * Static public functions.
     */
    static void SetLogFile(FILE *logFile)	{ m_logFile = logFile; }
    static FILE *GetLogFile(void)		{ return(m_logFile); }


protected:

    int updateInternals(void);
    /* Actual scan conversion. */
    int scanConvertAzi(int aziIdx, SPxReturn *rtn);
    unsigned char getValueForPixel(SPxScLocalPerPixelInfo_t *pix,
					int aziIdx,
					SPxPIM *pim);
    int updatePixel(int x, int y, unsigned char val);

    /* Zone output. */
    int outputDirtyZones(void);
    int addZoneToOutputList(UINT16 x, UINT16 y, UINT16 w, UINT16 h,
						UINT32 changeFlags);


    /* Calculations etc. */
    int recalcScTables(void);
    void recalcScTopLefts(int startX, int startY, int w, int h);
    int recalcScBoxes(void);
    void recalcScAziLists(void);
	/*
	* Static (i.e. per class, not per object) variables.
	*/
	static FILE *m_logFile;		/* Destination for debug messages */

								/* View control callback. */
	static int viewControlHandler(void *invokingObj,
		void *contextPtr,
		void *changeFlagsPtr);
	static int viewControlDeleted(void *invokingObject,
		void *userObject,
		void *arg);

	//friend class SPxScSourceLocal;
}; /* SPxScSourceLocalPrimitive */

/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/


#endif /* _SPX_SC_SOURCE_LOCAL_PRIMITIVE_H*/

/*********************************************************************
*
* End of file
*
**********************************************************************/
