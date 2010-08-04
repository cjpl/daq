/////////////////////////////////////////////////////////////////////////////
// Name:        appsettings.h
// Purpose:     
// Author:      NDA
// Modified by: 
// Created:     09/04/07 16:17:15
// RCS-ID:      $Id: appsettings.h,v 1.1 2009/02/18 08:25:35 Colombini Exp $
// Copyright:   CAEN S.p.A. All rights reserved
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _APPSETTINGS_H_
#define _APPSETTINGS_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "appsettings.h"
#endif

#include <wx/thread.h>
#include <wx/arrimpl.cpp> 
#include <wx/config.h>			
#include <wx/confbase.h>		
#include <wx/fileconf.h>		
#include <wx/filename.h>
#include <wx/dir.h>

#include "common_defs.h"

class wxMutex;

class AppSettings
{
public:
	//
	// Methods
	AppSettings( void);
	~AppSettings( void);
	bool Load( void);
	bool Save( void);

	//
	// Common lock
	wxMutex m_mutex;

	//
	// Properties
	wxArrayPtrVoid m_board_array;				// Boards array
	wxArrayString m_board_type_strings;			// Board type strings
	wxString m_template_config_folder;			// The folder where to retrive template configuration files from
	wxString m_root_dir;

	const wxString GetVMEBoardTypeString( ) const { return this->m_vme_board_type_string;}
	int GetVMELink( ) const { return this->m_vme_link;}
	int GetVMEBoardNum( ) const { return this->m_vme_board_num;}
    int GetVMEBaseAddreess( ) const { return this->m_vme_base_address;}
    double GetVMEClkinFreq( ) const { return this->m_vme_clkin_freq;}
    void SetVMEClkinFreq(double val) {this->m_vme_clkin_freq = val;} 

protected:
	wxString m_vme_board_type_string;

	int m_vme_link;
	int m_vme_base_address;
	int m_vme_board_num;
    double m_vme_clkin_freq;

};


#endif


