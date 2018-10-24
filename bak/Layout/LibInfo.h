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
    //���ݿ�MySQL����
    //����DB
    bool ConnectDB();
    //����DB
    bool ReConnectDB();

    //��ʼ������ֵ����
    bool InitFeatureManage();
    //��ȡ�ص������ֵ��Ϣ
    bool GetKeyLibFeatureFromDB();

    //��ʼ��Zeromq����
    bool InitZeromq();
    //���ص����������ֵд��DB
    bool InsertFeatureToDB(LPKEYLIBFEATUREDATA pFeatureData);
    //��DBɾ������ֵ
    bool DelFeatureFromDB(const char * pFaceUUID);
    //�������ֵ��
    bool ClearKeyLibFromDB(bool bDel = false);
private:
    //zeromq������Ϣ�ص�
    static void ZeromqSubMsg(LPSUBMESSAGE pSubMessage, void * pUser);
    //zeromq��ʱ�������ؿ���Ϣ�����ط���
    static void * PubLibInfoThread(void * pParam);
    void PubLibInfoAction();
    //����Zeromq json
    bool ParseZeromqJson(LPSUBMESSAGE pSubMessage);
    //���ͻ�Ӧ��Ϣ
    void SendResponseMsg(LPSUBMESSAGE pSubMessage, int nEvent);
    //��ȡ������˵��
    int GetErrorMsg(ErrorCode nError, char * pMsg);
private:
    bool m_bStopLib;
    CFeatureManage * m_pFeatureManage;    //����ֵ������Ϣ
    LPSINGLEFEATURENODE m_pSingleFeatureNode; //����ֵ�����Ϣ
    LPDEVICEINFO m_pDeviceInfo;
    MYSQL m_mysql;                      //����MySQL���ݿ�
    CZeromqManage * m_pZeromqManage;    //Zeromq����
    pthread_mutex_t m_cs;
    pthread_t m_hThreadPubLibInfo;          //�������ؿ���Ϣ


    char m_pServerIP[32];   //����IP
    int m_nSubPort;         //���ض��Ķ˿�
    int m_nPubPort;         //���ط����˿�

    int m_nPipe[2];
};

#define libinfo_h
#endif