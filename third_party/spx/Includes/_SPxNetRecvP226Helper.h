/*********************************************************************
*
* 
*
* File: $RCSfile: _SPxNetRecvP226Helper.h,v $
*
**********************************************************************/

#ifndef _SPX_NETRECVP226_HELPER_H
#define _SPX_NETRECVP226_HELPER_H

/*
 * Other headers required.
 */
#include "SPxLibUtils/SPxAtomic.h"
#include "SPxLibUtils/SPxCriticalSection.h"

/* We need the radar source base class header. */
#include "SPxLibData/SPxRadarSource.h"


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

/* Forward declare the classes we use. */
class _SPxNetRecvP226Helper.hHelper;
class _SPxNetRecvP226Helper.hLicenseHelper;
class SPxRIB;
class SPxThread;

/*
 * Define our receiver class, derived from generic radar source, not from
 * common network receiver base class.
 */
class SPxNetRecvP226Helper 
	: public Navico::Protocol::NRP::iImageClientStateObserver
    , public Navico::Protocol::NRP::iImageClientSpokeObserver
{
/* The helper class can access our private members. */
friend class _SPxNetRecvP226Helper.hHelper;

public:
    /*
     * Public functions.
     */
    /* Constructor, destructor etc. */
    SPxNetRecvP226Helper(SPxNetworkReceiveP226 *parent,SPxRIB *buffer);
    virtual ~SPxNetRecvP226Helper();
  //-------------------------------------------------------------------------
    //  Observer Callbacks
    //-------------------------------------------------------------------------
private:
   // iImageClientStateObserver callbacks  8  
	virtual void UpdateAdvancedState ( const Navico::Protocol::NRP::tAdvancedSTCState* pState );
	virtual void UpdateConfiguration ( const Navico::Protocol::NRP::tConfiguration* pConfiguration );
    virtual void UpdateProperties    ( const Navico::Protocol::NRP::tProperties* pProperties );	
	virtual void UpdateGuardZoneAlarm( const Navico::Protocol::NRP::tGuardZoneAlarm* pAlarm );
	virtual void UpdateMode          ( const Navico::Protocol::NRP::tMode* pMode );
	virtual void UpdateProperties    ( const Navico::Protocol::NRP::tTargetTrackingProperties* pProperties );
	virtual void UpdateRadarError    ( const Navico::Protocol::NRP::tRadarError* pAlarm );
	virtual void UpdateSetup         ( const Navico::Protocol::NRP::tSetup* pSetup );
	virtual void UpdateSetupExtended ( const Navico::Protocol::NRP::tSetupExtended* pSetupExtended );
    // iImageClientSpokeObserver callbacks  1
    virtual void UpdateSpoke         ( const Navico::Protocol::NRP::Spoke::t9174Spoke *pSpoke );

	/*     
	// iFeatureObserver callbacks
    virtual void UpdateFeature( const Navico::Protocol::NRP::tFeatureEnum* pFeature );

    // iTargetTrackingClientObserver callbacks
    virtual void UpdateTarget( const Navico::Protocol::NRP::tTrackedTarget* pTarget );

    // iTargetTrackingClientStateObserver callbacks
    virtual void UpdateNavigation( const Navico::Protocol::NRP::tNavigationState* pNavigationState );
    virtual void UpdateAlarmSetup( const Navico::Protocol::NRP::tTargetTrackingAlarmSetup* pAlarmSetup ); 	
	*/
	
private:
    /*
     * Private fields.
     */
    SPxNetworkReceiveP226 *m_parent;	
    /* Radar input buffer where data is written. */
    SPxRIB *m_rib;
    Navico::Protocol::NRP::tImageClient *       m_pImageClient;
}; /* SPxNetRecvP226Helper class */

/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* _SPX_NETRECVP226_HELPER_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
