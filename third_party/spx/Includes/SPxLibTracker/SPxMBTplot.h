#ifndef _SPX_MBT_PLOT_H
#define _SPX_MBT_PLOT_H

#include "./SPxLibProc/SPxBlobDetect.h"

class SPxMBTmodel;
class SPxMBTtrack;


/*!!!! ok sizeof=0x70 112  compare ida*/
struct SPxMBTplot
{
public:
	void Init();
	void Init(double rangeMetres, double aziDeg, double sizeRangeMetres, double sizeAzimuthDegrees, char isPlotNonQual, signed int weight, signed int strength, double radialSpeedMps);

public:
	int    id;
	int    isValid;
	double rangeMetres;
	double azimuthDegrees;
	double sizeRangeMetres;
	double sizeAzimuthDegrees;
	double xMetres;
	double yMetres;
	int    weight;
	//<alignment member> (size=4)
	double plotLevel;
	double radialSpeedMps;
	SPxMBTplot *prev;
	SPxMBTplot *next;
	SPxMBTmodel *attachedMBTmodel;/*该plot所关联或附属的模型，通过SPxMBTmodel::TestPlotCanBeAttached函数来检验 plot的属性是否符合模型参数的设置条件*/
	unsigned char sectorIndex;
	unsigned char rangeIndex;
	//<alignment member> (size=2)
	SPxMBTtrack *associatedMBTtrack;
	unsigned char isUsedByEstablishedTrack;
	unsigned char isPlotNonQual;
	//<alignment member> (size=2)
	SPxBlob_t *targetBlobPtr;
	// <alignment member> (size=4)
};



#endif//_SPX_MBT_PLOT_H