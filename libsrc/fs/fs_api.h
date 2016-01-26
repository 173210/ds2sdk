#ifndef __FS_API_H__
#define __FS_API_H__
//v1.0

#include "sys/stat.h"
#include "fatfile.h"
#include "fatdir.h"

typedef unsigned int mode_t;

extern int fat_init(void);

extern FILE_STRUCT* fat_fopen(const char *file, const char *mode);

extern size_t fat_fread(void *buf, size_t size, size_t count, FILE_STRUCT *fp);

extern size_t fat_fwrite(const void *buf, size_t size, size_t count, FILE_STRUCT *fp);

extern int fat_fclose(FILE_STRUCT *fp);

extern int fat_fseek(FILE_STRUCT *fp, long offset, int whence);

extern long fat_ftell(FILE_STRUCT *fp);

extern int fat_feof(FILE_STRUCT *fp);

extern int fat_ferror(FILE_STRUCT *fp);

extern void fat_clearerr(FILE_STRUCT *fp);

extern int fat_fflush(FILE_STRUCT *fp);

extern int fat_fgetc(FILE_STRUCT *fp);

extern char* fat_fgets(char *buf, int n, FILE_STRUCT *fp);

extern int fat_fputc(int ch, FILE_STRUCT *fp);

extern int fat_fputs(const char *s, FILE_STRUCT *fp);

extern int fat_remove(const char *filename);

extern int fat_rename(const char *oldName, const char *newName);

extern void fat_rewind(FILE_STRUCT *fp);

extern int fat_fstat(int fildes, struct stat *buf);

extern int fat_fprintf(void* fp, const char *format, ...);

extern int fat_fscanf(FILE_STRUCT *fp, const char *format, ...);

extern DIR_STATE_STRUCT* fat_opendir(const char *name);

extern DIR_ENTRY* fat_readdir(DIR_STATE_STRUCT *dirp);

extern long fat_telldir(DIR_STATE_STRUCT *dirp);

extern void fat_seekdir(DIR_STATE_STRUCT *dirp, long int loc);

extern int fat_closedir(DIR_STATE_STRUCT *dirp);

extern int fat_chdir(const char *path);

extern char* fat_getcwd(char *buf, size_t size);

extern int fat_mkdir(const char *path, mode_t mode);

extern int fat_rmdir(const char *path);

extern int fat_lstat(const char *path, struct stat *buf);

extern DIR_ENTRY* fat_readdir_ex(DIR_STATE_STRUCT *dirp, struct stat *statbuf);

//#define S_ISDIR(st)	(st.st_mode & S_IFDIR)

#define FILE	FILE_STRUCT
#define fopen	fat_fopen
#define fread	fat_fread
#define fwrite	fat_fwrite
#define fclose	fat_fclose
#define fseek	fat_fseek
#define ftell	fat_ftell
#define feof	fat_feof
#define ferror	fat_ferror
#define fclearerr	fat_clearerr
#define fflush	fat_fflush
#define fgetc	fat_fgetc
#define fgets	fat_fgets
#define fputc	fat_fputc
#define fputs	fat_fputs
#define fprintf	fat_fprintf
#define fscanf	fat_fscanf
#define remove	fat_remove

#define DIR		DIR_STATE_STRUCT
#define dirent	DIR_ENTRY
#define opendir	fat_opendir
#define readdir	fat_readdir
#define telldir	fat_telldir
#define seekdir	fat_seekdir
#define closedir	fat_closedir
#define chdir	fat_chdir
#define getcwd	fat_getcwd
#define mkdir	fat_mkdir
#define rmdir	fat_rmdir

#define lstat	fat_lstat
#define fstat	fat_fstat

//the extended version of readdir_ex
#define readdir_ex	fat_readdir_ex

#define MAX_PATH 512
#define MAX_FILE 512

//Misc function
extern bool fat_getDiskSpaceInfo( char * diskName, unsigned int *total, unsigned int  *used, unsigned int *freeSpace );

#endif //__FS_API_H__
