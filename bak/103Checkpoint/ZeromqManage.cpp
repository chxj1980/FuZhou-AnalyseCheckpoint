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

    m_ThreadSubMessageID = -1;
    m_ThreadPullMessageID = -1;
    for (int i = 0; i < THREADNUM; i++)
    {
        m_ThreadCallbackID[i] = -1;
    }
    
    pthread_mutex_init(&m_mutex, NULL);
    pipe(m_nPipe);
}

CZeromqManage::~CZeromqManage()
{
    pthread_mutex_destroy(&m_mutex);
    close(m_nPipe[0]);
    close(m_nPipe[1]);
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
    }
    
    for (int i = 0; i < m_nThreadNum * 20; i++)
    {
        LPSUBMESSAGE pSubMessage = new SUBMESSAGE;
        m_listSubMessageResource.push_back(pSubMessage);
    }
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

    return true;
}
void CZeromqManage::UnInit()
{
    m_bStopFlag = true;
    write(m_nPipe[1], "1", 1);
    usleep(10 * 1000);

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
    pthread_mutex_lock(&m_mutex);
    while (m_listSubMessageResource.size() > 0)
    {
        delete m_listSubMessageResource.front();
        m_listSubMessageResource.pop_front();
    }
    pthread_mutex_unlock(&m_mutex);
    return;
}
bool CZeromqManage::AddSubMessage(char * pSubMessage)
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
            printf("##Pub [%s] success\n", pSubMessage); 
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
void * CZeromqManage::SubMessageThread(void * pParam)
{
    CZeromqManage * pThis = (CZeromqManage *)pParam;
    pThis->SubMessageAction();
    return NULL;
}

void CZeromqManage::SubMessageAction()
{
    while (!m_bStopFlag)
    {
        LPSUBMESSAGE pSubMessage = GetFreeResource();
        if (NULL == pSubMessage)
        {
            printf("***Warning: Pub Thread Cann't Get Free Resource!\n");
            usleep(THREADWAITTIME * 100 * 1000);
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
                        pthread_mutex_lock(&m_mutex);
                        m_listSubMessageCallback.push_back(pSubMessage);
                        write(m_nPipe[1], "1", 1);
                        pthread_mutex_unlock(&m_mutex);

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
void * CZeromqManage::MessageCallbackThread(void * pParam)
{
    CZeromqManage * pThis = (CZeromqManage *)pParam;
    pThis->MessageCallbackAction();
    return NULL;
}
void CZeromqManage::MessageCallbackAction()
{
    char pPipeMsg[10] = { 0 };
    LPSUBMESSAGE pSubMessage = NULL;
    while (!m_bStopFlag)
    {
        read(m_nPipe[0], pPipeMsg, 1);
        do 
        {
            pthread_mutex_lock(&m_mutex);
            if (m_listSubMessageCallback.size() > 0)
            {
                pSubMessage = m_listSubMessageCallback.front();
                m_listSubMessageCallback.pop_front();
            }
            else
            {
                pSubMessage = NULL;
            }
            pthread_mutex_unlock(&m_mutex);
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
    pthread_mutex_lock(&m_mutex);
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
    pthread_mutex_unlock(&m_mutex);

    pSubMessage->Init();
    return pSubMessage;
}
void CZeromqManage::FreeResource(LPSUBMESSAGE pSubMessage)
{
    pthread_mutex_lock(&m_mutex);
    m_listSubMessageResource.push_back(pSubMessage);
    while (m_listSubMessageResource.size() > m_nThreadNum * 20)
    {
        delete m_listSubMessageResource.front();
        m_listSubMessageResource.pop_front();
    }
    pthread_mutex_unlock(&m_mutex);
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
        zmq_send(m_pPubSocket, pSubMessage->pHead, strlen(pSubMessage->pHead), ZMQ_SNDMORE);
        zmq_send(m_pPubSocket, pSubMessage->pOperationType, strlen(pSubMessage->pOperationType), ZMQ_SNDMORE);
        zmq_send(m_pPubSocket, pSubMessage->pSource, strlen(pSubMessage->pSource), ZMQ_SNDMORE);
        zmq_send(m_pPubSocket, pSubMessage->sPubJsonValue.c_str(), pSubMessage->sPubJsonValue.size(), 0);
    }
    else
    {
        printf("****Error: PubMessage Failed, NULL == m_pPubSocket!");
        return false;
    }
    
    return true;
}
bool CZeromqManage::PubMessage(char * pHead, char * pID)
{
    if (NULL != m_pPubSocket)
    {
        zmq_send(m_pPubSocket, pHead, strlen(pHead), ZMQ_SNDMORE);
        zmq_send(m_pPubSocket, pID, strlen(pID), 0);
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
        zmq_send(m_pPushSocket, pSubMessage->pHead, strlen(pSubMessage->pHead), ZMQ_SNDMORE);
        zmq_send(m_pPushSocket, pSubMessage->pOperationType, strlen(pSubMessage->pOperationType), ZMQ_SNDMORE);
        zmq_send(m_pPushSocket, pSubMessage->pSource, strlen(pSubMessage->pSource), ZMQ_SNDMORE);
        zmq_send(m_pPushSocket, pSubMessage->sPubJsonValue.c_str(), pSubMessage->sPubJsonValue.size(), 0);
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

    return true;
}
void * CZeromqManage::PullMessageThread(void * pParam)
{
    CZeromqManage * pThis = (CZeromqManage *)pParam;
    pThis->PullMessageAction();
    return NULL;
}
void CZeromqManage::PullMessageAction()
{
    while (!m_bStopFlag)
    {
        LPSUBMESSAGE pSubMessage = GetFreeResource();
        if (NULL == pSubMessage)
        {
            printf("***Warning: Pull Thread Cann't Get Free Resource!\n");
            usleep(THREADWAITTIME * 100 * 1000);
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
                        pthread_mutex_lock(&m_mutex);
                        m_listSubMessageCallback.push_back(pSubMessage);
                        write(m_nPipe[1], "1", 1);
                        pthread_mutex_unlock(&m_mutex);

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