#ifndef analyseserver_h

#include <map>
#include <string>
#include "ConfigRead.h"
#include "DataDefine.h"
#include "LibInfo.h"
#include "ZeromqManage.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include <mysql/mysql.h>
#include "SearchThread.h"
using namespace std;

class CAnalyseServer
{
public:
    CAnalyseServer();
    ~CAnalyseServer();
public:
    bool StartAnalyseServer();
    bool StopAnalyseServer();
private:
    bool Init();
    //连接DB
    bool ConnectDB();    
    //获取当前分析服务重点库信息
    bool GetKeyLibInfo();
    //初始化Zeromq服务
    bool InitZeromq();
    //初始经比对线程(cpu线程数)
    bool InitSearchThread();
    //保存卡口信息到DB, bAdd: true: 增加, false: 删除
    bool AddCheckpointToDB(char * pDeviceID, bool bAdd = true);
private:
    //zeromq订阅消息回调
    static void ZeromqSubMsg(LPSUBMESSAGE pSubMessage, void * pUser);
private:
    //解析Zeromq json
    bool ParseZeromqJson(LPSUBMESSAGE pSubMessage);
    //发送回应消息
    void SendResponseMsg(LPSUBMESSAGE pSubMessage, int nEvent);
    //获取错误码说明
    int GetErrorMsg(ErrorCode nError, char * pMsg);
private:
    CConfigRead m_ConfigRead;   //配置文件读取
    int m_nServerType;          //服务类型
    MYSQL m_mysql;                      //连接MySQL数据库
    CZeromqManage * m_pZeromqManage;    //Zeromq管理
    pthread_mutex_t m_cs;
    map<string, CLibInfo *> m_mapLibInfo;
    LPALLFEATURENODEINFO m_pAllFeatureNodeInfo;

    list<CSearchThread * > m_listSearchThread;
};

#define analyseserver_h
#endif