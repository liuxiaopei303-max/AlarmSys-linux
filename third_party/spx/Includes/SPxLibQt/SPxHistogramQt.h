/*********************************************************************
*
* (c) Copyright 2014 - 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxHistogramWin.h,v $
* ID: $Id: SPxHistogramWin.h,v 1.8 2016/03/03 10:59:08 rew Exp $
*
* Purpose:
*   A reusable histogram control.
*
* Revision Control:
*  03/03/16 v1.8    AGC	Fix clang warnings.
*
* Previous Changes:
*  01/10/15 1.7    AGC	Move std::vector usage into source file.
*  29/04/15 1.6    JP 	Add screen size fields.
*  27/03/15 1.5    JP 	Add on mouse leave event.
*  07/11/14 1.4    JP 	Add more fields etc.
*  22/10/14 1.3    JP 	Implemented line following mouse and
*			visible sample percentage caption.
*  15/10/14 1.2    JP 	Fix compiler warnings.
*  14/10/14 1.1    JP 	Initial version.
**********************************************************************/
/* Avoid multiple inclusion. */
#ifndef _SPXHISTOGRAMQT_H_
#define	_SPXHISTOGRAMQT_H_

/* Qt headers. */
#include <QWidget>
#include <QRect> 

/* SPx headers. */
#include "./SPxLibUtils/SPxHistogram.h"

/* Forward declarations */
 
class SPxRunProcess;

/* SPxHistogramWin dialog */
class SPxHistogramQT : public QWidget, public SPxHistogram
{

public:
	/*
	* Public functions.
	*/
	/* Construction and destruction. */
	explicit SPxHistogramQT(QWidget *parent = 0,
		SPxRunProcess* proStats = NULL);
	virtual ~SPxHistogramQT(void);


private:

	/*
	* Private variables.
	*/
 

 
	

	QPixmap *m_tmpPixmap;     /* Off-screen bitmap. */
	QPainter *m_tmpPixmapPainter; /* Graphics rendering object. */

	QRect m_winRect;        /* Window size. */
	QRect m_graphRect;        /* Graphic size. */
 
	QFont m_font;  /* Font to use. */
	LONG m_xCursorPoint;
	LONG m_yCursorPoint;

	/* Screen size. */
	unsigned int m_wRtn;
	unsigned int m_hRtn;

	/* Mouse over flag. */
	bool m_mouseOver;

	/*
	* Private functions.
	*/

	// Draw member functions.
	virtual void RequestRedraw(void);
	void Redraw(void);
	virtual void DrawBackground(void) const;
	virtual void DrawBar(const SPxHistogram::Bar& bar) const;
	virtual void DrawGrid(void) const;
	virtual void DrawAxisCaption(double x, int index) const;
	virtual void DrawMean(double x0, double y0, double x1, double y1) const;
	virtual void DrawSd(double x0, double y0, double x1, double y1) const;
	virtual void DrawNormalCurve(double cx, double cy) const;
	virtual void DrawSamplePercentageCaption(const char *caption) const;
	virtual void DrawCursorLine(void) const;
	virtual void DrawCursorCaption(const char *captionPos,
		const char *captionVal) const;

	virtual double GetX(void) const;
	virtual double GetY(void) const;
	virtual double GetWidth(void) const;
	virtual double GetHeight(void) const;
	virtual double GetCursorX(void) const;
	virtual double GetCursorY(void) const;

	// Dialog events member functions

	virtual void paintEvent(QPaintEvent *event);//OnPaint
	virtual void resizeEvent(QResizeEvent *event);//OnSize
	virtual void mouseMoveEvent(QMouseEvent *event);//OnMouseMove
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);//OnMouseLeave??
	virtual void wheelEvent(QWheelEvent *event);
	//afx_msg BOOL OnEraseBkgnd(CDC* pDC);
}; /* Class SPxHistogramQT. */

#endif

   /*********************************************************************
   *
   * End of file
   *
   **********************************************************************/