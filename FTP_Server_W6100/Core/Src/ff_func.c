//#include "ff.h"			/* Declarations of FatFs API */
#include "ff_func.h"
#include "stdio.h"
#include "ff.h"


#define _FF_F_DEBUG_

/*--------------------------------------------------------------------------

   Public Functions

--------------------------------------------------------------------------*/

FRESULT scan_files(char* path, char *buf, int * buf_len)
{
    unsigned int file_cnt = 0;
    FRESULT res;
    FILINFO fno;
    DIR dir;
    int i, len;
    char *fn;
   char temp_mon[12][4] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
   char *p_buf = buf;
   char temp_dir = 0;
   WORD temp_f_date = 0;
   WORD temp_f_time = 0;
    
 
//#if _USE_LFN
#if 0
    static char lfn[_MAX_LFN * (_DF1S ? 2 : 1) + 1];
    fno.altname = lfn;
    fno.lfsize = sizeof(lfn);
#endif

 printf("Open..");
    res = f_opendir(&dir, path);
 
    if (res == FR_OK) {
        i = strlen(path);
        printf("OK.");
        for (;;) {
            res = f_readdir(&dir, &fno);
            if (res != FR_OK || fno.fname[0] == 0) break;
            if(fno.fname[0] == '.') continue;
//#if _USE_LFN
#if 0
            fn = *fno.altname ? fno.altname : fno.fname;
#else
            fn = fno.fname;
#endif
            if(fno.fdate == 0)
            {
                fno.fdate = temp_f_date;
                fno.ftime = temp_f_time;
            }
            else
            {
                temp_f_date = fno.fdate;
                temp_f_time = fno.ftime;
            }
                //get_fileinfo(&dir, &fno);
            printf("f_readdir ret : %d, fname %c\r\n", res, fno.fname[0]);
            //printf("modtime = %02X%02X%02X%02X \r\n", (dir.dir + 22)[3], (dir.dir + 22)[2], (dir.dir + 22)[1],(dir.dir + 22)[0]);
            printf("f_date:%x, f_time:%x\r\n", fno.fdate, fno.ftime);

            if (fno.fattrib & AM_DIR) {
                temp_dir = 'd';
                //printf("[D]%s\r\n",fn);
            } else {
                temp_dir = '-';
                //printf("%s/%s : \t\t %dbyte\r\n", path, fn,fno.fsize);
            }
            len = sprintf(p_buf, "%crwxr-xr-x 1 ftp ftp %d %s %d %d %s\r\n",temp_dir, fno.fsize,temp_mon[((fno.fdate >> 5) & 0x0f) -1], (fno.fdate & 0x1f), (((fno.fdate >> 9) & 0x7f) + 1980), fn);
            printf("mon = %d, day = %d,  year = %d \r\n", ((fno.fdate >> 5) & 0x0f) -1, (fno.fdate & 0x1f), (((fno.fdate >> 9) & 0x7f) + 1980));
            printf("buf[%d]:%s", len, p_buf);
            p_buf += len;
        }
    }
    else{
        printf("path(%s) not found:Error(%d)\r\n",path,res);
    }
    *p_buf = 0;
    *buf_len = strlen(buf);
    printf("last[%d]=%s[end]\r\n",*buf_len, buf);
    return file_cnt;
}
#if 0
FRESULT scan_files(char* path, char *buf, int * buf_len)
{
	FRESULT res;
	FILINFO fno;
	DIR dir;
	int i, len, buf_ptr = 0;
	char *fn; 	/* This function is assuming no_Unicode cfg.*/
	char date_str[15];
	int date_str_ptr = 0;
    DWORD now_sect = 0;
//#ifdef _USE_LFN
#if 0
	static char lfn[_MAX_LFN + 1];
	fno.lfname = lfn;
	fno.lfsize = sizeof(lfn);
#endif

	res = f_opendir(&dir, path);
#if defined(_FF_F_DEBUG_)
    printf("%s->%s [%d]\r\n",  __FILE__, __FUNCTION__, __LINE__);
	printf("f_opendir res: %d path:%s\r\n", res, path);
#endif
	if(res == FR_OK){
		i = strlen(path);
#if defined(_FF_F_DEBUG_)
        printf("strlen of path: %s %d \r\n", path, i);
#endif
		for(;;){
			res = f_readdir(&dir, &fno);
            printf("f_readdir ret : %d, fname %c\r\n", res, fno.fname[0]);
			if(res != FR_OK || fno.fname[0] == 0) break;
			if(fno.fname[0] == '.') continue;
            #if 0
#ifdef _USE_LFN
			fn = *fno.lfname ? fno.lfname : fno.fname;
#else
			fn = fno.fname;
#endif
#else
            fn = fno.fname;
#endif
#if defined(_FF_F_DEBUG_)
            printf("1 sect : %ld\r\n", dir.sect);
#endif
            now_sect = dir.sect;
			switch((fno.fdate >> 5) & 0x0f)
			{
			case 1:
				len = sprintf(date_str, "JAN ");
				break;
			case 2:
				len = sprintf(date_str, "FEB ");
				break;
			case 3:
				len = sprintf(date_str, "MAR ");
				break;
			case 4:
				len = sprintf(date_str, "APR ");
				break;
			case 5:
				len = sprintf(date_str, "MAY ");
				break;
			case 6:
				len = sprintf(date_str, "JUN ");
				break;
			case 7:
				len = sprintf(date_str, "JUL ");
				break;
			case 8:
				len = sprintf(date_str, "AUG ");
				break;
			case 9:
				len = sprintf(date_str, "SEP ");
				break;
			case 10:
				len = sprintf(date_str, "OCT ");
				break;
			case 11:
				len = sprintf(date_str, "NOV ");
				break;
			case 12:
				len = sprintf(date_str, "DEC ");
				break;
			}
			date_str_ptr += len;
			len = sprintf(date_str + date_str_ptr, "%d ", (fno.fdate & 0x1f));
			date_str_ptr += len;
			len = sprintf(date_str + date_str_ptr, "%d", (((fno.fdate >> 9) & 0x7f) + 1980));
			date_str_ptr = 0;
#if defined(_FF_F_DEBUG_)
            printf("date str : %s \r\n date_str_ptr:%d\r\n", date_str, date_str_ptr);
#endif
#if defined(_FF_F_DEBUG_)
                        printf("2 sect : %ld\r\n", dir.sect);
#endif

			if(fno.fattrib & AM_DIR)
			{
				sprintf(buf + buf_ptr, "d");
			}else
			{
				sprintf(buf + buf_ptr, "-");
			}
			buf_ptr++;
			// drwxr-xr-x 1 ftp ftp              0 Apr 07  2014 $RECYCLE.BIN\r\n
			//len = sprintf(buf + buf_ptr, "rwxr-xr-x 1 ftp ftp              %d %s %s\r\n", fno.fsize, date_str, fn);
			len = sprintf(buf + buf_ptr, "rwxr-xr-x 1 ftp ftp %d %s %s\r\n", fno.fsize, date_str, fn);
			buf_ptr += len;
#if defined(_FF_F_DEBUG_)
            printf("fn: %s, buff_ptr:%d \r\n", fn, buf_ptr);
#endif
            if(dir.sect != now_sect)
            {
#if defined(_FF_F_DEBUG_)
                printf("now sect:%d, dir.sect: %d\r\n", now_sect, dir.sect);
#endif
                break;
            }
		}
#if defined(_FF_F_DEBUG_)
        *buf_len = strlen(buf);
        printf("[%s]\r\n", buf);
        printf("buf_len : %d, sizeof(buf): %d\r\n", buf_len, sizeof(buf));
#endif
		f_closedir(&dir);
	}
	return res;
}
#endif
int get_filesize(char* path, char *filename)
{
	FRESULT res;
	FILINFO fno;
	DIR dir;
	int i, len, buf_ptr = 0;
	char *fn; 	/* This function is assuming no_Unicode cfg.*/
//#ifdef _USE_LFN
#if 0
	static char lfn[_MAX_LFN + 1];
	fno.lfname = lfn;
	fno.lfsize = sizeof(lfn);
#endif

	if(*path == 0x00)
		res = f_opendir(&dir, "/");
	else
		res = f_opendir(&dir, path);
#if defined(_FF_F_DEBUG_)
    printf("f_opendir res: %d\r\n", res);
#endif
	if(res == FR_OK){
		for(;;){
			res = f_readdir(&dir, &fno);
			if(res != FR_OK || fno.fname[0] == 0) break;
			if(fno.fname[0] == '.') continue;
            #if 0
#ifdef _USE_LFN
			fn = *fno.lfname ? fno.lfname : fno.fname;
#else
			fn = fno.fname;
#endif
#else
            fn = fno.fname;
#endif
			if(!strcmp(fn, filename))
			{
				if(fno.fattrib & AM_DIR){
#if defined(_FF_F_DEBUG_)
                    printf("\r\n%s/%s is a directory\r\n", path, filename);
#endif
					return 0;
				}
				return fno.fsize;
			}
		}
#if defined(_FF_F_DEBUG_)
        printf("\r\n%s/%s was not found\r\n", path, filename);
#endif
        f_closedir(&dir);

	}
	return -1;
}

