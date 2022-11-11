#ifndef __GENERAL_SDCARD_H
#define __GENERAL_SDCARD_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include "main.h"
#include "ff.h"
bool readInfoFromSD(char *readFile,  char** staticInfo);
bool writeInfoToSD(char *writeFile, char *writeBuffer);

void mountSD(FATFS* fs);
void unmountSD(void);
int fileSize(FIL* fp);
/**
 * @brief 존재하는 파일
 * 
 * @param fileName File Name
 * @param finfo File info
 * @return FRESULT Return Code
 */
FRESULT existFile(char *fileName, FILINFO* finfo);
/**
 * @brief File Read
 * 
 * @param fileName File Name
 * @param fp File object
 * @return FRESULT Return Code
 */
FRESULT openFile(char *fileName, FIL* fp);
/**
 * @brief File Write
 * 
 * @param fileName File Name
 * @param fp File object
 * @return FRESULT Return Code
 */
FRESULT createFile(char *fileName, FIL* fp);
/**
 * @brief File로 부터 Data Read
 * 
 * @param fp File object
 * @param readBuffer Read Data
 * @param btr  Number of bytes to read 
 * @param br Pointer to number of bytes read
 * @return FRESULT 
 */
FRESULT readDataFromFile(FIL* fp, void* readBuffer, UINT btr, UINT* br);
int writeDataToFile(char *str ,FIL* fp);
void closeFile(FIL* fp);

#endif
