#ifndef zeromqmanage_h

#include "DataDefine.h"
#include <list>
#include <zmq.h>
using namespace std;

class CZeromqManage
{
public:
    CZeromqManage();
    ~CZeromqManage();
public:
    void UnInit();
    //������Ϣ
    bool InitPub(char * pLocalIP, int nLocalPort, char * pServerIP, int nServerPort);
    bool PubMessage(LPSUBMESSAGE pSubMessage);
    bool PubMessage(char * pHead, char * pID);
    //������Ϣ
    bool InitSub(char * pLocalIP, int nLocalPort, char * pServerIP, int nServerPort,
                 LPSubMessageCallback pSubMessageCallback, void * pUser, int nThreadNum = THREADNUM);
    bool AddSubMessage(char * pSubMessage);
    bool DelSubMessage(char * pSubMessage);

    //�󶨱���push�˿�
    bool InitPush(char * pLocalIP, int nLocalPushPort, char * pServerIP, int nServerPort);
    bool PushMessage(LPSUBMESSAGE pSubMessage);
    //pull
    bool InitPull(char * pLocalIP, int nLocalPushPort, char * pServerIP, int nServerPort, 
                    LPSubMessageCallback pSubMessageCallback, void * pUser, int nThreadNum = THREADNUM);
private:
    //������Ϣ����
    static void * SubMessageThread(void * pParam);
    void SubMessageAction();
    //pull��Ϣ����
    static void * PullMessageThread(void * pParam);
    void PullMessageAction();

    static void * MessageCallbackThread(void * pParam);
    void MessageCallbackAction();

    LPSUBMESSAGE GetFreeResource();
    void FreeResource(LPSUBMESSAGE pSubMessage);
private:
    LPSubMessageCallback m_pSubMsgCallback; //������Ϣ�ص�
    void * m_pUser;                         //������Ϣ�ص����س�ʼ���������
    bool m_bStopFlag;                       //�߳�ֹͣ��־
    pthread_mutex_t m_mutex;                  //�ٽ���
    int m_nThreadNum;                       //�ص��߳���, ������Դ*20

    void * m_pCtx;
    //����Pub
    void * m_pPubSocket;            //����socket
    //����Sub
    void * m_pSubSocket;            //����socket
    //����Push
    void * m_pPushSocket;            //����socket
    //��Pull
    void * m_pPullSocket;            //��socket

    pthread_t m_ThreadSubMessageID;             //������Ϣ�߳̾��
    pthread_t m_ThreadPullMessageID;            //pull��Ϣ�߳̾��
    pthread_t m_ThreadCallbackID[THREADNUM];    //�ص��߳̾��
    LISTSUBMESSAGE m_listSubMessageResource;
    LISTSUBMESSAGE m_listSubMessageCallback;
    int m_nPipe[2];
};
#define zeromqmanage_h
#endif
