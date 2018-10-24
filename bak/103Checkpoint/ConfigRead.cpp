#include "ConfigRead.h"

CConfigRead::CConfigRead(void)
{
    m_sDBIP = "";
    m_nDBPort = 0;
    m_sDBName = "";
    m_sDBUser = "";
    m_sDBPd = "";
}
CConfigRead::~CConfigRead(void)
{
}
bool CConfigRead::ReadConfig()
{
    m_sConfigFile = "config.txt";
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

        if ("ServerID" == tmpKey)
        {
            m_sServerID.assign(sLine, pos + 1, sLine.size() - 1 - pos);
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
        else if("DBName" == tmpKey)  
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
            break;
        }
    }  
    printf("DBInfo: %s:%d:%s\n", m_sDBIP.c_str(), m_nDBPort, m_sDBName.c_str());
    if("" == m_sDBIP || 0 == m_nDBPort || "" == m_sDBName || "" == m_sDBUser || "" == m_sDBPd)
    {
        printf("****Error: Get DB Info Failed!\n");
        return false;
    }

    return true;
}
