// Name:        appsettings.cpp
// Purpose:     
// Author:      NDA
// Modified by: 
// Created:     09/04/07 16:17:15
// RCS-ID:      $Id: appsettings.cpp,v 1.1 2009/02/18 08:25:35 Colombini Exp $
// Copyright:   CAEN S.p.A. All rights reserved
// Licence:     
/////////////////////////////////////////////////////////////////////////////


#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "appsettings.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "appsettings.h"
#include "V1720_board.h"
#include "V1721_board.h"
#include "V1731_board.h"
#include "V1724_board.h"
#include "V1740_board.h"

AppSettings::AppSettings( ): m_vme_link(0), m_vme_board_num(0),	m_vme_base_address(0x32100000), m_vme_clkin_freq(50.00), m_mutex( wxMUTEX_RECURSIVE)
{
	wxString setting_filename;

	#ifdef  __WIN32__
		setting_filename= wxGetCwd()+ wxFileName::GetPathSeparator()+ _("settings.txt");
	
	#else
		setting_filename= _("/etc/CAENPLLConfig/settings.txt");
	#endif
	if( wxTheApp->argc>= 2)
		setting_filename= wxTheApp->argv[ 1];
	if (!wxFile::Exists(setting_filename)) {
		 wxLogError(_T("Unable to find the settings file ")+ setting_filename);
		return;
	}
	wxConfigBase::Set( new wxFileConfig( wxEmptyString, wxEmptyString, setting_filename, setting_filename, wxCONFIG_USE_LOCAL_FILE ));
}

AppSettings::~AppSettings(void)
{
	for( size_t i= 0; i< this->m_board_array.GetCount(); i++) {
		delete (GenericBoard*)this->m_board_array[i];
	}
	delete wxConfigBase::Set((wxConfigBase *) NULL);
}
/*!
* Load data
*/
bool AppSettings::Load( void)
{
	/*
	Application setting file structure
	[MISC]
	|
	|- VME_BOARD_TYPE=V1718
	|- VME_LINK=0
	|- VME_BOARD_NUM=0
	|- TEMPLATE_CONFIG_FOLDER= "./templates/"
	|- 
	|- ACQ_BOARD_NUM= 4
	[ACQ_BOARDS]
	|
	|- [0]
	|   |
	|   |- BOARD_TYPE= V1724
	|   |- ( Other board data here )
	|   ...
	|- [1]
	|   |
	|   |- BOARD_TYPE= V1720
	|   |- ( Other board data here )
	|- [2]
	|   |
	|   |- BOARD_TYPE= V1721
	|   |- ( Other board data here )
	|- [3]
	|   |
	|   |- BOARD_TYPE= V1731
	|   |- ( Other board data here )
	|- [4]
	|   |
	|   |- BOARD_TYPE= V1731_DES
	|   |- ( Other board data here )
	|- [5]
	|   |
	|   |- BOARD_TYPE= V1740
	|   |- ( Other board data here )
	*/

	unsigned int i;
	// Delete previously stored boards
	this->m_board_array.Clear();
	wxConfigBase* config= wxConfigBase::Get( false);
	if( !config)
	{
		wxLogError( _("Cannot get configuration file\n"));
		return false;
	}

	wxMutexLocker lock( this->m_mutex);
	
	//
	// Get Record Folder
	config->Read(  _("/MISC/TEMPLATE_CONFIG_FOLDER"), &this->m_template_config_folder, _("./templates/"));
	// Sanity checks
	if( !wxDir::Exists( this->m_template_config_folder))
	{
		// Set to current directory
	  this->m_template_config_folder= _("./");
	}

	//
	// Get VME board type
	config->Read( _("/MISC/VME_BOARD_TYPE"), &this->m_vme_board_type_string, _("V1718"));
	//
	// Get VME board link
	config->Read( _("/MISC/VME_LINK"), &this->m_vme_link, 0);

    //
	// Get VME base adddress

	//
	// Get the board address
	wxString board_add_string= _("");
	config->Read( _("/MISC/VME_BASE_ADDRESS"), &board_add_string, _("32100000"));
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
    this->m_vme_base_address = (UINT32)board_add;


	// Get PLL CLKIN Frequency (MHz)
    config->Read( _("/MISC/CLKIN_FREQ"), &this->m_vme_clkin_freq, 50.00);

    //
	// Get VME board number
	config->Read( _("/MISC/VME_BOARD_NUM"), &this->m_vme_board_num, 0);

	int acq_board_num= 0;
	config->Read( _("/MISC/ACQ_BOARD_NUM"), &acq_board_num, 0);

	wxString board_base_string= _("/ACQ_BOARDS/");

	//
	// Boards loop
	for( i= 0; i< (unsigned int)acq_board_num; i++)
	{
		GenericBoard *board= ( GenericBoard *)0;
		wxString board_string= wxString::Format(_("%s%i/"), board_base_string.c_str(), i);

		//
		// Get the board type
		wxString board_type= _("");
		config->Read( board_string+ _("BOARD_TYPE"), &board_type, _("V1724"));
		// A board was specified : create a board of proper type
		if( !board_type.Trim().CmpNoCase( _("V1724")))
		{
			// V1724
			board= new V1724Board( this);
		}
		else if( !board_type.Trim().CmpNoCase( _("V1721")))
		{
			// V1721
			board= new V1721Board( this);
		}
		else if( !board_type.Trim().CmpNoCase( _("V1720")))
		{
			// V1720
			board= new V1720Board( this);
		}
		else if( !board_type.Trim().CmpNoCase( _("V1731")))
		{
			// V1731 Not DES mode
			board= new V1731Board( this, false);
		}
		else if( !board_type.Trim().CmpNoCase( _("V1731_DES")))
		{ 
			// V1731 DES mode 
			board= new V1731Board( this, true);
		}
		else if( !board_type.Trim().CmpNoCase( _("V1740")))
		{
			// V1740
			board= new V1740Board( this);
		}
		else
		{
			// Unknown board type
		  wxLogError( wxString::Format(_("Unknown board type '%s'\n"), board_type.c_str()));
			// return false;
		}
		if( !board)
		{
			continue;
		}
		this->m_board_array.Add( board);
		this->m_board_type_strings.Add( board_type);

		// Load board section
		if( !((GenericBoard*)this->m_board_array[i])->LoadConfig( config, board_string))
		{
			wxLogError( _("Error loading board settings\n"));
			// return false;
		}
	}
	return true;
}

/*!
* save data
*/
bool AppSettings::Save( void)
{

	/*
	Application setting file structure
	[MISC]
	|
	|- VME_BOARD_TYPE=V1718
	|- VME_LINK=0
	|- VME_BOARD_NUM=0
	|- TEMPLATE_CONFIG_FOLDER= "./templates/"
	|- 
	|- ACQ_BOARD_NUM= 4
	[ACQ_BOARDS]
	|
	|- [0]
	|   |
	|   |- BOARD_TYPE= V1724
	|   |- ( Other board data here )
	|   ...
	|- [1]
	|   |
	|   |- BOARD_TYPE= V1720
	|   |- ( Other board data here )
	|- [2]
	|   |
	|   |- BOARD_TYPE= V1721
	|   |- ( Other board data here )
	|- [3]
	|   |
	|   |- BOARD_TYPE= V1731
	|   |- ( Other board data here )
	|- [4]
	|   |
	|   |- BOARD_TYPE= V1731_DES
	|   |- ( Other board data here )
	|- [5]
	|   |
	|   |- BOARD_TYPE= V1740
	|   |- ( Other board data here )
	*/

	wxConfigBase* config= wxConfigBase::Get( false);
	if( !config)
		return false;
	wxMutexLocker lock( this->m_mutex);

	//
	// Delete groups
	wxString group_id;
	long group_index;
	while( config->GetFirstGroup( group_id, group_index))
	{
		config->DeleteGroup( group_id);
	}
	//
	// Set Record Folder
	if( !config->Write( _("/MISC/TEMPLATE_CONFIG_FOLDER"), this->m_template_config_folder))
		return false;

	//
	// Set VME board type
	if( !config->Write( _("/MISC/VME_BOARD_TYPE"), this->m_vme_board_type_string))
		return false;
	//
	// Set VME board link
	if( !config->Write( _("/MISC/VME_LINK"), this->m_vme_link))
		return false;

    //
	// Set VME base address
	wxString board_add_string= wxString::Format(_("0x%08x"), this->m_vme_base_address);
	if( !config->Write( _("/MISC/VME_BASE_ADDRESS"), board_add_string))
		return false;

	//
	// Set PLL CLKIN Frequency
    if( !config->Write( _("/MISC/CLKIN_FREQ"), this->m_vme_clkin_freq))
		return false;

    //
	// Set VME board number
	if( !config->Write( _("/MISC/VME_BOARD_NUM"), this->m_vme_board_num))
		return false;

	//
	// Set board number
	if( !config->Write( _("/MISC/ACQ_BOARD_NUM"), (int)this->m_board_array.GetCount()))
		return false;

	wxString board_base_string= _("/ACQ_BOARDS/");
	//
	// Boards loop
	for( size_t i= 0; i< this->m_board_array.GetCount(); i++)
	{
	  wxString board_string= wxString::Format(_("%s%i/"), board_base_string.c_str(), i);
		//
		// Set the board type
		if( !config->Write( board_string+ _("BOARD_TYPE"), this->m_board_type_strings[ i]))
			return false;

		// Save board section
		if( !((GenericBoard*)this->m_board_array[i])->SaveConfig( config, board_string))
			return false;
	}
	if( !config->Flush())
		return false;

	return true;
}
