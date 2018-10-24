#ifndef redismanage_h

#ifdef __WINDOWS__
#include "acl_cpp/lib_acl.hpp"
#include "acl/lib_acl.h"

#ifdef _DEBUG
#pragma comment( lib, "lib_acl_cpp_vc2010_x64d.lib" )
#pragma comment( lib, "lib_acl_vc2010_x64d.lib" )
#else
#pragma comment( lib, "lib_acl_cpp_vc2010_x64.lib" )
#pragma comment( lib, "lib_acl_vc2010_x64.lib" )
#endif

#else
#include <hiredis/hiredis.h> 
#include <map>
#include <string>
#include <string.h>
#include "DataDefine.h"
using namespace std;
#endif

class CRedisManage
{
public:
    CRedisManage();
    ~CRedisManage();
public:
    bool InitRedis(const char * pRedisIP, int nRedisPort);
    void UnInit();
    bool ZAddSortedSet(const char * pKey, double nScore[], const char *pMember[], int nSize);
    bool InsertHashValue(const char * pKey, const char * pName, const char * pValue, int nValueLen);
#ifdef __WINDOWS__
    bool InsertHashValue(const char * pKey, std::map<acl::string, acl::string> mapValues);
#else
    bool InsertHashValue(const char * pKey, map<string, string> mapValues);
    bool ReconnectRedis();
    bool ExecuteCommand(const char * pCommand);
#endif
    void SetExpireTime(const char * pKey, int nSecond);
private:
#ifdef __WINDOWS__
    char m_pRedisAddr[32];          //Redis地址
    acl::redis_client * m_pClient;
    acl::redis * m_pRedisCmd;       //Redis命令处理对象
    CRITICAL_SECTION m_cs;
#else
    char m_pRedisIP[32];          
    int m_nRedisPort;
    redisContext * m_pRedisContext;    
    pthread_mutex_t m_mutex;          
#endif
};

#define redismanage_h
#endif


