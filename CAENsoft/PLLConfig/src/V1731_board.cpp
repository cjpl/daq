/////////////////////////////////////////////////////////////////////////////
// Name:        V1731_board.cpp
// Purpose:     
// Author:      NDA
// Modified by: 
// Created:     09/04/07 14:18:40
// RCS-ID:      $Id: V1731_board.cpp,v 1.1 2009/02/18 08:25:34 Colombini Exp $
// Copyright:   CAEN S.p.A. All rights reserved
// Licence:     
/////////////////////////////////////////////////////////////////////////////


#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "V1731_board.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "V1731_board.h"
#include "appsettings.h"

V1731Board::V1731Board( AppSettings *p_app_settings, bool is_des_mode): 
			GenericBoard( p_app_settings), m_is_des_mode( is_des_mode)
{
}

