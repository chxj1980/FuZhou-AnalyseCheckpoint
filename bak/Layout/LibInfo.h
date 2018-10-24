#ifndef libinfo_h

#include <map>
#include <string>
#include <iostream>
#include <math.h>
#include "DataDefine.h"
#include "FeatureManage.h"
#include "ZeromqManage.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include <mysql/mysql.h>
using namespace std;



class CLibInfo
{
public:
    CLibInfo();
    ~CLibInfo();
public:
    bool Start(LPDEVICEINFO pDeviceInfo, LPSINGLEFEATURENODE pSingleFeatureNode);
    bool Stop();
    unsigned int GetTotalNum(){return m_pFeatureManage->GetTotalNum();}
private:
    bool Init();
    //数据库MySQL操作
    //连接DB
    bool ConnectDB();
    //重连DB
    bool ReConnectDB();

    //初始化特征值管理
    bool InitFeatureManage();
    //获取重点库特征值信息
    bool GetKeyLibFeatureFromDB();

    //初始化Zeromq服务
    bool InitZeromq();
    //向重点库增加特征值写入DB
    bool InsertFeatureToDB(LPKEYLIBFEATUREDATA pFeatureData);
    //从DB删除特征值
    bool DelFeatureFromDB(const char * pFaceUUID);
    //清空特征值库
    bool ClearKeyLibFromDB(bool bDel = false);
private:
    //zeromq订阅消息回调
    static void ZeromqSubMsg(LPSUBMESSAGE pSubMessage, void * pUser);
    //zeromq定时发布布控库信息给布控服务
    static void * PubLibInfoThread(void * pParam);
    void PubLibInfoAction();
    //解析Zeromq json
    bool ParseZeromqJson(LPSUBMESSAGE pSubMessage);
    //发送回应消息
    void SendResponseMsg(LPSUBMESSAGE pSubMessage, int nEvent);
    //获取错误码说明
    int GetErrorMsg(ErrorCode nError, char * pMsg);
private:
    bool m_bStopLib;
    CFeatureManage * m_pFeatureManage;    //特征值管理信息
    LPSINGLEFEATURENODE m_pSingleFeatureNode; //特征值结点信息
    LPDEVICEINFO m_pDeviceInfo;
    MYSQL m_mysql;                      //连接MySQL数据库
    CZeromqManage * m_pZeromqManage;    //Zeromq管理
    pthread_mutex_t m_cs;
    pthread_t m_hThreadPubLibInfo;          //发布布控库信息


    char m_pServerIP[32];   //本地IP
    int m_nSubPort;         //本地订阅端口
    int m_nPubPort;         //本地发布端口

    int m_nPipe[2];
};

#define libinfo_h
#endif