#pragma once
#include <occi.h>
#include <map>
#include <string>
#include <fstream>


using namespace std;
using namespace oracle::occi;

#define FILE_NAME_SIZE 32
#define BIG_BUFF_SIZE 1024
#define BUFF_SIZE 512

int WriteLog(char *szMessage);
int ParseFile(char *szFilePath, char *szRegID);

struct pc_info
{
	char MAC_Addr[BUFF_SIZE] = { 0 };
	char System[BUFF_SIZE] = { 0 };
	char Computer_Name[BUFF_SIZE] = { 0 };
	char IP_Addr[BUFF_SIZE] = { 0 };
	char Current_User_Name[BUFF_SIZE] = { 0 };
	char CPU[BUFF_SIZE] = { 0 };
	char CPU_Freq_in_MHz[BUFF_SIZE] = { 0 };
	char Memory_in_Mb[BUFF_SIZE] = { 0 };
	char Total_HDD_in_Mb[BUFF_SIZE] = { 0 };
	char Record_Date[BUFF_SIZE] = { 0 };
	char RegionID[BUFF_SIZE] = { 0 };
};

struct region_path_id
{
	char RegID[4] = { 0 };
	char FolderPath[MAX_PATH] = { 0 };
};

map<char*, char*> mapPathId;
map<char*, char*>::iterator it_mapPathId;
// String to log-file
char szLog[BIG_BUFF_SIZE] = { 0 };
// String to send query to DB
char szQuery[BIG_BUFF_SIZE] = { 0 };
// Connection string to DB
char szDBConnectionString[BUFF_SIZE] = { 0 };
char szDBUser[BUFF_SIZE] = { 0 };
char szDBPass[BUFF_SIZE] = { 0 };
HANDLE hLogFile = NULL;
// Work with DB
Environment* pEnv = nullptr;
Connection* pConn = nullptr;
Statement* pStmt = nullptr;