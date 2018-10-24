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
    //获取当前分析服务关联库信息
    bool GetLibInfo();
    //初始化Zeromq服务
    bool InitZeromq();
    //保存卡口信息到DB, bAdd: true: 增加, false: 删除
    bool AddCheckpointToDB(char * pDeviceID, int nType = 0, bool bAdd = true);
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
    CConfigRead m_ConfigRead;       //配置文件读取
    int m_nServerType;              //1: 卡口分析服务, 2: 重点库分析服务
    MYSQL m_mysql;                  //连接MySQL数据库
    CZeromqManage * m_pZeromqManage;//Zeromq管理
    pthread_mutex_t m_mutex;        //临界区
    map<string, CLibInfo *> m_mapLibInfo;
};

#define analyseserver_h
#endif