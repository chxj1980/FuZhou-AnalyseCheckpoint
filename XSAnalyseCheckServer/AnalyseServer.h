#ifndef analyseserver_h

#include <map>
#include <string>
#include "ConfigRead.h"
#include "DataDefine.h"
#include "LibInfo.h"
#include "ZeromqManage.h"
#include "MySQLOperation.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include <mysql/mysql.h>
#ifndef __WINDOWS__
#include <unistd.h>
#endif
using namespace std;

class CAnalyseServer
{
public:
    CAnalyseServer();
    ~CAnalyseServer();
public:
    bool StartAnalyseServer(int nServerType = 1);
    bool StopAnalyseServer();
private:
    bool Init();
    //��ʼ�����ڿ��˲��
    bool InitLib();
    //��ʼ��Zeromq����
    bool InitZeromq();
    //��ʼ��Redis
    bool InitRedis();
    //���濨����Ϣ��DB, bAdd: true: ����, false: ɾ��
    bool AddCheckpointToDB(char * pDeviceID, bool bAdd = true);
private:
    //zeromq������Ϣ�ص�
    static void ZeromqSubMsg(LPSUBMESSAGE pSubMessage, void * pUser);
    //zeromq��ʱ����״̬��Ϣ
#ifdef __WINDOWS__
    static DWORD WINAPI PubStatusThread(void * pParam);
#else
    static void * PubStatusThread(void * pParam);
#endif
    void PubStatusAction();
private:
    //����Zeromq json
    bool ParseZeromqJson(LPSUBMESSAGE pSubMessage);
    //���ͻ�Ӧ��Ϣ
    void SendResponseMsg(LPSUBMESSAGE pSubMessage, int nEvent);
    //��ȡ������˵��
    int GetErrorMsg(ErrorCode nError, char * pMsg);
private:
    CConfigRead m_ConfigRead;   //�����ļ���ȡ
    int m_nServerType;          //1: ���ڷ�������, 2: �ص���������
    CMySQLOperation * m_pMysqlOperation;    //DB����
    CZeromqManage * m_pZeromqManage;    //Zeromq����
    CRedisManage * m_pRedisManage;      //Redis����
#ifdef __WINDOWS__
    CRITICAL_SECTION m_cs;
    HANDLE m_hStopEvent;
    HANDLE m_hThreadPubStatus;          //����״̬�߳̾��
#else
    pthread_mutex_t m_mutex;        //�ٽ���
    int m_nPipe[2];
    pthread_t m_hThreadPubStatus;       //����״̬�߳̾��
#endif
    map<string, CLibInfo *> m_mapLibInfo;
};

#define analyseserver_h
#endif