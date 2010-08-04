/////////////////////////////////////////////////////////////////////////////
// Name:        common_defs.cpp
// Purpose:     
// Author:      NDA
// Modified by: 
// Created:     09/04/07 14:18:40
// RCS-ID:      $Id: common_defs.cpp,v 1.1 2009/02/18 08:25:35 Colombini Exp $
// Copyright:   CAEN S.p.A All rights reserved.
// Licence:     
/////////////////////////////////////////////////////////////////////////////

//#include "common_defs.h"
#include <memory.h>

const char* BinFormat( unsigned char value, char ret_buff[9])
{
	const int N_CHARS= 8;
	int i;

	ret_buff[ N_CHARS]= '\0';
	memset( ret_buff, '0', N_CHARS);
	for( i= N_CHARS- 1; value&& ( i>= 0); --i, value>>= 1)
	{
		if( value& 0x01)
			ret_buff[ i]= '1';
	}
	return (const char*)&ret_buff[0];
}
