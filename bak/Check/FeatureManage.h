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
    //��������ֵ, bInit: �Ƿ��ڳ�ʼ��ʱ��������ֵ����, ������Ҫ�ٷ���copy���������ֵ��pFeature
    //���ڿ���������ֵ
    int AddFeature(FeatureData * pFeatureData, bool bInit = false);
    //�ص����������ֵ
    int AddFeature(LPKEYLIBFEATUREDATA pFeatureData, bool bInit = false);
    //ɾ������ֵ
    int DelFeature(const char * pFaceUUID);
    //����ص��
    int ClearKeyLib();
    int GetCaptureImageInfo(LPSEARCHTASK pSearchTask);
    void AddSearchTask(LPSEARCHTASK pSearchTask);
    //��ȡ����ֵ�������
    unsigned int GetTotalNum();
    //��ȡ����ֵ��������
    unsigned int GetDayNum();
    bool GetDataDate(unsigned long long &begintime, unsigned long long &endtime);
private:
    //�����ȶ��߳�
    static void * LibTaskThread(void * pParam);
    void LibTaskAction();

    //�����ȴ��߳�
    static void * LibSearchWaitThread(void * pParam);
    void LibSearchWaitAction();

    //�����ȴ��߳�
    static void * LibBusinessThread(void * pParam);
    void LibBusinessAction();

    //�����߳̽���ص�
    static void ThreadSearchCallback(LPSEARCHTASK pSearchTask, LPTHREADSEARCHRESULT pThreadSearchResult, void * pUser);
    string ChangeSecondToTime(unsigned long long nSecond);

    //�ж�nTime�Ƿ�����ҹʱ��, ���򷵻�true, ���򷵻�false, ���޸�nTimeΪ��һ����ҹ���ʱ��
    bool CheckNightTime(int nNightStartTime, int nNightEndTime, int64_t &nTime);
public:
    map<int64_t, int> m_mapDayCount;    //ÿ�������ֵ����ͳ��
private:
    bool m_bStopTask;
    char m_pLibName[MAXLEN];
    int m_nServerType;
    int m_nMaxCount;
    void * m_pUser;
    LPSearchTaskCallback m_pTaskCallback;

    //���ڿ�, ͬһ������������ֵ���ȫ����һ��, �˲��ֿ����ڲ�ͬ���߳���
    FaceFeatureDB * m_pFaceFeaturedb;   //���ڿ�����ֵ���ݱ���
    pthread_t m_ThreadIDSearch;            //�����ȶ��߳̾��
    pthread_t m_ThreadIDWait;              //��������ȴ��߳̾��
    pthread_t m_ThreadIDBusiness;          //Ƶ�������ͳ���߳̾��

    pthread_mutex_t m_csTask; 
    pthread_mutex_t m_csWait; 
    pthread_mutex_t m_csBusiness; 

    LISTSEARCHTASK m_listTask;          //��������list
    LISTSEARCHTASK m_listWait;          //�����ȴ�����list
    LISTSEARCHTASK m_listBusiness;      //Ƶ�������ͳ������list

    int m_nPipeBusiness[2];
    int m_nPipeSearch[2];
    int m_nPipeWait[2];

    LISTFEATURETHREADINFO m_listFeatureSearch;  //����ֵ��, ÿ50w(�������ļ�ָ��)�����ݶ��ⴴ��һ�����������߳�

};

#define FeatureManage_h
#endif