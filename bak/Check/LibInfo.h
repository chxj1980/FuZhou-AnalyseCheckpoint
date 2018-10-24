#ifndef libinfo_h

#include <map>
#include <string>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include "DataDefine.h"
#include "FeatureManage.h"
#include "ZeromqManage.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include <mysql/mysql.h>
#include "RedisManage.h"
#include "ZBase64.h"
using namespace std;



class CLibInfo
{
public:
    CLibInfo();
    ~CLibInfo();
public:
    bool Start(LPDEVICEINFO pDeviceInfo);
    bool Stop();
    unsigned int GetTotalNum(){return m_pFeatureManage->GetTotalNum();}
    bool GetDataDate(unsigned long long &begintime, unsigned long long &endtime){return m_pFeatureManage->GetDataDate(begintime, endtime);}
private:
    bool Init();
    //数据库MySQL操作
    //连接DB
    bool ConnectDB();
    //重连DB
    bool ReConnectDB();

    //初始化特征值管理
    bool InitFeatureManage();
    //获取卡口特征值信息
    bool GetCheckpointFeatureFromDB();
    //获取重点库特征值信息
    bool GetKeyLibFeatureFromDB();

    //初始化Zeromq服务
    bool InitZeromq();
    //初始化Redis
    bool InitRedis();
    //向卡口增加特征值写入DB
    bool InsertFeatureToDB(FeatureData * pFeatureData);
    //向重点库增加特征值写入DB
    bool InsertFeatureToDB(LPKEYLIBFEATUREDATA pFeatureData);
    //从DB删除特征值
    bool DelFeatureFromDB(const char * pFaceUUID);
    //清空特征值库
    bool ClearKeyLibFromDB(bool bDel = false);
private:
    //zeromq订阅消息回调
    static void ZeromqSubMsg(LPSUBMESSAGE pSubMessage, void * pUser);
    //zeromq定时发布状态信息
    static void * PubStatusThread(void * pParam);
    void PubStatusAction();
    //搜索任务结果回调
    static void SearchTaskCallback(LPSEARCHTASK pSearchTask, void * pUser);
private:
    //解析Zeromq json
    bool ParseZeromqJson(LPSUBMESSAGE pSubMessage);
    //发送回应消息
    void SendResponseMsg(LPSUBMESSAGE pSubMessage, int nEvent, LPSEARCHTASK pSearchTask = NULL);
    //将搜索结果写入Redis
    int InsertSearchResultToRedis(LPSEARCHTASK pSearchTask);
    //获取错误码说明
    int GetErrorMsg(ErrorCode nError, char * pMsg);
    string ChangeSecondToTime(unsigned long long nSecond);
private:
    bool m_bStopLib;            
    CFeatureManage * m_pFeatureManage;    //卡口信息
    LPDEVICEINFO m_pDeviceInfo;
    MYSQL m_mysql;                      //连接MySQL数据库
    CZeromqManage * m_pZeromqManage;    //Zeromq管理
    CRedisManage * m_pRedisManage;      //Redis管理
    pthread_mutex_t m_mutex;            //临界区
    pthread_t m_hThreadPubStatus;       //发布状态线程句柄

    MAPSEARCHTASK m_mapBusinessTask;  //业务统计任务list, 用于发送进度

    int m_nPipe[2];
};

#define libinfo_h
#endif