#include "general_sdcard.h"
#include "main.h"
#include <stdio.h>
#include <string.h>


/*********************************************************/
/*                     SD CARD Basic Functions                                   */
/*********************************************************/

bool writeInfoToSD(char *writeFile, char *writeBuffer)
{
	FIL fp;
	uint8_t result = 0;
	int size = 0;
	
	result = createFile(writeFile, &fp);
	printf("write file is %s\r\n", writeFile);
	if(result != FR_OK){
		printf("create result is %d\r\n", result);
	  printf("Create Error: %s : %d \r\n",__FILE__, __LINE__);
	  return false;
	}
	else{
	  printf("File Create Success.\r\n"); 
		
	}
	size = writeDataToFile(writeBuffer,&fp);
	printf("File write Success, size:  %d \r\n",size);
	
	/* Close file */
	closeFile(&fp);
	return true;
}

bool readInfoFromSD(char *readFile,  char** staticInfo)
{
	uint8_t result = 0;
	int size = 0;
	FIL fp;
	uint32_t  bytesRead;
	

	result = openFile(readFile, &fp);		
	if(result != FR_OK){
		printf("File Open Error: %s : %d \r\n",__FILE__, __LINE__);
		return false;
	}
	else{
	 	printf("File Open Success.\r\n");
	}
	
	size = fileSize(&fp);
 	printf("file size : %d \n", size);
 	*staticInfo = (char*)malloc(size);
		
	result = readDataFromFile(&fp, *staticInfo, size, &bytesRead);
		
	if(result != FR_OK || bytesRead == 0){
	 	printf("f_read error result value is %d \r\n", result);
    printf("f_read Error: %s : %d \r\n",__FILE__, __LINE__);
	  return false;
  }
  else{
  	printf("File Read Success.\r\n");	
  }
	
	closeFile(&fp);
	return true;
}

/*********************************************************/
/*                     SD CARD Basic Functions                                   */
/*********************************************************/

/* Mount SD Card */
void mountSD(FATFS* fs)
{
  if(f_mount(fs, "", 0) != FR_OK)
	  printf("f_mount Error: %s : %d \r\n",__FILE__, __LINE__);
}

/* Unmount SDCARD */
void unmountSD(void)
{
  if(f_mount(NULL, "", 1) != FR_OK)
	  printf("f_mount Error: %s : %d \r\n",__FILE__, __LINE__);	
}

int fileSize(FIL* fp)
{
	return  f_size(fp);
}

FRESULT existFile(char *fileName, FILINFO* finfo)
{
	return  f_stat(fileName, finfo);
}


FRESULT openFile(char *fileName, FIL* fp)
{
	return  f_open(fp, fileName, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
}

FRESULT createFile(char *fileName, FIL* fp)
{
	return  f_open(fp, fileName, FA_CREATE_NEW | FA_WRITE);
}

	
FRESULT readDataFromFile(FIL* fp, void* readBuffer, UINT btr, UINT* br)
{
	return f_read(fp, readBuffer, btr, br);
}

	
int writeDataToFile(char *str ,FIL* fp)
{
	return f_puts(str, fp);

}

void closeFile(FIL* fp)
{
	if(f_close(fp) != FR_OK)
	  printf("f_close Error: %s : %d \r\n",__FILE__, __LINE__);
}
