/*
 * Copyright (C) 2006 Aricent Inc . All Rights Reserved
 *
 * $Id: osxstd.h,v 1.4 2012/11/16 11:12:13 siva Exp $
 *
 * Description:
 * Contains standard types used by fsap applications.
 * Exported file.
 *
 */

/********************************************************************/
/* Copyright (C) 2006 Aricent Inc . All Rights Reserved                           */
/* Licensee Aricent Inc.,1997-98                  */
/*                                                                  */
/*  FILE NAME             :  osxstd.h                               */
/*  PRINCIPAL AUTHOR      :  Aricent Inc.                        */
/*  SUBSYSTEM NAME        :  OSIX                                   */
/*  MODULE NAME           :  -                                      */
/*  LANGUAGE              :  C                                      */
/*  TARGET ENVIRONMENT    :  ANY                                    */
/*  DATE OF FIRST RELEASE :                                         */
/*  DESCRIPTION           : The OSIX exported file,                 */
/*                          containing standard defns.              */
/********************************************************************/
#ifndef _OSIX_STD_H
#define _OSIX_STD_H

/* Include the file containing the user configuration settings. */
#include <pthread.h>
#include <semaphore.h>
/*#include "fsapcfg.h"*/

/* Validate configuration 
#if !defined OSIX_HOST
#error OSIX_HOST not defined. Valid values are OSIX_LITTLE_ENDIAN, OSIX_BIG_ENDIAN.
#endif */


/************************************************************************
*                                                                       *
*                         Basic Types                                   *
*                                                                       *
*************************************************************************/

/*typedef char            BOOLEAN;
typedef char            BOOL1;
typedef char            CHR1;
typedef signed char     INT1;
typedef unsigned char   UINT1;
typedef UINT1           BYTE;
typedef void            VOID;
typedef signed short    INT2;
typedef unsigned short  UINT2;
typedef signed int      INT4;
typedef unsigned int    UINT4;
typedef float           FLT4;
typedef double          DBL8;
typedef unsigned long   FS_ULONG;
typedef signed long     FS_LONG;
typedef long long unsigned int AR_UINT8;*/


/* For driver writers, and in case you are using longjmp etc. */
/*typedef volatile signed char     VINT1;
typedef volatile unsigned char   VUINT1;
typedef volatile signed short    VINT2;
typedef volatile unsigned short  VUINT2;
typedef volatile signed int      VINT4;
typedef volatile unsigned long   VUINT4;*/

#ifdef __64BIT__
#define FSAP_OFFSETOF(StructType,Member)  ((UINT4)(FS_ULONG)(&(((StructType*)0)->Member)))
#else
#define FSAP_OFFSETOF(StructType,Member)  (UINT4)(&(((StructType*)0)->Member))
#endif

/* Undefine names that are defined in standard system headers. */
#undef PRIVATE
#undef PUBLIC
#undef VOLATILE
#undef EXPORT

/*#ifdef __STDC__
#define ARG_LIST(x)    x
#else
#define ARG_LIST(x)   ()
#endif*/ /* __STDC__ */

#ifndef NULL
#define NULL    (0)
#endif

#if !defined(PRIVATE)
#define PRIVATE static
#endif

#if !defined(VOLATILE)
#define VOLATILE volatile
#endif

#if !defined(PUBLIC)
#define PUBLIC  extern
#endif

#if !defined(FALSE)  || (FALSE != 0)
#define FALSE  (0)
#endif

#if !defined(TRUE) || (TRUE != 1)
#define TRUE   (1)
#endif

#ifndef EXPORT
#define EXPORT
#endif

#if OSIX_HOST == OSIX_LITTLE_ENDIAN
#define OSIX_NTOHL(x) (UINT4)(((x & 0xFF000000)>>24) | \
                              ((x & 0x00FF0000)>>8)  | \
                              ((x & 0x0000FF00)<<8 ) | \
                              ((x & 0x000000FF)<<24)   \
                             )
#define OSIX_NTOHS(x) (UINT2)(((x & 0xFF00)>>8) | ((x & 0x00FF)<<8))
#define OSIX_HTONL(x) (UINT4)(OSIX_NTOHL(x))
#define OSIX_HTONS(x) (UINT2)(OSIX_NTOHS(x))
#define OSIX_NTOHF(x) (FLT4)(FsNtohf (x))
#define OSIX_HTONF(x) (FLT4)(FsNtohf(x))
#else
#define OSIX_NTOHL(x) (UINT4)(x)
#define OSIX_NTOHS(x) (UINT2)(x)
#define OSIX_HTONL(x) (UINT4)(x)
#define OSIX_HTONS(x) (UINT2)(x)
#define OSIX_NTOHF(x) (FLT4)(x)
#define OSIX_HTONF(x) (FLT4)(x)
#endif

#endif
