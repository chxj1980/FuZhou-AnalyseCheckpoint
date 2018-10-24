#ifndef analyseserver_h

#include <map>
#include <string>
#include "ConfigRead.h"
#include "DataDefine.h"
#include "LibInfo.h"
#include "ZeromqManage.h"
#include "MySQLOperation.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include <mysql/mysql.h>
#ifndef __WINDOWS__
#include <unistd.h>
#endif
using namespace std;

class CAnalyseServer
{
public:
    CAnalyseServer();
    ~CAnalyseServer();
public:
    bool StartAnalyseServer(int nServerType = 1);
    bool StopAnalyseServer();
private:
    bool Init();
    //初始化卡口库或核查库
    bool InitLib();
    //初始化Zeromq服务
    bool InitZeromq();
    //初始化Redis
    bool InitRedis();
    //保存卡口信息到DB, bAdd: true: 增加, false: 删除
    bool AddCheckpointToDB(char * pDeviceID, bool bAdd = true);
private:
    //zeromq订阅消息回调
    static void ZeromqSubMsg(LPSUBMESSAGE pSubMessage, void * pUser);
    //zeromq定时发布状态信息
#ifdef __WINDOWS__
    static DWORD WINAPI PubStatusThread(void * pParam);
#else
    static void * PubStatusThread(void * pParam);
#endif
    void PubStatusAction();
private:
    //解析Zeromq json
    bool ParseZeromqJson(LPSUBMESSAGE pSubMessage);
    //发送回应消息
    void SendResponseMsg(LPSUBMESSAGE pSubMessage, int nEvent);
    //获取错误码说明
    int GetErrorMsg(ErrorCode nError, char * pMsg);
private:
    CConfigRead m_ConfigRead;   //配置文件读取
    int m_nServerType;          //1: 卡口分析服务, 2: 重点库分析服务
    CMySQLOperation * m_pMysqlOperation;    //DB连接
    CZeromqManage * m_pZeromqManage;    //Zeromq管理
    CRedisManage * m_pRedisManage;      //Redis管理
#ifdef __WINDOWS__
    CRITICAL_SECTION m_cs;
    HANDLE m_hStopEvent;
    HANDLE m_hThreadPubStatus;          //发布状态线程句柄
#else
    pthread_mutex_t m_mutex;        //临界区
    int m_nPipe[2];
    pthread_t m_hThreadPubStatus;       //发布状态线程句柄
#endif
    map<string, CLibInfo *> m_mapLibInfo;
};

#define analyseserver_h
#endif