#ifndef featuresearchthread_h

#include "DataDefine.h"
#include "FaceFeatureDB.h"
#include "FaceFeatureStruct.h"

#ifdef __WINDOWS__
#include "frsengineV.h"
#pragma comment( lib, "frsengineV.lib" )
using namespace NeoFaceV;
#else
#include <stdio.h>
#include <unistd.h>
#include "pthread.h"
#include "st_feature_comp.h"
#endif

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
    //�����ص������ֵ
    int AddFeature(LPKEYLIBFEATUREDATA pFeatureData);
    //ɾ������ֵ
    int DelFeature(const char * pFaceUUID);
    //����ص��
    int ClearKeyLib();
    //������������
    void AddSearchTask(LPSEARCHTASK pSearchTask);
private:
#ifdef __WINDOWS__
    static DWORD WINAPI SearchThread(void * pParam);
#else
    static void * SearchThread(void * pParam);
#endif
    void SearchAction();
    int VerifyFeature(unsigned char * pf1, unsigned char * pf2, int nLen, float & dbScore);
    string ChangeSecondToTime(unsigned long long nSecond);

    //�ж�nTime�Ƿ�����ҹʱ��, ���򷵻�true, ���򷵻�false, ���޸�nTimeΪ��һ����ҹ���ʱ��
    bool CheckNightTime(int nNightStartTime, int nNightEndTime, int64_t &nTime);

    void EnterMutex();
    void LeaveMutex();
private:
    int m_nServerType;      //������, 1: ���ڿ�, 2: �˲��
    LPThreadSearchCallback m_pResultCallback;   //��������ص�����
    void * m_pUser;
    bool m_bStopTask;                       //�����߳���ֹ��־λ

    //���ڿ�ʹ��
    FaceFeatureDB * m_pFaceFeaturedb;       //���ڿ�����ֵ���ݱ���
    int64_t m_nBeginTime;                   //��ǰ�߳�������������ֵ�Ŀ�ʼʱ��
    int64_t m_nEndTime;                     //��ǰ�߳�������������ֵ�Ľ���ʱ��
    //�˲��ʹ��
    LPKEYLIBINFO m_pKeyLibFeatureInfo;      //�ص�Ᵽ������ֵ����

    LISTSEARCHTASK m_listTask;              //��������list
    LPTHREADSEARCHRESULT m_pSearchResult;   //�����������

#ifdef __WINDOWS__
    CRITICAL_SECTION m_cs; 
    HANDLE m_hSearchEvent;                  //��������ʱ�¼�֪ͨ
    HANDLE m_hThreadID;                     //�����߳̾��
    CVerify m_FRSVerify;
#else
    pthread_mutex_t m_mutex;
    int m_nPipe[2];
    pthread_t m_hThreadID;
#endif
};


#define  featuresearchthread_h
#endif


