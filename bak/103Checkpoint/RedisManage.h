#ifndef redismanage_h

#include <hiredis/hiredis.h> 
#include "DataDefine.h"
#include <map>
#include <string>
#include <string.h>
using namespace std;
class CRedisManage
{
public:
    CRedisManage();
    ~CRedisManage();
public:
    bool InitRedis(const char * pRedisIP, int nRedisPort);
    void UnInit();
    bool ReconnectRedis();
    bool ZAddSortedSet(const char * pKey, double nScore[], const char *pMember[], int nSize);
    bool InsertHashValue(const char * pKey, const char * pName, const char * pValue, int nValueLen);
    bool InsertHashValue(const char * pKey, map<string, string> mapValues);
    void SetExpireTime(const char * pKey, int nSecond);
private:
    bool ExecuteCommand(const char * pCommand);
private:
    char m_pRedisIP[32];          //Redis��ַ
    int m_nRedisPort;
    redisContext * m_pRedisContext;    //Redis��������
    pthread_mutex_t m_mutex;            //�ٽ���
};

#define redismanage_h
#endif


