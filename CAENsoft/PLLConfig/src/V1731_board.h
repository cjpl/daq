/////////////////////////////////////////////////////////////////////////////
// Name:        V1731_board.h
// Purpose:     
// Author:      NDA
// Modified by: 
// Created:     09/04/07 14:18:40
// RCS-ID:      $Id: V1731_board.h,v 1.1 2009/02/18 08:25:34 Colombini Exp $
// Copyright:   CAEN S.p.A All rights reserved.
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _V1731_board_H_
#define _V1731_board_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "V1731_board.h"
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

class V1731Board :	public GenericBoard
{
public:
	V1731Board( AppSettings* p_app_settings, bool is_des_mode);

	//
	// Pure virtual implementation
	wxString GetTypeString() { return this->m_is_des_mode? _("V1731_DES"): _("V1731");};
	CVT_V17XX_TYPES GetType(){ return CVT_V1731;};
	double GetClockMHz() const { return CVT_V17XX_BOARD_CLOCK_KHZ[ this->m_is_des_mode? CVT_V1731_DES_BOARD_CLOCK_INDEX: CVT_V1731_BOARD_CLOCK_INDEX]*0.001;};
	int GetSampleBit() const { return 8;} 
	double GetVolt2Bit() const { return ((double)( 1<< 16)) / 1.081;};
	UINT8 GetChEnableMsk() const { return this->m_is_des_mode? 0x55: 0xff;} ;
	double GetFPGADivider() const { return 4;}

	//
	// Virtuals override
	virtual double GetDACUpdateFactor( void){ return 8.0;};


protected:
	bool m_is_des_mode;

};


#endif	// _V1731_board_H_

