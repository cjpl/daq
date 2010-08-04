/////////////////////////////////////////////////////////////////////////////
// Name:        V1721_board.h
// Purpose:     
// Author:      NDA
// Modified by: 
// Created:     09/04/07 14:18:40
// RCS-ID:      $Id: V1721_board.h,v 1.1 2009/02/18 08:25:34 Colombini Exp $
// Copyright:   CAEN S.p.A All rights reserved.
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _V1721_board_H_
#define _V1721_board_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "V1721_board.h"
#endif

/*!
 * Includes
 */

extern "C" 
{
	#include "cvt_V1724.h"
}
#include "generic_board.h"

class AppSettings;

class V1721Board :	public GenericBoard
{
public:
	V1721Board( AppSettings* p_app_settings);

	//
	// Pure virtual implememtation
	wxString GetTypeString() { return _("V1721");};
	CVT_V17XX_TYPES GetType(){ return CVT_V1721;};
	double GetClockMHz() const { return CVT_V17XX_BOARD_CLOCK_KHZ[ CVT_V1721_BOARD_CLOCK_INDEX]*0.001;};
	int GetSampleBit() const { return 8;} 
	double GetVolt2Bit() const { return ((double)( 1<< 16)) / 1.081;};
	UINT8 GetChEnableMsk() const { return 0xff;} ;
	double GetFPGADivider() const { return 4;}

	//
	// Virtuals override
	virtual double GetDACUpdateFactor( void){ return 8.0;};

};


#endif	// _V1721_board_H_

