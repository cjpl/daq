/////////////////////////////////////////////////////////////////////////////
// Name:        appsettings.h
// Purpose:     
// Author:      NDA
// Modified by: 
// Created:     09/04/07 16:17:15
// RCS-ID:      $Id: vme_wrapper.h,v 1.1 2009/02/18 08:25:37 Colombini Exp $
// Copyright:   CAEN S.p.A. All rights reserved
// Licence:     
// \todo:       Overwrite this according to your library specific needs
/////////////////////////////////////////////////////////////////////////////

#ifndef _VME_WRAPPER_H_
#define _VME_WRAPPER_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "vme_wrapper.h"
#endif

#include "common_defs.h"

//
// HACK: Overwrite this according to your library specific needs
// 
extern "C" 
{
	#include <CAENVMElib.h>
	#include <CAENVMEoslib.h>
	#include <CAENVMEtypes.h>
}

////////////////////////////////////////////////////////////////////////////////////////////////
/*! \struct  InputData
*   \brief   VME wrapper input data structure
*            
*            This is the data structure passed to the VME wrapper to initializate the VME connection.
*            You can redefine this structure according to your VME library APIs' specific needs
*/
////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct 
{
	int  m_vme_link;			/*!< \brief The link number */
	int  m_vme_board_num;		/*!< \brief The board number */
	UINT32 m_board_base_add;	/*!< \brief The board base address (32 bit) */
	char m_board_type[20];		/*!< \brief The board type */
} InputData;

////////////////////////////////////////////////////////////////////////////////////////////////
/*! \struct  Handle
*   \brief   VME wrapper handle to the VME connection 
*            
*            This is the VME handle ( or whatever other structure) returned by your VME library APIs.
*            You can redefine this structure according to your VME library APIs' specific needs
*/
////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct 
{
	int32_t m_vme_handle;		/*!< \brief  The VME library comunication handle */
} Handle;

////////////////////////////////////////////////////////////////////////////////////////////////
/*! \class   VMEWrapper
*   \brief   A wrapper for the VME library APIs
*            
*            This is the VME library wrapper, used by the application to perform VME comunications.
*            You can change the implementation of each method (and keep the external public interface)
*            to comply with your specific VME library APIs
*/
////////////////////////////////////////////////////////////////////////////////////////////////
class VMEWrapper
{
public:
	//
	// Methods
	VMEWrapper( void);
	~VMEWrapper( void);
	
	//! Connection initialization
	/*!
		This method is called once for each VME connection session : it have to perform all the specific initialization tasks
		required by your VME library, in order to get a valid connection handle.
		\param   input_data  The input data structure passed
		\return  true if VME connection opening was succesfully completed 
	*/
	bool Init( const InputData& input_data);
	//! End of connection
	/*!
		This method is called once for closing the VME connecion: it have to get release all the resources 
		allocated by the VME library. The connection handle must be invalidated and no more available for further 
		comunications.
		\return  true if VME connection closing was succesfully completed 
	*/
	bool End( void);
	//! Clear register bitmask
	/*!
		This method clears the bits setted in the reg_value parameter. The clear is perfomed by
		a read and write operation using the WriteReg and ReadReg wrapper methods. 
		You can reimplement this if your VME library provides a native API.
		\param   reg_offset The offset (relative to the board base address) offset of the affected register
		\param   reg_value  The bitmask applied
		\return  true if the operation completed successfully
	*/
	bool ClearBitmaskReg( UINT32 reg_offset, UINT32 reg_value);
	//! Set register bitmask
	/*!
		This method sets the bits setted in the reg_value parameter. The set is perfomed by
		a read and write operation using the WriteReg and ReadReg wrapper methods. 
		You can reimplement this if your VME library provides a native API.
		\param   reg_offset The offset (relative to the board base address) offset of the affected register
		\param   reg_value  The bitmask applied
		\return  true if the operation completed successfully
	*/
	bool SetBitmaskReg( UINT32 reg_offset, UINT32 reg_value);
	//! Write register value
	/*!
		This method writes the value stored into the reg_value parameter. 
		You must reimplement this using your VME library APIs.
		\param   reg_offset The offset (relative to the board base address) offset of the affected register
		\param   reg_value  The register value written
		\return  true if the operation completed successfully
	*/
	bool WriteReg( UINT32 reg_offset, UINT32 reg_value);
	//! Read register value
	/*!
		This method reads the register value and stores it into the reg_value parameter.
		You must reimplement this using your VME library APIs.
		\param   reg_offset The offset (relative to the board base address) offset of the affected register
		\param   reg_value  The register value read
		\return  true if the operation completed successfully
	*/
	bool ReadReg( UINT32 reg_offset, UINT32& reg_value);
protected:
	Handle m_handle;
	InputData m_input_data;
};


#endif


