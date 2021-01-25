#ifndef CAMEO_ARCH_MACRO_H
#define CAMEO_ARCH_MACRO_H


typedef char            BOOLEAN;
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
typedef long long unsigned int AR_UINT8;


/* For driver writers, and in case you are using longjmp etc. */
typedef volatile signed char     VINT1;
typedef volatile unsigned char   VUINT1;
typedef volatile signed short    VINT2;
typedef volatile unsigned short  VUINT2;
typedef volatile signed int      VINT4;
typedef volatile unsigned long   VUINT4;

#define CONST  const

#define SUCCESS    				6
#define FAILURE    				4

#define DEBUG_FLAG 				1
#define DEBUG()	do\
{ \
	if(DEBUG_FLAG == 1) \
		PRINTF("[%s][%s][%d]\n", __FILE__, __FUNCTION__, __LINE__); \
}while(0)


#endif /*END CAMEO_ARCH_MACRO_H*/

