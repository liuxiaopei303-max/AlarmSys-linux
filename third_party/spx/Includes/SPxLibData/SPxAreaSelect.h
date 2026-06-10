/*********************************************************************
*
* (c) Copyright 2007, 2010, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxAreaSelect.h,v $
* ID: $Id: SPxAreaSelect.h,v 1.4 2013/10/04 15:31:08 rew Exp $
*
* Purpose:
*   Header for area selection and remapping
*
* Revision Control:
*   04/10/13 v1.4    AGC	Simplify headers.
*
* Previous Changes:
*   21/12/11 1.3    DGJ	Support optional navdata
*   15/09/10 1.2    REW	Make destructor virtual.
*   17/09/07 1.1    DGJ	Initial Version
**********************************************************************/

#ifndef _SPX_AREA_SELECT_H
#define _SPX_AREA_SELECT_H

/* Forward declarations. */
class SPxActiveRegion;
class SPxLUT8;
class SPxNavData;

/* Class for an AreaSelect object, being a combination of an area
 * and a LUT that defines the action to take in that area.
 */
class SPxAreaSelect
{
    SPxActiveRegion *m_activeRegion;
    SPxLUT8 *m_lut;
    SPxNavData *m_navData;
    int m_lastProcessedAzimuth;
    int m_lutMemoryCreatedByClass;
public:
    SPxAreaSelect(SPxActiveRegion *activeRegion, SPxLUT8 *lut=0, SPxNavData *navData = 0);
    virtual ~SPxAreaSelect(void);
    SPxActiveRegion * GetActiveRegion(void) {return m_activeRegion;}
    SPxLUT8 *GetLUT(void) {return m_lut;}
    SPxNavData *GetNavData(void) {return m_navData;}
    int GetLastProcessedAzimuth(void) {return m_lastProcessedAzimuth;}
    void SetLastProcessedAzimuth(int azimuth) {m_lastProcessedAzimuth = azimuth;}
};

#endif /* _SPX_AREA_SELECT_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
