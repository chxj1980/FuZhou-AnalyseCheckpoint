#ifndef FeatureManage_h

#include "DataDefine.h"
#include "FeatureSearchThread.h"
#include "ZBase64.h"
#include <time.h>

typedef struct _FeatureThreadInfo
{
    CFeatureSearchThread * pFeatureSearchThread;
    int64_t nBeginTime;
    int64_t nEndTime;
    unsigned int nFeatureCount;
    _FeatureThreadInfo()
    {
        pFeatureSearchThread = NULL;
        nBeginTime = 0;
        nEndTime = 0;
        nFeatureCount = 0;
    }
    ~_FeatureThreadInfo()
    {
        if (NULL != pFeatureSearchThread)
        {
            delete pFeatureSearchThread;
            pFeatureSearchThread = NULL;
        }
    }
}FEATURETHREADINFO, *LPFEATURETHREADINFO;
typedef list<LPFEATURETHREADINFO> LISTFEATURETHREADINFO;
class CFeatureManage
{
public:
    CFeatureManage(void);
    ~CFeatureManage(void);
public:
    int Init(char * pDeviceID, int nServerType, int nMaxCount, LPSearchTaskCallback pTaskCallback, void * pUser);
    bool UnInit();
    //增加特征值, bInit: 是否在初始化时增加特征值数据, 是则不需要再反向copy解码后特征值到pFeature
    //卡口库增加特征值
    int AddFeature(FeatureData * pFeatureData, bool bInit = false);
    //重点库增加特征值
    int AddFeature(LPKEYLIBFEATUREDATA pFeatureData, bool bInit = false);
    //删除特征值
    int DelFeature(const char * pFaceUUID);
    //清空重点库
    int ClearKeyLib();
    int GetCaptureImageInfo(LPSEARCHTASK pSearchTask);
    void AddSearchTask(LPSEARCHTASK pSearchTask);
    //获取特征值结点总数
    unsigned int GetTotalNum();
    //获取特征值当天结点数
    unsigned int GetDayNum();
    bool GetDataDate(unsigned long long &begintime, unsigned long long &endtime);
private:
    //搜索比对线程
    static void * LibTaskThread(void * pParam);
    void LibTaskAction();

    //搜索等待线程
    static void * LibSearchWaitThread(void * pParam);
    void LibSearchWaitAction();

    //搜索等待线程
    static void * LibBusinessThread(void * pParam);
    void LibBusinessAction();

    //搜索线程结果回调
    static void ThreadSearchCallback(LPSEARCHTASK pSearchTask, LPTHREADSEARCHRESULT pThreadSearchResult, void * pUser);
    string ChangeSecondToTime(unsigned long long nSecond);

    //判断nTime是否处于深夜时间, 是则返回true, 否则返回false, 并修改nTime为下一个深夜结点时间
    bool CheckNightTime(int nNightStartTime, int nNightEndTime, int64_t &nTime);
public:
    map<int64_t, int> m_mapDayCount;    //每天的特征值数量统计
private:
    bool m_bStopTask;
    char m_pLibName[MAXLEN];
    int m_nServerType;
    int m_nMaxCount;
    void * m_pUser;
    LPSearchTaskCallback m_pTaskCallback;

    //卡口库, 同一卡口所有特征值结点全部放一起, 核查库分开放于不同的线程类
    FaceFeatureDB * m_pFaceFeaturedb;   //卡口库特征值数据保存
    pthread_t m_ThreadIDSearch;            //搜索比对线程句柄
    pthread_t m_ThreadIDWait;              //搜索结果等待线程句柄
    pthread_t m_ThreadIDBusiness;          //频繁出入等统计线程句柄

    pthread_mutex_t m_csTask; 
    pthread_mutex_t m_csWait; 
    pthread_mutex_t m_csBusiness; 

    LISTSEARCHTASK m_listTask;          //搜索任务list
    LISTSEARCHTASK m_listWait;          //搜索等待任务list
    LISTSEARCHTASK m_listBusiness;      //频繁出入等统计任务list

    int m_nPipeBusiness[2];
    int m_nPipeSearch[2];
    int m_nPipeWait[2];

    LISTFEATURETHREADINFO m_listFeatureSearch;  //特征值库, 每50w(由配置文件指定)条数据额外创建一条搜索管理线程

};

#define FeatureManage_h
#endif