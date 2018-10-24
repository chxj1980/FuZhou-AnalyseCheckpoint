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
    //��ʼ���ٽ���
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
//��ʼ����
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
//ֹͣ����
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
//��ʼ������
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

    //��ʼ���ȶԽӿ�
    printf("��ʼ��FRSLib...\n");
    int nRet = Initialize(29);
    if (1 != nRet)
    {
        printf("****Error: FRSLib Initialize Failed, ErrorCode: %d.\n", nRet);
        return false;
    }
#endif

    //��ȡ�����ļ�
    if(!m_ConfigRead.ReadConfig(m_nServerType))
    {
        printf("****Error: Read Config File Failed.\n");
        return false;
    }

    if (string(m_ConfigRead.m_sServerID, 10, 3) == "251")
    {
        m_nServerType = 1;  //���ڹ�����������
        printf("===��ȡ����ֵʱ��: %d��.\n"
                "===ͼƬʱ���Ƿ񱣴�Ϊ����ʱ��: %d\n"
                "===�Ƿ��ȡͼƬURL: %d.\n",
            m_ConfigRead.m_nLoadLimitTime, m_ConfigRead.m_nServerTime, m_ConfigRead.m_nLoadURL);
    }
    else if (string(m_ConfigRead.m_sServerID, 10, 3) == "252")
    {
        m_nServerType = 2;  //�˲��������
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
    //��ʼ��Redis����
    if (!InitRedis())
    {
        return false;
    }
    //��ʼ��Zeromq
    if (!InitZeromq())
    {
        return false;
    }
#ifdef __WINDOWS__
    m_hThreadPubStatus = CreateThread(NULL, 0, PubStatusThread, this, NULL, 0);   //������״̬�߳�
#else
    pthread_create(&m_hThreadPubStatus, NULL, PubStatusThread, (void*)this);
#endif
    printf("--Pub Status Thread Start.\n");
    return true;
}
//��ȡ����Ϣ
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
    if (1 == m_nServerType) //���ڷ�������, ֻȡ���ڿ���Ϣ
    {
        sprintf(pSQL, "select libname from %s where type = 1 order by libname", LIBINFOTABLE);
    }
    else                  //�˲��������, ȡ�˲��Ͳ��ؿ���Ϣ
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
//��ʼ��Zeromq
bool CAnalyseServer::InitZeromq()
{
    if (!m_pZeromqManage->InitSub(NULL, 0, (char*)m_ConfigRead.m_sPubServerIP.c_str(), m_ConfigRead.m_nPubServerPort, ZeromqSubMsg, this, 1))
    {
        printf("****Error: ����[%s:%d:%s]ʧ��!", (char*)m_ConfigRead.m_sPubServerIP.c_str(), m_ConfigRead.m_nPubServerPort, m_ConfigRead.m_sServerID.c_str());
        return false;
    }
    m_pZeromqManage->AddSubMessage((char *)m_ConfigRead.m_sServerID.c_str());
    m_pZeromqManage->AddSubMessage(HEARTBEATMSG);
    if (1 == m_nServerType)
    {
        m_pZeromqManage->AddSubMessage(SUBALLCHECKPOINT);   //�������п��ڶ�������
        map<string, CLibInfo *>::iterator it = m_mapLibInfo.begin();
        for (; it != m_mapLibInfo.end(); it++)
        {
            m_pZeromqManage->AddSubMessage(it->first.c_str());
        }
    }
    else if (2 == m_nServerType)
    {
        m_pZeromqManage->AddSubMessage(SUBALLLIB);      //�������к˲�ⶩ������
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
        printf("****Error: ��ʼ������ʧ��!");
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
//Zeromq��Ϣ�ص�
void CAnalyseServer::ZeromqSubMsg(LPSUBMESSAGE pSubMessage, void * pUser)
{
    CAnalyseServer * pThis = (CAnalyseServer *)pUser;
    pThis->ParseZeromqJson(pSubMessage);
}
//Zeromq�ص���Ϣ��������
int n = 0;
bool CAnalyseServer::ParseZeromqJson(LPSUBMESSAGE pSubMessage)
{
    int nRet = 0;
    //�ַ�zmq��Ϣ
    if (string(pSubMessage->pHead) != m_ConfigRead.m_sServerID) //���������������
    {
        //1. �ж��Ƿ�����
        //add by zjf 2018/4/18 ��������, ��ֹzmq��ʱ������Ϣ�Զ�����
        if (string(pSubMessage->pHead) == HEARTBEATMSG)
        {
            strcpy(pSubMessage->pHead, pSubMessage->pSource);
            strcpy(pSubMessage->pSource, m_ConfigRead.m_sServerID.c_str());
            m_pZeromqManage->PubMessage(pSubMessage);
        }
        //add end
        // add by zjf 2018/09/30
        //2. �ж��Ƿ������п���|�ص�ⶼ��Ҫ��Ӧ����Ϣ
        else if (string(pSubMessage->pHead) == SUBALLCHECKPOINT || string(pSubMessage->pHead) == SUBALLLIB) //ת�������п����߳�
        {
            printf("Recv: %s\n", pSubMessage->pHead);
            map<string, CLibInfo *>::iterator it = m_mapLibInfo.begin();
            for (; it != m_mapLibInfo.end(); it++)
            {
                it->second->ZeromqSubMsg(pSubMessage);
            }
        }
        //3. �ж����ĸ������Ϣ
        else
        {
            map<string, CLibInfo *>::iterator it = m_mapLibInfo.find(pSubMessage->pHead);
            if (m_mapLibInfo.end() != it)   //�ж��Ƿ���һ�����ڿ���ص�����Ϣ
            {
                it->second->ZeromqSubMsg(pSubMessage);
            }
            else if(2 == m_nServerType)     //�ж��Ƿ��ص��༭��Ϣ(ex: 12keylibedit)
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
    if(sCommand == COMMANDADDLIB)        //���ӿ��ڻ��ص��
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

            //��Ӧ��Ϣ�õ�
            strcpy(pSubMessage->pDeviceID, sDeviceID.c_str());
        }
        else
        {
            nRet = JsonFormatError;
        }
    }
    else if (sCommand == COMMANDDELLIB)  //ɾ�����ڻ��ص��
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
            //��Ӧ��Ϣ�õ�
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
//��Ӧ����
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
    //������Ӧ��Ϣ
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
    bool bUpdatePastCount = false;  //�Ƿ񼺲��뵱��֮ǰ��ÿ��ɼ�����ֵ����, ����һ�κ����ٸ���, ֻ��Ҫ���µ�ǰ����
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
            //��������״̬��Ϣ����������, ���ڼ�������ʱ���
            m_pZeromqManage->PubMessage(DEVICESTATUS, (char *)it->second->m_pDeviceInfo->sLibID.c_str());
            //printf("pub status: %s\n", m_pDeviceInfo->sLibID.c_str());

            if (1 == m_nServerType)
            {
                //���Ϳ���������ֵ�����͵�������ֵ������Redis, ����webҳ��ͳ��ץ������
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
        //�ȴ�30s, ���ź�����һ��ѭ��, ���ź�(ֻ����Ҫ�˳��߳�ʱ�����ź�)���˳��߳�
        tv.tv_sec = 14;
        tv.tv_usec = 0;
        fd_set fdPipe = fdsr;
        if (0 != select(m_nPipe[0] + 1, &fdPipe, NULL, NULL, &tv)) //����30s  
        {
            break;
        }
        //usleep(1000 * 1000 * 14);
    }
    while (true);
#endif
    printf("thread: pub status thread end!\n...");
}
//��ȡ������˵��
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
