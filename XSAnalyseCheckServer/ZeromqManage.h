#ifndef zeromqmanage_h

#include "DataDefine.h"
#include <list>
#include <zeromq/zmq.h>
#ifdef __WINDOWS__
#pragma comment( lib, "libzmq-v100-mt-gd-4_0_4.lib" )
#else
#include <unistd.h>
#endif
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
    bool PubMessage(const char * pHead, char * pID);
    //订阅消息
    bool InitSub(char * pLocalIP, int nLocalPort, char * pServerIP, int nServerPort,
                 LPSubMessageCallback pSubMessageCallback, void * pUser, int nThreadNum = THREADNUM);
    bool AddSubMessage(const char * pSubMessage);
    bool DelSubMessage(char * pSubMessage);

    //绑定本地push端口
    bool InitPush(char * pLocalIP, int nLocalPushPort, char * pServerIP, int nServerPort);
    bool PushMessage(LPSUBMESSAGE pSubMessage);
    //pull
    bool InitPull(char * pLocalIP, int nLocalPushPort, char * pServerIP, int nServerPort, 
                    LPSubMessageCallback pSubMessageCallback, void * pUser, int nThreadNum = THREADNUM);
private:
    //订阅消息接收
#ifdef __WINDOWS__
    static DWORD WINAPI SubMessageThread(void * pParam);
    static DWORD WINAPI PullMessageThread(void * pParam);
    static DWORD WINAPI MessageCallbackThread(void * pParam);
#else
    static void * SubMessageThread(void * pParam);
    static void * PullMessageThread(void * pParam);
    static void * MessageCallbackThread(void * pParam);
#endif
    //sub消息接收
    void SubMessageAction();
    //pull消息接收
    void PullMessageAction();
    //sub, pull接收消息回调
    void MessageCallbackAction();

    LPSUBMESSAGE GetFreeResource();
    void FreeResource(LPSUBMESSAGE pSubMessage);
    //进入临界区
    void EnterMutex();
    //退出临界区
    void LeaveMutex();
    void MsgNotify();

    void EnterPubMutex();
    void LeavePubMutex();
private:
    LPSubMessageCallback m_pSubMsgCallback; //订阅消息回调
    void * m_pUser;                         //订阅消息回调返回初始化传入参数
    bool m_bStopFlag;                       //线程停止标志
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

#ifdef __WINDOWS__
    CRITICAL_SECTION m_cs;
    CRITICAL_SECTION m_Pubcs;
    HANDLE m_ThreadSubMessageID;
    HANDLE m_ThreadPullMessageID;
    HANDLE m_ThreadCallbackID[THREADNUM];
    HANDLE m_hMsgEvent;
#else
    pthread_mutex_t m_mutex;                    //临界区
    pthread_mutex_t m_PubMutex;                 //发布消息临界区
    pthread_t m_ThreadSubMessageID;             //订阅消息线程句柄
    pthread_t m_ThreadPullMessageID;            //pull消息线程句柄
    pthread_t m_ThreadCallbackID[THREADNUM];    //回调线程句柄
    int m_nPipe[2];
#endif
    LISTSUBMESSAGE m_listSubMessageResource;
    LISTSUBMESSAGE m_listSubMessageCallback;
};
#define zeromqmanage_h
#endif
