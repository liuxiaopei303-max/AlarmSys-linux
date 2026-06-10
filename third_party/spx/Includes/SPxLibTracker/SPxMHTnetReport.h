#ifndef _SPX_MHTNETREPORT_H
#define _SPX_MHTNETREPORT_H
/*
* Other headers required.
*/
#include "SPxLibNet/SPxPacketSend.h"	/* Our base class */
#include "SPxLibData/SPxPackets.h"
#include "SPxMHT.h"
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

/*
* Forward-declare other classes we use in case headers are in wrong order.
*/
class SPxAsterixEncoder;
class SPxMHT;
class SPxTrackDatabase;
class SPxMHTtrack;
/*
*
0 No track reports are generated
48 ASTERIX Category-048 reports are generated
183 NMEA 0183 (TTM) reports
0x110 SPx format, minimal track reports
0x111 SPx format, normal track reports
0x112 SPx format, extended track reports
*/

/*sizeof=0x178  376*/
class SPxMHTnetReport :public SPxPacketSender
{
public:
	/*
	* Public fields.
	*/

	/*
	* Public functions.
	*/
	/* Constructor, destructor etc. */
	SPxMHTnetReport(SPxMHT *m_pMHT);
	virtual ~SPxMHTnetReport();

	/* Sender ID. */
	void SetSAC(UINT8 sac);
	UINT8 GetSAC(void);
	void SetSIC(UINT8 sic);
	UINT8 GetSIC(void);
    SPxErrorCode SetTrackDB(SPxTrackDatabase *trackDB);
	/* Generic parameter assignment. */
	virtual int  SetParameter(char *paraName, char *paraValue);
	virtual int  GetParameter(char *paraName, char *valueBuf, int bufLen);
	SPxErrorCode SetRadarLatLong(double LatDegs, double LongDegs, double RadarHeightMetres);
	void         SetSenderID(UINT16 SenderID);
	SPxErrorCode SetReportType(unsigned int TrackReportType);
	int  SetTrackReportExtFlags(UINT32 ExtFlags){
		this->m_mutex.Enter();
		this->m_reportExtFlags = ExtFlags;
		this->m_mutex.Leave();
		return 0;
	}
	int  BuildMinimalTrackReports(SPxPacketTrackExtended_tag *pExtRpt, int reportTrackStatus, SPxMHTtrack *pTrack);
	void BuildNormalTrackReports(SPxPacketTrackExtended_tag *pExtRpt, int reportTrackStatus, SPxMHTtrack *pTrack);
	void BuildExtendedTrackReports(SPxPacketTrackExtended_tag *pExtRpt, int a3, SPxMHTtrack *pMHTtrack, UINT16 ReportExtFlags);
	unsigned int  BuildTTM(char *sentence, int sizeBytes, int reportTrackStatus, SPxMHTtrack *pTrack);
	int  BuildCat48(unsigned char *buffer, unsigned int bufSizeBytes, int reportTrackStatus, SPxMHTtrack *pTrack);


	void BuildTrackerStatus(SPxPacketTrackerStatus_tag *, SPxTrackerStatus_t *);
	void ReportTrackerStatus(SPxTrackerStatus_t *trackerStatistics);
	void BuildCat34_XXX_Status();
	void BuildCat48_XXX_Status();
	void BuildCat34_Msg(SPxMHT *a2, int iSector_start_1_1, int a4);
	int BuildCat34_NorthMarker_Msg(int overloadFlags);
	int BuildCat34_SectorCrossing_Msg(int sectorIndex1);
	void TrackPackNet(int reportType, SPxMHTtrack *pTrack);
	int  CheckAndUpdate(int reportTrackStatus, SPxMHTtrack *pTrack);/*返回值1 表示成功 返回0 表示失败*/
	void ProcessAntiClockwise(struct SPxPacketTrackMinimal_tag *a2, struct SPxPacketTrackNormal_tag *a3, struct SPxPacketTrackExtended_tag *a4);
private:
	/*
	* Private fields.
	*/
	/* Engine we are reporting. */
	SPxMHT *  m_pMHT;
	int m_isAntiClockwise;/* Is the radar rotating anti-clockwise? */
	/* General. */
	SPxTrackDatabase *m_trackDB;         /* Track database. */
	/* Asterix encoder helper object. */
	SPxAsterixEncoder *m_asterixEncoder;
	/* Sender ID used to filter track reports */
	UINT16 m_senderID;
	int m_serverVersion;
	REAL64	m_radarLatDegs;
	REAL64 m_radarLongDegs;
	REAL64 m_heightMetres;
	int m_packetType;
	UINT32 m_reportExtFlags;
	int m_isReportProvisionals;
	int m_hitCountWindowSize;
	int m_reportPlotCentre;
	int m_reportMode; /*TrackReporter.Mode = 0	# Report all(0), designated(1), nearest(2) or newest(3)*/
	int m_newestTrackId;
	int m_newestTrackAge;
	int m_nearestTrackId;
	REAL64 m_nearestTrackRangeMetres;
	/* Buffer for building reports. */
	unsigned char *m_buffer;		/* Buffer for reports */
	unsigned int m_bufferSizeBytes;	/* Size of buffer */
	SPxPacketTrackExtended_tag *m_pExtRpt;
	char m_cat48TypWhenCoasted;
	char m_cat48TypWhenMerged;
	UINT16 m_numPSR;
	UINT16 m_numMisses;
	UINT16 m_testTargetID;
	REAL64 m_testTargetRange;
	REAL64 m_testTargetAzimuth;
	int m_curSector;
	int m_lastBuildCat48MsgTime_ms;
	int m_lastBuildCat34MsgTime_ms;
	/*
	* Private functions.
	*/
	/* Alert handlers. */
	static int staticAlertHandler(void *invokingObj, void *userArg, void *arg);
	SPxErrorCode alertHandler(struct SPxSafetyNetAlert_tag *alert);

}; /* SPxMHTnetReport class */

   /*********************************************************************
   *
   *   Function prototypes
   *
   **********************************************************************/


#endif//_SPX_MHTNETREPORT_H