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
    //�����ص������ֵ
    int AddFeature(LPKEYLIBFEATUREDATA pFeatureData);
    //ɾ������ֵ
    int DelFeature(const char * pFaceUUID);
    //����ص��
    int ClearKeyLib();
    //������������
    void AddSearchTask(LPSEARCHTASK pSearchTask);
private:
    static void * SearchThread(void * pParam);
    void SearchAction();
    int VerifyFeature(unsigned char * pf1, unsigned char * pf2, int nLen);
    string ChangeSecondToTime(unsigned long long nSecond);

    //�ж�nTime�Ƿ�����ҹʱ��, ���򷵻�true, ���򷵻�false, ���޸�nTimeΪ��һ����ҹ���ʱ��
    bool CheckNightTime(int nNightStartTime, int nNightEndTime, int64_t &nTime);
private:
    int m_nServerType;      //������, 1: ���ڿ�, 2: �˲��
    LPThreadSearchCallback m_pResultCallback;   //��������ص�����
    void * m_pUser;
    bool m_bStopTask;                       //�����߳���ֹ��־λ
    pthread_mutex_t m_mutex; //

    //���ڿ�ʹ��
    FaceFeatureDB * m_pFaceFeaturedb;       //���ڿ�����ֵ���ݱ���
    int64_t m_nBeginTime;                   //��ǰ�߳�������������ֵ�Ŀ�ʼʱ��
    int64_t m_nEndTime;                     //��ǰ�߳�������������ֵ�Ľ���ʱ��
    //�˲��ʹ��
    LPKEYLIBINFO m_pKeyLibFeatureInfo;      //�ص�Ᵽ������ֵ����

    pthread_t m_hThreadID;                  //�����߳̾��
    int m_nPipe[2];                         //�ܵ�
    LISTSEARCHTASK m_listTask;              //��������list
    LPTHREADSEARCHRESULT m_pSearchResult;   //�����������
};


#define  featuresearchthread_h
#endif


