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
    //发布消息
    bool InitPub(char * pLocalIP, int nLocalPort, char * pServerIP, int nServerPort);
    bool PubMessage(LPSUBMESSAGE pSubMessage);
    bool PubMessage(char * pHead, char * pID);
    //订阅消息
    bool InitSub(char * pLocalIP, int nLocalPort, char * pServerIP, int nServerPort,
                 LPSubMessageCallback pSubMessageCallback, void * pUser, int nThreadNum = THREADNUM);
    bool AddSubMessage(char * pSubMessage);
    bool DelSubMessage(char * pSubMessage);

    //绑定本地push端口
    bool InitPush(char * pLocalIP, int nLocalPushPort, char * pServerIP, int nServerPort);
    bool PushMessage(LPSUBMESSAGE pSubMessage);
    //pull
    bool InitPull(char * pLocalIP, int nLocalPushPort, char * pServerIP, int nServerPort, 
                    LPSubMessageCallback pSubMessageCallback, void * pUser, int nThreadNum = THREADNUM);
private:
    //订阅消息接收
    static void * SubMessageThread(void * pParam);
    void SubMessageAction();
    //pull消息接收
    static void * PullMessageThread(void * pParam);
    void PullMessageAction();

    static void * MessageCallbackThread(void * pParam);
    void MessageCallbackAction();

    LPSUBMESSAGE GetFreeResource();
    void FreeResource(LPSUBMESSAGE pSubMessage);
private:
    LPSubMessageCallback m_pSubMsgCallback; //订阅消息回调
    void * m_pUser;                         //订阅消息回调返回初始化传入参数
    bool m_bStopFlag;                       //线程停止标志
    pthread_mutex_t m_mutex;                  //临界区
    int m_nThreadNum;                       //回调线程数, 申请资源*20

    void * m_pCtx;
    //发布Pub
    void * m_pPubSocket;            //发布socket
    //订阅Sub
    void * m_pSubSocket;            //订阅socket
    //推送Push
    void * m_pPushSocket;            //推送socket
    //拉Pull
    void * m_pPullSocket;            //拉socket

    pthread_t m_ThreadSubMessageID;             //订阅消息线程句柄
    pthread_t m_ThreadPullMessageID;            //pull消息线程句柄
    pthread_t m_ThreadCallbackID[THREADNUM];    //回调线程句柄
    LISTSUBMESSAGE m_listSubMessageResource;
    LISTSUBMESSAGE m_listSubMessageCallback;
    int m_nPipe[2];
};
#define zeromqmanage_h
#endif
