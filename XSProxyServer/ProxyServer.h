#pragma once

#include <zmq.hpp>
#include <zhelpers.hpp>
#include "ConfigRead.h"

#pragma comment( lib, "libzmq-v100-mt-gd-4_0_4.lib" )
class CProxyServer
{
public:
    CProxyServer();
    ~CProxyServer();
public:
    void Start();
    void Stop();
private:
    bool m_bStop;
    CConfigRead m_ConfigRead;
};

