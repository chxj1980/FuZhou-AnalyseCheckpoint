#ifndef configread_h

#include <iostream>  
#include <string>  
#include <fstream> 
#include <stdio.h>
#include <stdlib.h>
using namespace std;

class CConfigRead
{
public:
    CConfigRead(void);
    ~CConfigRead(void);
public:
#ifdef __WINDOWS__
    string GetCurrentPath();
#endif
    bool ReadConfig(int nServerType);
public:
    string m_sConfigFile;
    string m_sCurrentPath;

    string m_sServerID;
    int m_nMaxCount;    //每50w数据额外创建一条搜索线程
    string m_sDBIP;
    int m_nDBPort;
    string m_sDBName;
    string m_sDBUser;
    string m_sDBPd;

    string m_sPubServerIP;
    int m_nPubServerPort;
    int m_nSubServerPort;

    string m_sRedisIP;
    int m_nRedisPort;

    int m_nLoadLimitTime;   //卡口分析服务读取图片限制(天)
    int m_nServerTime;      //是否启用本地服务器收到图片时间为图片保存时间, 默认0: 否, 1: 是
    int m_nLoadURL;         //是否读取图片URL信息(之前版本没有这个字段), 0: 否, 默认1: 是
};

#define configread_h
#endif