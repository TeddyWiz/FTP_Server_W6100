
#ifndef __FF_FUNC_H__
#define __FF_FUNC_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "ff.h"

FRESULT scan_files(char* path, char* buf, int * buf_len);
int get_filesize(char* path, char *filename);



#ifdef __cplusplus
}
#endif

#endif /* _FF_FUNC */

