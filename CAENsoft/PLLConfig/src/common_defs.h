/////////////////////////////////////////////////////////////////////////////
// Name:        common_defs.h
// Purpose:     
// Author:      NDA
// Modified by: 
// Created:     09/04/07 14:18:40
// RCS-ID:      $Id: common_defs.h,v 1.1 2009/02/18 08:25:35 Colombini Exp $
// Copyright:   CAEN S.p.A All rights reserved.
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _COMMON_DEFS_H_
#define _COMMON_DEFS_H_

extern "C" 
{
	#include "cvt_common_defs.h"
	#include "cvt_V1724.h"
}

//
// Status bar pane indexes
#define STS_BAR_PANE_TOOLBAR	0
#define STS_BAR_PANE_FILENAME	1

#define STS_BAR_NUM_PANES		2

 const char* BinFormat( unsigned char value, char ret_buff[9]);

#endif	// _COMMON_DEFS_H_
