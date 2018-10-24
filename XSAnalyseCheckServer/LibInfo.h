#ifndef libinfo_h

#include <map>
#include <string>
#include <iostream>
#include <math.h>
#include "DataDefine.h"
#include "FeatureManage.h"
#include "ZeromqManage.h"
#include "MySQLOperation.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include <mysql/mysql.h>
#include "RedisManage.h"
#include "ZBase64.h"
#ifndef __WINDOWS__
#include <stdlib.h>
#include <unistd.h>
#endif
using namespace std;



class CLibInfo
{
public:
    CLibInfo();
    ~CLibInfo();
public:
    bool Start(LPDEVICEINFO pDeviceInfoMYSQL, CMySQLOperation * pMysqlOperation, CZeromqManage * pZeromqManage, CRedisManage * pRedisManage);
    bool Stop();
    unsigned int GetTotalNum(){return m_pFeatureManage->GetTotalNum();}
    bool GetDataDate(unsigned long long &begintime, unsigned long long &endtime){return m_pFeatureManage->GetDataDate(begintime, endtime);}
    //zeromq订阅消息回调
    void ZeromqSubMsg(LPSUBMESSAGE pSubMessage);
private:
    bool Init();
    //初始化特征值管理
    bool InitFeatureManage();
    //获取卡口特征值信息
    bool GetCheckpointFeatureFromDB();
    //获取重点库特征值信息
    bool GetKeyLibFeatureFromDB();

    //初始化Zeromq服务
    bool InitZeromq();
    //从DB删除特征值
    bool DelFeatureFromDB(const char * pFaceUUID);
    //清空特征值库
    bool ClearKeyLibFromDB();
private:
    //搜索任务结果回调
    static void SearchTaskCallback(LPSEARCHTASK pSearchTask, void * pUser);
#ifdef __WINDOWS__
    static DWORD WINAPI PubMessageCommandThread(void * pParam);
#else
    static void * PubMessageCommandThread(void * pParam);
#endif
    void PubMessageCommandAction();
private:
    //解析Zeromq json
    bool ParseZeromqJson(LPSUBMESSAGE pSubMessage);
    //发送回应消息
    void SendResponseMsg(LPSUBMESSAGE pSubMessage, int nEvent, LPSEARCHTASK pSearchTask = NULL, LPFACEURLINFO pFaceURLInfo = NULL);
    //将搜索结果写入Redis
    int InsertSearchResultToRedis(LPSEARCHTASK pSearchTask);
    //获取错误码说明
    int GetErrorMsg(ErrorCode nError, char * pMsg);
    string ChangeSecondToTime(unsigned long long nSecond);
    void EnterMutex();
    void LeaveMutex();
public:
    LPDEVICEINFO m_pDeviceInfo;
    CFeatureManage * m_pFeatureManage;    //卡口信息
private:
    CMySQLOperation * m_pMysqlOperation;    //连接MySQL数据库
    CZeromqManage * m_pZeromqManage;    //Zeromq管理
    CRedisManage * m_pRedisManage;      //Redis管理
    MAPSEARCHTASK m_mapBusinessTask;  //业务统计任务list, 用于发送进度
    bool m_bStopLib;
#ifdef __WINDOWS__
    CRITICAL_SECTION m_cs;
    HANDLE m_hSubMsgEvent;
    HANDLE m_hThreadPubMessageCommand;          //发布状态线程句柄
#else
    pthread_mutex_t m_mutex;            //临界区
    int m_nPipe[2];
    pthread_t m_hThreadPubMessageCommand;       //发布状态线程句柄
#endif
    list<LPSUBMESSAGE> m_listPubMessage;
};

#define libinfo_h
#endif