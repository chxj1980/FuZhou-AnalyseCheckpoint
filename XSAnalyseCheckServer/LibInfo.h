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
    //zeromq������Ϣ�ص�
    void ZeromqSubMsg(LPSUBMESSAGE pSubMessage);
private:
    bool Init();
    //��ʼ������ֵ����
    bool InitFeatureManage();
    //��ȡ��������ֵ��Ϣ
    bool GetCheckpointFeatureFromDB();
    //��ȡ�ص������ֵ��Ϣ
    bool GetKeyLibFeatureFromDB();

    //��ʼ��Zeromq����
    bool InitZeromq();
    //��DBɾ������ֵ
    bool DelFeatureFromDB(const char * pFaceUUID);
    //�������ֵ��
    bool ClearKeyLibFromDB();
private:
    //�����������ص�
    static void SearchTaskCallback(LPSEARCHTASK pSearchTask, void * pUser);
#ifdef __WINDOWS__
    static DWORD WINAPI PubMessageCommandThread(void * pParam);
#else
    static void * PubMessageCommandThread(void * pParam);
#endif
    void PubMessageCommandAction();
private:
    //����Zeromq json
    bool ParseZeromqJson(LPSUBMESSAGE pSubMessage);
    //���ͻ�Ӧ��Ϣ
    void SendResponseMsg(LPSUBMESSAGE pSubMessage, int nEvent, LPSEARCHTASK pSearchTask = NULL, LPFACEURLINFO pFaceURLInfo = NULL);
    //���������д��Redis
    int InsertSearchResultToRedis(LPSEARCHTASK pSearchTask);
    //��ȡ������˵��
    int GetErrorMsg(ErrorCode nError, char * pMsg);
    string ChangeSecondToTime(unsigned long long nSecond);
    void EnterMutex();
    void LeaveMutex();
public:
    LPDEVICEINFO m_pDeviceInfo;
    CFeatureManage * m_pFeatureManage;    //������Ϣ
private:
    CMySQLOperation * m_pMysqlOperation;    //����MySQL���ݿ�
    CZeromqManage * m_pZeromqManage;    //Zeromq����
    CRedisManage * m_pRedisManage;      //Redis����
    MAPSEARCHTASK m_mapBusinessTask;  //ҵ��ͳ������list, ���ڷ��ͽ���
    bool m_bStopLib;
#ifdef __WINDOWS__
    CRITICAL_SECTION m_cs;
    HANDLE m_hSubMsgEvent;
    HANDLE m_hThreadPubMessageCommand;          //����״̬�߳̾��
#else
    pthread_mutex_t m_mutex;            //�ٽ���
    int m_nPipe[2];
    pthread_t m_hThreadPubMessageCommand;       //����״̬�߳̾��
#endif
    list<LPSUBMESSAGE> m_listPubMessage;
};

#define libinfo_h
#endif