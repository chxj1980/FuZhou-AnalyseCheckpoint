#ifndef configread_h

#include <iostream>  
#include <string>  
#include <fstream> 
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
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
    bool ReadConfig();
public:
    string m_sConfigFile;
    string m_sCurrentPath;

    string m_sLocalIP;
    int m_nPubPort;
    int m_nSubPort;
};

#define configread_h
#endif