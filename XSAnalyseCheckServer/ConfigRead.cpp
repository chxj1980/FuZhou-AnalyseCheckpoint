#include "stdafx.h"
#include "ConfigRead.h"


CConfigRead::CConfigRead(void)
{
    m_sDBIP = "";
    m_nDBPort = 0;
    m_sDBName = "";
    m_sDBUser = "";
    m_sDBPd = "";
    m_nLoadLimitTime = 60;
    m_nServerTime = 0;
    m_nLoadURL = 1;
}
#ifdef __WINDOWS__
string CConfigRead::GetCurrentPath()
{
    DWORD nBufferLenth = MAX_PATH;
    char szBuffer[MAX_PATH] = { 0 };
    DWORD dwRet = GetModuleFileNameA(NULL, szBuffer, nBufferLenth);
    char *sPath = strrchr(szBuffer, '\\');
    memset(sPath, 0, strlen(sPath));
    m_sCurrentPath = szBuffer;
    return m_sCurrentPath;
}
#endif
CConfigRead::~CConfigRead(void)
{
}
bool CConfigRead::ReadConfig(int nServerType)
{
#ifdef __WINDOWS__
    GetCurrentPath();
    m_sConfigFile = m_sCurrentPath + "/Config/config.txt";
#ifdef _DEBUG
    m_sConfigFile = "./Config/config.txt";
#endif
#else
    m_sConfigFile = "config.txt";
#endif
    fstream cfgFile;
    cfgFile.open(m_sConfigFile.c_str()); //打开文件      
    if(!cfgFile.is_open())  
    {  
        printf("can not open file[%s]!\n",  m_sConfigFile.c_str()); 
        return false;  
    }  

    char tmp[1000];  
    while(!cfgFile.eof())//循环读取每一行  
    {  
        cfgFile.getline(tmp, 1000);//每行读取前1000个字符，1000个应该足够了  
        string sLine(tmp);  
        size_t pos = sLine.find('=');//找到每行的“=”号位置，之前是key之后是value  
        if(pos == string::npos) 
        {
            printf("****Error: Config File Format Wrong: [%s]!\n", sLine.c_str());
            return false;  
        }
        string tmpKey = sLine.substr(0, pos); //取=号之前  
        if(sLine[sLine.size() - 1] == 13)
        {
            sLine.erase(sLine.size() -1, 1);
        }
        if ("CheckpointServerID" == tmpKey)
        {
            if (1 == nServerType)
            {
                m_sServerID.assign(sLine, pos + 1, sLine.size() - 1 - pos);
            }
        }
        else if ("CheckServerID" == tmpKey)
        {
            if (2 == nServerType)
            {
                m_sServerID.assign(sLine, pos + 1, sLine.size() - 1 - pos);
            }
        }
        else if ("ThreadMaxCount" == tmpKey)
        {
            string sCount = sLine.substr(pos + 1);//取=号之后  
            m_nMaxCount = atoi(sCount.c_str());
        }
        else if("DBIP" == tmpKey)  
        {  
            m_sDBIP.assign(sLine, pos + 1, sLine.size() - 1 - pos);
        }  
        else if("DBPort" == tmpKey)  
        {  
            string sDBPort = sLine.substr(pos + 1);//取=号之后  
            m_nDBPort = atoi(sDBPort.c_str());
        } 
        else if("DBCheckName" == tmpKey)  
        {  
            m_sDBName.assign(sLine, pos + 1, sLine.size() - 1 - pos);
        }  
        else if("DBUser" == tmpKey)  
        {  
            m_sDBUser.assign(sLine, pos + 1, sLine.size() - 1 - pos);
        } 
        else if("DBPd" == tmpKey)  
        {  
            m_sDBPd.assign(sLine, pos + 1, sLine.size() - 1 - pos);
        } 
        else if ("PubServerIP" == tmpKey)
        {
            m_sPubServerIP.assign(sLine, pos + 1, sLine.size() - 1 - pos);
        }
        else if ("PubServerPort" == tmpKey)
        {
            string sPort = sLine.substr(pos + 1);//取=号之后  
            m_nPubServerPort = atoi(sPort.c_str());
        }
        else if ("SubServerPort" == tmpKey)
        {
            string sPort = sLine.substr(pos + 1);//取=号之后  
            m_nSubServerPort = atoi(sPort.c_str());
        }
        else if ("RedisIP" == tmpKey)
        {
            m_sRedisIP.assign(sLine, pos + 1, sLine.size() - 1 - pos);
        }
        else if ("RedisPort" == tmpKey)
        {
            string sPort = sLine.substr(pos + 1);//取=号之后  
            m_nRedisPort = atoi(sPort.c_str());
        }
        else if ("LoadLimitTime" == tmpKey)
        {
            string sTime = sLine.substr(pos + 1);//取=号之后  
            m_nLoadLimitTime = atoi(sTime.c_str());
        }
        else if ("ServerTime" == tmpKey)
        {
            string sTime = sLine.substr(pos + 1);//取=号之后  
            m_nServerTime = atoi(sTime.c_str());
        }
        else if ("LoadURL" == tmpKey)
        {
            string sURL = sLine.substr(pos + 1);//取=号之后  
            m_nLoadURL = atoi(sURL.c_str());
            break;
        }
    }  

    m_nLoadLimitTime = m_nLoadLimitTime == 0 ? 45 : m_nLoadLimitTime;
    printf("DBInfo: %s:%d:%s\n", m_sDBIP.c_str(), m_nDBPort, m_sDBName.c_str());
    if("" == m_sDBIP || 0 == m_nDBPort || "" == m_sDBName || "" == m_sDBUser || "" == m_sDBPd)
    {
        printf("****Error: Get DB Info Failed!\n");
        return false;
    }

    return true;
}
