#include "StdAfx.h"
#include "ProxyServer.h"


CProxyServer::CProxyServer()
{
    m_bStop = false;
}


CProxyServer::~CProxyServer()
{
}
void CProxyServer::Start()
{
    if (!m_ConfigRead.ReadConfig())
    {
        printf("读取配置文件失败, 返回...\n");
        return;
    }

    char pInfo[128] = { 0 };
    sprintf_s(pInfo, sizeof(pInfo), "tcp://*:%d", m_ConfigRead.m_nSubPort);
    zmq::context_t context(1);
    zmq::socket_t front_socket(context, ZMQ_SUB);
    front_socket.bind(pInfo);
    front_socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);

    sprintf_s(pInfo, sizeof(pInfo), "tcp://*:%d", m_ConfigRead.m_nPubPort);
    zmq::socket_t back_socket(context, ZMQ_PUB);
    back_socket.bind(pInfo);

    printf("代理服务启动, 发布端口[%d], 订阅端口[%d].\n", m_ConfigRead.m_nPubPort, m_ConfigRead.m_nSubPort);
    std::string sSend = "";
    int more;
    size_t moresize = sizeof(more);
    //std::string strrev = "";
    int n = 0;
    while (!m_bStop)
    {
        std::string strrev = s_recv(front_socket);
        front_socket.getsockopt(ZMQ_RCVMORE, &more, &moresize);

        more ? s_sendmore(back_socket, strrev) : s_send(back_socket, strrev);
           
        /*sSend += strrev;
        sSend += "\n";
        if(!more)
        {
            printf("%s\nCount: %d\n", sSend.c_str(), ++n);
            sSend = "";
        }*/
    }
    return;
}
void CProxyServer::Stop()
{
    m_bStop = true;
    Sleep(100);
    return;
}
