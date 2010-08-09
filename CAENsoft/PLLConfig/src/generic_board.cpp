/////////////////////////////////////////////////////////////////////////////
// Name:        generic_board.cpp
// Purpose:     
// Author:      NDA
// Modified by: 
// Created:     09/04/07 14:18:40
// RCS-ID:      $Id: generic_board.cpp,v 1.2 2009/03/02 09:00:13 Colombini Exp $
// Copyright:   CAEN S.p.A. All rights reserved
// Licence:     
/////////////////////////////////////////////////////////////////////////////


#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "generic_board.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/tokenzr.h>
#include <wx/utils.h>
#endif

#include "appsettings.h"
#include "generic_board.h"
#include "config_doc.h"
#include "vme_wrapper.h"

GenericBoard::GenericBoard( AppSettings *p_app_settings): 
							m_p_app_settings( p_app_settings),
							m_template_config_file( _("")),
							m_VCXO_types( NULL),
							m_VCXO_types_num( 0)
{

}

GenericBoard::~GenericBoard( )
{	
	if( this->m_VCXO_types) 
	{
		delete this->m_VCXO_types;
	}
	this->m_VCXO_types_num= 0;
}

bool GenericBoard::LoadConfig( wxConfigBase* p_config, const wxString& base_section)
{
	/*
		Application setting board parameters' file structure

	      |- TEMPLATE_CONFIG_FILE= "vXXXX_template.txt"
		  |- MIN_FREQ_MHZ= 10
		  |- MAX_FREQ_MHZ= 100
		  |- VCXO_FREQ_MHZ= 1000
		  |- PFD_FREQ_MHZ= 10
		  |- PFD_FREQ_TOLERANCE_MHZ= 5
		  |- CLOCKOUT_DELAY_BASE_NS= 0.570
		  |- CLOCKOUT_DELAY_STEP_NS= 0.300
		  |- PRESCALER= 4
		  |- VCXO_TYPES_NUM= 3
		  |- [VCXO_TYPES]
		  |- [0]
		  |   |
		  |   |-- FREQ_MHZ= 1000
		  |- [1]
		  |   |
		  |   |-- FREQ_MHZ= 500
		  |- [2]
		  |   |
		  |   |-- FREQ_MHZ= 500

	*/
	//
	// Get the configuration template file name
	this->m_template_config_file= _("");
	p_config->Read( base_section+ _("TEMPLATE_CONFIG_FILE"), &this->m_template_config_file, _(""));

	//
	// Get minimum frequency (MHz)
	p_config->Read( base_section+ _("MIN_FREQ_MHZ"), &this->m_min_freq, 10);

	//
	// Get maximum frequency (MHz)
	p_config->Read( base_section+ _("MAX_FREQ_MHZ"), &this->m_max_freq, 100);

	//
	// Get VCXO frequency (MHz)
	p_config->Read( base_section+ _("VCXO_FREQ_MHZ"), &this->m_VCXO_freq_MHz, 1000);

	//
	// Get PFD frequency (MHz)
	p_config->Read( base_section+ _("PFD_FREQ_MHZ"), &this->m_PFD_freq_MHz, 10);
	
	//
	// Get PFD frequency tolerance (MHz)
	p_config->Read( base_section+ _("PFD_FREQ_TOLERANCE_MHZ"), &this->m_PFD_freq_tolerance_MHz, 5);

	//
	// Get Clock out delay base value (nS)
	p_config->Read( base_section+ _("CLOCKOUT_DELAY_BASE_NS"), &this->m_clockout_delay_base_ns, 0.570);

	//
	// Get Clock out delay step (nS)
	p_config->Read( base_section+ _("CLOCKOUT_DELAY_STEP_NS"), &this->m_clockout_delay_step_ns, 0.300);

	//
	// Get prescaler value 
	//
	//  P =  2	( 2/ 3)
	//  P =  4	( 4/ 5)
	//  P =  8	( 8/ 9)
	//  P = 16	(16/17)
	//  P = 32	(32/33)
	p_config->Read( base_section+ _("PRESCALER"), &this->m_prescaler, 4);

	this->m_VCXO_types_num= 0;
	p_config->Read( base_section+ _("VCXO_TYPES_NUM"), &this->m_VCXO_types_num, 0);

	if( this->m_VCXO_types)
	{
		delete this->m_VCXO_types;
		this->m_VCXO_types= NULL;
	}
	if( this->m_VCXO_types_num) {
		this->m_VCXO_types= new int[ this->m_VCXO_types_num];
	}
	wxString vcxo_types_base_string= base_section+ _("VCXO_TYPES/");
	//
	// Vcxo Types loop
	for( int i= 0; i< this->m_VCXO_types_num; i++)
	{
	  wxString vcxo_string= wxString::Format(_("%s%i/"), vcxo_types_base_string.c_str(), i);
		p_config->Read( vcxo_string+ _("FREQ_MHZ"), &this->m_VCXO_types[ i], 0);
	}

	this->UpdateParamFromTemplate();

	return true;
}
bool GenericBoard::SaveConfig( wxConfigBase* p_config, const wxString& base_section)
{
	/*
		Application setting board parameters' file structure

	      |- TEMPLATE_CONFIG_FILE= "vXXXX_template.txt"
		  |- MIN_FREQ_MHZ= 10
		  |- MAX_FREQ_MHZ= 100
		  |- VCXO_FREQ_MHZ= 1000
		  |- PFD_FREQ_MHZ= 10
		  |- PFD_FREQ_TOLERANCE_MHZ= 5
		  |- CLOCKOUT_DELAY_BASE_NS= 0.570
		  |- CLOCKOUT_DELAY_STEP_NS= 0.300
		  |- PRESCALER= 4
		  |- VCXO_TYPES_NUM= 3
		  |- [VCXO_TYPES]
		  |- [0]
		  |   |
		  |   |-- FREQ_MHZ= 1000
		  |- [1]
		  |   |
		  |   |-- FREQ_MHZ= 500
		  |- [2]
		  |   |
		  |   |-- FREQ_MHZ= 500
	*/

	//
	// Set the configuration template file name
	if( !p_config->Write( base_section+ _("TEMPLATE_CONFIG_FILE"), this->m_template_config_file))
		return false;

	//
	// Get minimum frequency (MHz)
	if( !p_config->Write( base_section+ _("MIN_FREQ_MHZ"), this->m_min_freq))
		return false;

	//
	// Get maximum frequency (MHz)
	if( !p_config->Write( base_section+ _("MAX_FREQ_MHZ"), this->m_max_freq))
		return false;

	//
	// Set VCXO frequency (MHz)
	if( !p_config->Write( base_section+ _("VCXO_FREQ_MHZ"), this->m_VCXO_freq_MHz))
		return false;

	//
	// Set PFD frequency (MHz)
	if( !p_config->Write( base_section+ _("PFD_FREQ_MHZ"), this->m_PFD_freq_MHz))
		return false;

	//
	// Set PFD frequency tolerance (MHz)
	if( !p_config->Write( base_section+ _("PFD_FREQ_TOLERANCE_MHZ"), this->m_PFD_freq_tolerance_MHz))
		return false;

	//
	// Set Clock out delay base value (nS)
	if( !p_config->Write( base_section+ _("CLOCKOUT_DELAY_BASE_NS"), this->m_clockout_delay_base_ns))
		return false;

	//
	// Set Clock out delay step (nS)
	if( !p_config->Write( base_section+ _("CLOCKOUT_DELAY_STEP_NS"), this->m_clockout_delay_step_ns))
		return false;

	//
	// Get prescaler value 
	//
	//  P =  2	( 2/ 3)
	//  P =  4	( 4/ 5)
	//  P =  8	( 8/ 9)
	//  P = 16	(16/17)
	//  P = 32	(32/33)
	if( !p_config->Write( base_section+ _("PRESCALER"), this->m_prescaler))
		return false;

	//
	// Set board number
	if( !p_config->Write( base_section+ _("VCXO_TYPES_NUM"), (int)this->m_VCXO_types_num))
		return false;

	wxString vcxo_types_base_string= base_section+ _("VCXO_TYPES/");
	//
	// Boards loop
	for( int i= 0; i< this->m_VCXO_types_num; i++)
	{
	  wxString vcxo_string= wxString::Format(_("%s%i/"), vcxo_types_base_string.c_str(), i);
		if( !p_config->Write( vcxo_string+ _("FREQ_MHZ"), this->m_VCXO_types[ i]))
			return false;
	}
	return true;
}

bool GenericBoard::GetVCXOFreq( int type, int &freq_mhz) const 
{
	if( !this->m_VCXO_types|| ( type< 0) || ( type >= this->m_VCXO_types_num) ) 
	{
		return false;
	}
	freq_mhz= this->m_VCXO_types[ type];
	return true;
}

bool GenericBoard::DownLoadFile( const wxString &filename, UINT32 board_add)
{
  return this->PLLUpgrade( filename.ToAscii(), board_add);
}

bool GenericBoard::UpdateParamFromTemplate()
{
	// Update some default settings from template file
	wxFileName template_file_name( this->m_p_app_settings->m_template_config_folder+ this->m_template_config_file);

	if( !template_file_name.IsAbsolute())
	{
		if( !template_file_name.MakeAbsolute( this->m_p_app_settings->m_root_dir))
		{
		  wxString msg= wxString::Format(_("%s: Cannot find template file '%s'."), template_file_name.GetFullPath().c_str());
			wxMessageBox( msg, wxT("CAENPLLConfig"), wxOK | wxCENTRE | wxICON_ERROR  );
			return false;
		}
	}

	if( !template_file_name.FileExists())
	{
	  wxString msg= wxString::Format(_("Cannot find template file '%s'."), template_file_name.GetFullPath().c_str());
		wxMessageBox( msg, wxT("CAENPLLConfig"), wxOK | wxCENTRE | wxICON_ERROR  );
		return false;
	}
	wxFileInputStream in_stream( template_file_name.GetFullPath());
	if( !in_stream.Ok())
	{
	  wxString msg= wxString::Format(_("Cannot open template file '%s' for reading."), template_file_name.GetFullPath().c_str());
		wxMessageBox( msg, wxT("CAENPLLConfig"), wxOK | wxCENTRE | wxICON_ERROR  );
		return false;
	}
	wxTextInputStream text_in_stream( in_stream);

	wxString line;
	// Search for first empty line ""
	while( in_stream.CanRead())
	{
		line= text_in_stream.ReadLine( );
		if( !line.Trim().Cmp(_("\"\"")))
		{
			// found
			break;
		}
	}

	// Process tokens
	while( in_stream.CanRead())
	{
		line= text_in_stream.ReadLine( ).Trim();

		if( !line.Cmp(_("\"\"")))
		{
			// end of register found
			break;
		}
		wxStringTokenizer tkz( line, _(","));
		if( tkz.CountTokens()!= 3)
		{
			// spurious lines ?
			continue;
		}
		wxString token= tkz.NextToken().Trim();
		if( token.Length()< 2)
		{
			// spurious lines ?
			continue;
		}
		// remove " ... "
		wxString number_token= token.Mid( 1, token.Length()- 2);
		// Hex Register Address
		long address= 0;
		if( !number_token.ToLong( &address, 16))
		{
			// spurious lines ?
			continue;
		}
		// Binary value: skip it
		token= tkz.NextToken().Trim();

		// Hex value
		token= tkz.NextToken().Trim();
		if( token.Length()< 2)
		{
			// spurious lines ?
			continue;
		}
		// remove " ... "
		number_token= token.Mid( 1, token.Length()- 2);
		long value= 0;
		if( !number_token.ToLong( &value, 16))
		{
			// spurious lines ?
			continue;
		}

		unsigned char value_msk;
		switch( address)
		{
		case 0x39:
			// Reg: 0x39 Output clock Delay Ramp Capacitor [5..3] Ramp Current [2..0]
			// Affected BitMask= 0x38 , 0x07
			{
				value_msk= 0x38| 0x07;
				value&= ~value_msk;
				int I= value& 0x07;
				int C= ( value& 0x38)>> 3;
				double IR= 200*( I+ 1);
				double NC= 0;
				while( C)
				{
					NC+= ( C& 0x01)? 1: 0;
					C>>= 1;
				}
				NC= 3- NC+ 1;

				this->m_clockout_delay_base_ns= 0.34+ ( 1600- IR)*0.0001+ 6*(NC- 1)/IR;
				this->m_clockout_delay_step_ns= 1.3286*200*(NC+ 3)/IR/31;
			}
			break;
		}
	}

	return true;
}
bool GenericBoard::MakeDownloadFile( const wxString &filename, const ConfigDoc &config)
{
	wxFileName template_file_name( this->m_p_app_settings->m_template_config_folder+ this->m_template_config_file);

	if( !template_file_name.IsAbsolute())
	{
		if( !template_file_name.MakeAbsolute( this->m_p_app_settings->m_root_dir))
		{
		  wxString msg= wxString::Format(_("Cannot find template file '%s'."), template_file_name.GetFullPath().c_str());
			wxMessageBox( msg, wxT("CAENPLLConfig"), wxOK | wxCENTRE | wxICON_ERROR  );
			return false;
		}
	}

	if( !template_file_name.FileExists())
	{
	  wxString msg= wxString::Format(_("Cannot find template file '%s'."), template_file_name.GetFullPath().c_str());
		wxMessageBox( msg, wxT("CAENPLLConfig"), wxOK | wxCENTRE | wxICON_ERROR  );
		return false;
	}
	wxFileInputStream in_stream( template_file_name.GetFullPath());
	if( !in_stream.Ok())
	{
	  wxString msg= wxString::Format(_("Cannot open template file '%s' for reading."), template_file_name.GetFullPath().c_str());
		wxMessageBox( msg, wxT("CAENPLLConfig"), wxOK | wxCENTRE | wxICON_ERROR  );
		return false;
	}
	wxTextInputStream text_in_stream( in_stream);

	wxFileOutputStream out_stream( filename);

	if( !out_stream.Ok())
	{
	  wxString msg= wxString::Format(_("Cannot open download file '%s' for writing."), filename.c_str());
		wxMessageBox( msg, wxT("CAENPLLConfig"), wxOK | wxCENTRE | wxICON_ERROR  );
		return false;
	}
	wxTextOutputStream text_out_stream( out_stream);
	// Parse template file and make new download file

	wxString line;
	// Search for first empty line ""
	while( in_stream.CanRead())
	{
		line= text_in_stream.ReadLine( );
		text_out_stream<< line<< '\n';

		if( !line.Trim().Cmp(_("\"\"")))
		{
			// found
			break;
		}
	}
	//// Search for first data line
	//while( in_stream.CanRead())
	//{
	//	line= text_in_stream.ReadLine( );
	//	text_out_stream<< line<< '\n';
	//	wxStringTokenizer tkz( line.Trim(), ",");
	//	if( tkz.CountTokens()!= 3)
	//	{
	//		// found
	//		break;
	//	}
	//}

	// Calculate divider
	int R= 1, N= 1;
	float Fin_pll= config.GetInputClock();
	if( config.GetPLLMode()) {
		// PLL mode
		float f_vcxo= config.GetVCXOFreq();
		if( !GenericBoard::ApproximateFin( Fin_pll, f_vcxo, this->m_PFD_freq_MHz, this->m_PFD_freq_tolerance_MHz, R, N)) {
			wxMessageBox( _("Cannot get a valid dividers set and/or PFD frequency for the specified input clock value."), wxT("CAENPLLConfig"), wxOK | wxCENTRE | wxICON_ERROR  );
			return false;
		}
	}

	//K= 1;
	//double Fin_pll= this->m_VCXO_freq_MHz/ (double)K;
	//R= (int)( double)( Fin_pll/ this->m_PFD_freq_MHz);
	//if( R& ~0x3fff)
	//{
	//	wxLogError( _("R value too large!"));
	//	return false;
	//}
	//N= K* R;
	int counter_A, counter_B;
	if( this->m_prescaler== 0)
	{
		wxLogError( _("Invalid prescaler value !"));
		return false;
	}
	int P= this->m_prescaler;
	counter_B= N/ P;
	counter_A= N- counter_B* P;

	if( counter_A& ~0x3f)
	{
		wxLogError( _("Counter A value too large! Increase prescaler value !"));
		return false;
	}
	if( counter_B& ~0x1fff)
	{
		wxLogError( _("Counter B value too large! Increase prescaler value !"));
		return false;
	}
	// Process tokens
	while( in_stream.CanRead())
	{
		line= text_in_stream.ReadLine( ).Trim();

		if( !line.Cmp(_("\"\"")))
		{
			// end of register found
			text_out_stream<< line<< '\n';
			break;
		}
		wxStringTokenizer tkz( line,_(","));
		if( tkz.CountTokens()!= 3)
		{
			// spurious lines ?
			text_out_stream<< line<< '\n';
			continue;
		}
		wxString token= tkz.NextToken().Trim();
		if( token.Length()< 2)
		{
			// spurious lines ?
			text_out_stream<< line<< '\n';
			continue;
		}
		// remove " ... "
		wxString number_token= token.Mid( 1, token.Length()- 2);
		//if( !number_token.IsNumber())
		//{
		//	// spurious lines ?
		//	text_out_stream<< line<< '\n';
		//	continue;
		//}
		// Hex Register Address
		long address= 0;
		if( !number_token.ToLong( &address, 16))
		{
			// spurious lines ?
			text_out_stream<< line<< '\n';
			continue;
		}
		// Binary value: skip it
		token= tkz.NextToken().Trim();

		// Hex value
		token= tkz.NextToken().Trim();
		if( token.Length()< 2)
		{
			// spurious lines ?
			text_out_stream<< line<< '\n';
			continue;
		}
		// remove " ... "
		number_token= token.Mid( 1, token.Length()- 2);
		//if( !number_token.IsNumber())
		//{
		//	// spurious lines ?
		//	text_out_stream<< line<< '\n';
		//	continue;
		//}
		long value= 0;
		if( !number_token.ToLong( &value, 16))
		{
			// spurious lines ?
			text_out_stream<< line<< '\n';
			continue;
		}

		char bin_buff[ 9];
		unsigned char value_msk;
		switch( address)
		{
		case 0x04:
			//  Reg: 0x04  N divider Counter A ( 6 LSb )
			//	bit [0..5]
			value_msk= 0x3f;
			value&= ~value_msk;
			value|= (unsigned char)(counter_A& value_msk);
			text_out_stream<< wxString::Format(_("\"%02X\",\"%s\",\"%02X\""), address, BinFormat( (unsigned char)value, bin_buff), value)<< '\n';
			break;
		case 0x05:
			//  Reg: 0x05  N divider Counter B MSB  ( 5 LSb )
			//	0x05   -> MSB bit [0..4]
			//	0x06   -> LSB bit [0..7]
			value_msk= 0x1f;
			value&= ~value_msk;
			value|= (unsigned char)((int)( counter_B>> 8)& value_msk);
			text_out_stream<< wxString::Format(_("\"%02X\",\"%s\",\"%02X\""), address, BinFormat( (unsigned char)value, bin_buff), value)<< '\n';
			break;
		case 0x06:
			//  Reg: 0x05  N divider Counter B LSB
			//	0x05   -> MSB bit [0..4]
			//	0x06   -> LSB bit [0..7]
			value_msk= 0xff;
			value&= ~value_msk;
			value|= (unsigned char)( counter_B& value_msk);
			text_out_stream<< wxString::Format(_("\"%02X\",\"%s\",\"%02X\""), address, BinFormat( (unsigned char)value, bin_buff), value)<< '\n';
			break;
		case 0x0b:
			//  Reg: 0x0b  R divider MSB  ( 6 LSb )
			//	0x0b   -> MSB bit [0..5]
			//	0x0c   -> LSB bit [0..7]
			value_msk= 0x3f;
			value&= ~value_msk;
			value|= (unsigned char)((int)( R>> 8)& value_msk);
			text_out_stream<< wxString::Format(_("\"%02X\",\"%s\",\"%02X\""), address, BinFormat( (unsigned char)value, bin_buff), value)<< '\n';
			break;

		case 0x0c:
			//  Reg: 0x0c  R divider LSB
			//	0x0b   -> MSB bit [0..5]
			//	0x0c   -> LSB bit [0..7]
			value_msk= 0xff;
			value&= ~value_msk;
			value|= (unsigned char)( R& value_msk);
			text_out_stream<< wxString::Format(_("\"%02X\",\"%s\",\"%02X\""), address, BinFormat( (unsigned char)value, bin_buff), value)<< '\n';
			break;
		case 0x38:
			// Reg: 0x38 Output clock Delay [6] bypass
			// Affected BitMask= 0x01
			value_msk= 0x01;
			value&= ~value_msk;
			value|= !config.GetClockOutDelayEnable()? value_msk: 0x00;
			text_out_stream<< wxString::Format(_("\"%02X\",\"%s\",\"%02X\""), address, BinFormat( (unsigned char)value, bin_buff), value)<< '\n';
			break;
		case 0x3A:
			// Reg: 0x3A Output clock Delay [1..5] fine adjust
			// Affected BitMask= 0x3e
			value_msk= 0x3e;
			value&= ~value_msk;
			value|= (unsigned char)( config.GetClockOutDelay()<< 1)& value_msk;
			text_out_stream<< wxString::Format(_("\"%02X\",\"%s\",\"%02X\""), address, BinFormat( (unsigned char)value, bin_buff), value)<< '\n';
			break;
		case 0x42:
			// Reg: 0x42 CLOCK_OUT enable
			// set bit 0 to enable
			value_msk= 0x01;
			value&= ~value_msk;
			value|= !config.GetClockOutEnable()? 0x01: 0x00;
			text_out_stream<< wxString::Format(_("\"%02X\",\"%s\",\"%02X\""), address, BinFormat( (unsigned char)value, bin_buff), value)<< '\n';
			break;
		case 0x45:
			// Reg: 0x45 Pll mode= 0x02 Bypass mode: 0x1d 
			// Affected BitMask= 0x1f
			value_msk= 0x1f;
			value&= ~value_msk;
			value|= config.GetPLLMode()? 0x02: 0x1d;
			text_out_stream<< wxString::Format(_("\"%02X\",\"%s\",\"%02X\""), address, BinFormat( (unsigned char)value, bin_buff), value)<< '\n';
			break;
		case 0x48:
		case 0x4a:
		case 0x4c:
		case 0x4e:
			// Reg: 0x48, 0x4a, 0x4c, 0x4e Divider [0..3]: Low cycle [7..4] High cycle [3..0]
			// Affected BitMask= 0xff
			value_msk= 0xff;
			value&= ~value_msk;
			{
				int divider= config.GetADCDivider();
				if( divider< 2)
					divider= 2;
				int low_cycles= divider>> 1;
				int high_cycles= divider- low_cycles;
				value|= (unsigned char)( ( (high_cycles- 1)<< 4)| ( low_cycles-1));
			}
			text_out_stream<< wxString::Format(_("\"%02X\",\"%s\",\"%02X\""), address, BinFormat( (unsigned char)value, bin_buff), value)<< '\n';
			break;
		case 0x49:
		case 0x4b:
		case 0x4d:
		case 0x4f:
			// Reg: 0x49, 0x4b, 0x4d, 0x4f Divider [0..3] bypass
			// Affected BitMask= 0x80
			value_msk= 0x80;
			value&= ~value_msk;
			value|= ( config.GetADCDivider()== 1)? value_msk: 0x00;
			text_out_stream<< wxString::Format(_("\"%02X\",\"%s\",\"%02X\""), address, BinFormat( (unsigned char)value, bin_buff), value)<< '\n';
			break;
		case 0x52:
			// Reg: 0x52 FPGA Divider [5]: Low cycle [7..4] High cycle [3..0]
			// Affected BitMask= 0xff
			value_msk= 0xff;
			value&= ~value_msk;
			{
				int divider= (int)((double)config.GetADCDivider()* this->GetFPGADivider());
				if( divider< 2)
					divider= 2;
				int low_cycles= divider>> 1;
				int high_cycles= divider- low_cycles;
				value|= (unsigned char)( ( (high_cycles- 1)<< 4)| ( low_cycles-1));
			}
			text_out_stream<< wxString::Format(_("\"%02X\",\"%s\",\"%02X\""), address, BinFormat( (unsigned char)value, bin_buff), value)<< '\n';
			break;
		case 0x53:
			// Reg: 0x53 FPGA Divider [5] bypass
			// Affected BitMask= 0x80
			value_msk= 0x80;
			value&= ~value_msk;
			value|= ( ( (int)((double)config.GetADCDivider()* this->GetFPGADivider())) == 1)? value_msk: 0x00;
			text_out_stream<< wxString::Format(_("\"%02X\",\"%s\",\"%02X\""), address, BinFormat( (unsigned char)value, bin_buff), value)<< '\n';
			break;
		case 0x54:
			// Reg: 0x54 Output clock Divider [6]: Low cycle [7..4] High cycle [3..0]
			// Affected BitMask= 0xff
			value_msk= 0xff;
			value&= ~value_msk;
			{
				int divider= config.GetClockOutDivider();
				if( divider< 2)
					divider= 2;
				int low_cycles= divider>> 1;
				int high_cycles= divider- low_cycles;
				value|= (unsigned char)( ( (high_cycles- 1)<< 4)| ( low_cycles-1));
			}
			text_out_stream<< wxString::Format(_("\"%02X\",\"%s\",\"%02X\""), address, BinFormat( (unsigned char)value, bin_buff), value)<< '\n';
			break;
		case 0x55:
			// Reg: 0x55 Output clock Divider [6] bypass
			// Affected BitMask= 0x80
			value_msk= 0x80;
			value&= ~value_msk;
			value|= !config.GetClockOutEnable()? value_msk: 0x00;
			text_out_stream<< wxString::Format(_("\"%02X\",\"%s\",\"%02X\""), address, BinFormat( (unsigned char)value, bin_buff), value)<< '\n';
			break;
		default:
			text_out_stream<< line<< '\n';
			break;
		}
	}

	// Process tokens
	while( in_stream.CanRead())
	{
		line= text_in_stream.ReadLine( ).Trim();

		if( !line.Cmp(_("\"\"")))
		{
			// end of register found
			text_out_stream<< line<< '\n';
			break;
		}
		wxStringTokenizer tkz( line, _(","));
		if( tkz.CountTokens()!= 2)
		{
			// spurious lines ?
			text_out_stream<< line<< '\n';
			continue;
		}
		wxString token= tkz.NextToken().Trim();
		if( !token.Upper().Cmp(_("\"REFIN:\"")) ||
		    !token.Upper().Cmp(_("\"CLKINP1:\"")))
		{
			float freq;
			if( config.GetPLLMode())
			{
				freq= Fin_pll;
			}
			else
			{
				freq= config.GetInputClock();
			}
			text_out_stream<< token<< ','<< freq<< '\n';
            // HACK: se ci forssero problemi con la stampa dei float con separatore virgola.  
            //text_out_stream<< token<< ','<< wxString::Format("%f",freq) << '\n';
		}
		else if( !token.Upper().Cmp(_("\"CLKINP2:\"")))
		{
            float f_vcxo= config.GetVCXOFreq();
			text_out_stream<< token<< ','<< (int)f_vcxo << '\n';
		}
		else
		{
			text_out_stream<< line<< '\n';
		}
	}
	return true;
}
bool GenericBoard::ReadVCXOType( int &type, UINT32 board_add) const {

	VMEWrapper vme_wrapper;
	InputData in_data;
	strncpy( in_data.m_board_type, this->m_p_app_settings->GetVMEBoardTypeString().ToAscii(), sizeof( in_data.m_board_type));
	in_data.m_vme_board_num= this->m_p_app_settings->GetVMEBoardNum();
	in_data.m_vme_link= this->m_p_app_settings->GetVMELink();
	in_data.m_board_base_add= board_add;
	
	if( !vme_wrapper.Init( in_data))
		return false;

	UINT32 reg_value= 0;
	if( !vme_wrapper.ReadReg( CVT_V1724_ROM_VCXO_TYPE_ADD, reg_value))
	{
		wxLogError( _("ReadReg: CVT_V1724_ROM_VCXO_TYPE_ADD read failed !"));
		return false;
	}
	
	type= reg_value& 0x000000ff;
	return true;
}

bool GenericBoard::PLLUpgrade( const char* filename, UINT32 board_add)
{
	bool ret= true;
	UINT8 page[ V1724_FLASH_PAGE_SIZE];
	char file_line[300];
	// int org_line= 15;
	int i;
	int data_size;
	FILE *from= ( FILE *)0;
	VMEWrapper vme_wrapper;

	InputData in_data;
	strncpy( in_data.m_board_type, this->m_p_app_settings->GetVMEBoardTypeString().ToAscii(), sizeof( in_data.m_board_type));
	in_data.m_vme_board_num= this->m_p_app_settings->GetVMEBoardNum();
	in_data.m_vme_link= this->m_p_app_settings->GetVMELink();
	in_data.m_board_base_add= board_add;
	
	if( !vme_wrapper.Init( in_data))
		return false;

	if( ( from= fopen( filename, "rt"))== NULL)
	{
		wxLogError( _("PLLUpgrade: file open failure!"));
		ret= false;
		goto exit_point;
	}
	// Strip header lines
	for( i= 0; i< 4; i++)
	{
		if( fgets( file_line, 300, from)== NULL)
		{
			wxLogError( _("PLLUpgrade: bad file format !"));
			ret= false;
			goto exit_point;						
		}
	}
	data_size= 0;
	// Get data lines
	while( TRUE)
	{
		char *token;
		char *trash;
		int value;
		if( fgets( file_line, 300, from)== NULL)
		{
			if( !feof( from))
			{
				wxLogError( _("PLLUpgrade: file read error !"));
				ret= false;
				goto exit_point;
			}
			break;
		}
		if( data_size>= V1724_FLASH_PAGE_SIZE)
		{
			wxLogError( _("PLLUpgrade: file too large for flash page size !"));
			ret= false;
			goto exit_point;
		}
		// Get Address
		token= strtok( file_line, ",");
		// Skip a value
		trash= strtok( NULL, ",");

		if( ( token== 0)|| (trash== 0))
			break;
		if( !strcmp( token, "\"\""))
			break;
		sscanf( token, "\"%x\"", &value);
		page[ data_size++]= value& 0xff;

		// Get value
		token= strtok( NULL, ",");
		sscanf( token, "\"%x\"", &value);
		page[ data_size++]= value& 0xff;
	}

	if( !this->WriteFlashPage( page, CVT_V1724_PLL_FLASH_PAGE, vme_wrapper))
	{
		wxLogError( _("PLLUpgrade: flash write failed !"));
		ret= false;
		goto exit_point;
	}

	//
	// Reload configuration
	{
		UINT32 reg_value32= 0;
		if( !vme_wrapper.WriteReg( CVT_V1724_RELOAD_CONFIG_ADD, reg_value32))
		{
			wxLogError( _("PLLUpgrade: RELOAD_CONFIG write failed !"));
			ret= false;
			goto exit_point;
		}
	}
exit_point:
	if( from)
	{
		fclose( from);
	}
	vme_wrapper.End( );

	return ret;
}
bool GenericBoard::WriteFlashPage( const UINT8* page_buff, UINT32 page_index, VMEWrapper& vme_wrapper)
{
	int i;
	UINT32 reg_value;
	UINT32 flash_addr;
	bool ret_val= true;

	flash_addr= page_index << 9;

	//
	// enable flash
	reg_value= CVT_V1724_FLEN_EN_MSK;
	if( !vme_wrapper.ClearBitmaskReg( CVT_V1724_FLASH_EN_ADD, reg_value))
	{
		wxLogError( _("WriteFlashPage: CVT_V1724_FLASH_EN write failed !"));
		return false;
	}

	// write opcode
	reg_value= CVT_V1724_FOP_PAGE_PROG_TH_BUF1;
	if( !vme_wrapper.WriteReg( CVT_V1724_FLASH_DATA_ADD, reg_value))
	{
		wxLogError( _("WriteFlashPage: CVT_V1724_FLASH write failed !"));
		ret_val= false;
		goto exit_point;
	}
	// write address
	reg_value= (unsigned char)( flash_addr>> 16);
	if( !vme_wrapper.WriteReg( CVT_V1724_FLASH_DATA_ADD, reg_value))
	{
		wxLogError( _("WriteFlashPage: CVT_V1724_FLASH write failed !"));
		ret_val= false;
		goto exit_point;
	}
	reg_value= (unsigned char)( flash_addr>> 8);
	if( !vme_wrapper.WriteReg( CVT_V1724_FLASH_DATA_ADD, reg_value))
	{
		wxLogError( _("WriteFlashPage: CVT_V1724_FLASH write failed !"));
		ret_val= false;
		goto exit_point;
	}
	reg_value= (unsigned char) flash_addr;
	if( !vme_wrapper.WriteReg( CVT_V1724_FLASH_DATA_ADD, reg_value))
	{
		wxLogError( _("WriteFlashPage: CVT_V1724_FLASH write failed !"));
		ret_val= false;
		goto exit_point;
	}
	// write flash page
	for (i= 0; i< V1724_FLASH_PAGE_SIZE; i++)
	{
		reg_value= page_buff[ i];
		if( !vme_wrapper.WriteReg( CVT_V1724_FLASH_DATA_ADD, reg_value))
		{
			wxLogError( _("WriteFlashPage: CVT_V1724_FLASH write failed !"));
			ret_val= false;
			goto exit_point;
		}
	}

exit_point:
	// disable flash
	reg_value= CVT_V1724_FLEN_EN_MSK;
	if( !vme_wrapper.SetBitmaskReg( CVT_V1724_FLASH_EN_ADD, reg_value))
	{
		wxLogError( _("WriteFlashPage: CVT_V1724_FLASH_EN write failed !"));
		ret_val= false;
	}

	// wait 50ms
	::wxMilliSleep( 50);
	return ret_val;
}
bool GenericBoard::ApproximateFin( float &f_in, float f_vcxo, float f_pfd, float f_pfd_tolerance, int &R, int &N) {
	/*
		Gets the input f_in and return the approximated value to satisfy the flooawing constraints:
			f_in= f_vcxo* R/ N   R| min( abs( f_in/R- f_pfd))
		f_in must be initialized on init and will be updated on successfull exit
	*/
	if( f_in<= 0) {
		return false;
	}
	double min_err= 99999;
	double new_err;
	float approx_f_in, org_f_in;
	int r, n;

	org_f_in= f_in;
	for( r= 1; r<= 0x3fff; r++) {
		n= (int)(double)(( f_vcxo* (double)r)/ (double)org_f_in);
		if( n< 1) {
			continue;
		}
		approx_f_in= ( f_vcxo* r)/ n;
		new_err= fabs( approx_f_in/ (double)r- (double)f_pfd);
		if( ( new_err< min_err)&& ( new_err< f_pfd_tolerance)) {
			R= r;
			N= n;
			f_in= approx_f_in;
			min_err= new_err;
			if( min_err< 0.000001) {
				break;
			}
		}
	}
	return min_err!= 99999;
}

