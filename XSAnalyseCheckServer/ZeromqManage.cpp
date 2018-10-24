#include "stdafx.h"
#include "ZeromqManage.h"

CZeromqManage::CZeromqManage()
{
    m_pSubMsgCallback = NULL;
    m_pUser = NULL;
    m_bStopFlag = false;

    m_pCtx = NULL;
    m_pPubSocket = NULL;
    m_pSubSocket = NULL;
    m_pPullSocket = NULL;
    m_pPushSocket = NULL;
#ifdef __WINDOWS__
    m_ThreadSubMessageID = INVALID_HANDLE_VALUE;;
    m_ThreadPullMessageID = INVALID_HANDLE_VALUE;
    for (int i = 0; i < THREADNUM; i++)
    {
        m_ThreadCallbackID[i] = INVALID_HANDLE_VALUE;
    }
    InitializeCriticalSection(&m_cs);
    InitializeCriticalSection(&m_Pubcs);
    m_hMsgEvent = CreateEvent(NULL, true, false, NULL);
#else
    m_ThreadSubMessageID = -1;
    m_ThreadPullMessageID = -1;
    for (int i = 0; i < THREADNUM; i++)
    {
        m_ThreadCallbackID[i] = -1;
    }
    pthread_mutex_init(&m_mutex, NULL);
    pthread_mutex_init(&m_PubMutex, NULL);
    pipe(m_nPipe);
#endif
}

CZeromqManage::~CZeromqManage()
{
#ifdef __WINDOWS__
    DeleteCriticalSection(&m_cs);
    DeleteCriticalSection(&m_Pubcs);
    CloseHandle(m_hMsgEvent);
#else
    pthread_mutex_destroy(&m_mutex);
    pthread_mutex_destroy(&m_PubMutex);
    close(m_nPipe[0]);
    close(m_nPipe[1]);
#endif
}
bool CZeromqManage::InitSub(char * pLocalIP, int nLocalPort, char * pServerIP, int nServerPort,
                            LPSubMessageCallback pSubMessageCallback, void * pUser, int nThreadNum)
{
    m_pSubMsgCallback = pSubMessageCallback;
    m_pUser = pUser;
    m_nThreadNum = nThreadNum;

    if (NULL == m_pCtx)
    {
        if (NULL == (m_pCtx = zmq_ctx_new()))
        {
            printf("****Error: zmq_ctx_new Failed[%s]!\n", zmq_strerror(errno));
            return false;
        }
    }
    
    if (NULL == m_pSubSocket)
    {
        if (NULL == (m_pSubSocket = zmq_socket(m_pCtx, ZMQ_SUB)))
        {
            zmq_ctx_destroy(m_pCtx);
            printf("****Error: zmq_socket Failed[%s]!\n", zmq_strerror(errno));
            return false;
        }
    }
    if (NULL != pLocalIP && 0 != nLocalPort)
    {
        char pAddress[32] = { 0 };
        sprintf(pAddress, "tcp://%s:%d", pLocalIP, nLocalPort);
        if (zmq_bind(m_pSubSocket, pAddress) < 0)
        {
            zmq_close(m_pSubSocket);
            zmq_ctx_destroy(m_pCtx);
            printf("****Error: zmq_bind[%s:%d] Failed[%s]!\n", pLocalIP, nLocalPort, zmq_strerror(errno));
            return false;
        }
    }
    if (NULL != pServerIP && 0 != nServerPort)
    {
        char pSubAddress[32] = { 0 };
        sprintf(pSubAddress, "tcp://%s:%d", pServerIP, nServerPort);
        if (zmq_connect(m_pSubSocket, pSubAddress) < 0)
        {
            zmq_close(m_pSubSocket);
            zmq_ctx_destroy(m_pCtx);
            printf("****Error: zmq_connect Failed[%s]!\n", zmq_strerror(errno));
            return false;
        }
        printf("sub zeromq[%s] success.\n", pSubAddress);
    }
    
    for (int i = 0; i < m_nThreadNum * 20; i++)
    {
        LPSUBMESSAGE pSubMessage = new SUBMESSAGE;
        m_listSubMessageResource.push_back(pSubMessage);
    }

#ifdef __WINDOWS__
    if (INVALID_HANDLE_VALUE == m_ThreadSubMessageID)
    {
        m_ThreadSubMessageID = CreateThread(NULL, 0, SubMessageThread, this, NULL, 0);
    }
    for (int i = 0; i < m_nThreadNum; i++)
    {
        if (INVALID_HANDLE_VALUE == m_ThreadCallbackID[i])
        {
            m_ThreadCallbackID[i] = CreateThread(NULL, 0, MessageCallbackThread, this, NULL, 0);
        }
    }
#else
    if (-1 == m_ThreadSubMessageID)
    {
        pthread_create(&m_ThreadSubMessageID, NULL, SubMessageThread, (void*)this);
    }
    for (int i = 0; i < m_nThreadNum; i++)
    {
        if (-1 == m_ThreadCallbackID[i])
        {
            pthread_create(&m_ThreadCallbackID[i], NULL, MessageCallbackThread, (void*)this);
        }
    }
#endif
    return true;
}
void CZeromqManage::UnInit()
{
    m_bStopFlag = true;
    MsgNotify();
#ifdef __WINDOWS__
    Sleep(10);
#else
    usleep(10 * 1000);
#endif

    //关闭 pub socket
    if (NULL != m_pPubSocket)
    {
        zmq_close(m_pPubSocket);
        m_pPubSocket = NULL;
    }
    //关闭 push socket
    if (NULL != m_pPushSocket)
    {
        zmq_close(m_pPushSocket);
        m_pPushSocket = NULL;
    }
    
    //关闭context(如直接关闭sub socket 或 pull socket, 会导致正在阻塞的zmq_recv报错),
    //zmq_ctx_destroy 会一直阻塞, 直到所有依赖于此的socket全部关闭
    if (NULL != m_pCtx)
    {
        zmq_ctx_destroy(m_pCtx);
        m_pCtx = NULL;
    }
    //sub socket, pull socket在线程中关闭, 不用在此处理, 关闭context时, zmq_recv返回-1
#ifdef __WINDOWS__
    if (INVALID_HANDLE_VALUE != m_ThreadSubMessageID)
    {
        WaitForSingleObject(m_ThreadSubMessageID, INFINITE);
        m_ThreadSubMessageID = INVALID_HANDLE_VALUE;
        printf("--Thread Sub Message End...\n");
    }
    if (INVALID_HANDLE_VALUE != m_ThreadPullMessageID)
    {
        WaitForSingleObject(m_ThreadPullMessageID, INFINITE);
        m_ThreadPullMessageID = INVALID_HANDLE_VALUE;
        printf("--Thread Pull Message End...\n");
    }
    for (int i = 0; i < m_nThreadNum; i++)
    {
        if (INVALID_HANDLE_VALUE != m_ThreadCallbackID[i])
        {
            WaitForSingleObject(m_ThreadCallbackID[i], INFINITE);
            m_ThreadCallbackID[i] = INVALID_HANDLE_VALUE;
            printf("--Thread[%d] Callback Sub Message End...\n", i);
        }
    }
#else
    if (-1 != m_ThreadSubMessageID)
    {
        pthread_join(m_ThreadSubMessageID, NULL);
        m_ThreadSubMessageID = -1;
        printf("--Thread Sub Message End...\n");
    }
    if (-1 != m_ThreadPullMessageID)
    {
        pthread_join(m_ThreadPullMessageID, NULL);
        m_ThreadPullMessageID = -1;
        printf("--Thread Pull Message End...\n");
    }
    for (int i = 0; i < m_nThreadNum; i++)
    {
        if (-1 != m_ThreadCallbackID[i])
        {
            pthread_join(m_ThreadCallbackID[i], NULL);
            m_ThreadCallbackID[i] = -1;
            printf("--Thread[%d] Callback Sub Message End...\n", i);
        }
    }
#endif

    EnterMutex();
    while (m_listSubMessageResource.size() > 0)
    {
        delete m_listSubMessageResource.front();
        m_listSubMessageResource.pop_front();
    }
    LeaveMutex();
    return;
}
bool CZeromqManage::AddSubMessage(const char * pSubMessage)
{
    bool bRet = false;
    if (NULL != m_pSubSocket)
    {
        if (zmq_setsockopt(m_pSubSocket, ZMQ_SUBSCRIBE, pSubMessage, strlen(pSubMessage)) < 0)
        {
            printf("****Error: zmq_setsockopt Subscribe[%s] Failed[%s]!\n", pSubMessage, zmq_strerror(errno));
        }
        else
        {
            printf("##Sub [%s] success\n", pSubMessage); 
            bRet = true;
        }
    }
    else
    {
        printf("***Warning: Sub Socket Not Initial!\n");
    }
    
    return bRet;
}
bool CZeromqManage::DelSubMessage(char * pSubMessage)
{
    bool bRet = false;
    if (NULL != m_pSubSocket)
    {
        if (zmq_setsockopt(m_pSubSocket, ZMQ_UNSUBSCRIBE, pSubMessage, strlen(pSubMessage)) < 0)
        {
            printf("****Error: zmq_setsockopt UnSubscribe[%s] Failed[%s]!\n", pSubMessage, zmq_strerror(errno));
        }
        else
        {
            printf("##取消订阅[%s]成功!\n", pSubMessage);
            bRet = true;
        }
    }
    else
    {
        printf("***Warning: Sub Socket Not Initial!\n");
    }
    return bRet;
}
#ifdef __WINDOWS__
DWORD CZeromqManage::SubMessageThread(void * pParam)
{
    CZeromqManage * pThis = (CZeromqManage *)pParam;
    pThis->SubMessageAction();
    return 0;
}
#else 
void * CZeromqManage::SubMessageThread(void * pParam)
{
    CZeromqManage * pThis = (CZeromqManage *)pParam;
    pThis->SubMessageAction();
    return NULL;
}
#endif
void CZeromqManage::SubMessageAction()
{
    while (!m_bStopFlag)
    {
        LPSUBMESSAGE pSubMessage = GetFreeResource();
        if (NULL == pSubMessage)
        {
            printf("***Warning: Pub Thread Cann't Get Free Resource!\n");
#ifdef __WINDOWS__
            Sleep(THREADWAITTIME * 100);
#else
            usleep(THREADWAITTIME * 100 * 1000);
#endif
            continue;
        }
        int nRet = zmq_recv(m_pSubSocket, pSubMessage->pHead, sizeof(pSubMessage->pHead), 0);       //无消息时会阻塞
        if(nRet < 0)
        {
            printf("****error: %s\n", zmq_strerror(errno));
            delete pSubMessage;
            zmq_close(m_pSubSocket);
            m_pSubSocket = NULL;
            return;
        }
        else
        {
            bool bRecv = false;
            
            int nMore = 0;
            size_t nMoreSize = sizeof(nMore);
            zmq_getsockopt(m_pSubSocket, ZMQ_RCVMORE, &nMore, &nMoreSize);
            if (nMore)
            {
                zmq_recv(m_pSubSocket, pSubMessage->pOperationType, sizeof(pSubMessage->pOperationType), 0);
                zmq_setsockopt(m_pSubSocket, ZMQ_RCVMORE, &nMore, nMoreSize);
                if (nMore)
                {
                    zmq_recv(m_pSubSocket, pSubMessage->pSource, sizeof(pSubMessage->pSource), 0);
                    //printf("\n------------Recv: \n");
                    //printf("%s\n%s\n%s\n------------\n", pSubMessage->pHead, pSubMessage->pOperationType, pSubMessage->pSource);
                    zmq_setsockopt(m_pSubSocket, ZMQ_RCVMORE, &nMore, nMoreSize);
                    if (nMore)
                    {
                        zmq_recv(m_pSubSocket, pSubMessage->pSubJsonValue, sizeof(pSubMessage->pSubJsonValue), 0);
                        EnterMutex();
                        m_listSubMessageCallback.push_back(pSubMessage);
                        MsgNotify();
                        LeaveMutex();

                        bRecv = true;
                    }
                }
            }

            if(!bRecv)
            {
                printf("***Warning: Recv more Failed.\n");
                FreeResource(pSubMessage);
            }
        }
    }
    return;
}
#ifdef __WINDOWS__
DWORD CZeromqManage::MessageCallbackThread(void * pParam)
{
    CZeromqManage * pThis = (CZeromqManage *)pParam;
    pThis->MessageCallbackAction();
    return 0;
}
#else 
void * CZeromqManage::MessageCallbackThread(void * pParam)
{
    CZeromqManage * pThis = (CZeromqManage *)pParam;
    pThis->MessageCallbackAction();
    return NULL;
}
#endif
void CZeromqManage::MessageCallbackAction()
{
    char pPipeMsg[10] = { 0 };
    LPSUBMESSAGE pSubMessage = NULL;
    while (!m_bStopFlag)
    {
#ifdef __WINDOWS__
        if (WAIT_TIMEOUT == WaitForSingleObject(m_hMsgEvent, THREADWAITTIME))
        {
            continue;
        }
        ResetEvent(m_hMsgEvent);
#else
        read(m_nPipe[0], pPipeMsg, 1);
#endif
        do 
        {
            EnterMutex();
            if (m_listSubMessageCallback.size() > 0)
            {
                pSubMessage = m_listSubMessageCallback.front();
                m_listSubMessageCallback.pop_front();
            }
            else
            {
                pSubMessage = NULL;
            }
            LeaveMutex();
            if (NULL != pSubMessage)
            {
                m_pSubMsgCallback(pSubMessage, m_pUser);
                FreeResource(pSubMessage);
            }
        } while (NULL != pSubMessage);
    }
}
LPSUBMESSAGE CZeromqManage::GetFreeResource()
{
    LPSUBMESSAGE pSubMessage = NULL;
    EnterMutex();
    if (m_listSubMessageResource.size() > 0)
    {
        pSubMessage = m_listSubMessageResource.front();
        m_listSubMessageResource.pop_front();
    }
    else
    {
        pSubMessage = new SUBMESSAGE;
        if (NULL == pSubMessage)
        {
            printf("****Error: new Failed!\n");
        }
    }
    LeaveMutex();

    pSubMessage->Init();
    return pSubMessage;
}
void CZeromqManage::FreeResource(LPSUBMESSAGE pSubMessage)
{
    EnterMutex();
    m_listSubMessageResource.push_back(pSubMessage);
    //while (m_listSubMessageResource.size() > m_nThreadNum * 20)
    {
        //delete m_listSubMessageResource.front();
        //m_listSubMessageResource.pop_front();
    }
    LeaveMutex();
}
bool CZeromqManage::InitPub(char * pLocalIP, int nLocalPort, char * pServerIP, int nServerPort)
{
    if (NULL == m_pCtx)
    {
        if (NULL == (m_pCtx = zmq_ctx_new()))
        {
            printf("****Error: zmq_ctx_new Failed[%s]!\n", zmq_strerror(errno));
            return false;
        }
    }
    if (NULL == m_pPubSocket)
    {
        if (NULL == (m_pPubSocket = zmq_socket(m_pCtx, ZMQ_PUB)))
        {
            zmq_ctx_destroy(m_pCtx);
            printf("****Error: zmq_socket Failed[%s]!\n", zmq_strerror(errno));
            return false;
        }
    }
    if (NULL != pLocalIP && 0 != nLocalPort)
    {
        char pAddress[32] = { 0 };
        sprintf(pAddress, "tcp://%s:%d", pLocalIP, nLocalPort);
        if (zmq_bind(m_pPushSocket, pAddress) < 0)
        {
            zmq_close(m_pPubSocket);
            zmq_ctx_destroy(m_pCtx);
            printf("****Error: InitPub::zmq_bind[%s:%d] Failed[%s]!\n", pLocalIP, nLocalPort, zmq_strerror(errno));
            return false;
        }
    }
    if (NULL != pServerIP && 0 != nServerPort)
    {
        char pServerAddress[32] = { 0 };
        sprintf(pServerAddress, "tcp://%s:%d", pServerIP, nServerPort);
        if (zmq_connect(m_pPubSocket, pServerAddress) < 0)
        {
            zmq_close(m_pPubSocket);
            zmq_ctx_destroy(m_pCtx);
            printf("****Error: InitPub::zmq_connect[%s:%d] Failed[%s]!\n", pServerIP, nServerPort, zmq_strerror(errno));
            return false;
        }
    }
        
    return true;
}

bool CZeromqManage::PubMessage(LPSUBMESSAGE pSubMessage)
{
    if (NULL != m_pPubSocket)
    {
        EnterPubMutex();
        zmq_send(m_pPubSocket, pSubMessage->pHead, strlen(pSubMessage->pHead), ZMQ_SNDMORE);
        zmq_send(m_pPubSocket, pSubMessage->pOperationType, strlen(pSubMessage->pOperationType), ZMQ_SNDMORE);
        zmq_send(m_pPubSocket, pSubMessage->pSource, strlen(pSubMessage->pSource), ZMQ_SNDMORE);
        zmq_send(m_pPubSocket, pSubMessage->sPubJsonValue.c_str(), pSubMessage->sPubJsonValue.size(), 0);
        LeavePubMutex();
    }
    else
    {
        printf("****Error: PubMessage Failed, NULL == m_pPubSocket!");
        return false;
    }
    
    return true;
}
bool CZeromqManage::PubMessage(const char * pHead, char * pID)
{
    if (NULL != m_pPubSocket)
    {
        EnterPubMutex();
        zmq_send(m_pPubSocket, pHead, strlen(pHead), ZMQ_SNDMORE);
        zmq_send(m_pPubSocket, pID, strlen(pID), 0);
        LeavePubMutex();
    }
    else
    {
        printf("****Error: PubMessage Failed, NULL == m_pPubSocket!");
        return false;
    }

    return true;
}
bool CZeromqManage::InitPush(char * pLocalIP, int nLocalPushPort, char * pServerIP, int nServerPort)
{
    if (NULL == m_pCtx)
    {
        if (NULL == (m_pCtx = zmq_ctx_new()))
        {
            printf("****Error: zmq_ctx_new Failed[%s]!\n", zmq_strerror(errno));
            return false;
        }
    }
    if (NULL == m_pPushSocket)
    {
        if (NULL == (m_pPushSocket = zmq_socket(m_pCtx, ZMQ_PUSH)))
        {
            zmq_ctx_destroy(m_pCtx);
            printf("****Error: zmq_socket Failed[%s]!\n", zmq_strerror(errno));
            return false;
        }
    }
    if (NULL != pLocalIP && 0 != nLocalPushPort)
    {
        char pAddress[32] = { 0 };
        sprintf(pAddress, "tcp://%s:%d", pLocalIP, nLocalPushPort);
        if (zmq_bind(m_pPushSocket, pAddress) < 0)
        {
            zmq_close(m_pPushSocket);
            zmq_ctx_destroy(m_pCtx);
            printf("****Error: InitPush::zmq_bind Failed[%s]!\n", zmq_strerror(errno));
            return false;
        }
    }
    if (NULL != pServerIP && 0 != nServerPort)
    {
        char pAddress[32] = { 0 };
        sprintf(pAddress, "tcp://%s:%d", pServerIP, nServerPort);
        if (zmq_connect(m_pPushSocket, pAddress) < 0)
        {
            zmq_close(m_pPushSocket);
            zmq_ctx_destroy(m_pCtx);
            printf("****Error: InitPush::zmq_connect[%s:%d] Failed[%s]!\n",pServerIP, nServerPort, zmq_strerror(errno));
            return false;
        }
    }
    
    return true;
}
bool CZeromqManage::PushMessage(LPSUBMESSAGE pSubMessage)
{
    if (NULL != m_pPushSocket)
    {
        EnterPubMutex();
        zmq_send(m_pPushSocket, pSubMessage->pHead, strlen(pSubMessage->pHead), ZMQ_SNDMORE);
        zmq_send(m_pPushSocket, pSubMessage->pOperationType, strlen(pSubMessage->pOperationType), ZMQ_SNDMORE);
        zmq_send(m_pPushSocket, pSubMessage->pSource, strlen(pSubMessage->pSource), ZMQ_SNDMORE);
        zmq_send(m_pPushSocket, pSubMessage->sPubJsonValue.c_str(), pSubMessage->sPubJsonValue.size(), 0);
        LeavePubMutex();
    }
    else
    {
        printf("****Error: PushMessage Failed, NULL == m_pPushSocket!");
        return false;
    }

    return true;
}
bool CZeromqManage::InitPull(char * pLocalIP, int nLocalPushPort, char * pServerIP, int nServerPort,
                                LPSubMessageCallback pSubMessageCallback, void * pUser, int nThreadNum)
{
    m_pSubMsgCallback = pSubMessageCallback;
    m_pUser = pUser;
    m_nThreadNum = nThreadNum;

    if (NULL == m_pCtx)
    {
        if (NULL == (m_pCtx = zmq_ctx_new()))
        {
            printf("****Error: zmq_ctx_new Failed[%s]!\n", zmq_strerror(errno));
            return false;
        }
    }

    if (NULL == m_pPullSocket)
    {
        if (NULL == (m_pPullSocket = zmq_socket(m_pCtx, ZMQ_PULL)))
        {
            zmq_ctx_destroy(m_pCtx);
            printf("****Error: zmq_socket Failed[%s]!\n", zmq_strerror(errno));
            return false;
        }
    }

    if (NULL != pLocalIP && 0 != nLocalPushPort)
    {
        char pAddress[32] = { 0 };
        sprintf(pAddress, "tcp://%s:%d", pLocalIP, nLocalPushPort);
        if (zmq_bind(m_pPullSocket, pAddress) < 0)
        {
            zmq_close(m_pPullSocket);
            zmq_ctx_destroy(m_pCtx);
            printf("****Error: zmq_bind[%s:%d] Failed[%s]!\n", pLocalIP, nLocalPushPort, zmq_strerror(errno));
            return false;
        }
    }

    if (NULL != pServerIP && 0 != nServerPort)
    {
        char pAddress[32] = { 0 };
        sprintf(pAddress, "tcp://%s:%d", pServerIP, nServerPort);
        if (zmq_connect(m_pPullSocket, pAddress) < 0)
        {
            zmq_close(m_pPullSocket);
            zmq_ctx_destroy(m_pCtx);
            printf("****Error: zmq_connect Failed[%s]!\n", zmq_strerror(errno));
            return false;
        }
    }
    

    if (0 == m_listSubMessageResource.size())
    {
        for (int i = 0; i < m_nThreadNum * 20; i++)
        {
            LPSUBMESSAGE pSubMessage = new SUBMESSAGE;
            m_listSubMessageResource.push_back(pSubMessage);
        }
    }
#ifdef __WINDOWS__
    if (INVALID_HANDLE_VALUE == m_ThreadPullMessageID)
    {
        m_ThreadPullMessageID = CreateThread(NULL, 0, PullMessageThread, this, NULL, 0);   //搜索比对线程
    }
    for (int i = 0; i < m_nThreadNum; i++)
    {
        if (INVALID_HANDLE_VALUE == m_ThreadCallbackID[i])
        {
            m_ThreadCallbackID[i] = CreateThread(NULL, 0, MessageCallbackThread, this, NULL, 0);   //搜索比对线程
        }
    }
#else    
    if (-1 == m_ThreadPullMessageID)
    {
        pthread_create(&m_ThreadPullMessageID, NULL, PullMessageThread, (void*)this);
    }
    for (int i = 0; i < m_nThreadNum; i++)
    {
        if (-1 == m_ThreadCallbackID[i])
        {
            pthread_create(&m_ThreadCallbackID[i], NULL, MessageCallbackThread, (void*)this);
        }
    }
#endif
    return true;
}
#ifdef __WINDOWS__
DWORD CZeromqManage::PullMessageThread(void * pParam)
{
    CZeromqManage * pThis = (CZeromqManage *)pParam;
    pThis->PullMessageAction();
    return 0;
}
#else 
void * CZeromqManage::PullMessageThread(void * pParam)
{
    CZeromqManage * pThis = (CZeromqManage *)pParam;
    pThis->PullMessageAction();
    return NULL;
}
#endif
void CZeromqManage::PullMessageAction()
{
    while (!m_bStopFlag)
    {
        LPSUBMESSAGE pSubMessage = GetFreeResource();
        if (NULL == pSubMessage)
        {
            printf("***Warning: Pull Thread Cann't Get Free Resource!\n");
#ifdef __WINDOWS__
            Sleep(THREADWAITTIME * 100);
#else
            usleep(THREADWAITTIME * 100 * 1000);
#endif
            continue;
        }
        int nRet = zmq_recv(m_pPullSocket, pSubMessage->pHead, sizeof(pSubMessage->pHead), 0);       //无消息时会阻塞
        if (nRet < 0)
        {
            printf("****error: %s\n", zmq_strerror(errno));
            delete pSubMessage;
            zmq_close(m_pPullSocket);
            m_pPullSocket = NULL;
            return;
        }
        else
        {
            bool bRecv = false;

            int nMore = 0;
            size_t nMoreSize = sizeof(nMore);
            zmq_getsockopt(m_pPullSocket, ZMQ_RCVMORE, &nMore, &nMoreSize);
            if (nMore)
            {
                zmq_recv(m_pPullSocket, pSubMessage->pOperationType, sizeof(pSubMessage->pOperationType), 0);
                zmq_setsockopt(m_pPullSocket, ZMQ_RCVMORE, &nMore, nMoreSize);
                if (nMore)
                {
                    zmq_recv(m_pPullSocket, pSubMessage->pSource, sizeof(pSubMessage->pSource), 0);
                    //printf("\n------------Recv: \n");
                    //printf("%s\n%s\n%s\n------------\n", pSubMessage->pHead, pSubMessage->pOperationType, pSubMessage->pSource);
                    zmq_setsockopt(m_pPullSocket, ZMQ_RCVMORE, &nMore, nMoreSize);
                    if (nMore)
                    {
                        zmq_recv(m_pPullSocket, pSubMessage->pSubJsonValue, sizeof(pSubMessage->pSubJsonValue), 0);
                        EnterMutex();
                        m_listSubMessageCallback.push_back(pSubMessage);
                        MsgNotify();
                        LeaveMutex();

                        bRecv = true;
                    }
                }
            }

            if (!bRecv)
            {
                printf("***Warning: Recv more Failed.\n");
                FreeResource(pSubMessage);
            }
        }
    }
    return;
}
//进入临界区
void CZeromqManage::EnterMutex()
{
#ifdef __WINDOWS__
    EnterCriticalSection(&m_cs);
#else
    pthread_mutex_lock(&m_mutex);
#endif
}
//退出临界区
void CZeromqManage::LeaveMutex()
{
#ifdef __WINDOWS__
    LeaveCriticalSection(&m_cs);
#else
    pthread_mutex_unlock(&m_mutex);
#endif
}
void CZeromqManage::MsgNotify()
{
#ifdef __WINDOWS__
    SetEvent(m_hMsgEvent);
#else
    write(m_nPipe[1], "1", 1);
#endif
}
//进入临界区
void CZeromqManage::EnterPubMutex()
{
#ifdef __WINDOWS__
    EnterCriticalSection(&m_Pubcs);
#else
    pthread_mutex_lock(&m_PubMutex);
#endif
}
//退出临界区
void CZeromqManage::LeavePubMutex()
{
#ifdef __WINDOWS__
    LeaveCriticalSection(&m_Pubcs);
#else
    pthread_mutex_unlock(&m_PubMutex);
#endif
}