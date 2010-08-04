/////////////////////////////////////////////////////////////////////////////
// Name:        V17XX_board.cpp
// Purpose:     
// Author:      NDA
// Modified by: 
// Created:     04/03/06 14:18:40
// RCS-ID:      
// Copyright:   CAEN S.p.A. All rights reserved
// Licence:     
/////////////////////////////////////////////////////////////////////////////


#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "V17XX_board.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "V17XX_board.h"
#include "V17XX_board_channel.h"
#include "appsettings.h"

V17XXBoard::V17XXBoard( int board_index, void (*scope_refresh)( int, bool), void (*update_samples_ready_led)( bool), wxMutex *p_data_mutex, AppSettings *p_app_settings, V17XXBoard::BOARD_VARIANT board_variant): 
						GenericBoard( board_index, scope_refresh, update_samples_ready_led, p_data_mutex, p_app_settings) 
{
	m_board_variant= board_variant;
	this->m_p_data= (cvt_board_data*)new cvt_V1724_data;
}

V17XXBoard::~V17XXBoard(void)
{
	if( this->m_p_data!= NULL)
	{
		cvt_V1724_close( (cvt_V1724_data* )this->m_p_data);
		delete (cvt_V1724_data* )this->m_p_data;
		this->m_p_data= NULL;
	}
}

bool V17XXBoard::Open( long /* vme_handle*/)
{
	return true;
}

bool V17XXBoard::Init( long vme_handle)
{
	//
	// Open the CAENVMETool module
	if( !cvt_V1724_open( (cvt_V1724_data* )this->m_p_data, this->m_p_data->m_base_address, vme_handle, this->GetType()))
		return false;
	//
	// Write trigger mode
	if( !this->WriteTriggerMode( false))
		return false;
	// 
	// Front Panel IO
	if( !this->SetFrontPanelIO( this->m_p_app_settings->m_use_TTL))
		return false;

	//
	// Check if board id and firmware revision are correct
	UINT16 firmware_rev;
	CVT_V1724_ROM_CONFIG rom_config;
	BOOL is_board_type_correct;
	BOOL is_firmware_rev_correct;
	//if( !cvt_V1724_get_system_info( (cvt_V1724_data* )this->m_p_data, &firmware_rev, &rom_config, &is_board_type_correct, &is_firmware_rev_correct))
	//{
	//	return false;
	//}
	if( !V17XXBoard::v1724_get_system_info_wrapper( (cvt_V1724_data* )this->m_p_data, &firmware_rev, &rom_config, &is_board_type_correct, &is_firmware_rev_correct))
	{
		return false;
	}
	if( !is_board_type_correct)
		wxLogError( wxString::Format( "Settings's board type '%s' doesn't match configuration rom board type\n", this->GetTypeString().c_str()));
	if( !is_firmware_rev_correct)
		wxLogError( wxString::Format( "The board firmware revision '%04x' is lesser than CAENVMETool's minimum firmware revision '%04x'. Please update the board's firmware.\n", firmware_rev, CVT_V1724_MIN_FIRMARE_REV));
	return true;
}

bool V17XXBoard::v1724_get_system_info_wrapper( cvt_V1724_data* p_data, UINT16 *p_firmware_rev, CVT_V1724_ROM_CONFIG *p_rom_config, BOOL *p_is_board_type_correct, BOOL *p_is_firmware_rev_correct) 
{
	// wrapper for the cvt_V1724_get_system_info API which doesn't know about DT and NIM ADCs
	if( !cvt_V1724_get_system_info( p_data, p_firmware_rev, p_rom_config, p_is_board_type_correct, p_is_firmware_rev_correct))
	{
		return false;
	}
	// Overridde the p_is_board_type_correct flag to support the DT and NIM ADCs
	switch( p_rom_config->m_board_id)
	{
	case CVT_V17XX_RCFG_BOARD_ID_V1724:			//  V1724
	case CVT_V17XX_RCFG_BOARD_ID_VX1724:		// VX1724
	case CVT_V17XX_RCFG_BOARD_ID_DT5724:		// DT5724
	case CVT_V17XX_RCFG_BOARD_ID_N6724:			//  N6724
		if( p_data->m_type== CVT_V1724)
			*p_is_board_type_correct= TRUE;
		break;
	case CVT_V17XX_RCFG_BOARD_ID_V1721:			//  V1721
	case CVT_V17XX_RCFG_BOARD_ID_VX1721:		// VX1721
	case CVT_V17XX_RCFG_BOARD_ID_DT5721:		// DT5721
	case CVT_V17XX_RCFG_BOARD_ID_N6721:			//  N6721
		if( p_data->m_type== CVT_V1721)
			*p_is_board_type_correct= TRUE;
		break;
	case CVT_V17XX_RCFG_BOARD_ID_V1731:			//  V1731
	case CVT_V17XX_RCFG_BOARD_ID_VX1731:		// VX1731
	case CVT_V17XX_RCFG_BOARD_ID_DT5731:		// DT5731
	case CVT_V17XX_RCFG_BOARD_ID_N6731:			//  N6731
		if( p_data->m_type== CVT_V1731)
			*p_is_board_type_correct= TRUE;
		break;
	case CVT_V17XX_RCFG_BOARD_ID_V1720:			//  V1720
	case CVT_V17XX_RCFG_BOARD_ID_VX1720:		// VX1720
	case CVT_V17XX_RCFG_BOARD_ID_DT5720:		// DT5720
	case CVT_V17XX_RCFG_BOARD_ID_N6720:			//  N6720
		if( p_data->m_type== CVT_V1720)
			*p_is_board_type_correct= TRUE;
		break;
	case CVT_V17XX_RCFG_BOARD_ID_V1740:			//  V1740
	case CVT_V17XX_RCFG_BOARD_ID_VX1740:		// VX1740
	case CVT_V17XX_RCFG_BOARD_ID_DT5740:		// DT5740
	case CVT_V17XX_RCFG_BOARD_ID_N6740:			//  N6740
		if( p_data->m_type== CVT_V1740)
			*p_is_board_type_correct= TRUE;
		break;
	case CVT_V17XX_RCFG_BOARD_ID_V1751:			//  V1751
	case CVT_V17XX_RCFG_BOARD_ID_VX1751:		// VX1751
	case CVT_V17XX_RCFG_BOARD_ID_DT5751:		// DT5751
	case CVT_V17XX_RCFG_BOARD_ID_N6751:			//  N6751
		if( p_data->m_type== CVT_V1751)
			*p_is_board_type_correct= TRUE;
		break;
	}
	return true;
}

GenericBoardChannel* V17XXBoard::CreateChannel( int ch_index, void (* scope_refresh)( int, bool), wxMutex *p_data_mutex)
{
	return new V17XXBoardChannel( this, ch_index, this->m_p_data, scope_refresh, p_data_mutex, this->m_p_app_settings);
}

bool V17XXBoard::WriteSampleBufferSize( void)
{
	CVT_V1724_NUM_BLOCKS num_block_written;
	UINT16 num_k_samples= this->m_p_app_settings->m_num_sample>> 10;
	
	if( !cvt_V1724_set_buffer_samples( (cvt_V1724_data* )this->m_p_data, num_k_samples, &num_block_written))
		return false;
	// Adjust the X size to the buffer size really setted 
	if( !cvt_V1724_get_buffer_samples( (cvt_V1724_data* )this->m_p_data, &num_k_samples, &num_block_written))
		return false;
	this->m_p_app_settings->m_max_log_X= ((int)num_k_samples)<< 10;

	if( !cvt_V1724_set_acquisition_mode( (cvt_V1724_data* )this->m_p_data, false, num_block_written, CVT_V1724_ACQCTRL_ACQ_MODE_REGISTER_CTRL, FALSE, 0))
		return false;
	return cvt_V1724_data_clear( (cvt_V1724_data* )this->m_p_data)!= FALSE;
}

bool V17XXBoard::WriteTriggerMode( bool started)
{
	bool falling_edge_enable= this->m_p_app_settings->m_trigger_edge_type== AppSettings::FALLING_EDGE;
	bool trigger_out_enable= this->m_p_app_settings->m_trigger_edge_type!= AppSettings::NO_TRIGGER;
	unsigned char ch_msk= 0;
	int i;

	// HACK: delete when fixed on next firmware release
	if( this->m_p_app_settings->m_post_trigger< 5)
		this->m_p_app_settings->m_post_trigger= 5;

	UINT8 ch_enable_msk= this->GetChEnableMsk();
	for( i= 0; i< (int)this->m_channel_array.GetCount(); i++)
	{
		ch_msk|= ( (GenericBoardChannel*)this->m_channel_array[i])->m_enabled? ( 1<< i): 0;
		( (GenericBoardChannel*)this->m_channel_array[i])->SetEnable( !started, !(ch_enable_msk& ( 1<< i)));
	}
	if( !( started? 
		cvt_V1724_set_trigger_mode( (cvt_V1724_data* )this->m_p_data, falling_edge_enable, this->m_p_app_settings->m_trigger_msk& ( int)AppSettings::EXT_TRIGGER_MSK, true, trigger_out_enable? ch_msk: 0x00, false, this->m_p_app_settings->m_post_trigger):
		cvt_V1724_set_trigger_mode( (cvt_V1724_data* )this->m_p_data, false, false, false, 0x00, false, this->m_p_app_settings->m_post_trigger)))
			return false;
	// Data clear
	return cvt_V1724_data_clear( (cvt_V1724_data* )this->m_p_data)!= FALSE;
}

bool V17XXBoard::WriteSoftwareTrigger( void)
{
	wxMutexLocker lock( this->m_p_app_settings->m_mutex);

	return cvt_V1724_software_trigger( (cvt_V1724_data* )this->m_p_data)!= FALSE;
}

bool V17XXBoard::SetFrontPanelIO( bool use_TTL)
{
	if( !cvt_V1724_set_front_panel_IO( (cvt_V1724_data* )this->m_p_data, use_TTL, TRUE, 0, CVT_V1724_FPIO_MODES_GPIO))
		return false;
	return cvt_V1724_data_clear( (cvt_V1724_data* )this->m_p_data)!= FALSE;
}
bool V17XXBoard::ReadBoardData( UINT32 *p_max_num_samples, UINT32* p_num_events)
{
	return cvt_V1724_read_data( (cvt_V1724_data* )this->m_p_data, p_max_num_samples, p_num_events)!= FALSE;
}
bool V17XXBoard::DoStartAcquisition( void)
{
	unsigned char ch_msk= 0;
	int i= 0;
	for( i= 0; i< (int)this->m_channel_array.GetCount(); i++)
	{
		ch_msk|= ( (GenericBoardChannel*)this->m_channel_array[i])->m_enabled? ( 1<< i): 0;
	}
	if( cvt_V1724_set_readout_mode( (cvt_V1724_data* )this->m_p_data, TRUE, 1)== FALSE)
	{
		return false;
	}
	return cvt_V1724_start_acquisition( (cvt_V1724_data* )this->m_p_data, ch_msk)!= FALSE;
}
bool V17XXBoard::DoStopAcquisition( void)
{
	return cvt_V1724_stop_acquisition( (cvt_V1724_data* )this->m_p_data)!= FALSE;
}

bool V17XXBoard::GetBoardId( cvt_board_data data, int &board_id)
{
	//UINT32 reg_value32= 0;
	//// 
	//// read the board id
	//if( !cvt_read( &data, CVT_V1724_BOARD_INFO_ADD, &reg_value32, CVT_V1724_BOARD_INFO_AM, CVT_V1724_BOARD_INFO_DATA_SIZE))
	//{
	//	return false;
	//}
	UINT32 reg_value= 0;
	board_id= 0;
	if( !cvt_read( &data, CVT_V1724_ROM_BOARD_ID_0_ADD, &reg_value, CVT_V1724_ROM_BOARD_ID_0_AM, CVT_V1724_ROM_BOARD_ID_0_DATA_SIZE))
	{
		return false;
	}
	board_id= reg_value& 0x000000ff;
	if( !cvt_read( &data, CVT_V1724_ROM_BOARD_ID_1_ADD, &reg_value, CVT_V1724_ROM_BOARD_ID_1_AM, CVT_V1724_ROM_BOARD_ID_1_DATA_SIZE))
	{
		return false;
	}
	board_id|= (reg_value& 0x000000ff)<< 8;
	if( !cvt_read( &data, CVT_V1724_ROM_BOARD_ID_2_ADD, &reg_value, CVT_V1724_ROM_BOARD_ID_2_AM, CVT_V1724_ROM_BOARD_ID_2_DATA_SIZE))
	{
		return false;
	}
	board_id|= (reg_value& 0x000000ff)<< 16;

	return true;
}

