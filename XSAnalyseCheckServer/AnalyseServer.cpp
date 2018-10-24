#include "stdafx.h"

#include "AnalyseServer.h"
#include "stdio.h"
#include "time.h"

#ifdef __WINDOWS__
CLogRecorder g_LogRecorder;
#endif
CAnalyseServer::CAnalyseServer()
{
    m_pMysqlOperation = NULL;
    m_pZeromqManage = NULL;
    m_pRedisManage = NULL;
    m_nServerType = 1;
    //初始化临界区
#ifdef __WINDOWS__
    InitializeCriticalSection(&m_cs);
    m_hStopEvent = CreateEvent(NULL, true, false, NULL);
    m_hThreadPubStatus = INVALID_HANDLE_VALUE;
#else
    pthread_mutex_init(&m_mutex, NULL);
    pipe(m_nPipe);
    m_hThreadPubStatus = -1;
#endif
}
CAnalyseServer::~CAnalyseServer()
{
#ifdef __WINDOWS__
    DeleteCriticalSection(&m_cs);
    CloseHandle(m_hStopEvent);
#else
    pthread_mutex_destroy(&m_mutex);
    close(m_nPipe[0]);
    close(m_nPipe[1]);
#endif
}
//开始服务
bool CAnalyseServer::StartAnalyseServer(int nServerType)
{
    m_nServerType = nServerType;
    if(!Init())
    {
        printf("***Warning: StartAnalyseServer::Init Failed.\n");
        return false;
    }

#ifdef __WINDOWS__
    Sleep(1000);
#else
    usleep(1000 * 1000);
#endif
    string sServerType = m_nServerType == 1 ? "Checkpoint" : "Check";
    printf("******************%s AnalyseServer Start******************\n\n", sServerType.c_str());

#ifdef __WINDOWS__
#ifdef _DEBUG
    unsigned char pIn;
    while(true)
    {
        printf("\nInput: \n"
                "a: Show All LibInfo\n"
                "e: quit Service\n"
                );
        cin >> pIn;
        switch(pIn)
        {
        case 'a':
            {
                printf("\n==============================\n");
                printf("ServerID: %s\n", m_ConfigRead.m_sServerID.c_str());
                printf("LoadLimitTime: %d\n", m_ConfigRead.m_nLoadLimitTime);
                printf("Lib Info: \n");
                int nSeq = 1;
                for (map<string, CLibInfo *>::iterator it = m_mapLibInfo.begin();
                    it != m_mapLibInfo.end(); it++)
                {
                    printf("  [%03d]  %s  Input Number: %d\n", nSeq++, it->first.c_str(), it->second->GetTotalNum());

                    if (1 == m_nServerType)
                    {
                        unsigned long long tBegin = 0;
                        unsigned long long tEnd = 0;
                        it->second->GetDataDate(tBegin, tEnd);
                        struct tm *tmBegin = localtime((time_t*)&(tBegin));
                        char pBegin[20] = { 0 };
                        strftime(pBegin, sizeof(pBegin), "%Y-%m-%d %H:%M:%S", tmBegin);

                        struct tm *tmEnd = localtime((time_t*)&(tEnd));
                        char pEnd[20] = { 0 };
                        strftime(pEnd, sizeof(pEnd), "%Y-%m-%d %H:%M:%S", tmEnd);
                        printf("        Time[%s  %s]\n", pBegin, pEnd);
                    }

                }
               
                printf("\n==============================\n");
                break;
            }
        case 'e':
            {
                StopAnalyseServer();
                return true;
            }
        default:
            break;
        }
		Sleep(1000);
    }
#else
    WaitForSingleObject(m_hStopEvent, INFINITE);
#endif

#else
    unsigned char pIn;
    while (true)
    {
        printf("\nInput: \n"
            "a: Show All LibInfo\n"
            "e: quit Service\n"
        );
        cin >> pIn;
        switch (pIn)
        {
        case 'a':
            {
                printf("\n==============================\n");
                printf("ServerID: %s\n", m_ConfigRead.m_sServerID.c_str());
                printf("LoadLimitTime: %d\n", m_ConfigRead.m_nLoadLimitTime);
                printf("Lib Info: \n");
                int nSeq = 1;
                for (map<string, CLibInfo *>::iterator it = m_mapLibInfo.begin();
                    it != m_mapLibInfo.end(); it++)
                {
                    printf("  [%03d]  %s  Input Number: %d\n", nSeq++, it->first.c_str(), it->second->GetTotalNum());

                    if (1 == m_nServerType)
                    {
                        unsigned long long tBegin = 0;
                        unsigned long long tEnd = 0;
                        it->second->GetDataDate(tBegin, tEnd);
                        struct tm *tmBegin = localtime((time_t*)&(tBegin));
                        char pBegin[20] = { 0 };
                        strftime(pBegin, sizeof(pBegin), "%Y-%m-%d %H:%M:%S", tmBegin);

                        struct tm *tmEnd = localtime((time_t*)&(tEnd));
                        char pEnd[20] = { 0 };
                        strftime(pEnd, sizeof(pEnd), "%Y-%m-%d %H:%M:%S", tmEnd);
                        printf("        Time[%s  %s]\n", pBegin, pEnd);
                    }

                }

                printf("\n==============================\n");
                break;
            }
        case 'e':
            {
                StopAnalyseServer();
                return true;
            }
        default:
            break;
        }
        sleep(1);
    }
#endif
    return true;
}
//停止服务
bool CAnalyseServer::StopAnalyseServer()
{
#ifdef __WINDOWS__
    SetEvent(m_hStopEvent);
    if (INVALID_HANDLE_VALUE != m_hThreadPubStatus)
    {
        WaitForSingleObject(m_hThreadPubStatus, INFINITE);
    }
#else
    write(m_nPipe[1], "1", 1);
    if (-1 != m_hThreadPubStatus)
    {
        pthread_join(m_hThreadPubStatus, NULL);
    }
#endif
    printf("--Pub Status Thread End.\n");

    if (NULL != m_pZeromqManage)
    {
        m_pZeromqManage->UnInit();
        delete m_pZeromqManage;
        m_pZeromqManage = NULL;
    }
    if (NULL != m_pRedisManage)
    {
        m_pRedisManage->UnInit();
        delete m_pRedisManage;
        m_pRedisManage = NULL;
    }
    while(m_mapLibInfo.size() > 0)
    {
        map<string, CLibInfo *>::iterator it = m_mapLibInfo.begin();
        it->second->Stop();
        delete it->second;
        m_mapLibInfo.erase(it);
    }
    if (NULL != m_pMysqlOperation)
    {
        m_pMysqlOperation->DisConnectDB();
        delete m_pMysqlOperation;
        m_pMysqlOperation = NULL;
    }
    

    printf("************************Stop Analyse Server************************\n");
    return true;
}
//初始化服务
bool CAnalyseServer::Init()
{
#ifdef __WINDOWS__
    DWORD nBufferLenth = MAX_PATH;
    char szBuffer[MAX_PATH] = { 0 };
    DWORD dwRet = GetModuleFileNameA(NULL, szBuffer, nBufferLenth);
    char *sPath = strrchr(szBuffer, '\\');
    memset(sPath, 0, strlen(sPath));
    string sConfigPath = szBuffer;
    sConfigPath += "/Config/XSAnalyseServer_x64_config.properties";
#ifdef _DEBUG
    sConfigPath = "./Config/XSAnalyseServer_x64_config.properties";
#endif
    g_LogRecorder.InitLogger(sConfigPath.c_str(), "XSAnalyseServer_x64Logger", "XSAnalyseServer_x64");

    //初始化比对接口
    printf("初始化FRSLib...\n");
    int nRet = Initialize(29);
    if (1 != nRet)
    {
        printf("****Error: FRSLib Initialize Failed, ErrorCode: %d.\n", nRet);
        return false;
    }
#endif

    //读取配置文件
    if(!m_ConfigRead.ReadConfig(m_nServerType))
    {
        printf("****Error: Read Config File Failed.\n");
        return false;
    }

    if (string(m_ConfigRead.m_sServerID, 10, 3) == "251")
    {
        m_nServerType = 1;  //卡口关联分析服务
        printf("===获取特征值时间: %d天.\n"
                "===图片时间是否保存为本地时间: %d\n"
                "===是否获取图片URL: %d.\n",
            m_ConfigRead.m_nLoadLimitTime, m_ConfigRead.m_nServerTime, m_ConfigRead.m_nLoadURL);
    }
    else if (string(m_ConfigRead.m_sServerID, 10, 3) == "252")
    {
        m_nServerType = 2;  //核查分析服务
    }
    else
    {
        printf("****Error: ServerID[%s] Error!\n", m_ConfigRead.m_sServerID.c_str());
        return false;
    }

    if (NULL == m_pZeromqManage)
    {
        m_pZeromqManage = new CZeromqManage;
    }
    if(NULL == m_pRedisManage)
    {
        m_pRedisManage = new CRedisManage;
    }
    if (!InitLib())
    {
        printf("***Warning: connect MySQL Failed.\n");
        return false;
    }
    //初始化Redis连接
    if (!InitRedis())
    {
        return false;
    }
    //初始化Zeromq
    if (!InitZeromq())
    {
        return false;
    }
#ifdef __WINDOWS__
    m_hThreadPubStatus = CreateThread(NULL, 0, PubStatusThread, this, NULL, 0);   //发布库状态线程
#else
    pthread_create(&m_hThreadPubStatus, NULL, PubStatusThread, (void*)this);
#endif
    printf("--Pub Status Thread Start.\n");
    return true;
}
//获取库信息
bool CAnalyseServer::InitLib()
{
    if (NULL == m_pMysqlOperation)
    {
        m_pMysqlOperation = new CMySQLOperation(m_ConfigRead.m_nLoadURL);
    }
    if (!m_pMysqlOperation->ConnectDB(m_ConfigRead.m_sDBIP.c_str(), m_ConfigRead.m_sDBUser.c_str(),
        m_ConfigRead.m_sDBPd.c_str(), m_ConfigRead.m_sDBName.c_str(), m_ConfigRead.m_nDBPort))
    {
        return false;
    }

    char pSQL[SQLMAXLEN] = { 0 };
    if (1 == m_nServerType) //卡口分析服务, 只取卡口库信息
    {
        sprintf(pSQL, "select libname from %s where type = 1 order by libname", LIBINFOTABLE);
    }
    else                  //核查分析服务, 取核查库和布控库信息
    {
        sprintf(pSQL, "select libname from %s where type != 1 order by libname", LIBINFOTABLE);
    }

    MYSQL_RES * result = NULL;
    if (!m_pMysqlOperation->GetQueryResult(pSQL, result))
    {
        return false;
    }

    int nRowCount = mysql_num_rows(result);
    int nNum = 1;
    if (nRowCount > 0)
    {
        LPDEVICEINFO pDeviceInfo = new DEVICEINFO;
        pDeviceInfo->nServerType = m_nServerType;
        pDeviceInfo->nMaxCount = m_ConfigRead.m_nMaxCount;
        pDeviceInfo->sDBIP = m_ConfigRead.m_sDBIP;
        pDeviceInfo->nDBPort = m_ConfigRead.m_nDBPort;
        pDeviceInfo->sDBUser = m_ConfigRead.m_sDBUser;
        pDeviceInfo->sDBPassword = m_ConfigRead.m_sDBPd;
        pDeviceInfo->sDBName = m_ConfigRead.m_sDBName;
        pDeviceInfo->sPubServerIP = m_ConfigRead.m_sPubServerIP;
        pDeviceInfo->nPubServerPort = m_ConfigRead.m_nPubServerPort;
        pDeviceInfo->nSubServerPort = m_ConfigRead.m_nSubServerPort;
        pDeviceInfo->sRedisIP = m_ConfigRead.m_sRedisIP;
        pDeviceInfo->nRedisPort = m_ConfigRead.m_nRedisPort;
        pDeviceInfo->nLoadLimitTime = m_ConfigRead.m_nLoadLimitTime;
        pDeviceInfo->nServerTime = m_ConfigRead.m_nServerTime;
        pDeviceInfo->nLoadURL = m_ConfigRead.m_nLoadURL;

        MYSQL_ROW row = NULL;
        row = mysql_fetch_row(result);
        while (NULL != row)
        {
            pDeviceInfo->sLibID = row[0];
            if (m_mapLibInfo.find(pDeviceInfo->sLibID) == m_mapLibInfo.end())
            {
                CLibInfo * pLibInfo = new CLibInfo;
                printf("InitLib[%d][%d]...\n", nRowCount, nNum++);
                if (pLibInfo->Start(pDeviceInfo, m_pMysqlOperation, m_pZeromqManage, m_pRedisManage))
                {
                    m_mapLibInfo.insert(make_pair(pDeviceInfo->sLibID, pLibInfo));
                }
                else
                {
                    printf("***Warning: Checkpoint[%s] Start Failed.\n", pDeviceInfo->sLibID.c_str());
                }
            }
            else
            {
                printf("***Warning: Lib[%s] Repeat!\n");
            }
            
            row = mysql_fetch_row(result);
        }

        delete pDeviceInfo;
        pDeviceInfo = NULL;
    }
    mysql_free_result(result);
    return true;
}
//初始化Zeromq
bool CAnalyseServer::InitZeromq()
{
    if (!m_pZeromqManage->InitSub(NULL, 0, (char*)m_ConfigRead.m_sPubServerIP.c_str(), m_ConfigRead.m_nPubServerPort, ZeromqSubMsg, this, 1))
    {
        printf("****Error: 订阅[%s:%d:%s]失败!", (char*)m_ConfigRead.m_sPubServerIP.c_str(), m_ConfigRead.m_nPubServerPort, m_ConfigRead.m_sServerID.c_str());
        return false;
    }
    m_pZeromqManage->AddSubMessage((char *)m_ConfigRead.m_sServerID.c_str());
    m_pZeromqManage->AddSubMessage(HEARTBEATMSG);
    if (1 == m_nServerType)
    {
        m_pZeromqManage->AddSubMessage(SUBALLCHECKPOINT);   //搜索所有卡口订阅命令
        map<string, CLibInfo *>::iterator it = m_mapLibInfo.begin();
        for (; it != m_mapLibInfo.end(); it++)
        {
            m_pZeromqManage->AddSubMessage(it->first.c_str());
        }
    }
    else if (2 == m_nServerType)
    {
        m_pZeromqManage->AddSubMessage(SUBALLLIB);      //搜索所有核查库订阅命令
        char pSubAdd[MAXLEN] = { 0 };
        map<string, CLibInfo *>::iterator it = m_mapLibInfo.begin();
        for (; it != m_mapLibInfo.end(); it++)
        {
            m_pZeromqManage->AddSubMessage(it->first.c_str());
            sprintf(pSubAdd, "%sedit", it->first.c_str());
            m_pZeromqManage->AddSubMessage(pSubAdd);
        }
    }

    if (!m_pZeromqManage->InitPub(NULL, 0, (char*)m_ConfigRead.m_sPubServerIP.c_str(), m_ConfigRead.m_nSubServerPort))
    {
        printf("****Error: 初始化发布失败!");
        return false;
    }

    return true;
}
bool CAnalyseServer::InitRedis()
{
    if (NULL == m_pRedisManage)
    {
        m_pRedisManage = new CRedisManage;
    }

    if (!m_pRedisManage->InitRedis((char*)m_ConfigRead.m_sRedisIP.c_str(), m_ConfigRead.m_nRedisPort))
    {
        return false;
    }
    return true;
}
//Zeromq消息回调
void CAnalyseServer::ZeromqSubMsg(LPSUBMESSAGE pSubMessage, void * pUser)
{
    CAnalyseServer * pThis = (CAnalyseServer *)pUser;
    pThis->ParseZeromqJson(pSubMessage);
}
//Zeromq回调消息解析处理
int n = 0;
bool CAnalyseServer::ParseZeromqJson(LPSUBMESSAGE pSubMessage)
{
    int nRet = 0;
    //分发zmq消息
    if (string(pSubMessage->pHead) != m_ConfigRead.m_sServerID) //如果不是主程序订阅
    {
        //1. 判断是否心跳
        //add by zjf 2018/4/18 增加心跳, 防止zmq长时间无消息自动断连
        if (string(pSubMessage->pHead) == HEARTBEATMSG)
        {
            strcpy(pSubMessage->pHead, pSubMessage->pSource);
            strcpy(pSubMessage->pSource, m_ConfigRead.m_sServerID.c_str());
            m_pZeromqManage->PubMessage(pSubMessage);
        }
        //add end
        // add by zjf 2018/09/30
        //2. 判断是否是所有卡口|重点库都需要响应的消息
        else if (string(pSubMessage->pHead) == SUBALLCHECKPOINT || string(pSubMessage->pHead) == SUBALLLIB) //转发给所有卡口线程
        {
            printf("Recv: %s\n", pSubMessage->pHead);
            map<string, CLibInfo *>::iterator it = m_mapLibInfo.begin();
            for (; it != m_mapLibInfo.end(); it++)
            {
                it->second->ZeromqSubMsg(pSubMessage);
            }
        }
        //3. 判断是哪个库的消息
        else
        {
            map<string, CLibInfo *>::iterator it = m_mapLibInfo.find(pSubMessage->pHead);
            if (m_mapLibInfo.end() != it)   //判断是否哪一个卡口库或重点库的信息
            {
                it->second->ZeromqSubMsg(pSubMessage);
            }
            else if(2 == m_nServerType)     //判断是否重点库编辑信息(ex: 12keylibedit)
            {
                string sKeylib(pSubMessage->pHead);
                if (sKeylib.find("edit") != string::npos)
                {
                    sKeylib.assign(sKeylib, 0, sKeylib.size() - 4);
                    it = m_mapLibInfo.find(sKeylib);
                    if (m_mapLibInfo.end() != it)
                    {
                        it->second->ZeromqSubMsg(pSubMessage);
                    }
                }
            }
        }
        return true;
        //add end
    }

    string sCommand(pSubMessage->pOperationType);
    rapidjson::Document document;
    if (string(pSubMessage->pSubJsonValue) != "" && pSubMessage->pSubJsonValue[0] == '{')
    {
        document.Parse(pSubMessage->pSubJsonValue);
        if (document.HasParseError())
        {
            printf("***Warning: Parse Json Format Failed[%s].\n", pSubMessage->pSubJsonValue);
            SendResponseMsg(pSubMessage, JsonFormatError);
            return false;
        }
    }
    else
    {
        printf("***Warning: Parse Json Format Failed[%s].\n", pSubMessage->pSubJsonValue);
        SendResponseMsg(pSubMessage, JsonFormatError);
        return false;
    }
    if(sCommand == COMMANDADDLIB)        //增加卡口或重点库
    {
        if (document.HasMember(JSONDLIBID) && document[JSONDLIBID].IsString())
        {
            pSubMessage->nTaskType = ADDLIB;
            string sDeviceID = document[JSONDLIBID].GetString();
            map<string, CLibInfo *>::iterator it = m_mapLibInfo.find(sDeviceID);
            if (it == m_mapLibInfo.end())
            {
                LPDEVICEINFO pDeviceInfo = new DEVICEINFO;
                pDeviceInfo->nServerType = m_nServerType;
                pDeviceInfo->sLibID = sDeviceID;
                pDeviceInfo->nMaxCount = m_ConfigRead.m_nMaxCount;
                pDeviceInfo->sDBIP = m_ConfigRead.m_sDBIP;
                pDeviceInfo->nDBPort = m_ConfigRead.m_nDBPort;
                pDeviceInfo->sDBUser = m_ConfigRead.m_sDBUser;
                pDeviceInfo->sDBPassword = m_ConfigRead.m_sDBPd;
                pDeviceInfo->sDBName = m_ConfigRead.m_sDBName;
                pDeviceInfo->sPubServerIP = m_ConfigRead.m_sPubServerIP;
                pDeviceInfo->nPubServerPort = m_ConfigRead.m_nPubServerPort;
                pDeviceInfo->nSubServerPort = m_ConfigRead.m_nSubServerPort;
                pDeviceInfo->sRedisIP = m_ConfigRead.m_sRedisIP;
                pDeviceInfo->nRedisPort = m_ConfigRead.m_nRedisPort;
                pDeviceInfo->nServerTime = m_ConfigRead.m_nServerTime;
                pDeviceInfo->nLoadURL = m_ConfigRead.m_nLoadURL;

                CLibInfo * pLibInfo = new CLibInfo;
                if (pLibInfo->Start(pDeviceInfo, m_pMysqlOperation, m_pZeromqManage, m_pRedisManage))
                {
                    if (!AddCheckpointToDB((char*)pDeviceInfo->sLibID.c_str()))
                    {
                        pLibInfo->Stop();
                        delete pLibInfo;
                        pLibInfo = NULL;
                        delete pDeviceInfo;
                        pDeviceInfo = NULL;
                        nRet = MysqlQueryFailed;
                    }
                    else
                    {
                        m_mapLibInfo.insert(make_pair(pDeviceInfo->sLibID, pLibInfo));
                    }
                }
                else
                {
                    printf("***Warning: Checkpoint[%s] Start Failed.\n", pDeviceInfo->sLibID.c_str());
                    delete pLibInfo;
                    pLibInfo = NULL;
                    delete pDeviceInfo;
                    pDeviceInfo = NULL;
                    nRet = CheckpointInitFailed;
                }
            }
            else
            {
                printf("***Warning: Add Checkpoint[%s] Aleady Exists!\n", sDeviceID.c_str());
                nRet = LibAleadyExist;
            }

            //回应消息用到
            strcpy(pSubMessage->pDeviceID, sDeviceID.c_str());
        }
        else
        {
            nRet = JsonFormatError;
        }
    }
    else if (sCommand == COMMANDDELLIB)  //删除卡口或重点库
    {
        if (document.HasMember(JSONDLIBID) && document[JSONDLIBID].IsString())
        {
            pSubMessage->nTaskType = DELLIB;
            string sDeviceID = document[JSONDLIBID].GetString();
            map<string, CLibInfo *>::iterator it = m_mapLibInfo.find(sDeviceID);
            if (it == m_mapLibInfo.end())
            {
                printf("***Warning: Del Checkpoint[%s] Not Exist!\n", sDeviceID.c_str());
                nRet = LibNotExist;
            }
            else
            {
                if (!AddCheckpointToDB((char*)sDeviceID.c_str(), false))
                {
                    nRet = MysqlQueryFailed;
                }
                else
                {
                    it->second->Stop();
                    printf("####Lib[%s] Stop End.\n", it->first.c_str());
                    delete it->second;
                    m_mapLibInfo.erase(it);
                }
            }
            //回应消息用到
            strcpy(pSubMessage->pDeviceID, sDeviceID.c_str());
        }
        else
        {
            nRet = JsonFormatError;
        }
    }
    else
    {
        nRet = CommandNotFound;
    }

    strcpy(pSubMessage->pHead, pSubMessage->pSource);
    strcpy(pSubMessage->pSource, m_ConfigRead.m_sServerID.c_str());
    SendResponseMsg(pSubMessage, nRet);

    return true;
}
bool CAnalyseServer::AddCheckpointToDB(char * pDeviceID, bool bAdd)
{
    bool bRet = true;
    char pSQL[SQLMAXLEN] = { 0 };
    if (!bAdd)
    {
        sprintf(pSQL, "delete from %s where libname = '%s'", LIBINFOTABLE, pDeviceID);
    }
    else
    {
        sprintf(pSQL, "insert into %s(libname, type) values('%s', %d)", LIBINFOTABLE, pDeviceID, m_nServerType);

    }

    if (!m_pMysqlOperation->QueryCommand(pSQL))
    {
        bRet = false;
    }
    else
    {
        if (!bAdd && 2 == m_nServerType)
        {
            sprintf(pSQL, "drop table %s", pDeviceID);
            if (!m_pMysqlOperation->QueryCommand(pSQL))
            {
                bRet = false;
            }
        }
    }
    return bRet;
}
//回应请求
void CAnalyseServer::SendResponseMsg(LPSUBMESSAGE pSubMessage, int nEvent)
{
    char pErrorMsg[64] = {0};
    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType&allocator = document.GetAllocator();
    rapidjson::Value array(rapidjson::kArrayType);
    if(nEvent == 0)
    {
        document.AddMember("result", rapidjson::StringRef("success"), allocator);
        document.AddMember(JSONDLIBID, rapidjson::StringRef(pSubMessage->pDeviceID), allocator);
    }
    else
    {
        GetErrorMsg((ErrorCode)nEvent, pErrorMsg);
        document.AddMember("result", rapidjson::StringRef("error"), allocator);
        if (pSubMessage->nTaskType == ADDLIB || pSubMessage->nTaskType == DELLIB)
        {
            document.AddMember(JSONDLIBID, rapidjson::StringRef(pSubMessage->pDeviceID), allocator);
        }
        document.AddMember("errorcode", nEvent, allocator);
        document.AddMember("errorMessage", rapidjson::StringRef(pErrorMsg), allocator);
    }
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    pSubMessage->sPubJsonValue = buffer.GetString();
    //发布回应消息
    m_pZeromqManage->PubMessage(pSubMessage);
    printf("Rep Client json:\n%s\n==============================\n", pSubMessage->sPubJsonValue.c_str());
    return;
}
#ifdef __WINDOWS__
DWORD WINAPI CAnalyseServer::PubStatusThread(void * pParam)
{
    CAnalyseServer * pThis = (CAnalyseServer *)pParam;
    pThis->PubStatusAction();
    return 0;
}
#else
void * CAnalyseServer::PubStatusThread(void * pParam)
{
    CAnalyseServer * pThis = (CAnalyseServer *)pParam;
    pThis->PubStatusAction();
    return NULL;
}
#endif
void CAnalyseServer::PubStatusAction()
{
    bool bUpdatePastCount = false;  //是否己插入当天之前的每天采集特征值数量, 插入一次后不用再更新, 只需要更新当前日期
    char pTableName[MAXLEN * 2] = { 0 };
    char pKey[MAXLEN] = { 0 };
    char pValue[MAXLEN] = { 0 };

#ifdef __WINDOWS__
    Sleep(1000 * 2);
#else
    sleep(2);
    fd_set fdsr;
    FD_ZERO(&fdsr);
    FD_SET(m_nPipe[0], &fdsr);
    struct timeval tv;
#endif

    LPSUBMESSAGE pSubMessage = new SUBMESSAGE;
    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType&allocator = document.GetAllocator();
    char pProgress[10] = { 0 };

    do
    {
        map<string, CLibInfo *>::iterator it = m_mapLibInfo.begin();
        for (; it != m_mapLibInfo.end(); it++)
        {
            //发布卡口状态消息给检索服务, 用于检索任务超时检查
            m_pZeromqManage->PubMessage(DEVICESTATUS, (char *)it->second->m_pDeviceInfo->sLibID.c_str());
            //printf("pub status: %s\n", m_pDeviceInfo->sLibID.c_str());

            if (1 == m_nServerType)
            {
                //推送卡口总特征值数量和当天特征值数量到Redis, 用于web页面统计抓拍数量
                int nTotalCount = it->second->m_pFeatureManage->GetTotalNum();
#ifdef __WINDOWS__
                map<acl::string, acl::string> mapValues;
#else
                map<string, string> mapValues;
#endif
                sprintf(pTableName, "%s.count", it->second->m_pDeviceInfo->sLibID.c_str());
                sprintf(pValue, "%d", nTotalCount);
                mapValues.insert(make_pair("totalcount", pValue));

                if (it->second->m_pFeatureManage->m_mapDayCount.size() > 0)
                {
                    if (!bUpdatePastCount)
                    {
                        for (map<int64_t, int>::iterator itDay = it->second->m_pFeatureManage->m_mapDayCount.begin();
                            itDay != it->second->m_pFeatureManage->m_mapDayCount.end(); itDay++)
                        {
                            sprintf(pKey, "%lld", itDay->first);
                            sprintf(pValue, "%d", itDay->second);
                            mapValues.insert(make_pair(pKey, pValue));
                            //printf("[%s]%s %d\n", m_pDeviceInfo->sLibID.c_str(), ChangeSecondToTime(it->first / 1000).c_str(), it->second);
                        }
                        bUpdatePastCount = true;
                    }
                    else
                    {
                        map<int64_t, int>::iterator itToday = it->second->m_pFeatureManage->m_mapDayCount.end();
                        itToday--;
                        sprintf(pKey, "%lld", itToday->first);
                        sprintf(pValue, "%d", itToday->second);
                        mapValues.insert(make_pair(pKey, pValue));
                        //printf("[%s]%s %d\n", m_pDeviceInfo->sLibID.c_str(), ChangeSecondToTime(it->first / 1000).c_str(), it->second);
                    }
                }

                m_pRedisManage->InsertHashValue(pTableName, mapValues);
                //printf("[%s]all Record: %d, Current Day Record: %d\n", m_pDeviceInfo->sLibID.c_str(), nTotalCount, nDayCount);
            }
        }
        //printf("Send Status again\n");
       
#ifdef __WINDOWS__
    } while (WAIT_TIMEOUT == WaitForSingleObject(m_hStopEvent, 14 * 1000));
#else
        //等待30s, 无信号则下一次循环, 有信号(只有需要退出线程时才有信号)则退出线程
        tv.tv_sec = 14;
        tv.tv_usec = 0;
        fd_set fdPipe = fdsr;
        if (0 != select(m_nPipe[0] + 1, &fdPipe, NULL, NULL, &tv)) //阻塞30s  
        {
            break;
        }
        //usleep(1000 * 1000 * 14);
    }
    while (true);
#endif
    printf("thread: pub status thread end!\n...");
}
//获取错误码说明
int CAnalyseServer::GetErrorMsg(ErrorCode nError, char * pMsg)
{
    int nRet = 0;
    switch(nError)
    {
    case ServerNotInit:
        strcpy(pMsg, "SERVER_NOT_INIT");
        break;
    case DBAleadyExist:
        strcpy(pMsg, "DB_ALEADY_EXIST");
        break;
    case DBNotExist:
        strcpy(pMsg, "DB_NOT_EXIST");
        break;
    case FaceUUIDAleadyExist:
        strcpy(pMsg, "FACEUUID_ALEADY_EXIST");
        break;
    case FaceUUIDNotExist:
        strcpy(pMsg, "FACEUUID_NOT_EXIST");
        break;
    case ParamIllegal:
        strcpy(pMsg, "PARAM_ILLEGAL");
        break;
    case NewFailed:
        strcpy(pMsg, "NEW_FAILED");
        break;
    case JsonFormatError:
        strcpy(pMsg, "JSON_FORMAT_ERROR");
        break;
    case CommandNotFound:
        strcpy(pMsg, "COMMAND_NOT_FOUND");
        break;
    case HttpMsgUpperLimit:
        strcpy(pMsg, "HTTPMSG_UPPERLIMIT");
        break;
    case PthreadMutexInitFailed:
        strcpy(pMsg, "PTHREAD_MUTEX_INIT_FAILED");
        break;
    case FeatureNumOverMax:
        strcpy(pMsg, "FEATURE_NUM_OVER_MAX");
        break;
    case JsonParamIllegal:
        strcpy(pMsg, "JSON_PARAM_ILLEGAL");
        break;
    case MysqlQueryFailed:
        strcpy(pMsg, "MYSQL_QUERY_FAILED");
        break;
    case ParamLenOverMax:
        strcpy(pMsg, "PARAM_LEN_OVER_MAX");
        break;
    case LibAleadyExist:
        strcpy(pMsg, "LIB_ALEADY_EXIST");
        break;
    case LibNotExist:
        strcpy(pMsg, "LIB_NOT_EXIST");
        break;
    case CheckpointInitFailed:
        strcpy(pMsg, "CHECKPOINT_INIT_FAILED");
        break;
    case VerifyFailed:
        strcpy(pMsg, "VERIFY_FAILED");
        break;
    case HttpSocketBindFailed:
        strcpy(pMsg, "HTTP_SOCKET_BIND_FAILED");
        break;
    case CreateTableFailed:
        strcpy(pMsg, "CREATE_TABLE_FAILED");
        break;
    case SearchTimeWrong:
        strcpy(pMsg, "SEARCH_TIME_WRONG");
        break;
    case SearchNoDataOnTime:
        strcpy(pMsg, "SEARCH_NO_DATA_ON_TIME");
        break;
    case AddFeatureToCheckpointFailed:
        strcpy(pMsg, "ADD_FEATURE_TO_CHECKPOINT_FAILED");
        break;
    case SocketInitFailed:
        strcpy(pMsg, "SOCKET_INIT_FAILED");
        break;
    default:
        nRet = -1;
        break;
    }

    return nRet;
}
