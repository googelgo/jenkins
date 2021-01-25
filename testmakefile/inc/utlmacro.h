/*
 *  Copyright (C) 2006 Aricent Inc . All Rights Reserved 
 *
 * $Id: utlmacro.h,v 1.19 2012/10/11 09:42:37 siva Exp $
 *
 * Description: Macro definitions for ANSI-C functions.
 *
 */

#ifndef _UTLMACRO_H
#define _UTLMACRO_H



/* Undef to avoid any redefinition warnings arising
 * from presence of similar macros on the target system
 * libraries.
 */
#undef ATOI
#undef ATOL
#undef INET_ADDR
#undef INET_NTOA
#undef INET_NTOP
#undef INET_PTON
#undef ISDIGIT
#undef ISALPHA
#undef ISSPACE
#undef ISXDIGIT
#undef MEM_CALLOC
#undef MEMCMP
#undef MEMCPY
#undef MEM_FREE
#undef MEM_MALLOC
#undef MEM_REALLOC
#undef MEMSET
#undef OSIX_RAND
#undef OSIX_RAND_MAX
#undef OSIX_SEED
#undef OSIX_SRAND
#undef PRINTF
#undef RAND
#undef SCANF
#undef SNPRINTF
#undef SPRINTF
#undef SRAND
#undef SSCANF
#undef STRCASECMP
#undef STRNCASECMP
#undef STRCAT
#undef STRNCAT
#undef STRCHR
#undef STRCMP
#undef STRCPY
#undef STRLEN
#undef STRNLEN
#undef STRNCMP
#undef STRNCPY
#undef STRRCHR
#undef STRSTR
#undef STRTOK
#undef TOLOWER
#undef TOUPPER
#undef AF_INET
#undef AF_INET6

/******  ANSI-C Macros Definition  ********/
#define  FSAP_ASSERT(x)    assert(x)
#define  SPRINTF           sprintf
#define  SNPRINTF          UtlSnprintf
#define  VSNPRINTF         UtlVsnprintf
#ifndef LINUX_KERN
#define  PRINTF            printf  
#else
#define  PRINTF            printk  
#endif

#define FOPEN fopen
#define FCLOSE fclose
#define FWRITE fwrite
#define FREAD fread

#define  SCANF             scanf   
#define  SSCANF            sscanf  

#define  ISDIGIT(c)        isdigit((int)c)
#define  ISALPHA(c)        isalpha((int)c)
#define  ISALNUM(c)        isalnum((int)c)
#define  ISSPACE(ch)       isspace(ch)

#define  STRCPY(d,s)       strcpy ((char *)(d),(const char *)(s))
#define  STRNCPY(d,s,n)    strncpy ((char *)(d), (const char *)(s), n)

#define  STRCAT(d,s)       strcat((char *)(d),(const char *)(s))
#define  STRNCAT(d,s,n)    strncat((char *)(d),(const char *)(s), n)

#define  STRCMP(s1,s2)     strcmp  ((const char *)(s1), (const char *)(s2))
#define  STRNCMP(s1,s2,n)  strncmp ((const char *)(s1), (const char *)(s2), n)

#define  STRCHR(s,c)       strchr((const char *)(s), (int)c)
#define  STRRCHR(s,c)      strrchr((const char *)(s), (int)c)

#define  STRLEN(s)         strlen((const char *)(s))
#define  STRNLEN(s, n)     UtlStrnlen((const char *)(s), n)

#define  STRTOK(s,d)       strtok  ((char *)(s), (const char *)(d))
#define  STRTOK_R(s,d,p)   strtok_r ((char *)(s), (const char *)(d), (char **)(p))
#define  STRTOUL(s,e,b)    UtilStrtoul ((char *)(s), (char **)(e), (int)(b))
#define  STRCSPN(s,r)      UtilStrcspn ((char *)(s), (char *)(r))

       

#ifdef LINUX_KERN
aa
#define  KMALLOC_ALLOC_LIMIT  131072 /* kmalloc allocation limit is 128KB */
#define  MEM_MALLOC(s,t)   (t *)LkMalloc(s)
#define  MEM_CALLOC(s,c,t) (t *)LkCalloc(s,c)
#define  MEM_FREE(p)       LkFree(p)
#else
#ifdef CAMEO_DEBUG_WANTED  /*CAMEO Memory Debug Method*/
                                     
#define MEM_MALLOC(s,t)    					(t *)CmAPIDbgMemMalloc(__LINE__, __PRETTY_FUNCTION__, CM_MEM_SYSALLOC, s)
#define MEM_CALLOC(s,c,t)  					(t *)CmAPIDbgMemCalloc(__LINE__, __PRETTY_FUNCTION__, CM_MEM_SYSALLOC, s, c)
#define MEM_REALLOC(p,s,t) 					(t *)CmAPIDbgMemRalloc(__LINE__, __PRETTY_FUNCTION__, CM_MEM_SYSALLOC, p, s)                                 
#define MEM_FREE(p)        CmAPIDbgMemFree(p,CM_MEM_SYSALLOC)


#define POOL_MEM_MALLOC(s,t)    			(t *)CmAPIDbgMemMalloc(__LINE__, __PRETTY_FUNCTION__, CM_MEM_POOLALLOC, s)
#define POOL_MEM_CALLOC(s,c,t)  			(t *)CmAPIDbgMemCalloc(__LINE__, __PRETTY_FUNCTION__, CM_MEM_POOLALLOC, s, c)
#define POOL_MEM_REALLOC(p,s,t) 			(t *)CmAPIDbgMemRalloc(__LINE__, __PRETTY_FUNCTION__, CM_MEM_POOLALLOC, p, s)         
#define POOL_MEM_FREE(p)        CmAPIDbgMemFree(p,CM_MEM_POOLALLOC)

#define BUDDY_MEM_MALLOC(s,t)    			(t *)CmAPIDbgMemMalloc(__LINE__, __PRETTY_FUNCTION__, CM_MEM_BUDDYALLOC, s)                     	 
#define BUDDY_MEM_CALLOC(s,c,t)  			(t *)CmAPIDbgMemCalloc(__LINE__, __PRETTY_FUNCTION__, CM_MEM_BUDDYALLOC, s, c)                         	  
#define BUDDY_MEM_REALLOC(p,s,t) 			(t *)CmAPIDbgMemRalloc(__LINE__, __PRETTY_FUNCTION__, CM_MEM_BUDDYALLOC, p, s)     
#define BUDDY_MEM_FREE(p)        CmAPIDbgMemFree(p,CM_MEM_BUDDYALLOC)

#define OSS_MEM_MALLOC(s,t,fun,line)		(t *)CmAPIDbgMemMalloc(line, fun, CM_MEM_OSSALLOC, s)
#define OSS_MEM_CALLOC(s,c,t,fun,line)  	(t *)CmAPIDbgMemCalloc(line, fun, CM_MEM_OSSALLOC, s, c)
#define OSS_MEM_REALLOC(p,s,t,fun,line) 	(t *)CmAPIDbgMemRalloc(line, fun, CM_MEM_OSSALLOC, p, s)
#define OSS_MEM_FREE(p,fun,line)        	CmAPIDbgMemFree(p,CM_MEM_OSSALLOC)

#else /*!CAMEO_DEBUG_WANTED*/

#define  MEM_MALLOC(s,t)   (t *)malloc(s)
#define  MEM_CALLOC(s,c,t) (t *)calloc(s, c) 
#define  MEM_FREE(p)       free(p)
#define  MEM_REALLOC(p,s,t) (t *)realloc(p, s)

#define  POOL_MEM_MALLOC(s,t)   (t *)malloc(s)
#define  POOL_MEM_CALLOC(s,c,t) (t *)calloc(s, c) 
#define  POOL_MEM_FREE(p)       free(p)
#define  POOL_MEM_REALLOC(p,s,t) (t *)realloc(p, s)

#define  BUDDY_MEM_MALLOC(s,t)   (t *)malloc(s)
#define  BUDDY_MEM_CALLOC(s,c,t) (t *)calloc(s, c) 
#define  BUDDY_MEM_FREE(p)       free(p)
#define  BUDDY_MEM_REALLOC(p,s,t) (t *)realloc(p, s)

#define  OSS_MEM_MALLOC(s,t,f,l)   			(t *)malloc(s)
#define  OSS_MEM_CALLOC(s,c,t,f,l) 			(t *)calloc(s, c) 
#define  OSS_MEM_FREE(p,f,l)       			free(p)
#define  OSS_MEM_REALLOC(p,s,t,f,l) 		(t *)realloc(p, s)

#endif /*CAMEO_DEBUG_WANTED*/
#endif /* LINUX_KERN */


#define  MEMCPY(d,s,n)     memcpy((void *)(d), (const void *)(s), n)
#define  MEMCMP(s1,s2,n)   memcmp((const void *)(s1), (const void *)(s2), n)
#define  MEMSET(s,c,n)     memset((void *)(s), (int)c, n)
#define  MEMMOVE(d,s,n)    UtilMemMove((void *)(d), (const void *) (s), (n))
#define  ATOI(p)           UtlAtoi((const char *)(p))
#define  RAND              UtilRand    
#define  SRAND             srand   

#define  OSIX_RAND_MAX     RAND_MAX
#define  OSIX_SEED(seed)   OsixGetSysTime((tOsixSysTime *)&(seed))
#define  OSIX_SRAND(seed)  srand(seed)
#define  OSIX_RAND(m, M)   (m+(int) (((double)(M-m+1))*RAND()/(OSIX_RAND_MAX+1.0)))

#define STRSTR(S,s)        strstr((const char *)(S), (const char *)(s))
#define STRCASECMP(s1, s2) UtlStrCaseCmp((const char *)(s1), (const char *)(s2))
#define STRNCASECMP(s1, s2, n)\
        UtlStrnCaseCmp((const char *)(s1), (const char *)(s2), (UINT4)(n))
#define ATOL(s)            atol((const char *)(s))
#define ISXDIGIT(i)        isxdigit((int)(i))
#define IS_ASCII(c)        ((UINT4)(c)<=0177)
#define TOLOWER(c)         tolower(c)
#define TOUPPER(c)         toupper(c)
#define ISUPPER(c)         isupper(c)

#define INET_ADDR(s)       UtlInetAddr((const char *)s)
#define INET_NTOA(in)      UtlInetNtoa(in) 

#define INET_NTOP(af, s, d, n)\
        inet_ntop((int)af, (const void *)(s), (char *)d, n)
#define INET_PTON(af, s, d)\
        inet_pton(af, (const char *)(s), (void *)(d))

#define INET_NTOA6(in)      UtlInetNtoa6(in)
#define INET_ATON6(s, pin)  UtlInetAton6((const CHR1 *)s, (tUtlIn6Addr *)pin)

#define FLOOR(x)  UtilFloor(x)
#define CEIL(x)   UtilCeil(x)
#define LOG(x)    UtilLog(x)
#define POW(x,y)  UtilPow(x,y)
#define EXP(x)    UtilExp(x)

/* max and min values of FLT4 (float) and DBL8 (double) */
#define FS_DBL_MIN         4.94065645841246544e-324
#define FS_FLT_MIN         ((FLT4)1.40129846432481707e-45)
#define FS_DBL_MAX         1.79769313486231470e+308
#define FS_FLT_MAX         ((FLT4)3.40282346638528860e+38)

/* The following macros are to be used whenever transferring
 * data between a value and a pointer to memory. These macros
 * take care of proper endian conversions.
 * ALIGN_SAFE (cmn/fsapcfg.h) is 'OFF' by default.
 * This should be set to FSAP_ON on processors like arm/mips/sparc.
 *
 * NOTE:
 *   These macros are meant to be used in the context of network packets.
 *   i/e, the data is assumed to be in network order.
 *
 * Semantics: ASSIGN  TO (ptr)   FROM value
 *          : FETCH (value) FROM ptr
 */

#if (ALIGN_SAFE == FSAP_ON)
#define PTR_FETCH2(v,p)  (v = (UINT2)(((*(UINT1 *)p) << 8) | (*((UINT1 *)p+1))))
#define PTR_FETCH4(v,p)  (v = (((*(UINT1 *)p) << 24) | (*((UINT1 *)p+1) << 16) | (*((UINT1 *)p+2) << 8) | *((UINT1 *)p+3)))

#define PTR_ASSIGN2(p,v)               \
        *((UINT1 *)p+1) = (UINT1)(v);           \
        *((UINT1 *)p)   = (UINT1)((v) >> 8);    \

#define PTR_ASSIGN4(p,v)             \
        PTR_ASSIGN2((p), (v) >> 16); \
        PTR_ASSIGN2(p+2, v);
#else

#define PTR_ASSIGN2(p,v)  \
         {\
                UINT2 u2ValToPtr;\
                u2ValToPtr = OSIX_HTONS(v);\
                MEMCPY( (CHR1 *) p, (CHR1 *) &u2ValToPtr, sizeof(UINT2));\
         }

#define PTR_ASSIGN4(p,v)  \
         {\
                UINT4 u4ValToPtr;\
                u4ValToPtr = OSIX_HTONL(v);\
                MEMCPY( (CHR1 *) p, (CHR1 *) &u4ValToPtr, sizeof(UINT4));  \
         }


#define PTR_FETCH2(v,p)  \
         {\
               UINT2 u2PtrToVal;  \
               MEMCPY( (CHR1 *) &u2PtrToVal, (CHR1 *) p, sizeof(UINT2));  \
               v = (UINT2)OSIX_HTONS(u2PtrToVal);               \
          }

#define PTR_FETCH4(v,p)  \
         {\
               UINT4 u4PtrToVal;  \
               MEMCPY( (CHR1 *) &u4PtrToVal, (CHR1 *) p, sizeof(UINT4));  \
               v = OSIX_HTONL(u4PtrToVal);               \
          }

#endif

#define IS_LEAP(yr)      ((yr) % 4 == 0 && ((yr) % 100 != 0 || (yr) % 400 == 0))
#define DAYS_IN_YEAR(yr) (IS_LEAP((yr)) ? 366 : 365)
#define SECS_IN_YEAR(yr) (IS_LEAP((yr)) ? 31622400 : 31536000);
#define SECS_IN_DAY      86400
#define SECS_IN_HOUR     3600
#define SECS_IN_MINUTE   60
#define HOURS_IN_DAY     24
#define MINUTES_IN_HOUR  60
#define TM_BASE_YEAR     2000
#define TIME_STR_BUF_SIZE        30
#define TM_SEC_SYS_BASE_2_FSAP_BASE_YEAR  (946665000) 


#define MEM_MAX_BYTES(NoBytes,MAX)   (((NoBytes) <= (MAX)) ? (NoBytes) : (MAX))
                                                                          
#define UTL_TRACE_TOKEN_DELIMITER ' '    /* space */

#define UTL_TRACE_ENABLE    1
#define UTL_TRACE_DISABLE   2
#define UTL_MAX_TRC_LEN     15

#define  AF_INET            2
#define  AF_INET6           10
#define  UTL_INET_ADDRSTRLEN    16
#define  UTL_INET6_ADDRSTRLEN   46

typedef struct _tUtlValidTraces
{
    UINT1   *pu1TraceStrings;
    UINT2    u2MaxTrcTypes;
    UINT1    au1Pad[2];
}tUtlValidTraces;

/* FSAP's equivalent for the standard struct in_addr (netinet/in.h)  */
typedef struct
{
    UINT4    u4Addr;
} tUtlInAddr;

/* FSAP's equivalent for the standard struct in6_addr (netinet/in.h)  */
typedef struct
{
    UINT1  u1addr[16];
} tUtlIn6Addr;


/* FSAP's equivalent for the standard struct tm (time.h)             */
/* The names of the structure elements are retained for compatibility*/
typedef struct
{
    UINT4               tm_sec;     /* seconds */
    UINT4               tm_min;     /* minutes */
    UINT4               tm_hour;    /* hours */
    UINT4               tm_mday;    /* day of the month */
    UINT4               tm_mon;     /* month */
    UINT4               tm_year;    /* year */
    UINT4               tm_wday;    /* day of the week : NOT USED */
    UINT4               tm_yday;    /* day in the year */
    UINT4               tm_isdst;   /* daylight saving time : NOT USED*/
} tUtlTm;

/* This structure is used to set/get the Precise time of system 
 * Epoch (00:00:00 UTC, January 1, 1970) */

typedef struct _tUtlSysPreciseTime {

    UINT4 u4Sec;           /* Numbers of Seconds */ 
    UINT4 u4NanoSec;       /* nanoseconds (000000000..999999999) */
    UINT4 u4SysTimeUpdate; /* If system time update required */
                           /* ( OSIX_TRUE/OSIX_FALSE ) */

} tUtlSysPreciseTime;

/* UINT8 datatype implementation */
typedef struct {
    UINT4 u4Hi;
    UINT4 u4Lo;
} FS_UINT8;

#define UINT8_HI(pVal) (pVal)->u4Hi
#define UINT8_LO(pVal) (pVal)->u4Lo

#define FSAP_U8_INC(pu8Val)            \
        do {                           \
        UINT4 u4Lo = UINT8_LO(pu8Val); \
        UINT8_LO(pu8Val) += 1;         \
        if (UINT8_LO(pu8Val) < u4Lo) { \
            UINT8_HI(pu8Val) += 1;     \
        }                              \
    } while (0)

#define FSAP_U8_DEC(pu8Val)            \
        do {                           \
        UINT4 u4Lo = UINT8_LO(pu8Val); \
        UINT8_LO(pu8Val) -= 1;         \
        if (UINT8_LO(pu8Val) > u4Lo) { \
            UINT8_HI(pu8Val) -= 1;     \
        }                              \
    } while (0)

#define FSAP_U8_ADD(pu8Result, pu8Val1, pu8Val2)                             \
        do {                                                                 \
        UINT4 u4V1Lo = UINT8_LO(pu8Val1);                                    \
        UINT8_LO(pu8Result) = UINT8_LO(pu8Val1) + UINT8_LO(pu8Val2);         \
        if (UINT8_LO(pu8Result) < u4V1Lo) {                                  \
            UINT8_HI(pu8Result) = UINT8_HI(pu8Val1) + UINT8_HI(pu8Val2) + 1; \
        } else {                                                             \
            UINT8_HI(pu8Result) = UINT8_HI(pu8Val1) + UINT8_HI(pu8Val2);     \
        }                                                                    \
    } while (0)

#define FSAP_U8_SUB(pu8Result, pu8Val1, pu8Val2)                             \
        do {                                                                 \
        UINT4 u4V1Lo = UINT8_LO(pu8Val1);                                    \
        UINT8_LO(pu8Result) = UINT8_LO(pu8Val1) - UINT8_LO(pu8Val2);         \
        if (UINT8_LO(pu8Result) > u4V1Lo) {                                  \
            UINT8_HI(pu8Result) = UINT8_HI(pu8Val1) - UINT8_HI(pu8Val2) + 1; \
        } else {                                                             \
            UINT8_HI(pu8Result) = UINT8_HI(pu8Val1) - UINT8_HI(pu8Val2);     \
        }                                                                    \
    } while (0)

#define FSAP_U8_MUL(pResult, pVal1, pVal2) \
     UtlU8Mul (pResult, pVal1, pVal2)

#define FSAP_U8_DIV(pResult, pReminder, pVal1, pVal2) \
     UtlU8Div (pResult, pReminder, pVal1, pVal2)

#define FSAP_STR2_U8(pStr,pu8Val) \
     UtlStr2U8 (pStr, pu8Val)

#define FSAP_U8_2STR(pu8Val,pStr) \
     UtlU82Str(pu8Val, pStr)

#define FSAP_U8_FETCH_LO(pu8Val)        ((pu8Val)->u4Lo)
#define FSAP_U8_FETCH_HI(pu8Val)        ((pu8Val)->u4Hi)

#define FSAP_U8_CLR(pu8Val) (pu8Val)->u4Hi = (pu8Val)->u4Lo = 0
#define FSAP_U8_ASSIGN_LO(pu8Val,u4Val) ((pu8Val)->u4Lo = u4Val)
#define FSAP_U8_ASSIGN_HI(pu8Val,u4Val) ((pu8Val)->u4Hi = u4Val)

#define FSAP_U8_CMP(p1,p2) ((p1)->u4Hi > (p2)->u4Hi ? 1 :    \
                            ((p1)->u4Hi < (p2)->u4Hi ? -1 :  \
                             ((p1)->u4Lo > (p2)->u4Lo ? 1 :  \
                              ((p1)->u4Lo < (p2)->u4Lo ? -1 :\
                               0))))

#define FSAP_U8_ASSIGN(pu8To,pu8From)  (pu8To)->u4Hi = (pu8From)->u4Hi;\
                                       (pu8To)->u4Lo = (pu8From)->u4Lo

#define CM_SNMP_COUNTER64_TO_UINT8(u8Val)	\
					((((AR_UINT8)u8Val.msn) <<32) |  u8Val.lsn)

#endif  /*  _UTLMACRO_H  */
