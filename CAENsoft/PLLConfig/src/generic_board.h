/////////////////////////////////////////////////////////////////////////////
// Name:        generic_board.h
// Purpose:     
// Author:      NDA
// Modified by: 
// Created:     09/04/07 14:18:40
// RCS-ID:      $Id: generic_board.h,v 1.1 2009/02/18 08:25:35 Colombini Exp $
// Copyright:   CAEN S.p.A All rights reserved.
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _GENERIC_BOARD_H_
#define _GENERIC_BOARD_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "generic_board.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

/*!
 * Includes
 */
#include <wx/arrimpl.cpp> 
#include <wx/arrimpl.cpp> 
#include <wx/config.h>			
#include <wx/confbase.h>		
#include <wx/fileconf.h>		
#include <wx/filename.h>
#include <wx/dir.h>

extern "C" 
{
	#include "cvt_board_commons.h"
	#include "cvt_V1724.h"
}
class AppSettings;
class ConfigDoc;
class VMEWrapper;

class GenericBoard
{
public:
	GenericBoard( AppSettings *p_app_settings);
	virtual ~GenericBoard(void);

	//
	// Properties

	//
	// Methods
	virtual bool LoadConfig( wxConfigBase* p_config, const wxString& base_section);
	virtual bool SaveConfig( wxConfigBase* p_config, const wxString& base_section);
	const wxString& GetTemplateConfigFile(){ return this->m_template_config_file;};
	int GetMinFreq( ) const { return this->m_min_freq;}
	int GetMaxFreq( ) const { return this->m_max_freq;}
	int GetPFDFreqMHz( ) const { return this->m_PFD_freq_MHz;}
	int GetPFDFreqToleranceMHz( ) const { return this->m_PFD_freq_tolerance_MHz;}
	double GetClockOutDelayBaseNS( ) const { return this->m_clockout_delay_base_ns;}
	double GetClockOutDelayStepNS( ) const { return this->m_clockout_delay_step_ns;}
	int GetPrescaler( ) const { return this->m_prescaler;}
	static bool ApproximateFin( float &f_in, float f_vcxo, float f_pfd, float f_pfd_tolerance, int &R, int &N);

	// 
	// Pure virtual methods
	virtual wxString GetTypeString()= 0;
	virtual CVT_V17XX_TYPES GetType()= 0;
	virtual double GetClockMHz() const = 0;
	virtual int GetSampleBit() const = 0;
	virtual double GetVolt2Bit() const = 0;
	virtual UINT8 GetChEnableMsk() const = 0;
	virtual double GetFPGADivider() const = 0;

	// 
	// Virtual methods
	virtual double GetDACUpdateFactor( void){ return 1.0;};
	virtual bool DownLoadFile( const wxString &filename, UINT32 board_add);
	virtual bool MakeDownloadFile( const wxString &filename, const ConfigDoc &config);
	virtual bool ReadVCXOType( int &type, UINT32 board_add) const;
	virtual bool GetVCXOFreq( int type, int &freq_mhz) const;
protected:
	bool UpdateParamFromTemplate();
	bool PLLUpgrade( const char* filename, UINT32 board_add);
	bool WriteFlashPage( const UINT8* page_buff, UINT32 page_index, VMEWrapper& vme_wrapper);
	AppSettings* m_p_app_settings;
	wxString m_template_config_file;
	int m_min_freq;
	int m_max_freq;

	int m_VCXO_freq_MHz;
	int m_PFD_freq_MHz;
	int m_PFD_freq_tolerance_MHz;
	double m_clockout_delay_base_ns;
	double m_clockout_delay_step_ns;
	int m_prescaler;
	
	int m_VCXO_types_num;
	int *m_VCXO_types;

};


#endif		// _GENERIC_BOARD_H_

