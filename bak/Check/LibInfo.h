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
    //���ݿ�MySQL����
    //����DB
    bool ConnectDB();
    //����DB
    bool ReConnectDB();

    //��ʼ������ֵ����
    bool InitFeatureManage();
    //��ȡ��������ֵ��Ϣ
    bool GetCheckpointFeatureFromDB();
    //��ȡ�ص������ֵ��Ϣ
    bool GetKeyLibFeatureFromDB();

    //��ʼ��Zeromq����
    bool InitZeromq();
    //��ʼ��Redis
    bool InitRedis();
    //�򿨿���������ֵд��DB
    bool InsertFeatureToDB(FeatureData * pFeatureData);
    //���ص����������ֵд��DB
    bool InsertFeatureToDB(LPKEYLIBFEATUREDATA pFeatureData);
    //��DBɾ������ֵ
    bool DelFeatureFromDB(const char * pFaceUUID);
    //�������ֵ��
    bool ClearKeyLibFromDB(bool bDel = false);
private:
    //zeromq������Ϣ�ص�
    static void ZeromqSubMsg(LPSUBMESSAGE pSubMessage, void * pUser);
    //zeromq��ʱ����״̬��Ϣ
    static void * PubStatusThread(void * pParam);
    void PubStatusAction();
    //�����������ص�
    static void SearchTaskCallback(LPSEARCHTASK pSearchTask, void * pUser);
private:
    //����Zeromq json
    bool ParseZeromqJson(LPSUBMESSAGE pSubMessage);
    //���ͻ�Ӧ��Ϣ
    void SendResponseMsg(LPSUBMESSAGE pSubMessage, int nEvent, LPSEARCHTASK pSearchTask = NULL);
    //���������д��Redis
    int InsertSearchResultToRedis(LPSEARCHTASK pSearchTask);
    //��ȡ������˵��
    int GetErrorMsg(ErrorCode nError, char * pMsg);
    string ChangeSecondToTime(unsigned long long nSecond);
private:
    bool m_bStopLib;            
    CFeatureManage * m_pFeatureManage;    //������Ϣ
    LPDEVICEINFO m_pDeviceInfo;
    MYSQL m_mysql;                      //����MySQL���ݿ�
    CZeromqManage * m_pZeromqManage;    //Zeromq����
    CRedisManage * m_pRedisManage;      //Redis����
    pthread_mutex_t m_mutex;            //�ٽ���
    pthread_t m_hThreadPubStatus;       //����״̬�߳̾��

    MAPSEARCHTASK m_mapBusinessTask;  //ҵ��ͳ������list, ���ڷ��ͽ���

    int m_nPipe[2];
};

#define libinfo_h
#endif