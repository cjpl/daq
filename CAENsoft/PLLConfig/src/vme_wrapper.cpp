// Name:        VMEWrapper.cpp
// Purpose:     
// Author:      NDA
// Modified by: 
// Created:     09/04/07 16:17:15
// RCS-ID:      $Id: vme_wrapper.cpp,v 1.1 2009/02/18 08:25:37 Colombini Exp $
// Copyright:   CAEN S.p.A. All rights reserved
// Licence:     
/////////////////////////////////////////////////////////////////////////////


#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "vme_wrapper.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "vme_wrapper.h"

#ifndef CAEN_VME_INVALID_HANDLE
	#define CAEN_VME_INVALID_HANDLE			-1
#endif


VMEWrapper::VMEWrapper( )
{
	memset( &this->m_input_data, 0, sizeof( this->m_input_data));
	memset( &this->m_handle, 0, sizeof( this->m_handle));
	this->m_handle.m_vme_handle= CAEN_VME_INVALID_HANDLE;
}

VMEWrapper::~VMEWrapper(void)
{
    End();
}
bool VMEWrapper::Init( const InputData& input_data)
{
	//
	// HACK: Overwrite this according to your VME library specific needs
	// 

	memcpy( &this->m_input_data, &input_data, sizeof( this->m_input_data));

	wxString vme_board_type_string( this->m_input_data.m_board_type);

	// Init CAEN VME handle
	if( !vme_board_type_string.CmpNoCase(_("V1718")))
	{
		CVErrorCodes cv_error_code;
		if( ( cv_error_code= CAENVME_Init( cvV1718, this->m_input_data.m_vme_link, this->m_input_data.m_vme_board_num, &this->m_handle.m_vme_handle))!= cvSuccess)
		{
			wxLogError( wxString::Format( _("VMEWrapper::Init failure board type= %s link= %i board num= %i error= %s"), vme_board_type_string.c_str(), this->m_input_data.m_vme_link, this->m_input_data.m_vme_board_num, CAENVME_DecodeError( cv_error_code) ));
			return false;
		}
	}
	else if( !vme_board_type_string.CmpNoCase(_("V2718")))
	{
		CVErrorCodes cv_error_code;
		if( ( cv_error_code= CAENVME_Init( cvV2718, this->m_input_data.m_vme_link, this->m_input_data.m_vme_board_num, &this->m_handle.m_vme_handle))!= cvSuccess)
		{
			wxLogError( wxString::Format( _("VMEWrapper::Init failure board type= %s link= %i board num= %i error= %s"), vme_board_type_string.c_str(), this->m_input_data.m_vme_link, this->m_input_data.m_vme_board_num, CAENVME_DecodeError( cv_error_code) ));
			return false;
		}
	}
	else
	{
		// Automatically detect board type
		CVErrorCodes cv_error_code;
		// try V1718
		if( ( cv_error_code= CAENVME_Init( cvV1718, this->m_input_data.m_vme_link, this->m_input_data.m_vme_board_num, &this->m_handle.m_vme_handle))== cvSuccess)
		{
			return true;
		}
		// try V2718
		if( ( cv_error_code= CAENVME_Init( cvV2718, this->m_input_data.m_vme_link, this->m_input_data.m_vme_board_num, &this->m_handle.m_vme_handle))== cvSuccess)
		{
			return true;
		}
		wxLogError( wxString::Format( _("VMEWrapper::Init Cannot find any vme board (V1718 or V2718) on link '%d' and board number '%d'"), this->m_input_data.m_vme_link, this->m_input_data.m_vme_board_num));
		return false;
	}
	return true;
}
bool VMEWrapper::End( void)
{
	//
	// HACK: Overwrite this according to your VME library specific needs
	// 

	CVErrorCodes err_code;
    if(this->m_handle.m_vme_handle == CAEN_VME_INVALID_HANDLE)
        return true;

	if( ( err_code= CAENVME_End( this->m_handle.m_vme_handle))!= cvSuccess)
	{
		wxLogError( wxString::Format( _("VMEWrapper::End failure board type= %s link= %i board num= %i error= '%s'"), this->m_input_data.m_board_type, this->m_input_data.m_vme_link, this->m_input_data.m_vme_board_num, CAENVME_DecodeError( err_code)));
		return false;
	}		
	this->m_handle.m_vme_handle= CAEN_VME_INVALID_HANDLE;
	return true;
}

bool VMEWrapper::ClearBitmaskReg( UINT32 reg_offset, UINT32 reg_value)
{
	UINT32 org_reg_value= 0;
	if( !this->ReadReg( reg_offset, org_reg_value))
		return false;
	org_reg_value&= ~reg_value;
	if( !this->WriteReg( reg_offset, org_reg_value))
		return false;
	return true;
}
bool VMEWrapper::SetBitmaskReg( UINT32 reg_offset, UINT32 reg_value)
{
	UINT32 org_reg_value= 0;
	if( !this->ReadReg( reg_offset, org_reg_value))
		return false;
	org_reg_value|= reg_value;
	if( !this->WriteReg( reg_offset, org_reg_value))
		return false;
	return true;
}
bool VMEWrapper::WriteReg( UINT32 reg_offset, UINT32 reg_value)
{
	//
	// HACK: Overwrite this according to your VME library specific needs
	// 
	if( this->m_handle.m_vme_handle== CAEN_VME_INVALID_HANDLE)
	{
		wxLogError( wxString::Format( _("VMEWrapper::WriteReg error '%s'"), "Invalid handle value"));
		return false;
	}
	CVErrorCodes err_code;
	if( ( err_code= CAENVME_WriteCycle( this->m_handle.m_vme_handle, this->m_input_data.m_board_base_add+ reg_offset, (void*)&reg_value, cvA32_S_DATA, cvD32))!= cvSuccess)
	{
		wxLogError( wxString::Format( _("VMEWrapper::WriteReg error '%s'"), CAENVME_DecodeError( err_code)));
		return false;
	}
	return true;
}

bool VMEWrapper::ReadReg( UINT32 reg_offset, UINT32& reg_value)
{
	//
	// HACK: Overwrite this according to your VME library specific needs
	// 

	if( this->m_handle.m_vme_handle== CAEN_VME_INVALID_HANDLE)
	{
		wxLogError( wxString::Format( _("VMEWrapper::ReadReg error '%s'"), "Invalid handle value"));
		return false;
	}
	CVErrorCodes err_code;
	if( ( err_code= CAENVME_ReadCycle( this->m_handle.m_vme_handle, this->m_input_data.m_board_base_add+ reg_offset, (void*)&reg_value, cvA32_S_DATA, cvD32))!= cvSuccess)
	{
		wxLogError( wxString::Format( _("VMEWrapper::ReadReg error '%s'"), CAENVME_DecodeError( err_code)));
		return false;
	}
	return true;
}

