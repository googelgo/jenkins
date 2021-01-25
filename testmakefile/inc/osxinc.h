/*
 * Copyright (C) 2006 Aricent Inc . All Rights Reserved
 *
 * $Id: osxinc.h,v 1.1.1.1 2007/11/19 05:28:24 allprojects Exp $
 *
 * Description:
 * Header of all headers.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <errno.h>
#include <assert.h>

#include "macro.h"
#include "osxsys.h"
#include "osix.h"
#include "utlmacro.h"
#include "osxprot.h"
#include "srmmem.h"
#include "srmmemi.h"


VOID UtlTrcClose (VOID);
