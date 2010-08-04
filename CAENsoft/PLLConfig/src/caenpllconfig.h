/////////////////////////////////////////////////////////////////////////////
// Name:        caenpllconfig.h
// Purpose:     
// Author:      NDA
// Modified by: 
// Created:     04/09/2007 14:49:23
// RCS-ID:      $Id: caenpllconfig.h,v 1.1 2009/02/18 08:25:35 Colombini Exp $
// Copyright:   CAEN S.p.A All rights reserved.
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _CAENPLLCONFIG_H_
#define _CAENPLLCONFIG_H_

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "caenpllconfig.h"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/image.h"
#include "mainframe.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
////@end control identifiers

/*!
 * CAENPLLConfigApp class declaration
 */

class CAENPLLConfigApp: public wxApp
{    
    DECLARE_CLASS( CAENPLLConfigApp )
    DECLARE_EVENT_TABLE()

public:
    /// Constructor
    CAENPLLConfigApp();

    /// Initialises the application
    virtual bool OnInit();

    /// Called on exit
    virtual int OnExit();

////@begin CAENPLLConfigApp event handler declarations
////@end CAENPLLConfigApp event handler declarations

////@begin CAENPLLConfigApp member function declarations
////@end CAENPLLConfigApp member function declarations

////@begin CAENPLLConfigApp member variables
////@end CAENPLLConfigApp member variables
};

/*!
 * Application instance declaration 
 */

////@begin declare app
DECLARE_APP(CAENPLLConfigApp)
////@end declare app

#endif
    // _CAENPLLCONFIG_H_
