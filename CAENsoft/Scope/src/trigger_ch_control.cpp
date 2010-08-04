/////////////////////////////////////////////////////////////////////////////
// Name:        trigger_ch_control.cpp
// Purpose:     
// Author:      NDA
// Modified by: 
// Created:     05/04/2006 15:38:25
// RCS-ID:      
// Copyright:   CAEN S.p.A All rights reserved.
// Licence:     
/////////////////////////////////////////////////////////////////////////////

// Generated by DialogBlocks (unregistered), 05/04/2006 15:38:25

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "trigger_ch_control.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

////@begin includes
////@end includes

#include "trigger_ch_control.h"

////@begin XPM images
////@end XPM images

/*!
 * TriggerChControl type definition
 */

IMPLEMENT_DYNAMIC_CLASS( TriggerChControl, wxPanel )

/*!
 * TriggerChControl event table definition
 */

BEGIN_EVENT_TABLE( TriggerChControl, wxPanel )

////@begin TriggerChControl event table entries
    EVT_SPINCTRL( ID_THRESHOLD_SPINCTRL, TriggerChControl::OnThresholdSpinctrlUpdated )

    EVT_SPINCTRL( ID_THR_SAMPLES_SPINCTRL, TriggerChControl::OnThrSamplesSpinctrlUpdated )

////@end TriggerChControl event table entries
    EVT_TEXT_ENTER( ID_THRESHOLD_SPINCTRL, TriggerChControl::OnThresholdSpinctrlTextUpdated )
    EVT_TEXT_ENTER( ID_THR_SAMPLES_SPINCTRL, TriggerChControl::OnThrSamplesSpinctrlTextUpdated )

END_EVENT_TABLE()

/*!
 * TriggerChControl constructors
 */

TriggerChControl::TriggerChControl( )
{
}

TriggerChControl::TriggerChControl( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * TriggerChControl creator
 */

bool TriggerChControl::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin TriggerChControl member initialisation
    m_main_sizer_text = NULL;
    m_ch_threshold_control = NULL;
    m_thr_sample_control = NULL;
////@end TriggerChControl member initialisation

////@begin TriggerChControl creation
    wxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
    Centre();
////@end TriggerChControl creation
    return true;
}

/*!
 * Control creation for TriggerChControl
 */

void TriggerChControl::CreateControls()
{    
////@begin TriggerChControl content construction
    // Generated by DialogBlocks, 18/10/2006 10:35:56 (unregistered)

    TriggerChControl* itemPanel1 = this;

    this->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Verdana")));
    m_main_sizer_text = new wxStaticBox(itemPanel1, wxID_ANY, _("???"));
    wxStaticBoxSizer* itemStaticBoxSizer2 = new wxStaticBoxSizer(m_main_sizer_text, wxVERTICAL);
    itemPanel1->SetSizer(itemStaticBoxSizer2);

    m_ch_threshold_control = new wxSpinCtrl( itemPanel1, ID_THRESHOLD_SPINCTRL, _T("0"), wxDefaultPosition, wxSize(75, -1), wxSP_ARROW_KEYS, -1250, 1250, 0 );
    m_ch_threshold_control->SetHelpText(_("Set trigger threshold (mV)"));
    if (ShowToolTips())
        m_ch_threshold_control->SetToolTip(_("Set trigger threshold (mV)"));
    itemStaticBoxSizer2->Add(m_ch_threshold_control, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP, 3);

    m_thr_sample_control = new wxSpinCtrl( itemPanel1, ID_THR_SAMPLES_SPINCTRL, _T("0"), wxDefaultPosition, wxSize(75, -1), wxSP_ARROW_KEYS, 0, 99999, 0 );
    m_thr_sample_control->SetHelpText(_("Set threshold samples"));
    if (ShowToolTips())
        m_thr_sample_control->SetToolTip(_("Set threshold samples"));
    itemStaticBoxSizer2->Add(m_thr_sample_control, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP, 3);

////@end TriggerChControl content construction

}

/*!
 * Should we show tooltips?
 */

bool TriggerChControl::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap TriggerChControl::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin TriggerChControl bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end TriggerChControl bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon TriggerChControl::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin TriggerChControl icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end TriggerChControl icon retrieval
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_THRESHOLD_SPINCTRL
 */

void TriggerChControl::OnThresholdSpinctrlUpdated( wxSpinEvent& event )
{
	this->UpdateThreshold( event.GetPosition());
}


/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_THR_SAMPLES_SPINCTRL
 */

void TriggerChControl::OnThrSamplesSpinctrlUpdated( wxSpinEvent& event )
{
	this->UpdateThrSamples( event.GetPosition());
}

bool TriggerChControl::SetupBoard( GenericBoard* p_board, int ch_index, int ch_count)
{
	this->m_p_board= p_board;
	this->m_ch_index= ch_index;
	this->m_ch_count= ch_count;
	if( ( size_t)this->m_ch_index>= this->m_p_board->m_channel_array.GetCount())
		return false;
	this->m_p_board_channel= (PhysicalBoardChannel*)this->m_p_board->m_channel_array[ this->m_ch_index];
	
	if( !this->UpdateControls())
		return false;
	return true;
}
bool TriggerChControl::UpdateControls( )
{
	this->m_main_sizer_text->SetLabel( wxString::Format( "%d", this->m_ch_count));
	this->m_ch_threshold_control->SetValue( (int)(double)(this->m_p_board_channel->m_trigger_threshold_volt* 1000.0));
	this->m_thr_sample_control->SetValue( this->m_p_board_channel->m_trigger_thr_sample);

	this->m_p_board_channel->WriteChannelTrigger();

	return true;
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_THRESHOLD_SPINCTRL
 */

void TriggerChControl::OnThresholdSpinctrlTextUpdated( wxCommandEvent& /* event*/ )
{
    int value= atoi( this->m_ch_threshold_control->GetLabel( ));
	this->UpdateThreshold( value);
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_THR_SAMPLES_SPINCTRL
 */

void TriggerChControl::OnThrSamplesSpinctrlTextUpdated( wxCommandEvent& /* event*/ )
{
    int value= atoi( this->m_thr_sample_control->GetLabel( ));
	this->UpdateThrSamples( value);
}

void TriggerChControl::UpdateThreshold( int value)
{
	this->m_p_board_channel->m_trigger_threshold_volt= (double)value/ 1000.0;
	this->m_p_board_channel->WriteChannelTrigger();
	for( int i= 0; i< SCOPE_NUM_PANELS; i++)
	{
		(this->m_p_board_channel->ScopeRefresh)( i, true);
	}
}

void TriggerChControl::UpdateThrSamples( int value)
{
	this->m_p_board_channel->m_trigger_thr_sample= value;
	this->m_p_board_channel->WriteChannelTrigger();
	for( int i= 0; i< SCOPE_NUM_PANELS; i++)
	{
		(this->m_p_board_channel->ScopeRefresh)( i, true);
	}
}