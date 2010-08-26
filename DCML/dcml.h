
#ifndef __DCMLFE_H__
#define __DCMLFE_H__  1

#include <stdio.h>
#include <stdlib.h>

#include "midas.h"

#include "experim.h"

#define V1724_MAX_CH_SAMPLES  0x80000 /* 512k = 512*1024 = 524288 */
#define V1724_SAMPLE_BITS     14      /* 14-bit, 2**14 = 16384 */
#define V1724_THRESHOLD_MAX   1<<13   /* 2**13 = 8192 */
#define V1724_THRES_SCALE     0.25    /* (double)(1<<14)/(double)(1<<16) = 2**-2 */
#define V1724_DAC_FACTOR      1.0

/* Return status of frontend */
#define FE_ERROR     -256

#endif /* __DCMLFE_H__ */

