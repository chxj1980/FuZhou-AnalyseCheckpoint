#include "stdafx.h"
#include "ConfigRead.h"


CConfigRead::CConfigRead(void)
{
    m_nPubPort = 8100;
    m_nSubPort = 8101;
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
bool CConfigRead::ReadConfig()
{
    GetCurrentPath();
    m_sConfigFile = m_sCurrentPath + "/Proxyconfig.txt";
#ifdef _DEBUG
    m_sConfigFile = "./Proxyconfig.txt";
#endif
    fstream cfgFile;
    cfgFile.open(m_sConfigFile.c_str()); //���ļ�      
    if(!cfgFile.is_open())  
    {  
        printf("can not open file[%s]!\n",  m_sConfigFile.c_str()); 
        return false;  
    }  

    char tmp[1000];  
    while(!cfgFile.eof())//ѭ����ȡÿһ��  
    {  
        cfgFile.getline(tmp, 1000);//ÿ�ж�ȡǰ1000���ַ���1000��Ӧ���㹻��  
        string sLine(tmp);  
        size_t pos = sLine.find('=');//�ҵ�ÿ�еġ�=����λ�ã�֮ǰ��key֮����value  
        if(pos == string::npos) 
        {
            printf("****Error: Config File Format Wrong: [%s]!\n", sLine.c_str());
            return false;  
        }
        string tmpKey = sLine.substr(0, pos); //ȡ=��֮ǰ  
        if(sLine[sLine.size() - 1] == 13)
        {
            sLine.erase(sLine.size() -1, 1);
        }
        if ("LocalIP" == tmpKey)
        {
            m_sLocalIP.assign(sLine, pos + 1, sLine.size() - 1 - pos);
        }
        else if ("PubPort" == tmpKey)
        {
            string sPort = sLine.substr(pos + 1);//ȡ=��֮��  
            m_nPubPort = atoi(sPort.c_str());
        }
        else if("SubPort" == tmpKey)  
        {  
            string sPort = sLine.substr(pos + 1);//ȡ=��֮��  
            m_nSubPort = atoi(sPort.c_str());
            break;
        }  
    }  

    return true;
}
