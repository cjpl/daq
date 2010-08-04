// Name:        appsettings.cpp
// Purpose:     
// Author:      NDA
// Modified by: 
// Created:     09/04/07 16:17:15
// RCS-ID:      $Id: config_doc.cpp,v 1.1 2009/02/18 08:25:35 Colombini Exp $
// Copyright:   CAEN S.p.A. All rights reserved
// Licence:     
/////////////////////////////////////////////////////////////////////////////


#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "config_doc.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#include <wx/wfstream.h>

#include "config_doc.h"


ConfigDoc::ConfigDoc( ):	m_board_type( CVT_V1724),
							m_input_clock(0),
							m_PLL_mode(true),
							m_clock_out_enable(false),
							m_clock_out_delay(0),
							m_clock_out_delay_enable(false),
							m_vcxo_freq(1000),
							m_is_dirty(false)
{
}

ConfigDoc::~ConfigDoc(void)
{

}
/*!
* Load data
*/
bool ConfigDoc::Load( const wxString& filename)
{
	/*
	Application setting file structure
	[MISC]
	|
	|- BOARD_TYPE= V1724
	|- BOARD_ADDRESS= 0x32100000
	|- INPUT_CLOCK_MHZ=10
	|- VCXO_FREQ_MHZ=1000
	|- PLL_MODE=1
	|- CLOCK_OUT_ENABLE=1
	|- CLOCK_OUT_DELAY= 0
	|- CLOCK_OUT_DELAY_ENABLE= 0
	|- CLOCK_OUT_DIVIDER= 1
	|- ADC_DIVIDER= 1
	*/


	if( !::wxFileExists( filename))
	{
		wxString msg= wxString::Format( "Cannot find configuration file '%s'\n", filename.c_str());
		wxLogError( msg);
		return false;
	}

	wxFileInputStream in_stream( filename);
	if( !in_stream.Ok())
	{
		wxString msg= wxString::Format( "Errors opening configuration file '%s'\n", filename.c_str());
		wxLogError( msg);
		return false;
	}

	wxFileConfig config( in_stream) ;

	//
	// Get the board type
	this->m_board_type= CVT_V1724;

	wxString board_type_string= _("");
	config.Read( _("/MISC/BOARD_TYPE"), &board_type_string, _("V1724"));
	// A board was specified : create a board of proper type
	if( !board_type_string.Trim().CmpNoCase( _("V1724")))
	{
		// V1724
		this->m_board_type= CVT_V1724;
	}
	else if( !board_type_string.Trim().CmpNoCase( _("V1721")))
	{
		// V1721
		this->m_board_type= CVT_V1721;
	}
	else if( !board_type_string.Trim().CmpNoCase( _("V1720")))
	{
		// V1720
		this->m_board_type= CVT_V1720;
	}
	else if( !board_type_string.Trim().CmpNoCase( _("V1731")))
	{
		// V1731 Not DES mode
		this->m_board_type= CVT_V1731;
	}
	else if( !board_type_string.Trim().CmpNoCase( _("V1740")))
	{
		// V1740
		this->m_board_type= CVT_V1740;
	}
	else
	{
		// Unknown board type
		wxLogError( wxString::Format( "Unknown board type '%s'\n", board_type_string.c_str()));
		// return false;
	}

	//
	// Get the board address
	wxString board_add_string= _("");
	config.Read( _("/MISC/ADDRESS"), &board_add_string, _("0"));
	unsigned long board_add= 0;
	wxString hex_part;
	if( board_add_string.Upper().StartsWith(_("0X"), &hex_part))
	{
		// Hex format
		if( !hex_part.Trim().ToULong( &board_add, 16))
		{
			board_add= 0;
		}
	}
	else
	{
		// Decimal format
		if( !board_add_string.Trim().ToULong( &board_add, 10))
		{
			board_add= 0;
		}
	}
	this->m_board_base_address= (UINT32)board_add;

	config.Read( _("/MISC/INPUT_CLOCK_MHZ"), &this->m_input_clock, 100);
	config.Read( _("/MISC/VCXO_FREQ_MHZ"), &this->m_vcxo_freq, 1000);
	int tmp= 0;
	config.Read( _("/MISC/PLL_MODE"), &tmp, 1);
	this->m_PLL_mode= tmp!= 0;
	config.Read( _("/MISC/CLOCK_OUT_ENABLE"), &tmp, 0);
	this->m_clock_out_enable= tmp!= 0;
	config.Read( _("/MISC/CLOCK_OUT_DELAY"), &this->m_clock_out_delay, 0);
	config.Read( _("/MISC/CLOCK_OUT_DELAY_ENABLE"), &tmp, 0);
	this->m_clock_out_delay_enable= tmp!= 0;
	config.Read( _("/MISC/CLOCK_OUT_DIVIDER"), &this->m_clock_out_divider, 1);
	config.Read( _("/MISC/ADC_DIVIDER"), &this->m_adc_divider, 1);

	this->m_is_dirty= false;
	this->m_filename= filename;
	return true;
}

bool ConfigDoc::Save( )
{
	return this->Save( this->m_filename);
}
/*!
* save data
*/
bool ConfigDoc::Save( const wxString& filename)
{
	/*
	Application setting file structure
	[MISC]
	|
	|- BOARD_TYPE= V1724
	|- BOARD_ADDRESS= 0x32100000
	|- INPUT_CLOCK_MHZ=10
	|- VCXO_FREQ_MHZ=1000
	|- PLL_MODE=1
	|- CLOCK_OUT_ENABLE=1
	|- CLOCK_OUT_DELAY= 0
	|- CLOCK_OUT_DELAY_ENABLE= 0
	|- CLOCK_OUT_DIVIDER= 1
	|- ADC_DIVIDER= 1
	*/

	wxFileOutputStream out_stream( filename);
	if( !out_stream.Ok())
	{
		wxString msg= wxString::Format( "Errors creating configuration file '%s'\n", filename.c_str());
		wxLogError( msg);
		return false;
	}
	wxFileInputStream in_stream( filename);
	if( !in_stream.Ok())
	{
		wxString msg= wxString::Format( "Errors creating configuration file '%s'\n", filename.c_str());
		wxLogError( msg);
		return false;
	}

	wxFileConfig config( in_stream) ;
	//
	// Delete groups
	wxString group_id;
	long group_index;
	while( config.GetFirstGroup( group_id, group_index))
	{
		config.DeleteGroup( group_id);
	}

	//
	// Set the board type
	switch( this->m_board_type)
	{
	case CVT_V1724:
		if( !config.Write( _("/MISC/BOARD_TYPE"), _("V1724")))
			return false;
		break;
	case CVT_V1721:
		if( !config.Write( _("/MISC/BOARD_TYPE"), _("V1721")))
			return false;
		break;
	case CVT_V1720:
		if( !config.Write( _("/MISC/BOARD_TYPE"), _("V1720")))
			return false;
		break;
	case CVT_V1731:
		if( !config.Write( _("/MISC/BOARD_TYPE"), _("V1731")))
			return false;
		break;
	case CVT_V1740:
		if( !config.Write( _("/MISC/BOARD_TYPE"), _("V1740")))
			return false;
		break;
	default:
		wxLogError( wxString::Format( "Unknown board type '%d'\n", this->m_board_type));
		break;
	}
	//
	// Set the board address
	wxString board_add_string= wxString::Format( "0x%08x", this->m_board_base_address);
	if( !config.Write( _("/MISC/ADDRESS"), board_add_string))
		return false;
	if( !config.Write( _("/MISC/INPUT_CLOCK_MHZ"), this->m_input_clock))
		return false;
	if( !config.Write( _("/MISC/VCXO_FREQ_MHZ"), this->m_vcxo_freq))
		return false;
	if( !config.Write( _("/MISC/PLL_MODE"), this->m_PLL_mode? 1: 0))
		return false;
	if( !config.Write( _("/MISC/CLOCK_OUT_ENABLE"), this->m_clock_out_enable? 1: 0))
		return false;
	if( !config.Write( _("/MISC/CLOCK_OUT_DELAY"), this->m_clock_out_delay))
		return false;
	if( !config.Write( _("/MISC/CLOCK_OUT_DELAY_ENABLE"), this->m_clock_out_delay_enable? 1: 0))
		return false;
	if( !config.Write( _("/MISC/CLOCK_OUT_DIVIDER"), this->m_clock_out_divider))
		return false;
	if( !config.Write( _("/MISC/ADC_DIVIDER"), this->m_adc_divider))
		return false;
	
	if( !config.Save( out_stream))
	{
		wxString msg= wxString::Format( "Errors saving configuration file '%s'\n", filename.c_str());
		wxLogError( msg);
		return false;
	}
	this->m_is_dirty= false;
	return true;
}
