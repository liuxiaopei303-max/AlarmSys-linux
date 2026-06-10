//-----------------------------------------------------------------------------
// SPxNetRecvP226Helper Class Implementation
//-----------------------------------------------------------------------------
using namespace Navico::Protocol::NRP;

SPxNetRecvP226Helper::SPxNetRecvP226Helper(SPxNetworkReceiveP226 *parent,SPxRIB *buffer)
{
    this->m_parent = parent;
    this->m_rib    = buffer;
    if ( parent ) {
        this->field_14       = timeGetTime();
        this->m_pImageClient = new tImageClient();
        if (m_pImageClient) {
            m_pImageClient->AddStateObserver( this );
            m_pImageClient->AddSpokeObserver( this );
            m_pImageClient->Connect(this->m_parent->m_serialNumber,this->m_parent->m_imageStream);
            m_pImageClient->SetAutoSendClientWatchdog( true );
            m_pImageClient->SetPower(1);
            m_pImageClient->QueryAll();
        }
    }
    else {
        SPxError(SPX_ERR_SYSTEM, SPX_ERR_ASSERT, 0, 0, "SPxNetRecvP226Helper", "NULL parent");
    }
}
//
// 4.4.1. void iImageClientStateObserver::UpdateAdvancedState( const tAdvancedSTCState * pState )
//
// This message is used to communicate with the client about advanced configuration options.
//  The tImgeClient::QueryAll or tImgeClient::QueryAdvancedSetup methods
// may be used to explicitly ask the radar to send this information.
void   SPxNetRecvP226Helper::UpdateAdvancedState(const tAdvancedSTCState* pState)
{
    this->field_14 = timeGetTime();
    if ( pState ) {
        if ( SPxNetworkReceiveP226::m_debug & SPX_NET_RECV_P226_DEBUG_STATUS ) {
            if ( SPxNetworkReceiveP226::m_logFile )
                fprintf(SPxNetworkReceiveP226::m_logFile, "%u: P226 radar advanced-state:\n", v3);
        }
    }
    else {
        SPxError(SPX_ERR_SYSTEM, SPX_ERR_BAD_ARGUMENT, 0, 0, "SPxNetRecvP226Helper::UpdateAdvancedState", 0);
    }
}
void   SPxNetRecvP226Helper::UpdateConfiguration(const tConfiguration *pConfiguration)
{
    this->field_14 = timeGetTime();
    if ( pConfiguration ) {
        if ( SPxNetworkReceiveP226::m_debug & SPX_NET_RECV_P226_DEBUG_STATUS ) {
            if ( SPxNetworkReceiveP226::m_logFile ) {
                fprintf(
                    SPxNetworkReceiveP226::m_logFile,
                    "%u: P226 radar configuration:\n\tZeroBearingOffset = %.1f degrees\n\tAntennaHeight     = %.3f m\n",
                    this->field_14,
                    pConfiguration->zeroBearing_ddeg * 0.1,
                    pConfiguration->antennaHeight_mm * 0.001);
            }
        }
        this->m_parent->m_actualRadarHeightMetres    = pConfiguration->antennaHeight_mm * 0.001;
        this->m_parent->m_actualRadarZeroBearingDegs = pConfiguration->zeroBearing_ddeg * 0.1;
    }
    else {
        SPxError(SPX_ERR_SYSTEM, SPX_ERR_BAD_ARGUMENT, 0, 0, "SPxNetRecvP226Helper::UpdateConfiguration", 0);
    }
}

void  SPxNetRecvP226Helper::UpdateGuardZoneAlarm(const tGuardZoneAlarm *pAlarm)
{
    this->field_14 = timeGetTime();
    if ( pAlarm ) {
        if ( SPxNetworkReceiveP226::m_debug & 0x200 ) {
            if ( SPxNetworkReceiveP226::m_logFile )
                fprintf(SPxNetworkReceiveP226::m_logFile, "%u: P226 radar guard-zone-alarm.\n", this->field_14);
        }
    }
    else {
        SPxError(SPX_ERR_SYSTEM, SPX_ERR_BAD_ARGUMENT, 0, 0, "SPxNetRecvP226Helper::UpdateGuardZoneAlarm", 0);
    }
}

void  SPxNetRecvP226Helper::UpdateMode(const tMode *pMode)
{
    this->field_14 = timeGetTime();
    if ( pMode ) {
        if ( SPxNetworkReceiveP226::m_debug & SPX_NET_RECV_P226_DEBUG_STATUS ) {
            if ( SPxNetworkReceiveP226::m_logFile ) {
                fprintf(SPxNetworkReceiveP226::m_logFile, 
				"%u: P226 radar mode:\n\tState      = %u (%s)\n\tWarmup     = %u seconds\n",
                        this->field_14,
						pMode->state, 
						getStringForModeState(pMode->state), 
						pMode->warmupTime_sec);
            }
        }
        this->m_parent->m_radarModeState      = pMode->state;
        this->m_parent->m_radarModeWarmupSecs = pMode->warmupTime_sec;
    }
    else {
        SPxError(SPX_ERR_SYSTEM, SPX_ERR_BAD_ARGUMENT, 0, 0, "SPxNetRecvP226Helper::UpdateMode", 0);
    }
}

void  SPxNetRecvP226Helper::UpdateProperties(const tProperties *pProperties)
{
    this->field_14 = timeGetTime();
    if ( pProperties ) {
        if ( SPxNetworkReceiveP226::m_debug & SPX_NET_RECV_P226_DEBUG_STATUS ) {
            if ( SPxNetworkReceiveP226::m_logFile ) {
                fprintf(
                    SPxNetworkReceiveP226::m_logFile,
                    "%u: P226 radar properties:\n"
                    "\tRadar Type    = %u\n"
                    "\tPower Cycles  = %u\n"
                    "\tTransmit Time = %u\n"
                    "\tMax Range     = %.1f\n"
                    "\tWarmup Secs   = %u\n",
                    this->field_14,
                    pProperties->scannerType,
                    pProperties->powerCycles,
                    pProperties->operationTime_hour,
                    pProperties->maxRange_dm * 0.1,
                    pProperties->warmupTime_sec);
            }
        }
        this->m_parent->m_radarType           = pProperties->scannerType;
        this->m_parent->m_radarPowerCycles    = pProperties->powerCycles;
        this->m_parent->m_radarUpTime         = pProperties->operationTime_hour;
        this->m_parent->m_radarMaxRangeMetres = pProperties->maxRange_dm * 0.1;
    }
    else {
        SPxError(SPX_ERR_SYSTEM, SPX_ERR_BAD_ARGUMENT, 0, 0, "SPxNetRecvP226Helper::UpdateProperties", 0);
    }
}

void  SPxNetRecvP226Helper::UpdateRadarError(const tRadarError *pRadarError)
{
    this->field_14 = timeGetTime();
    if ( pRadarError ) {
        if ( SPxNetworkReceiveP226::m_debug & SPX_NET_RECV_P226_DEBUG_STATUS ) {
            if ( SPxNetworkReceiveP226::m_logFile ) {
                fprintf(SPxNetworkReceiveP226::m_logFile, "%u: P226 radar error: type %u, params %u, %u (%s).\n",
                        this->field_14, pRadarError->type, 
						pRadarError->param1, pRadarError->param2, 
						getStringForRadarError(pRadarError->type));
            }
        }
        SPxError(SPX_ERR_SYSTEM, SPX_ERR_P226, v8, pRadarError->param1, "P226 Radar Error", getStringForRadarError(pRadarError->type));
    }
    else {
        SPxError(SPX_ERR_SYSTEM, SPX_ERR_BAD_ARGUMENT, 0, 0, "SPxNetRecvP226Helper::UpdateRadarError", 0);
    }
}

void  SPxNetRecvP226Helper::UpdateSetup(const tSetup *pSetup)
{
    this->field_14 = timeGetTime();
    if ( pSetup ){
        if ( SPxNetworkReceiveP226::m_debug & SPX_NET_RECV_P226_DEBUG_STATUS ){
            if ( SPxNetworkReceiveP226::m_logFile ){
                fprintf(
                    SPxNetworkReceiveP226::m_logFile,
                    "%u: P226 radar setup:\n"
                    "\tRange = %.1fm\n"
                    "\tGain  = Type %u, Level %d\n"
                    "\tSea   = Type %u, Level %d\n"
                    "\tRain  = Type %u, Level %d\n"
                    "\tFTC   = Level %d\n"
                    "\tIR    = Level %u\n"
                    "\tStretch = %u\n"
                    "\tBoost   = %u\n",
                    this->field_14,
                    pSetup->range_dm * 0.1,
                    pSetup->gain[0].type,
                    pSetup->gain[0].value,
                    pSetup->gain[1].type,
                    pSetup->gain[1].value,
                    pSetup->gain[2].type,
                    pSetup->gain[2].value,
                    pSetup->ftc.value,
                    pSetup->interferenceReject,
                    pSetup->targetStretch,
                    pSetup->targetBoost);
            }
        }
        this->m_parent->m_actualRangeMetres        = pSetup->range_dm * 0.1;
        this->m_parent->m_actualGainAuto           = pSetup->gain[0].type != 0;
        this->m_parent->m_actualGainLevel          = pSetup->gain[0].value;
        this->m_parent->m_actualSeaClutterAuto     = pSetup->gain[1].type;
        this->m_parent->m_actualSeaClutterLevel    = pSetup->gain[1].value;
        this->m_parent->m_actualRainLevel          = pSetup->gain[2].value;
        this->m_parent->m_actualFTCLevel           = pSetup->ftc.value;
        this->m_parent->m_actualInterferenceReject = pSetup->interferenceReject;
        this->m_parent->m_actualTargetStretch      = pSetup->targetStretch;
        this->m_parent->m_actualTargetBoost        = pSetup->targetBoost;
    }
    else{
        SPxError(SPX_ERR_SYSTEM, SPX_ERR_BAD_ARGUMENT, 0, 0, "SPxNetRecvP226Helper::UpdateSetup", 0);
    }
}

void  SPxNetRecvP226Helper::UpdateSetupExtended(const tSetupExtended *pSetupExtended)
{
    this->field_14 = timeGetTime();
    if ( a2 ){
        if ( SPxNetworkReceiveP226::m_debug & SPX_NET_RECV_P226_DEBUG_STATUS ){
            if ( SPxNetworkReceiveP226::m_logFile ) {
                fprintf(
                    SPxNetworkReceiveP226::m_logFile,
                    "%u: P226 radar setup-extended:\n"
                    "\tFastScanMode  = %d\n"
                    "\tRPM           = %.1f\n"
                    "\tLocalIR       = %d\n"
                    "\tNoiseReject   = %d\n"
                    "\tBeamSharpen   = %d\n"
                    "\tSTCCurveType  = %d\n",
                    this->field_14,
                    pSetupExtended->fastScanMode,
                    pSetupExtended->rpmX10 * 0.1,
                    pSetupExtended->localIR,
                    pSetupExtended->noiseReject,
                    pSetupExtended->beamSharpening,
                    pSetupExtended->stcCurveType);
            }
        }
        this->m_parent->m_actualFastScanMode   = pSetupExtended->fastScanMode;
        this->m_parent->m_actualRPM            = pSetupExtended->rpmX10 * 0.1;
        this->m_parent->m_actualLocalIR        = pSetupExtended->localIR;
        this->m_parent->m_actualNoiseReject    = pSetupExtended->noiseReject;
        this->m_parent->m_actualBeamSharpening = pSetupExtended->beamSharpening;
        this->m_parent->m_actualSTCCurveType   = pSetupExtended->stcCurveType;
    }
    else{
        SPxError(SPX_ERR_SYSTEM, SPX_ERR_BAD_ARGUMENT, 0, 0, "SPxNetRecvP226Helper::UpdateSetupExtended", 0);
    }
}

// The pSpoke contains all the information to build the radar image. See section 3.1 for more information about spoke definitions.
void  SPxNetRecvP226Helper::UpdateSpoke(const t9174Spoke *pSpoke)
{
    DWORD v3; // esi
    unsigned int v4; // eax
    int v6; // ebx
    SPxRIB *m_rib; // esi
    unsigned int nOfSamples; // ebx
    int v9; // eax
    int v11; // esi
    unsigned __int8 *m_decodeBuffer; // eax MAPDST
    unsigned int i; // esi
    unsigned __int8 v16; // cl
    unsigned int v17; // edx
    unsigned __int16 azimuth_0_65535; // cx
    unsigned int spokeAzimuth; // eax
    bool v20; // cf
    char v21; // al
    int rangeCellsDiv2; // edx
    SPxNetworkReceiveP226 *m_parent; // ecx MAPDST
    int v24; // edx
    struct SPxReturnHeader_tag a2; // [esp+20h] [ebp-34h]

   this->field_14 = timeGetTime();

    if ( !pSpoke ){
        SPxError(SPX_ERR_SYSTEM, SPX_ERR_BAD_ARGUMENT, 0, 0, "SPxNetRecvP226Helper::UpdateSpoke", 0);
        return;
    }

        if ( SPxNetworkReceiveP226::m_debug & SPX_NET_RECV_P226_DEBUG_SPOKE && SPxNetworkReceiveP226::m_logFile )
            fprintf(
                SPxNetworkReceiveP226::m_logFile,
                "%u: P226 radar spoke: azi = %u.\n",
                this->field_14,
                pSpoke->spokeAzimuth);
				
        ++this->m_parent->m_prfSpokesSinceLastRef;
		
        unsigned int v4 = SPxTimeGetDiff(this->m_parent->m_prfRefTimeMsecs, this->field_14);
        if ( v4 >= 2000 ){
            this->m_parent->m_prfUsecsPerSpoke = 1000
                    * v4
                    / this->m_parent->m_prfSpokesSinceLastRef;
            if ( m_parent->m_prfUsecsPerSpoke > 0xFFFFu )
                m_parent->m_prfUsecsPerSpoke = 0;
            this->m_parent->m_prfRefTimeMsecs = v3;
            this->m_parent->m_prfSpokesSinceLastRef = 0;
        }

        // uint32_t bitsPerSample: 4;         //!< number of bits per sample, normally is set to 4
        if ( pSpoke->header.bitsPerSample!= 4 ){
            SPxError(SPX_ERR_USER,SPX_ERR_NET_BAD_FMT,pSpoke->header.bitsPerSample,4,"SPxNetRecvP226Helper","Invalid bitsPerSample");
            return;
        }

        // uint32_t nOfSamples : 12;          //!< number of samples present in the spoke
		unsigned int nOfSamples = pSpoke->header.nOfSamples;
        if ( m_rib ){
            v9 = SPxAtomic_int__operator_int(&m_rib->m_bytesToBeRead);
            v11 = m_rib->m_size - v9;
            if ( m_parent->m_ribFull ){
                if ( v11 > 10 * SPxAtomic_int__operator_int(&this->m_rib->m_bytesToBeRead) )
                    this->m_parent->m_ribFull = 0;
            }
            else if ( v11 < (signed int)(pSpoke->header.nOfSamples + 48) ){
                m_parent->m_ribFull = 1;
            }
			
            if ( m_parent->m_ribFull ){
                SPxSystemHealth::GlobalLatchOverload();
                return;
            }
			
            m_decodeBuffer = m_parent->m_decodeBuffer;
            if ( m_decodeBuffer ) {
                if ( m_parent->m_decodeBufferSize >= pSpoke->header.nOfSamples ){
LABEL_27:
                    i = 0;
                    if ( (pSpoke->header.nOfSamples + 1) >> 1 ) {
                        do
                        {
                            unsigned char two_data = pSpoke->data[i];
                            m_decodeBuffer[2 * i + 0]  = two_data & 0xF | 16 * (v16 & 0xF);
                            m_decodeBuffer[2 * i + 1]  = v16 & 0xF0 | ((unsigned __int8)(v16 & 0xF0) >> 4);
							i++;
                        }
                        while ( i < (pSpoke->header.nOfSamples + 1) >> 1 );
                    }
					
                    memset(&a2, 0, 48u);
					
                    a2.magic1          = 0xC0DE5837;
					a2.protocolVersion = 1;
                    a2.headerSize      = 48;
					
					
                    //     UINT16 azimuth;     // The azimuth (0..65536)
                    
                    a2.radarVideoSize = pSpoke->header.nOfSamples;
					a2.numTriggers    = 1;
					
                    a2.sourceCode     = 226;
					a2.count          = pSpoke->header.sequenceNumber;
					
                    a2.nominalLength  = pSpoke->header.nOfSamples;
					 a2.thisLength    = pSpoke->header.nOfSamples;
					 
					azimuth_0_65535 = 16 * pSpoke->header.spokeAzimuth;
                    if ( pSpoke->header.spokeAzimuth > 4095 )
                        azimuth_0_65535 = 65520;

                    a2.azimuth    = azimuth_0_65535;         //     UINT16 azimuth;     // The azimuth (0..65536)
					a2.packing    = 0;
                    a2.totalSize  = pSpoke->header.nOfSamples + 48;
                   
                    a2.heading    = 16 * pSpoke->header.spokeCompass;
					
					a2.timeInterval = m_parent->m_prfUsecsPerSpoke;
                    if ( pSpoke->header.compassInvalid )
                        a2.pimFlags = 0;//#define	SPX_PIM_FLAGS_HEADING_SET 0x40	/* Return has valid 'heading' */
					else
						a2.pimFlags = 64;
					
                    a2.startRange   = 0.0;                   
                    // simrad 返回的是毫米
                    a2.endRange     = 2 * pSpoke->header.rangeCellSize_mm * pSpoke->header.rangeCellsDiv2 * 0.001;
					a2.magic2       = 0xC0DE6948;
					
                    SPxRadarSource::registerReturn((SPxRadarSource *)m_parent, &a2);
                    SPxRIB::Write(this->m_rib, &a2, m_decodeBuffer, a2.thisLength, 0);
                    return;
                }
                if ( m_decodeBuffer )
                    free(m_decodeBuffer);
            }
			
            this->m_parent->m_decodeBufferSize = nOfSamples;
            this->m_parent->m_decodeBuffer     = (unsigned __int8 *)malloc(this->m_parent->m_decodeBufferSize);
            
            m_decodeBuffer = m_parent->m_decodeBuffer;
            if ( !m_decodeBuffer )
            {
                SPxError(
                    SPX_ERR_SYSTEM,
                    SPX_ERR_BAD_MALLOC,
                    m_parent->m_decodeBufferSize,
                    0,
                    "SPxNetRecvP226Helper",
                    "Decode buffer");
                this->m_parent->m_decodeBufferSize = 0;
                return;
            }
            goto LABEL_27;
        }
    
}

const char *__cdecl getStringForModeState(eScannerState a1)
{
    const char *result; // eax

    switch ( a1 )
    {
    case eOff:
        result = "Off";
        break;
    case eStandby:
        result = "Standby";
        break;
    case eTransmit:
        result = "Transmit";
        break;
    case eWarming:
        result = "Warming";
        break;
    case eNoScanner:
        result = "NoScanner";
        break;
    case eDetectingScanner:
        result = "DetectingScanner";
        break;
    default:
        result = "UNKNOWN";
        break;
    }
    return result;
}
const char *__cdecl getStringForRadarError(unsigned int a1)
{
    const char *result; // eax

    if ( a1 > 0x10001 )
    {
        switch ( a1 )
        {
        case 0x10002u:
            result = "BearingPulseFault";
            break;
        case 0x10003u:
            result = "MotorNotRunning";
            break;
        case 0x10004u:
            result = "CommsNotActive";
            break;
        case 0x10005u:
            result = "HeaterVoltage";
            break;
        case 0x10006u:
            result = "ModulationVoltage";
            break;
        case 0x10007u:
            result = "TriggerFault";
            break;
        case 0x10008u:
            result = "VideoFault";
            break;
        case 0x10009u:
            result = "FanFault";
            break;
        case 0x1000Au:
            result = "ScannerConfigFault";
            break;
        case 0x1000Bu:
            result = "PowerSupplyTransient";
            break;
        case 0x1000Cu:
            result = "ScannerDetectFail";
            break;
        default:
            goto LABEL_18;
        }
    }
    else if ( a1 == 0x10001 )
    {
        result = "ZeroBearingFault";
    }
    else if ( a1 == 1 )
    {
        result = "PersistenceCorrupt";
    }
    else
    {
LABEL_18:
        result = "UNKNOWN";
    }
    return result;


}