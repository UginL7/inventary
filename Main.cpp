// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <Windows.h>
#include "tinyxml2.h"
#include "Structures.h"

#pragma warning(disable : 4996)

#define LOG_ON TRUE

#ifdef _DEBUG
#pragma comment (lib, "oraocci12d.lib")
#else
#pragma comment (lib, "oraocci12.lib")
#endif // _DEBUG


// Translte code error to message
void GetLastErrorMessage(DWORD dwErrCode, char *szErrText)
{
	LPSTR szBuff = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dwErrCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&szBuff, 0, NULL);
	sprintf(szErrText, "\t==>Error\n\tCode = %d(0x%x)   Message:%s", dwErrCode, dwErrCode, szBuff);

	LocalFree(szBuff);
}

// Generation file name like this result_YYYY-MM-DD.log
void GenerateFileName(char *szFileName, int nSize)
{
	char szTmp[10] = { 0 };
	SYSTEMTIME Time;
	GetSystemTime(&Time);
	sprintf_s(szFileName, nSize, "result_%d_%.2d_%02d.log", Time.wYear, Time.wMonth, Time.wDay);
}

// Writing log-file
int WriteLog(char *szMessage)
{
	BOOL bResult = FALSE;
	DWORD dwReal;
	char szFileName[FILE_NAME_SIZE] = { 0 };

	if (hLogFile == NULL)
	{
		GenerateFileName(szFileName, FILE_NAME_SIZE);
		hLogFile = CreateFile(szFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hLogFile == INVALID_HANDLE_VALUE)
		{
			char szErrMessage[BIG_BUFF_SIZE] = { 0 };
			DWORD dwErr = GetLastError();
			GetLastErrorMessage(dwErr, szErrMessage);
			MessageBox(NULL, szErrMessage, "Error", MB_OK);
			return -1;
		}
		SetFilePointer(hLogFile, 0, NULL, FILE_END);
	}
	

	bResult = WriteFile(hLogFile, szMessage, strlen(szMessage), &dwReal, NULL);
	if (bResult == FALSE)
	{
		char szErrMessage[BIG_BUFF_SIZE] = { 0 };
		DWORD dwErr = GetLastError();
		GetLastErrorMessage(dwErr,szErrMessage);
		MessageBox(NULL, szErrMessage, "Error", MB_OK);
		return -1;
	}
	return 0;
}


/************************************************************************/
/*						processing file                                 */
/************************************************************************/

// Parsing a xml-file that contains the path to the folder
int ReadPath()
{
#if LOG_ON
	WriteLog("\t===>LOG\n\t\t---===ReadPath===---\n");
#endif
	int nFolderPathLength = 0;
	char szFolderPath[BUFF_SIZE] = { 0 };
	char szFullXmlFilePath[BUFF_SIZE] = { 0 };
	
	nFolderPathLength = GetModuleFileName(NULL, szFolderPath, BUFF_SIZE);
	if (nFolderPathLength == 0)
	{
		char szErrMessage[BIG_BUFF_SIZE] = { 0 };
		DWORD dwErr = GetLastError();
		GetLastErrorMessage(dwErr, szErrMessage);
		MessageBox(NULL, szErrMessage, "Error", MB_OK);
		return -1;
	}
	strncpy_s(szFullXmlFilePath, BUFF_SIZE, szFolderPath, nFolderPathLength - strlen("invetory.exe") - 1);
	strncpy_s(szFullXmlFilePath + strlen(szFullXmlFilePath), BUFF_SIZE - strlen(szFullXmlFilePath), "path.xml", strlen("path.xml"));

	tinyxml2::XMLDocument pXmlDoc;
	tinyxml2::XMLError xmlErr = pXmlDoc.LoadFile(szFullXmlFilePath);
	if (xmlErr != tinyxml2::XML_SUCCESS)
	{
		WriteLog("Error >> path.xml not opened\n");
		return -1;
	}

	tinyxml2::XMLElement *pXmlElement = pXmlDoc.FirstChildElement("path")->FirstChildElement("Region");
	if (pXmlElement == nullptr)
	{
		WriteLog("Error >> FirstChild not opened\n");
		return -1;
	}

	while(pXmlElement != nullptr)
	{
		char *szFolder = nullptr;
		char *szId = nullptr;

		szId = (char *)malloc(BIG_BUFF_SIZE);
		if (szId == nullptr)
		{
			WriteLog("\t===> Error\n\tNot enough memory for szId");
			return -1;
		}
		strset(szId, 0);
		szFolder = (char *)malloc(BIG_BUFF_SIZE);
		if (szFolder == nullptr)
		{
			WriteLog("\t===> Error\n\tNot enough memory for szFolder");
			return -1;
		}
		strset(szFolder, 0);

		strcpy_s(szFolder, BIG_BUFF_SIZE, pXmlElement->Attribute("folder_path"));
		strcpy_s(szId, BIG_BUFF_SIZE, pXmlElement->Attribute("id"));
		mapPathId[szFolder] = szId;
		
		pXmlElement = pXmlElement->NextSiblingElement("Region");
	}

	return 0;
}

// Searching file in folder
int SearchFileinFolder(char *szFolder, char *szRegID)
{
#if LOG_ON
	WriteLog("\t===>LOG\n\t\t---===SearchFileinFolder===---\n");
#endif
	char szMask[BIG_BUFF_SIZE] = { 0 };
	char szFilePath[BIG_BUFF_SIZE] = { 0 };
	WIN32_FIND_DATA findData;
	sprintf_s(szMask, BIG_BUFF_SIZE, "%s*",szFolder);

	HANDLE hFind = FindFirstFile(szMask, &findData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		
		char szErr[BIG_BUFF_SIZE] = { 0 };
		DWORD dwErr = GetLastError();
		GetLastErrorMessage(dwErr, szErr);
		WriteLog(szErr);
		return -1;
	}

	do 
	{
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			continue;
		}
		strset(szFilePath, 0);
		sprintf_s(szFilePath, "%s%s", szFolder, findData.cFileName);

		ParseFile(szFilePath, szRegID);
	} while (FindNextFile(hFind, &findData) != FALSE);
	
	return 0;
}

// Reading data from file
int ParseFile(char *szFilePath, char *szRegID)
{
#if LOG_ON
	WriteLog("\t===>LOG\n\t\t---===ParseFile===---\n");
#endif
	int nTotalFiledComplete = 0;
	string sStringFromFile;
	ifstream fileToParse(szFilePath);

	struct pc_info *pPcInfo = nullptr;
	pPcInfo = (pc_info*)malloc(sizeof(pc_info));
	if (pPcInfo == nullptr)
	{
		WriteLog("\t===> Error\n\tNot enough memory for pc_info\n");
		return -1;
	}

	memset(pPcInfo, 0, sizeof(pc_info));
	while (getline(fileToParse, sStringFromFile))
	{
		size_t pos;
		
		if (nTotalFiledComplete == 10)
		{
			strcpy_s(pPcInfo->RegionID, BUFF_SIZE, szRegID);
			break;
		}

		pos = sStringFromFile.find("MAC_Addr=");
		if (pos == 0)
		{
			strcpy_s(pPcInfo->MAC_Addr, BUFF_SIZE, sStringFromFile.substr(pos + strlen("MAC_Addr="), sStringFromFile.length() - strlen("MAC_Addr=")).c_str());
			nTotalFiledComplete++;
			continue;
		}

		pos = sStringFromFile.find("System=");
		if (pos == 0)
		{
			strcpy_s(pPcInfo->System, BUFF_SIZE, sStringFromFile.substr(pos + strlen("System="), sStringFromFile.length() - strlen("System=")).c_str());
			nTotalFiledComplete++;
			continue;
		}

		pos = sStringFromFile.find("Computer_Name=");
		if (pos == 0)
		{
			strcpy_s(pPcInfo->Computer_Name, BUFF_SIZE, sStringFromFile.substr(pos + strlen("Computer_Name="), sStringFromFile.length() - strlen("Computer_Name=")).c_str());
			nTotalFiledComplete++;
			continue;
		}

		pos = sStringFromFile.find("IP_Addr=");
		if (pos == 0)
		{
			strcpy_s(pPcInfo->IP_Addr, BUFF_SIZE, sStringFromFile.substr(pos + strlen("IP_Addr="), sStringFromFile.length() - strlen("IP_Addr=")).c_str());
			nTotalFiledComplete++;
			continue;
		}

		pos = sStringFromFile.find("Current_User_Name=");
		if (pos == 0)
		{
			strcpy_s(pPcInfo->Current_User_Name, BUFF_SIZE, sStringFromFile.substr(pos + strlen("Current_User_Name="), sStringFromFile.length() - strlen("Current_User_Name=")).c_str());
			nTotalFiledComplete++;
			continue;
		}

		pos = sStringFromFile.find("CPU=");
		if (pos == 0)
		{
			strcpy_s(pPcInfo->CPU, BUFF_SIZE, sStringFromFile.substr(pos + strlen("CPU="), sStringFromFile.length() - strlen("CPU=")).c_str());
			nTotalFiledComplete++;
			continue;
		}

		
		pos = sStringFromFile.find("CPU_Freq_in_MHz=");
		if (pos == 0)
		{
			strcpy_s(pPcInfo->CPU_Freq_in_MHz, BUFF_SIZE, sStringFromFile.substr(pos + strlen("CPU_Freq_in_MHz="), sStringFromFile.length() - strlen("CPU_Freq_in_MHz=")).c_str());
			nTotalFiledComplete++;
			continue;
		}

		pos = sStringFromFile.find("Memory_in_Mb=");
		if (pos == 0)
		{
			strcpy_s(pPcInfo->Memory_in_Mb, BUFF_SIZE, sStringFromFile.substr(pos + strlen("Memory_in_Mb="), sStringFromFile.length() - strlen("Memory_in_Mb=")).c_str());
			nTotalFiledComplete++;
			continue;
		}

		pos = sStringFromFile.find("Total_HDD_in_Mb=");
		if (pos == 0)
		{
			strcpy_s(pPcInfo->Total_HDD_in_Mb, BUFF_SIZE, sStringFromFile.substr(pos + strlen("Total_HDD_in_Mb="), sStringFromFile.length() - strlen("Total_HDD_in_Mb=")).c_str());
			nTotalFiledComplete++;
			continue;
		}

		pos = sStringFromFile.find("Record_Date=");
		if (pos == 0)
		{
			strcpy_s(pPcInfo->Record_Date, BUFF_SIZE, sStringFromFile.substr(pos + strlen("Record_Date="), sStringFromFile.length() - strlen("Record_Date=")).c_str());
			nTotalFiledComplete++;
			continue;
		}
	}
	
	if (nTotalFiledComplete == 10)
	{
		strset(szLog, 0);
		strset(szQuery, 0);
		sprintf_s(szQuery, BIG_BUFF_SIZE, "begin tech_conf_parse_test ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s');end;", pPcInfo->MAC_Addr, pPcInfo->System, pPcInfo->Computer_Name, pPcInfo->IP_Addr, 
			pPcInfo->Current_User_Name, pPcInfo->CPU, pPcInfo->CPU_Freq_in_MHz, pPcInfo->Memory_in_Mb, pPcInfo->Total_HDD_in_Mb, pPcInfo->Record_Date, pPcInfo->RegionID);

		if (pConn != nullptr)
		{
			try
			{				
				pStmt->executeQuery(szQuery);					
			}
			catch (SQLException e)
			{
				char szError[BIG_BUFF_SIZE] = { 0 };
				sprintf_s(szError, BIG_BUFF_SIZE, "code: %d\t%s", e.getErrorCode(), e.getMessage());
				WriteLog(szError);
			}
		}
		sprintf_s(szLog, BIG_BUFF_SIZE, "Parsing file : %s\n\tQuery string : %s\n", szFilePath, szQuery);
	}
	else
	{
		strset(szLog, 0);
		sprintf_s(szLog, BIG_BUFF_SIZE, "Parsing file : %s\n\tNo data from file select\n", szFilePath);
	}
	WriteLog(szLog);

	free(pPcInfo);
	return 0;
}


/************************************************************************/
/*						Working with DB                                 */
/************************************************************************/

int CreateSession()
{
#if LOG_ON
	WriteLog("\t===>LOG\n\t\t---===CreateSession===---\n");
#endif
	try
	{
		pEnv = Environment::createEnvironment("CL8MSWIN1251", "CL8MSWIN1251", Environment::DEFAULT );
		pConn = pEnv->createConnection(DB_USER_NAME, DB_PASSWORD, DB_CONNECT_STRING);
		pStmt = pConn->createStatement();
	}
	catch (SQLException e)
	{
		char szError[BIG_BUFF_SIZE] = { 0 };
		sprintf_s(szError, BIG_BUFF_SIZE, "code: %d\t%s", e.getErrorCode(), e.getMessage());
		WriteLog(szError);
	}
	return 0;
}

void FreeMemory()
{
#if LOG_ON
	WriteLog("\t===>LOG\n\t\t---===FreeMemory===---\n");
#endif
	for (it_mapPathId = mapPathId.begin(); it_mapPathId != mapPathId.end(); ++it_mapPathId)
	{
		if (it_mapPathId->first != nullptr)
		{
			free(it_mapPathId->first);
		}
		if (it_mapPathId->second)
		{
			free(it_mapPathId->second);
		}
	}
}

int
APIENTRY
WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	int nRet = 0;
	WriteLog("----====BEGIN====----\n");
	nRet = CreateSession();
	if(nRet < 0)
	{
		WriteLog("----====END====----\n\0");
		return nRet;
	}
	
	nRet = ReadPath();
	if (nRet < 0)
	{
		if (pStmt != nullptr)
		{
			pConn->terminateStatement(pStmt);
		}
		if (pConn != nullptr)
		{
			pEnv->terminateConnection(pConn);
		}
		if (pEnv != nullptr)
		{
			Environment::terminateEnvironment(pEnv);
		}
		FreeMemory();
		CloseHandle(hLogFile);
		return nRet;
	}

	for (it_mapPathId = mapPathId.begin(); it_mapPathId != mapPathId.end(); ++it_mapPathId)
	{
		nRet = SearchFileinFolder(it_mapPathId->first, it_mapPathId->second);
		if (nRet < 0)
		{			
			continue;
		}
		
	}
	FreeMemory();
	WriteLog("----====END====----\n\0");

	if (pStmt != nullptr)
	{
		pConn->terminateStatement(pStmt);
	}
	if (pConn != nullptr)
	{
		pEnv->terminateConnection(pConn);
	}
	if (pEnv != nullptr)
	{
		Environment::terminateEnvironment(pEnv);
	}
	CloseHandle(hLogFile);
	return 0;
}