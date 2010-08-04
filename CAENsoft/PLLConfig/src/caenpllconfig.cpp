	/////////////////////////////////////////////////////////////////////////////
// Name:        caenpllconfig.cpp
// Purpose:     
// Author:      NDA
// Modified by: 
// Created:     04/09/2007 14:49:23
// RCS-ID:      $Id: caenpllconfig.cpp,v 1.1 2009/02/18 08:25:35 Colombini Exp $
// Copyright:   CAEN S.p.A All rights reserved.
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "caenpllconfig.h"
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

#include "caenpllconfig.h"

////@begin XPM images

////@end XPM images

/*!
 * Application instance implementation
 */

////@begin implement app
IMPLEMENT_APP( CAENPLLConfigApp )
////@end implement app

/*!
 * CAENPLLConfigApp type definition
 */

IMPLEMENT_CLASS( CAENPLLConfigApp, wxApp )

/*!
 * CAENPLLConfigApp event table definition
 */

BEGIN_EVENT_TABLE( CAENPLLConfigApp, wxApp )

////@begin CAENPLLConfigApp event table entries
////@end CAENPLLConfigApp event table entries

END_EVENT_TABLE()

/*!
 * Constructor for CAENPLLConfigApp
 */

CAENPLLConfigApp::CAENPLLConfigApp()
{
////@begin CAENPLLConfigApp member initialisation
////@end CAENPLLConfigApp member initialisation
}

/*!
 * Initialisation for CAENPLLConfigApp
 */

bool CAENPLLConfigApp::OnInit()
{    
////@begin CAENPLLConfigApp initialisation
    // Remove the comment markers above and below this block
    // to make permanent changes to the code.

#if wxUSE_XPM
    wxImage::AddHandler(new wxXPMHandler);
#endif
#if wxUSE_LIBPNG
    wxImage::AddHandler(new wxPNGHandler);
#endif
#if wxUSE_LIBJPEG
    wxImage::AddHandler(new wxJPEGHandler);
#endif
#if wxUSE_GIF
    wxImage::AddHandler(new wxGIFHandler);
#endif
    MainFrame* mainWindow = new MainFrame( NULL, ID_MAIN_FRAME );
    mainWindow->Show(true);
////@end CAENPLLConfigApp initialisation

    return true;
}

/*!
 * Cleanup for CAENPLLConfigApp
 */
int CAENPLLConfigApp::OnExit()
{    
	wxImage::CleanUpHandlers();
////@begin CAENPLLConfigApp cleanup
    return wxApp::OnExit();
////@end CAENPLLConfigApp cleanup
}

