/////////////////////////////////////////////////////////////////////////////
// Name:        V1740_board.cpp
// Purpose:     
// Author:      NDA
// Modified by: 
// Created:     21/05/08
// RCS-ID:      $Id: V1740_board.cpp,v 1.1 2009/02/18 08:25:34 Colombini Exp $
// Copyright:   CAEN S.p.A. All rights reserved
// Licence:     
/////////////////////////////////////////////////////////////////////////////


#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "V1740_board.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "V1740_board.h"
#include "appsettings.h"

V1740Board::V1740Board( AppSettings *p_app_settings): 
			GenericBoard( p_app_settings)
{
}
