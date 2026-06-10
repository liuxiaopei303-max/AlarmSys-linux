/*
*/
#ifndef _SPX_MHTTRACKERPARAMETER_H
#define _SPX_MHTTRACKERPARAMETER_H

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

/* Magic number to allow us to sanity-check the base class objects. */
#define SPX_MHTOBJ_MAGIC	0xC0DE1234


/*********************************************************************
*
*   Type definitions
*
**********************************************************************/


/*
* Base class of all other XXX classes.
*/
class SPxMHT;
class SPxMHTtrackerParameter;
class SPxActiveRegion;
class SPxMBTmodel;

/*
1>  class SPxMHTtrackerParameterValue	size(0x20):
1>  	+---
1>   0	| {vfptr}
1>   8	| m_parent
1>  0xc	| m_valueI
1>  0x10	| m_valueR
1>  0x18	| m_valueS
1>    	| <alignment member> (size=4)
*/
class SPxMHTtrackerParameterValue
{
public:
	/*
	* Public functions.
	*/
	/* Constructor/destructor. */
	SPxMHTtrackerParameterValue(SPxMHTtrackerParameter *pMHTtrackerParameter);
	virtual ~SPxMHTtrackerParameterValue(void);
	char* GetValue(char *value_buf, unsigned int bufLen);
	int GetValueI(){return this->m_valueI;}
	REAL64 GetValueR(){return this->m_valueR;}
private:
	/*
	* Private functions.
	*/

	/*
	* Private variables.
	*/

	/* Magic number of verification, to denote start of object. Value is
	* set as SPX_MHTOBJ_MAGIC on object construction.
	*/
	SPxMHTtrackerParameter* m_parent;
	int       m_valueI;
	double    m_valueR;
	const char* m_valueS;
	/* Prevent use of copy-constructor and assignment operator on SPxObj. */
	SPxMHTtrackerParameterValue(const SPxMHTtrackerParameterValue&) SPX_FUNCTION_DELETE;
	SPxMHTtrackerParameterValue& operator=(const SPxMHTtrackerParameterValue&) SPX_FUNCTION_DELETE;

	friend class SPxMHTtrackerParameter;
}; /* SPxMHTtrackerParameterValue */


class SPxMHTtrackerParameterAreaValue
{
public:
	/*
	* Public functions.
	*/
	/* Constructor/destructor. */
	SPxMHTtrackerParameterAreaValue(SPxMHTtrackerParameter *parent, SPxActiveRegion *activeRegion);
	virtual ~SPxMHTtrackerParameterAreaValue();
	SPxActiveRegion* GetActiveRegion();
	int GetValueI() { return this->m_value->GetValueI(); }
	REAL64 GetValueR() { return this->m_value->GetValueR(); }
protected:
	SPxMHTtrackerParameter* m_parent;
	SPxActiveRegion *m_activeRegion;
	SPxMHTtrackerParameterValue* m_value;
	SPxMHTtrackerParameterAreaValue *m_next;
	int m_isValid;
	/* Prevent use of copy-constructor and assignment operator on SPxObj. */
	SPxMHTtrackerParameterAreaValue(const SPxMHTtrackerParameterAreaValue&) SPX_FUNCTION_DELETE;
	SPxMHTtrackerParameterAreaValue& operator=(const SPxMHTtrackerParameterAreaValue&) SPX_FUNCTION_DELETE;

	static int objDeleteFn(void *invokingObject, SPxMHTtrackerParameterAreaValue *userObject, void * /*arg*/);
	friend class SPxMHTtrackerParameter;
};

/* Maximum length of radar name. */
#define	SPX_MHT_PARAMETER_MAX_DESC_LEN	64

class SPxMHTtrackerParameter 
{
public:
	/*
	* Public functions.
	*/
	/* Constructor/destructor. */
	SPxMHTtrackerParameter(SPxMHT* m_pMHT, const char* name,int belongTabItemIndex, const char* desc,
		int defaultI,int minI,int maxI,int display_flag,int isAreaDependent);
	SPxMHTtrackerParameter(SPxMHT* m_pMHT, const char* name, int belongTabItemIndex, const char* desc,
		double defaultR, double minR, double maxR, int display_flag, int isAreaDependent);
	virtual ~SPxMHTtrackerParameter();
	char* SPxGetParameterValueString(char *value_buf, unsigned int bufLen);
	void SetValue(double valueR, SPxActiveRegion *activeRegion);
	void SetValue(int valueI,    SPxActiveRegion *activeRegion);
	void SetValue(const char *valueBuffer, SPxActiveRegion *activeRegion);
	int    GetValueI();
	double GetValueR();
	const char * GetValueS();
	SPxMHTtrackerParameterValue * GetOrCreateParameterAreaValue(SPxActiveRegion *activeRegion);
	SPxMHTtrackerParameterValue* GetParameterValue(SPxActiveRegion *pActiveRegion);
	SPxMHTtrackerParameterAreaValue* GetParameterAreaValue(SPxActiveRegion *pActiveRegion);
	void RemoveAreaValue(SPxMHTtrackerParameterAreaValue *pRemoveAreaValue);
	double GetAreaValueR(double range_m, double azimuth_deg, SPxMBTmodel *model);
	int    GetAreaValueI(double range_m, double azimuth_deg, SPxMBTmodel *model);
	SPxMHTtrackerParameter* GetNext(){return this->m_next;}
	SPxMHTtrackerParameterValue* GetValuePtr(){return this->m_value;}
	int GetDefaultValueI(){return this->m_defaultI;}
	REAL64 GetDefaultValueR(){return this->m_defaultR;}
	SPxProcParamType GetParaValueType(){return this->m_type;}
	char* GetParaName(){return this->m_name;}
	int GetId() { return this->m_id; }
	/*该函数的作用是获取下一个满足显示模式设置的参数*/
	SPxMHTtrackerParameter * GetNextShowParaPtr();
	void PopAreaValue(SPxMHTtrackerParameterAreaValue *cur, SPxMHTtrackerParameterAreaValue *prev);
	void PushAreaValue(SPxMHTtrackerParameterAreaValue *pAreaValue);
protected:
	void Init(SPxMHT *m_pMHT, const char *name, int belongTabItemIndex, const char *desc);
public:
	int m_id;///* A reference id. */
	int m_belongTabItemIndex;
	char m_shortDesc[SPX_MHT_PARAMETER_MAX_DESC_LEN];/*  description comment */
	SPxMHTtrackerParameterValue* m_value;
	SPxMHTtrackerParameterAreaValue *m_firstAreaValueInChain;
	SPxMHTtrackerParameterAreaValue *m_lastAreaValueInChain;
	int m_isEnable;
	/*
		bit 0 : 1表示该参数可以用于全部显示
		bit 1 ：1表示该参数可以用于BASIC显示
		bit 2 ：1表示该参数可以用于Custom显示
		bit 3 ：1表示该参数不在GUI上显示

		#define SPX_MHT_PARAMETER_SHOW_MODE_FULL      0x00000001
		#define SPX_MHT_PARAMETER_SHOW_MODE_BASIC     0x00000002
		#define SPX_MHT_PARAMETER_SHOW_MODE_CUSTOM    0x00000004
		#define SPX_MHT_PARAMETER_NOT_SHOW            0x00000008
	*/
	int m_showModeBitFlag;//参数支持的显示模式，每一bit不同的显示模式
	/* Flag to say the parameter may be area-dependent. */
	int m_isAreaDependent;

	/* Type and default values. */

	int m_defaultI;
	double m_defaultR;
	char  *m_defaultS;
	SPxProcParamType m_type;
	char *m_name;
	SPxMHT* m_pMHT;
	double m_maxR;
	double m_minR;
	int m_maxI;
	int m_minI;
	/* Linked list support. */
	SPxMHTtrackerParameter *m_next;/* Next link in chain */

	static int m_numberParameters;	/* Number of instances */
};

class SPxMHTtpTerm
{
public:
	/*
	* Public functions.
	*/
	/* Constructor/destructor. */
	SPxMHTtpTerm() {};
	virtual ~SPxMHTtpTerm() {};
	SPxMHTtpTerm* GetNext() { return this->m_next; }
protected:
	SPxMHTtpTerm *m_next;
	SPxMHTtrackerParameter *m_parameter;
	int m_isEnable;
	friend class SPxMHTtpControlRule;
};

/*sizeof=0x24  36 */
class SPxMHTtpControlRule
{
public:
	/*
	* Public functions.
	*/
	/* Constructor/destructor. */
	SPxMHTtpControlRule(SPxMHT *pMHT, const char *paraName, const char *parameterValueS, int a5);
	virtual ~SPxMHTtpControlRule();
	void NewTpTerm(const char *paraName, int isEnable=TRUE);
	void  SetEnableFlagBasedOnParaValue();
	SPxMHTtpControlRule* GetNext() { return this->m_next; }
protected:
	/*0	| {vfptr}*/
	SPxMHT* m_pMHT;
	SPxMHTtpControlRule *m_next;
	SPxMHTtrackerParameter *m_parameter;
	char *m_parameterValueS;
	SPxMHTtpTerm *m_firstTpTerm;
	SPxMHTtpTerm *m_lastTpTerm;
	int field_1C;
	int field_20;
	friend class SPxMHTtrackerParameter;
};

#endif /* _SPX_MHTTRACKERPARAMETER_H */

   /*********************************************************************
   *
   * End of file
   *
   **********************************************************************/
