/////////////////////////////////////////////////////////////////////////////
// Name:        V1740_board.h
// Purpose:     
// Author:      NDA
// Modified by: 
// Created:     21/05/08
// RCS-ID:      $Id: V1740_board.h,v 1.1 2009/02/18 08:25:34 Colombini Exp $
// Copyright:   CAEN S.p.A All rights reserved.
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _V1740_board_H_
#define _V1740_board_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "V1740_board.h"
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

class V1740Board :	public GenericBoard
{
public:
	V1740Board( AppSettings* p_app_settings);

	//
	// Pure virtual implememtation
	wxString GetTypeString() { return _("V1740");};
	CVT_V17XX_TYPES GetType(){ return CVT_V1740;};
	double GetClockMHz() const { return CVT_V17XX_BOARD_CLOCK_KHZ[ CVT_V1740_BOARD_CLOCK_INDEX]*0.001;};
	int GetSampleBit() const { return 12;} 
	double GetVolt2Bit() const { return ((double)( 1<< 16)) / 2.00;}
	UINT8 GetChEnableMsk() const { return 0xff;}
	double GetFPGADivider() const { return 0.5;}

	//
	// Virtuals override
	virtual double GetDACUpdateFactor( void){ return 4.0;};

};


#endif	// _V1740_board_H_

