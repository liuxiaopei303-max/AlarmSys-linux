/*
*/
#ifndef _SPX_MBTPARAMETER_H
#define _SPX_MBTPARAMETER_H

/* Standard headers */
#include <stdio.h>

/* Include SPxCommon.h for UINT32 type definition */
#include "SPxLibUtils/SPxTypes.h"
#include "SPxLibData/SPxProcParam.h"



/*********************************************************************
*
*   Constants
*
**********************************************************************/
class SPxMBTmodel;

/*sizeof=0x110 272 */
class SPxMBTparameter 
{
public:
	/*
	* Public functions.
	*/
	/* Constructor/destructor. */
	SPxMBTparameter(){}
	SPxMBTparameter(SPxMBTmodel* pMBTmodel, const char* name,    int defaultI,    int minI,    int maxI, const char* desc, int groupFlag);
	SPxMBTparameter(SPxMBTmodel* pMBTmodel, const char* name, double defaultR, double minR, double maxR, const char* desc, int groupFlag);

	virtual ~SPxMBTparameter() {}
	virtual REAL64 GetValueR() { return this->m_valueR; }

	int GetValueI() { return this->m_valueI; }
	int GetDefaultValueI() { return this->m_defaultI; }
	REAL64 GetDefaultValueR() { return this->m_defaultR; }
	SPxProcParamType GetParaValueType() { return (SPxProcParamType)this->m_type; }
	const char* GetParaName() { return this->m_name; }
	SPxMBTparameter* GetNext() { return this->m_next; }

	void SetDefaultValue(double defaultValueR);
	void SetDefaultValue(int defaultValueI);
	void SetValue(double valueR);
	void SetValue(int valueI);
	void SetShowFlag(int isShow) { this->m_isShow = isShow; }
protected:
	void Init(SPxMBTmodel* pMBTmodel, const char *name, const char *desc, int belongTabItemIndex);
protected:
	int m_id;///* A reference id. */
	char m_type;/*SPxProcParamType */
	char dummy1;
	char  m_isShow;
	char dummy2;
	//<alignment member> (size=1)
	int m_belongToParaGroupFlag;
	char m_name[64];
	char m_shortDesc[128];/*  description comment */
	//<alignment member> (size=4)
	double m_valueR;
	int m_valueI;
	int m_minI;
	int m_maxI;
	int m_defaultI;
	double m_minR;
	double m_maxR;
	double m_defaultR;
	SPxMBTparameter *m_next;
	/* Linked list support. */
	SPxMBTmodel *m_pMBTmodel;/* Next link in chain */
	static int m_numberParameters;	/* Number of instances */
	friend class SPxMBT;
	friend class SPxMBTmodel;
};

class SPxMBTparameterMHT : public SPxMBTparameter 
{
public:
	SPxMBTparameterMHT(SPxMBTmodel* pMBTmodel, const char* name, double valueR);
	int GetMHTid() { return this->m_MHT_id; }
	void SetMHTid(int mhtId) { this->m_MHT_id = mhtId; }
private:
	int m_MHT_id;/*在SPxMBTmodel 的m_MBTparameterMHTarray中的索引 */
};

#endif /* _SPX_MBTPARAMETER_H */

   /*********************************************************************
   *
   * End of file
   *
   **********************************************************************/
