#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define FAT1_START 512
#define ROOT_START 0x2600
#define DATA_START 0x3e00
#define BytsPerSec 512

typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define MAX 100

char input[MAX];

typedef struct {
	char	BPB_BytsPerSec[2];													//每扇区字节数
	char	BPB_SecPerClus[1];													//每簇扇区数
	char	BPB_RsvdSecCnt[2];													//Boot记录占用多少扇区
	char	BPB_NumFATs[1];														//共有多少FAT表
	char	BPB_RootEntCnt[2];													//根目录文件数最大值
	char	BPB_TotSec16[2];													//扇区总数
	char	BPB_Media[1];														//介质描述符
	char	BPB_FATSz16[2];														//每FAT扇区数  
	char	BPB_SecPerTrk[2];													//每磁道扇区数
	char	BPB_NumHeads[2];													//磁头数
	char	BPB_HiddSec[4];														//隐藏扇区数
	char	BPB_TotSec32[4];													//如果BPB_TotSec16为0 则由这个值记录扇区数
} BPB;

typedef struct {
	char	DIR_Name[11];														//文件名8字节扩展名3字节
	unsigned char	DIR_Attr[1];														//文件属性
	char	DIR_Reserved[10];													//保留位
	char	DIR_WrtTime[2];														//最后一次写入时间
	char	DIR_WrtDate[2];														//最后一次写入日期
	unsigned	char	DIR_FstClus[2];														//此条目对应的开始簇号
	char	DIR_FileSize[4];													//文件大小
} RootDir;

void readFile(FILE* file, char* path, int offset, RootDir* rd);
void readDirByPath(FILE* file, char* originPath, char* path, int offset, RootDir* rd);
void readFileByPath(FILE* file,char* name, char* originPath, char* path, int offset, RootDir* rd, BOOL* found);
BOOL contains(char* str1,char* str2);
BOOL belongTo(char* str1,char* str2);
BOOL pathEquals(char* str1,char* str2);
BOOL fileEquals(char* str1,char* str2);
BOOL isInputDir(char* input);
BOOL isEmpty(RootDir* rd);
BOOL isDir(RootDir* rd);
BOOL isValid(RootDir* rd);
void getInputFileName(char* str1,char* str2);
void getInputDir(char* str1,char* str2);
void readContent(FILE* file,int fst_Clus); 
void addToPath(char* path, char* oldPath);
void printFile(char* path, RootDir* rd);
void printPath(char* path);
void my_print(char* c, int length);
void change_color();
void ret_color();
int getFATValue(FILE* fat12,int num);
int TotalSection;

int main(){
	FILE *file = fopen("a.img", "r");											//打开 a.img
	RootDir* rd = (RootDir*)malloc(sizeof(RootDir));							//分配地址空间
	memset(rd, 0, sizeof(RootDir));

	readFile(file, "", ROOT_START, rd);

	while(TRUE){
		fseek(file, 0, SEEK_SET);
		memset(input,0,sizeof(input));
		printf(">>");
		gets(input);
		//printf("\n");
		//printf("%s\n",input);
		if(input[0]=='!')
			break;
		if(isInputDir(input)){
			readDirByPath(file,input,"", ROOT_START, rd);
		}
		else{
			char name[20];
			char dir[MAX];
			BOOL found=FALSE;
			getInputFileName(name,input);
			getInputDir(dir,input);
			readFileByPath(file,name,dir,"", ROOT_START, rd, &found);
			if(!found)
				printf("Not Found!\n");
		}
	}

	free(rd);
	fclose(file);

	return 0;
}

void getInputFileName(char* name,char* input){
	int ocur=0;
	int i=0;
	for(i=strlen(input)-1;i>=0;i--){
		if(input[i]=='/'){
			ocur=i+1;
			break;
		}
	}

	for(i=ocur;i<strlen(input);i++){
		name[i-ocur]=input[i];
	}

	name[i-ocur]='\0';
}

void getInputDir(char* dir,char* input){
	int ocur=0;
	int i=0;
	for(i=strlen(input)-1;i>=0;i--){
		if(input[i]=='/'){
			ocur=i;
			break;
		}
	}

	for(i=0;i<=ocur;i++){
		dir[i]=input[i];
	}

	dir[ocur+1]='\0';
}

void readFile(FILE* file, char* path, int offset, RootDir* rd){

	int RD_Size = sizeof(RootDir);

	long point = ftell(file);
	fseek(file, offset, SEEK_SET);
	int cnt=0;
	do {
		memset(rd, 0, RD_Size);
		fread(rd, RD_Size, 1, file);
		if (isDir(rd)){
			int fst_Clus = rd->DIR_FstClus[0] + rd->DIR_FstClus[1] * 0x10;
			char* newPath = (char*)malloc(MAX);
			memset(newPath, 0, strlen(newPath));
			memcpy(newPath, path, strlen(path));
			RootDir oldRd;
			memcpy(&oldRd, rd, RD_Size);
			addToPath(newPath, rd->DIR_Name);
			readFile(file, newPath, (fst_Clus * 0x200 + DATA_START), rd);
			memcpy(rd, &oldRd, RD_Size);
			free(newPath);
			cnt++;
		}else{
			if (isValid(rd)){
				// printf("%s%s\n",path, rd->DIR_Name);
				printFile(path, rd);
				cnt++;
			}
		}
	}while(!isEmpty(rd));
	fseek(file, point, SEEK_SET);

	if(!cnt){
		//printf("path:");
		//puts(path);
		printPath(path);
	}
}

void readDirByPath(FILE* file, char* originPath, char* path, int offset, RootDir* rd){

	int RD_Size = sizeof(RootDir);

	long point = ftell(file);
	fseek(file, offset, SEEK_SET);
	if(contains(path,originPath)||belongTo(path,originPath)){
		int cnt=0;
		do {
			memset(rd, 0, RD_Size);
			fread(rd, RD_Size, 1, file);
			if (isDir(rd)){
					int fst_Clus = rd->DIR_FstClus[0] + rd->DIR_FstClus[1] * 0x10;
					char* newPath = (char*)malloc(MAX);
					memset(newPath, 0, strlen(newPath));
					memcpy(newPath, path, strlen(path));
					RootDir oldRd;
					memcpy(&oldRd, rd, RD_Size);
					addToPath(newPath, rd->DIR_Name);
					readDirByPath(file, originPath, newPath, (fst_Clus * 0x200 + DATA_START), rd);
					memcpy(rd, &oldRd, RD_Size);
					free(newPath);
					cnt++;
			}else{
				if (contains(path,originPath)&&isValid(rd)){
					// printf("%s%s\n",path, rd->DIR_Name);
					printFile(path, rd);
					cnt++;
				}
			}
		}while(!isEmpty(rd));

		if(!cnt)
			printPath(path);
	}
	fseek(file, point, SEEK_SET);

}

void readFileByPath(FILE* file, char* name, char* originPath, char* path, int offset, RootDir* rd, BOOL* found){

	int RD_Size = sizeof(RootDir);

	long point = ftell(file);
	fseek(file, offset, SEEK_SET);
	//printf("INpath:\n");
	//puts(path);
	//printf("\n");
	if(belongTo(path,originPath)){
		//printf("belongTo\n");
		do {
			memset(rd, 0, RD_Size);
			fread(rd, RD_Size, 1, file);
			if (isDir(rd)){
					int fst_Clus = rd->DIR_FstClus[0] + rd->DIR_FstClus[1] * 0x10;
					char* newPath = (char*)malloc(MAX);
					memset(newPath, 0, strlen(newPath));
					memcpy(newPath, path, strlen(path));
					RootDir oldRd;
					memcpy(&oldRd, rd, RD_Size);
					addToPath(newPath, rd->DIR_Name);
					readFileByPath(file,name,originPath, newPath, (fst_Clus * 0x200 + DATA_START), rd,found);
					memcpy(rd, &oldRd, RD_Size);
					free(newPath);
			}else{
				if (pathEquals(path,originPath)&&fileEquals(rd->DIR_Name,name)&&isValid(rd)){
					// printf("%s%s\n",path, rd->DIR_Name);
					int fatVal=rd->DIR_FstClus[0] + rd->DIR_FstClus[1] * 0x10;
					while(fatVal<0xFF8){
						readContent(file,fatVal);
						*found=TRUE;
						fatVal=getFATValue(file,fatVal);
						//printf("FATVAL:%3X\n",fatVal);
						if(fatVal==0xFF7){
							my_print("坏",1);
							my_print("簇",1);
							break;
						}
					}

					my_print("\n",1);
					return;
				}
			}
		}while(!isEmpty(rd));
	}
	fseek(file, point, SEEK_SET);

}

void readContent(FILE* file,int fst_Clus){
	int offset=fst_Clus * 0x200 + DATA_START;
	//printf("clus[0]:%d,clus[1]:%d\n",clus[0],clus[1]);
	//printf("fst_clu:%d\n",fst_Clus);
	//printf("DATA_START:%6X\n",DATA_START);
	//printf("offset:%6X\n",offset);
	fseek(file, offset, SEEK_SET);
	
	char* content = (char* )malloc(BytsPerSec);  //暂存从簇中读出的数据   

	fread(content,1,BytsPerSec,file);
	
	int i=0;
	//change_color();
	while(TRUE){
		if(i>=BytsPerSec||content[i]=='\0'||content[i]==-1)
			break;
		//printf("!%2X",content[i]);
		my_print(content+i,1);
		//printf("A:%2X",content[i]);
		++i;	
	}
	//ret_color();
	//my_print("\n",1);

	free(content);
}

//path="home    house   bed",orgin="home/"
//origin contains path
BOOL contains(char* path,char* origin){
	int len=strlen(path);
	int cnt=0;
	int a=0;
	for(a=0;a<strlen(origin);a++){
		if(origin[a]=='/')
			cnt++;
	}

	int oIndex=0;
	int i=0;
	for(i=0;i<cnt;i++){
		BOOL isSpace=FALSE;
		int j=0;
		for(j=0;j<8;j++){
			int k=8*i+j;
			if(k>=len)
				return FALSE;

			if(isSpace){
				if(path[k]!=' ')
					return FALSE;
			}
			else{
				if(origin[oIndex]=='/'){
					isSpace=TRUE;
					if(path[k]!=' ')
						return FALSE;
				}
				else if(path[k]!=origin[oIndex])
					return FALSE;
				
				oIndex++;
			}
		}
	}

	return TRUE;
}

//path="home    ",origin="home/house/"
//origin belong to path
BOOL belongTo(char* path,char* origin){
	int len=strlen(path);

	int cnt=0;
	int a=0;
	for(a=0;a<strlen(origin);a++){
		if(origin[a]=='/')
			cnt++;
	}
	
	if(cnt<len/8)
		return FALSE;

	int oIndex=0;
	int i=0;
	for(i=0;i<len/8;i++){
		BOOL isSpace=FALSE;
		int j=0;
		for(j=0;j<8;j++){
			int k=8*i+j;
			if(k>=len)
				return FALSE;

			if(isSpace){
				if(path[k]!=' ')
					return FALSE;
			}
			else{
				if(origin[oIndex]=='/'){
					isSpace=TRUE;
					if(path[k]!=' ')
						return FALSE;
				}
				else if(path[k]!=origin[oIndex])
					return FALSE;
				
				oIndex++;
			}
		}
	}

	return TRUE;
}

BOOL pathEquals(char* str1,char* str2){
	return contains(str1,str2)&&belongTo(str1,str2);
}

//str1 for fat12 dir_name
BOOL fileEquals(char* str1,char* str2){
   int i=0;
   for(i=0;i<strlen(str2);i++){
       if(str2[i]=='.')
           break;
   }

   if(i==strlen(str2)){
       i=20;
   }

   int j=0;
   for(j=0;j<8;j++){
       if(j<i){
           if(str1[j]!=str2[j])
               return FALSE;
       }
       else{
           if(str1[j]!=' ')
               return FALSE;
       }
   }
   for(j=8;j<11;j++){
       if(i!=20&&j-8+i+1<strlen(str2)){
           if(str1[j]!=str2[j+i+1-8])
               return FALSE;
       }
       else{
           if(str1[j]!=' ')
               return FALSE;
       }
   }

   return TRUE;
}

void addToPath(char* path, char* oldPath){
	int p = 0;
	while (path[p] != 0)
		p += 8;
	int i = 0;
	for (i = 0; i < 8; i++){
		path[p + i] = oldPath[i];
	}
	path[p+8]=0;
}

BOOL isInputDir(char* input){
//	puts(input);
//	printf("here!\n");
	if(input[strlen(input)-1]=='/'){
//		printf("true\n");
		return TRUE;
	}
	return FALSE;
}

BOOL isEmpty(RootDir* rd){
	int i = 0;
	for (i = 0; i < 0x20; i++){
		if(((char*)rd)[i] > 0){
			return FALSE;
		}
	}
	return TRUE;
}

BOOL isDir(RootDir* rd){
	unsigned char c = rd->DIR_Attr[0];
	if(c&&0x10)
		return TRUE;
	else
		return FALSE;
}

BOOL isValid(RootDir* rd){
	int j = 0;
		for (j = 0; j < 11; j++){
			char c = rd->DIR_Name[j];
			if (! ((c >= '0' && c <= '9') 
				|| (c >= 'a' && c <= 'z')
				|| (c >= 'A' && c <= 'Z')
				|| c == ' ')){
				return FALSE;
			} 
		}
	return TRUE;
}

void printFile(char* path, RootDir* rd){
	int i = 0;
	change_color();
	char* p = "/";
	while (i < strlen(path)){
		if (path[i] != ' '){
			my_print(&path[i], 1);
		}
		if (i % 8 == 7){
			my_print(p, 1);
		}
		i++;
	}
	ret_color();

	for (i = 0; i < 8; i++){
		if (rd->DIR_Name[i] == ' '){
			break;
		}
		my_print(&rd->DIR_Name[i], 1);
	}
	my_print(".", 1);
	for (i = 8; i < 11; i++){
		if (rd->DIR_Name[i] == ' '){
			break;
		}
		my_print(&rd->DIR_Name[i], 1);
	}
	my_print("\n",1);
}

void printPath(char* path){
	int i = 0;
	change_color();
	char* p = "/";
	while (i < strlen(path)-1){
		if (path[i] != ' '){
			my_print(&path[i], 1);
		}
		if (i % 8 == 7){
			my_print(p, 1);
		}
		i++;
	}
	ret_color();

	my_print("\n",1);
}

int getFATValue(FILE * fat12 , int num) {
	//FAT1的偏移字节
	int fatBase = BytsPerSec;
	//FAT项的偏移字节
	int fatPos = fatBase + num*3/2;
	//奇偶FAT项处理方式不同，分类进行处理，从0号FAT项开始
	
	//先读出FAT项所在的两个字节
	unsigned short bytes;
	unsigned short* bytes_ptr = &bytes;
	int check;
	check = fseek(fat12,fatPos,SEEK_SET);
	
	check = fread(bytes_ptr,1,2,fat12);
	
	//u16为short，结合存储的小尾顺序和FAT项结构可以得到
	//type为0的话，取byte2的低4位和byte1构成的值，type为1的话，取byte2和byte1的高4位构成的值
	//printf("%4X",bytes);
	if (num%2 == 0) {
		//printf("oushu!\n");
		return bytes%0x01000;
	} else {
		//printf("jishu!\n");
		return bytes>>4;
	}

} 
/*
void my_print(char* c, int length){
	putchar(c[0]);
}
void change_color(){}
void ret_color(){}
*/
