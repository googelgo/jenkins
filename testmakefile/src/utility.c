#ifndef ARCH_SYS_C
#define ARCH_SYS_C

/* ************************************************************************** **
 * Copyright (C) 2010-2012 Cameo Communications, Inc.							
 *																			
 * ************************************************************************** */

/* ************************************************************************** **
 *	 SUBSYSTEM NAME 		: SYSTEM MANAGER
 *	 MODULE NAME 			: CMARCH
 *	 LANGUAGE 				: C
 *	 TARGET ENVIRONMENT 	: Any
 *	 DATE OF FIRST RELEASE	: 2012/12/13
 *	 MODULE DESCRIPTION 	: ARCH
 *								
 * --------------------------------------------------------------------------
 *	 Filename 				: cmarchsys.c
 *	 Version 				: 1.0
 *	 Author 				: CRDC_Ricann
 *	 Date 					: 16 Nov 2012
 *	 FILE DESCRIPTION 		: ARCH module system code.
** ************************************************************************** */

#include "macro.h"
#include "proto.h"


/*****************************************************************************
*	  Function Name 	   	: UtilityTest
 *	  Description		   	: Test
 *	  INPUT			   		: None
 *     Output         		: None
 *	  Returns			   	: ARCH_SUCCESS/ARCH_FAILURE
*****************************************************************************/
INT4 UtilityTest(VOID)
{
	printf("[%s][%d] \n", __FUNCTION__, __LINE__);

	return SUCCESS;
}

#endif /*ARCH_SYS_C*/

