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
    int m_nMaxCount;    //ÿ50w���ݶ��ⴴ��һ�������߳�
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

    int m_nLoadLimitTime;   //���ڷ��������ȡͼƬ����(��)
    int m_nServerTime;      //�Ƿ����ñ��ط������յ�ͼƬʱ��ΪͼƬ����ʱ��, Ĭ��0: ��, 1: ��
    int m_nLoadURL;         //�Ƿ��ȡͼƬURL��Ϣ(֮ǰ�汾û������ֶ�), 0: ��, Ĭ��1: ��
};

#define configread_h
#endif