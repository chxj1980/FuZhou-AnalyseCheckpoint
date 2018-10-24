#ifndef featuresearchthread_h

#include <stdio.h>
#include "DataDefine.h"
#include "FaceFeatureDB.h"
#include "FaceFeatureStruct.h"
#include "pthread.h"
#include "st_feature_comp.h"

typedef void (*LPThreadSearchCallback)(LPSEARCHTASK pSearchTask, LPTHREADSEARCHRESULT pThreadSearchResult, void * pUser);
class CFeatureSearchThread
{
public:
    CFeatureSearchThread();
    ~CFeatureSearchThread();
public:
    bool Init(int nServerType, LPThreadSearchCallback pCallback, void * pUser, FaceFeatureDB * pFaceFeatureDB = NULL);
    void UnInit();
    void SetCheckpointBeginTime(int64_t nBeginTime);
    void SetCheckpointEndTime(int64_t nEndTime);
    //增加重点库特征值
    int AddFeature(LPKEYLIBFEATUREDATA pFeatureData);
    //删除特征值
    int DelFeature(const char * pFaceUUID);
    //清空重点库
    int ClearKeyLib();
    //增加搜索任务
    void AddSearchTask(LPSEARCHTASK pSearchTask);
private:
    static void * SearchThread(void * pParam);
    void SearchAction();
    int VerifyFeature(unsigned char * pf1, unsigned char * pf2, int nLen);
    string ChangeSecondToTime(unsigned long long nSecond);

    //判断nTime是否处于深夜时间, 是则返回true, 否则返回false, 并修改nTime为下一个深夜结点时间
    bool CheckNightTime(int nNightStartTime, int nNightEndTime, int64_t &nTime);
private:
    int m_nServerType;      //库类型, 1: 卡口库, 2: 核查库
    LPThreadSearchCallback m_pResultCallback;   //搜索结果回调函数
    void * m_pUser;
    bool m_bStopTask;                       //搜索线程终止标志位
    pthread_mutex_t m_mutex; //

    //卡口库使用
    FaceFeatureDB * m_pFaceFeaturedb;       //卡口库特征值数据保存
    int64_t m_nBeginTime;                   //当前线程搜索卡口特征值的开始时间
    int64_t m_nEndTime;                     //当前线程搜索卡口特征值的结束时间
    //核查库使用
    LPKEYLIBINFO m_pKeyLibFeatureInfo;      //重点库保存特征值数据

    pthread_t m_hThreadID;                  //搜索线程句柄
    int m_nPipe[2];                         //管道
    LISTSEARCHTASK m_listTask;              //搜索任务list
    LPTHREADSEARCHRESULT m_pSearchResult;   //搜索结果保存
};


#define  featuresearchthread_h
#endif


