/////////////////////////////////////////////////////////////////////////////
// Name:        appsettings.h
// Purpose:     
// Author:      NDA
// Modified by: 
// Created:     09/04/07 16:17:15
// RCS-ID:      $Id: config_doc.h,v 1.1 2009/02/18 08:25:35 Colombini Exp $
// Copyright:   CAEN S.p.A. All rights reserved
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _CONFIG_DOC_H_
#define _CONFIG_DOC_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "config_doc.h"
#endif

#include <wx/arrimpl.cpp> 
#include <wx/config.h>			
#include <wx/confbase.h>		
#include <wx/fileconf.h>		
#include <wx/filename.h>
#include <wx/dir.h>

#include "common_defs.h"

class ConfigDoc
{
public:
	//
	// Methods
	ConfigDoc( void);
	~ConfigDoc( void);
	bool Load( const wxString & filename);
	bool Save( const wxString & filename);
	bool Save( );

	bool IsDirty(){ return this->m_is_dirty;};
	void SetDirty(bool value)
	{ 
		this->m_is_dirty= value;
	}

	CVT_V17XX_TYPES GetBoardType(){ return this->m_board_type;}
	double GetInputClock() const { return this->m_input_clock;};
	bool GetPLLMode() const { return this->m_PLL_mode;}
	bool GetClockOutEnable() const { return this->m_clock_out_enable;}
	int GetClockOutDelay() const { return this->m_clock_out_delay;}
	bool GetClockOutDelayEnable() const { return this->m_clock_out_delay_enable;}
	int GetClockOutDivider() const { return this->m_clock_out_divider;}
	int GetADCDivider() const { return this->m_adc_divider;}
	UINT32 GetBoardBaseAddress() const { return this->m_board_base_address;}
	int GetVCXOFreq() const { return this->m_vcxo_freq;}

	void SetBoardType( CVT_V17XX_TYPES value)
	{ 
		if( this->m_board_type== value)
			return;
		this->m_board_type= value;
		this->m_is_dirty= true;
	}

	void SetInputClock( double value)
	{ 
		if( this->m_input_clock== value)
			return;
		this->m_input_clock= value;
		this->m_is_dirty= true;
	}
	void SetPLLMode( bool value)
	{ 
		if( this->m_PLL_mode== value)
			return;
		this->m_PLL_mode= value;
		this->m_is_dirty= true;
	}
	void SetClockOutEnable(bool value)
	{ 
		if( this->m_clock_out_enable== value)
			return;
		this->m_clock_out_enable= value;
		this->m_is_dirty= true;
	}
	void SetClockOutDelay(int value)
	{ 
		if( this->m_clock_out_delay== value) 
			return;
		this->m_clock_out_delay= value;
		this->m_is_dirty= true;
	}
	void SetClockOutDelayEnable(bool value)
	{ 
		if( this->m_clock_out_delay_enable== value)
			return;
		this->m_clock_out_delay_enable= value;
		this->m_is_dirty= true;
	}
	void SetClockOutDivider( int value)
	{ 
		if( this->m_clock_out_divider== value)
			return;
		this->m_clock_out_divider= value;
		this->m_is_dirty= true;
	}
	void SetADCDivider( int value)
	{ 
		if( this->m_adc_divider== value)
			return;
		this->m_adc_divider= value;
		this->m_is_dirty= true;
	}
	void SetBoardBaseAddress( UINT32 value)
	{ 
		if( this->m_board_base_address== value)
			return;
		this->m_board_base_address= value;
		this->m_is_dirty= true;
	}
	void SetVCXOFreq( int value)
	{ 
		if( this->m_vcxo_freq== value)
			return;
		this->m_vcxo_freq= value;
		this->m_is_dirty= true;
	}

	const wxString &GetFilename() { return this->m_filename;}

protected:
	CVT_V17XX_TYPES m_board_type;
	double m_input_clock;
	bool m_PLL_mode;
	bool m_clock_out_enable;
	int m_clock_out_delay;
	int m_clock_out_divider;
	int m_adc_divider;
	bool m_clock_out_delay_enable;
	UINT32 m_board_base_address;
	int m_vcxo_freq;

	wxString m_filename;

	bool m_is_dirty;
};


#endif


