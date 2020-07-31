
#define RSK_COMPAT_H 


#include "config.h"






#define LC_LINEBUF_LEN 1024












#include <inttypes.h>


#include <memory.h>












#include <sys/types.h>





















#include <stdio.h>






#include <sys/time.h>
#include <time.h>














#define lc_fopen(path, mode) fopen(path, mode)



