#include "stdafx.h"

#include "LibInfo.h"
#include "stdio.h"
#include "time.h"

CLibInfo::CLibInfo()
{
    m_pFeatureManage = NULL;
    m_pZeromqManage = NULL;
    m_pDeviceInfo = NULL;
    m_pRedisManage = NULL;
    m_pMysqlOperation = NULL;
    m_bStopLib = false;
#ifdef __WINDOWS__
    InitializeCriticalSection(&m_cs);
    m_hSubMsgEvent = CreateEvent(NULL, true, false, NULL);
    m_hThreadPubMessageCommand = INVALID_HANDLE_VALUE;
#else
    pthread_mutex_init(&m_mutex, NULL);
    pipe(m_nPipe);
    m_hThreadPubMessageCommand = -1;
#endif
}
CLibInfo::~CLibInfo()
{
#ifdef __WINDOWS__
    DeleteCriticalSection(&m_cs);
    CloseHandle(m_hSubMsgEvent);
#else
    pthread_mutex_destroy(&m_mutex);
    close(m_nPipe[0]);
    close(m_nPipe[1]);
#endif
}
//开始服务
bool CLibInfo::Start(LPDEVICEINFO pDeviceInfo, CMySQLOperation * pMysqlOperation, CZeromqManage * pZeromqManage, CRedisManage * pRedisManage)
{
    m_pDeviceInfo = new DEVICEINFO;
    m_pDeviceInfo->nServerType = pDeviceInfo->nServerType;
    m_pDeviceInfo->sLibID = pDeviceInfo->sLibID;
    m_pDeviceInfo->nMaxCount = pDeviceInfo->nMaxCount;
    m_pDeviceInfo->sDBIP = pDeviceInfo->sDBIP;
    m_pDeviceInfo->nDBPort = pDeviceInfo->nDBPort;
    m_pDeviceInfo->sDBName = pDeviceInfo->sDBName;
    m_pDeviceInfo->sDBUser = pDeviceInfo->sDBUser;
    m_pDeviceInfo->sDBPassword = pDeviceInfo->sDBPassword;
    m_pDeviceInfo->sPubServerIP = pDeviceInfo->sPubServerIP;
    m_pDeviceInfo->nPubServerPort = pDeviceInfo->nPubServerPort;
    m_pDeviceInfo->nSubServerPort = pDeviceInfo->nSubServerPort;
    m_pDeviceInfo->sRedisIP = pDeviceInfo->sRedisIP;
    m_pDeviceInfo->nRedisPort = pDeviceInfo->nRedisPort;
    m_pDeviceInfo->nLoadLimitTime = pDeviceInfo->nLoadLimitTime;
    m_pDeviceInfo->nServerTime = pDeviceInfo->nServerTime;
    m_pDeviceInfo->nLoadURL = pDeviceInfo->nLoadURL;

    m_pMysqlOperation = pMysqlOperation;
    m_pZeromqManage = pZeromqManage;
    m_pRedisManage = pRedisManage;

    if (!Init())
    {
        printf("***Warning: StartLibInfo[%s]::Init Failed.\n", m_pDeviceInfo->sLibID.c_str());
        Stop();
        return false;
    }
    return true;
}
//停止服务
bool CLibInfo::Stop()
{
    printf("--Stop LibInfo[%s].\n", m_pDeviceInfo->sLibID.c_str());
    m_bStopLib = true;
#ifdef __WINDOWS__
    SetEvent(m_hSubMsgEvent);
    if (INVALID_HANDLE_VALUE != m_hThreadPubMessageCommand)
    {
        WaitForSingleObject(m_hThreadPubMessageCommand, INFINITE);
    }
#else
    write(m_nPipe[1], "1", 1);
    if (-1 != m_hThreadPubMessageCommand)
    {
        pthread_join(m_hThreadPubMessageCommand, NULL);
    }
#endif
    EnterMutex();
    while (m_listPubMessage.size() > 0)
    {
        delete m_listPubMessage.front();
        m_listPubMessage.pop_front();
    }
    LeaveMutex();
    if (NULL != m_pFeatureManage)
    {
        m_pFeatureManage->UnInit();
        delete m_pFeatureManage;
        m_pFeatureManage = NULL;
    }
    if (NULL != m_pDeviceInfo)
    {
        delete m_pDeviceInfo;
        m_pDeviceInfo = NULL;
    }

    return true;
}
//初始化服务
bool CLibInfo::Init()
{
    printf("---------------\nLib [%s] Init...\n", m_pDeviceInfo->sLibID.c_str());
    if (!InitFeatureManage())
    {
        printf("***Warning: InitFeatureManage Failed.\n");
        return false;
    }
    //初始化Zeromq
    /*if (!InitZeromq())
    {
        return false;
    }*/

#ifdef __WINDOWS__
    m_hThreadPubMessageCommand = CreateThread(NULL, 0, PubMessageCommandThread, this, NULL, 0);   //sub消息处理
#else
    pthread_create(&m_hThreadPubMessageCommand, NULL, PubMessageCommandThread, (void*)this);
#endif
    printf("--Pub Message Command Thread Start.\n");

    printf("---------------Lib [%s] Init End.\n", m_pDeviceInfo->sLibID.c_str());
    return true;
}

bool CLibInfo::InitFeatureManage()
{
    int nRet = INVALIDERROR;
    m_pFeatureManage = new CFeatureManage;
    if (NULL == m_pFeatureManage)
    {
        printf("***Warning: new CFeatureManage Failed.\n");
        return false;
    }
    else
    {
        nRet = m_pFeatureManage->Init((char*)m_pDeviceInfo->sLibID.c_str(), m_pDeviceInfo->nServerType, m_pDeviceInfo->nMaxCount, SearchTaskCallback, this);
        if (nRet < 0)
        {
            printf("***Warning: Checkpoint[%s] Init Failed.\n", m_pDeviceInfo->sLibID.c_str());
            m_pFeatureManage->UnInit();
            delete m_pFeatureManage;
            m_pFeatureManage = NULL;
            return false;
        }
    }

    //获取库特征值信息
    if (1 == m_pDeviceInfo->nServerType)
    {
        if (!GetCheckpointFeatureFromDB())
        {
            printf("***Warning: CLibInfo::GetCheckpointFeatureFromDB Failed.\n");
            return false;
        }
    }
    else
    {
        if (!GetKeyLibFeatureFromDB())
        {
            printf("***Warning: CKeyLibInfo::GetCheckpointFeatureFromDB Failed.\n");
            return false;
        }
    }

    return true;
}
bool CLibInfo::GetCheckpointFeatureFromDB()
{
    char pSQL[SQLMAXLEN] = { 0 };
    int nRet = INVALIDERROR;

    //增加卡口特征值表 
    if (m_pDeviceInfo->nLoadURL)
    {
        sprintf(pSQL, "create table `%s%s`(faceuuid varchar(%d) not null, feature blob(%d), ftlen int(10), "
            "time bigint(20), imagedisk varchar(1), imageserverip varchar(20), facerect varchar(20), face_url varchar(2048), bkg_url varchar(2048), ",
            "PRIMARY KEY (faceuuid), UNIQUE INDEX index_faceuuid (faceuuid) USING BTREE,  INDEX index_time (time) USING BTREE)",
            CHECKPOINTPREFIX, m_pDeviceInfo->sLibID.c_str(), MAXLEN, FEATURELEN);
    }
    else
    {
        sprintf(pSQL, "create table `%s%s`(faceuuid varchar(%d) not null, feature blob(%d), ftlen int(10), "
            "time bigint(20), imagedisk varchar(1), imageserverip varchar(20), facerect varchar(20), "
            "PRIMARY KEY (faceuuid), UNIQUE INDEX index_faceuuid (faceuuid) USING BTREE,  INDEX index_time (time) USING BTREE)",
            CHECKPOINTPREFIX, m_pDeviceInfo->sLibID.c_str(), MAXLEN, FEATURELEN);
    }

    if (!m_pMysqlOperation->QueryCommand(pSQL))
    {
        return false;
    }

    {
        sprintf(pSQL, "select count(*) from %s%s", CHECKPOINTPREFIX, m_pDeviceInfo->sLibID.c_str());
        MYSQL_RES * result = NULL;
        if (!m_pMysqlOperation->GetQueryResult(pSQL, result))
        {
            return false;
        }
        int nRowCount = mysql_num_rows(result);
        if (nRowCount > 0)
        {
            MYSQL_ROW row = NULL;
            row = mysql_fetch_row(result);
            if (NULL != row)
            {
                m_pFeatureManage->m_nTotalCount = atoi(row[0]);
                printf("Checkpoint[%s] Total Count: %d\n", m_pDeviceInfo->sLibID.c_str(), m_pFeatureManage->m_nTotalCount);
            }
        }
        mysql_free_result(result);
    }

    //获取卡口特征值信息
    long ts = time(NULL);
    int64_t tLimit = ts;
    int64_t t = ONEDAYTIME;
    int64_t Loadtime = m_pDeviceInfo->nLoadLimitTime;
    tLimit = tLimit * 1000 - t * Loadtime;

    if (m_pDeviceInfo->nLoadURL)   //是否获取图片URL
    {
        sprintf(pSQL, "select faceuuid, feature, ftlen, time, imagedisk, imageserverip, facerect, face_url, bkg_url from `%s%s` "
            "where face_url is not null and time > %lld order by time",
            //sprintf(pSQL, "select faceuuid, feature, ftlen, time, imagedisk, imageserverip, facerect from %s%s where ftlen > 1000 order by time",
            CHECKPOINTPREFIX, m_pDeviceInfo->sLibID.c_str(), tLimit);
    }
    else
    {
         sprintf(pSQL, "select faceuuid, feature, ftlen, time, imagedisk, imageserverip, facerect from %s%s order by time",
                        CHECKPOINTPREFIX, m_pDeviceInfo->sLibID.c_str());
    }
    
    
    MYSQL_RES * result = NULL;
    if(!m_pMysqlOperation->GetQueryResult(pSQL, result))
    {
        return false;
    }
    int nRowCount = mysql_num_rows(result);
    printf("Checkpoint[%s] Record Read: %d.\n", m_pDeviceInfo->sLibID.c_str(), nRowCount);

    if (nRowCount > 0)
    {
        MYSQL_ROW row = NULL;
        row = mysql_fetch_row(result);
        char pFaceUUID[MAXLEN] = { 0 };
        char pFeature[FEATURELEN] = { 0 };
        int nFtLen = 0;
        int64_t nCollectTime = 0;
        char pDisk[MAXLEN] = { 0 };
        char pImageServerIP[MAXLEN] = { 0 };
        char pFaceRect[MAXLEN] = { 0 };
        char pFaceURL[2048] = { 0 };
        char pBkgURL[2048] = { 0 };
        while (NULL != row)
        {
            if (NULL == row[0] || NULL == row[1] || NULL == row[2] || NULL == row[3] || NULL == row[4] || NULL == row[5] || NULL == row[6])
            {
                printf("***Warning: GetCheckpointFeatureFromDB::卡口[%s]特征值表有数据为空!\n", m_pDeviceInfo->sLibID.c_str());
            }
            else
            {
                strcpy(pFaceUUID, row[0]);
                nFtLen = atoi(row[2]);
                memcpy(pFeature, row[1], nFtLen);
#ifdef __WINDOWS__
                nCollectTime = _atoi64(row[3]);
#else
                nCollectTime = strtol(row[3], NULL, 10);
#endif
                strcpy(pDisk, row[4]);
                strcpy(pImageServerIP, row[5]);
                strcpy(pFaceRect, row[6]);

                FeatureData * pFeatureData = new FeatureData;
                //1. 保存FaceUUID
                int nLen = strlen(pFaceUUID);
                pFeatureData->FaceUUID = new char[nLen + 1];
                strcpy(pFeatureData->FaceUUID, pFaceUUID);
                pFeatureData->FaceUUID[nLen] = '\0';
                //2. 保存特征值
                pFeatureData->FeatureValue = new unsigned char[nFtLen];
                memcpy(pFeatureData->FeatureValue, pFeature, nFtLen);
                pFeatureData->FeatureValueLen = nFtLen;
                //3. 保存特征值时间
                pFeatureData->CollectTime = nCollectTime;
                //4. 保存盘符
                strcpy(pFeatureData->pDisk, pDisk);
                //5. 保存图片保存服务器IP
                nLen = strlen(pImageServerIP);
                pFeatureData->pImageIP = new char[nLen + 1];
                strcpy(pFeatureData->pImageIP, pImageServerIP);
                pFeatureData->pImageIP[nLen] = '\0';
                //6. 保存人脸图片坐标
                nLen = strlen(pFaceRect);
                pFeatureData->pFaceRect = new char[nLen + 1];
                strcpy(pFeatureData->pFaceRect, pFaceRect);
                pFeatureData->pFaceRect[nLen] = '\0';

                //7.  保存人脸图片URL及背景URL
                if (m_pDeviceInfo->nLoadURL && NULL != row[7] && NULL != row[8])
                {
                    strcpy(pFaceURL, row[7]);
                    strcpy(pBkgURL, row[8]);

                    nLen = strlen(pFaceURL);
                    pFeatureData->pFaceURL = new char[nLen + 1];
                    strcpy(pFeatureData->pFaceURL, pFaceURL);
                    pFeatureData->pFaceURL[nLen] = '\0';

                    nLen = strlen(pBkgURL);
                    pFeatureData->pBkgURL = new char[nLen + 1];
                    strcpy(pFeatureData->pBkgURL, pBkgURL);
                    pFeatureData->pBkgURL[nLen] = '\0';
                }
                
                //生成特征值链表
                m_pFeatureManage->AddFeature(pFeatureData, true);
            }
            
            row = mysql_fetch_row(result);
        }
    }
    printf("---Success: Add Checkpoint[%s] Feature End, Num: %d\n", m_pDeviceInfo->sLibID.c_str(), m_pFeatureManage->GetTotalNum());
    mysql_free_result(result);

    unsigned long long tBegin = 0;
    unsigned long long tEnd = 0;
    m_pFeatureManage->GetDataDate(tBegin, tEnd);
    struct tm *tmBegin = localtime((time_t*)&(tBegin));
    char pBegin[20] = { 0 };
    strftime(pBegin, sizeof(pBegin), "%Y-%m-%d %H:%M:%S", tmBegin);

    struct tm *tmEnd = localtime((time_t*)&(tEnd));
    char pEnd[20] = { 0 };
    strftime(pEnd, sizeof(pEnd), "%Y-%m-%d %H:%M:%S", tmEnd);
    printf("Time[%s  %s]\n", pBegin, pEnd);

    return true;
}
bool CLibInfo::GetKeyLibFeatureFromDB()
{
    char pSQL[SQLMAXLEN] = { 0 };
    int nRet = INVALIDERROR;

    //增加重点库特征值表
    if (m_pDeviceInfo->nLoadURL)
    {
        sprintf(pSQL,
            "create table %s(faceuuid varchar(%d) not null, feature blob(%d), ftlen int(10), time bigint(20), "
            "imagedisk varchar(1), imageserverip varchar(20), face_url varchar(2048), ",
            "PRIMARY KEY (faceuuid), UNIQUE INDEX index_faceuuid (faceuuid) USING BTREE)",
            m_pDeviceInfo->sLibID.c_str(), MAXLEN, FEATURELEN);
    }
    else
    {
        sprintf(pSQL,
            "create table %s(faceuuid varchar(%d) not null, feature blob(%d), ftlen int(10), time bigint(20), "
            "imagedisk varchar(1), imageserverip varchar(20), ",
            "PRIMARY KEY (faceuuid), UNIQUE INDEX index_faceuuid (faceuuid) USING BTREE)",
            m_pDeviceInfo->sLibID.c_str(), MAXLEN, FEATURELEN);
    }
    
    if (!m_pMysqlOperation->QueryCommand(pSQL))
    {
        return false;
    }

    //获取重点库特征值信息
    int nRead = 0;

    if (m_pDeviceInfo->nLoadURL)
    {
        sprintf(pSQL, "select faceuuid, feature, ftlen, imagedisk, imageserverip, face_url from %s", m_pDeviceInfo->sLibID.c_str());
    }
    else
    {
        sprintf(pSQL, "select faceuuid, feature, ftlen, imagedisk, imageserverip from %s", m_pDeviceInfo->sLibID.c_str());
    }
    MYSQL_RES * result = NULL;
    if (!m_pMysqlOperation->GetQueryResult(pSQL, result))
    {
        return false;
    }
    int nRowCount = mysql_num_rows(result);
    m_pFeatureManage->m_nTotalCount = nRowCount;
    printf("Keylib[%s] Record Total: %d.\n", m_pDeviceInfo->sLibID.c_str(), nRowCount);

    if (nRowCount > 0)
    {
        MYSQL_ROW row = NULL;
        row = mysql_fetch_row(result);
        char pFaceUUID[MAXLEN] = { 0 };
        char pFeature[FEATURELEN] = { 0 };
        int nFtLen = 0;
        char pDisk[MAXLEN] = { 0 };
        char pImageServerIP[MAXLEN] = { 0 };
        char pFaceURL[2048] = { 0 };
        while (NULL != row)
        {
            if (NULL == row[0] || NULL == row[1] || NULL == row[2] || NULL == row[3] || NULL == row[4])
            {
                printf("***Warning: GetKeyLibFeatureFromDB::重点库[%s]特征值表有数据为空!\n", m_pDeviceInfo->sLibID.c_str());
            }
            else
            {
                strcpy(pFaceUUID, row[0]);
                nFtLen = atoi(row[2]);
                memcpy(pFeature, row[1], nFtLen);
                strcpy(pDisk, row[3]);
                strcpy(pImageServerIP, row[4]);

                LPKEYLIBFEATUREDATA pKeyLibFeatureData = new KEYLIBFEATUREDATA;
                //1. 保存FaceUUID
                int nLen = strlen(pFaceUUID);
                pKeyLibFeatureData->pFaceUUID = new char[nLen + 1];
                strcpy(pKeyLibFeatureData->pFaceUUID, pFaceUUID);
                pKeyLibFeatureData->pFaceUUID[nLen] = '\0';
                //2. 保存特征值
                pKeyLibFeatureData->pFeature = new char[nFtLen];
                memcpy(pKeyLibFeatureData->pFeature, pFeature, nFtLen);
                pKeyLibFeatureData->nFeatureLen = nFtLen;
                //3. 保存盘符
                strcpy(pKeyLibFeatureData->pDisk, pDisk);
                //4. 保存图片保存服务器IP
                nLen = strlen(pImageServerIP);
                pKeyLibFeatureData->pImageIP = new char[nLen + 1];
                strcpy(pKeyLibFeatureData->pImageIP, pImageServerIP);
                pKeyLibFeatureData->pImageIP[nLen] = '\0';

                //5.  保存人脸图片URL
                if (m_pDeviceInfo->nLoadURL && NULL != row[5])
                {
                    strcpy(pFaceURL, row[5]);
                    nLen = strlen(pFaceURL);
                    pKeyLibFeatureData->pFaceURL = new char[nLen + 1];
                    strcpy(pKeyLibFeatureData->pFaceURL, pFaceURL);
                    pKeyLibFeatureData->pFaceURL[nLen] = '\0';
                }

                //生成特征值链表
                m_pFeatureManage->AddFeature(pKeyLibFeatureData, true);

                nRead++;
            }

            row = mysql_fetch_row(result);
        }
    }
    printf("---Success: Add KeyLib[%s] Feature End, Num: %d\n", m_pDeviceInfo->sLibID.c_str(), m_pFeatureManage->GetTotalNum());
    mysql_free_result(result);

    return true;
}
//初始化Zeromq
bool CLibInfo::InitZeromq()
{
    if (NULL == m_pZeromqManage)
    {
        return false;
    }
    m_pZeromqManage->AddSubMessage((char*)m_pDeviceInfo->sLibID.c_str());   //卡口: add, search, capture, get; 重点库: search

    if (2 == m_pDeviceInfo->nServerType)
    {
        char pSubAdd[MAXLEN] = { 0 };
        //2018-09-30 不知道这个订阅是干啥的, 先注释
        /*sprintf(pSubAdd, "lib.%s", m_pDeviceInfo->sLibID.c_str());
        m_pZeromqManage->AddSubMessage(pSubAdd);*/

        sprintf(pSubAdd, "%sedit", m_pDeviceInfo->sLibID.c_str());
        m_pZeromqManage->AddSubMessage(pSubAdd);        //订阅核查库特征值编辑(add, del, clear)命令(ex: 12keylibedit)
    }

    return true;
}
//从DB删除特征值
bool CLibInfo::DelFeatureFromDB(const char * pFaceUUID)
{
    char pSQL[SQLMAXLEN] = { 0 };
    int nSQLLen = 0;

    if (1 == m_pDeviceInfo->nServerType)
    {
        sprintf(pSQL,
            "delete from `%s%s` where faceuuid = '%s'",
            CHECKPOINTPREFIX, m_pDeviceInfo->sLibID.c_str(), pFaceUUID);
    }
    else
    {
        sprintf(pSQL,
            "delete from %s where faceuuid = '%s'",
            m_pDeviceInfo->sLibID.c_str(), pFaceUUID);
    }
    
    return m_pMysqlOperation->QueryCommand(pSQL);
}
//清空重点库特征值
bool CLibInfo::ClearKeyLibFromDB()
{
    char pSQL[SQLMAXLEN] = { 0 };

    sprintf(pSQL,
        "delete from %s",
        m_pDeviceInfo->sLibID.c_str());
    return m_pMysqlOperation->QueryCommand(pSQL);
}
//Zeromq消息回调
void CLibInfo::ZeromqSubMsg(LPSUBMESSAGE pSubMessage)
{
    LPSUBMESSAGE pMessage = new SUBMESSAGE;
    //复制
    strcpy(pMessage->pHead, pSubMessage->pHead);
    strcpy(pMessage->pOperationType, pSubMessage->pOperationType);
    strcpy(pMessage->pSource, pSubMessage->pSource);
    strcpy(pMessage->pSubJsonValue, pSubMessage->pSubJsonValue);
    //放入list
    EnterMutex();
    m_listPubMessage.push_back(pMessage);
#ifdef __WINDOWS__
    SetEvent(m_hSubMsgEvent);
#else
    write(m_nPipe[1], "1", 1);
#endif
    LeaveMutex();
}
#ifdef __WINDOWS__
DWORD WINAPI CLibInfo::PubMessageCommandThread(void * pParam)
{
    CLibInfo * pThis = (CLibInfo *)pParam;
    pThis->PubMessageCommandAction();
    return 0;
}
#else
void * CLibInfo::PubMessageCommandThread(void * pParam)
{
    CLibInfo * pThis = (CLibInfo *)pParam;
    pThis->PubMessageCommandAction();
    return NULL;
}
#endif
void CLibInfo::PubMessageCommandAction()
{
    //从list取出submessage处理
    char pPipeRead[10] = { 0 };
    LPSUBMESSAGE pSubMesage = NULL;
    bool bTask = false;
    while (!m_bStopLib)
    {
#ifdef __WINDOWS__
        if (WAIT_TIMEOUT == WaitForSingleObject(m_hSubMsgEvent, THREADWAITTIME))
        {
            continue;
        }
        ResetEvent(m_hSubMsgEvent);
#else
        read(m_nPipe[0], pPipeRead, 1);
#endif
        do
        {
            EnterMutex();
            if (m_listPubMessage.size() > 0)
            {
                pSubMesage = m_listPubMessage.front();
                m_listPubMessage.pop_front();
                bTask = true;
            }
            else
            {
                pSubMesage = NULL;
                bTask = false;
            }
            LeaveMutex();
            if (NULL != pSubMesage)
            {
                //printf("CLibInfo::%s\n%s\n%s\n", pSubMesage->pHead, pSubMesage->pOperationType, pSubMesage->pSource);
                ParseZeromqJson(pSubMesage);
                delete pSubMesage;
                pSubMesage = NULL;
            }
        } while (bTask);
    }
    return;
}
//Zeromq回调消息解析处理
bool CLibInfo::ParseZeromqJson(LPSUBMESSAGE pSubMessage)
{
    int nRet = INVALIDERROR;
    string sCommand(pSubMessage->pOperationType);
    rapidjson::Document document;
    if (string(pSubMessage->pSubJsonValue) != "")
    {
        document.Parse(pSubMessage->pSubJsonValue);
        if (document.HasParseError())
        {
            printf("***Warning: Parse Json Format Failed[%s].\n", pSubMessage->pSubJsonValue);
            strcpy(pSubMessage->pHead, pSubMessage->pSource);
            strcpy(pSubMessage->pSource, m_pDeviceInfo->sLibID.c_str());
            SendResponseMsg(pSubMessage, JsonFormatError);
            return false;
        }
    }

    if (sCommand == COMMANDADD)        //增加图片
    {
        pSubMessage->nTaskType = LIBADDFEATURE;
        if (1 == m_pDeviceInfo->nServerType)
        {
            if (document.HasMember(JSONFACEUUID) && document[JSONFACEUUID].IsString() && strlen(document[JSONFACEUUID].GetString()) < MAXLEN       &&
                document.HasMember(JSONFEATURE) && document[JSONFEATURE].IsString() && strlen(document[JSONFEATURE].GetString()) < FEATURELEN    &&
                document.HasMember(JSONTIME) && document[JSONTIME].IsInt64() && strlen(document[JSONFEATURE].GetString()) > FEATUREMIXLEN &&
                document.HasMember(JSONDRIVE) && document[JSONDRIVE].IsString() && strlen(document[JSONDRIVE].GetString()) == 1 &&
                document.HasMember(JSONSERVERIP) && document[JSONSERVERIP].IsString() && strlen(document[JSONSERVERIP].GetString()) < MAXIPLEN     &&
                document.HasMember(JSONFACERECT) && document[JSONFACERECT].IsString() && strlen(document[JSONFACERECT].GetString()) < MAXIPLEN)
            {
                FeatureData * pFeatureData = new FeatureData;
                //1. 保存FaceUUID
                int nLen = strlen(document[JSONFACEUUID].GetString());
                pFeatureData->FaceUUID = new char[nLen + 1];
                strcpy(pFeatureData->FaceUUID, document[JSONFACEUUID].GetString());
                pFeatureData->FaceUUID[nLen] = '\0';
                //2. 保存特征值
                strcpy(pSubMessage->pFeature, document[JSONFEATURE].GetString());
                //----解码特征值为二进制数据
                int nDLen = 0;
                string sDFeature = ZBase64::Decode(pSubMessage->pFeature, strlen(pSubMessage->pFeature), nDLen);
                pFeatureData->FeatureValue = new unsigned char[sDFeature.size()];
                memcpy(pFeatureData->FeatureValue, sDFeature.c_str(), sDFeature.size());
                pFeatureData->FeatureValueLen = sDFeature.size();
                //3. 保存特征值时间
                if (m_pDeviceInfo->nServerTime)
                {
                    long ts = time(NULL);
                    int64_t tLimit = ts;
                    pFeatureData->CollectTime = tLimit * 1000;
                }
                else
                {
                    pFeatureData->CollectTime = document[JSONTIME].GetInt64();
                }
                //4. 保存盘符
                strcpy(pFeatureData->pDisk, document[JSONDRIVE].GetString());
                //5. 保存图片保存服务器IP
                nLen = strlen(document[JSONSERVERIP].GetString());
                pFeatureData->pImageIP = new char[nLen + 1];
                strcpy(pFeatureData->pImageIP, document[JSONSERVERIP].GetString());
                pFeatureData->pImageIP[nLen] = '\0';
                //6. 保存人脸图片坐标
                nLen = strlen(document[JSONFACERECT].GetString());
                pFeatureData->pFaceRect = new char[nLen + 1];
                strcpy(pFeatureData->pFaceRect, document[JSONFACERECT].GetString());
                pFeatureData->pFaceRect[nLen] = '\0';

                //福州项目增加: 判断是否有faceurl和bkgurl
                if (m_pDeviceInfo->nLoadLimitTime &&
                    document.HasMember(JSONFACEURL) && document[JSONFACEURL].IsString() && strlen(document[JSONFACEURL].GetString()) < 2048 &&
                    document.HasMember(JSONBKGURL) && document[JSONBKGURL].IsString() && strlen(document[JSONBKGURL].GetString()) < 2048)
                {
                    //7. 保存海康云存人脸图片URL
                    nLen = strlen(document[JSONFACEURL].GetString());
                    pFeatureData->pFaceURL = new char[nLen + 1];
                    strcpy(pFeatureData->pFaceURL, document[JSONFACEURL].GetString());
                    pFeatureData->pFaceURL[nLen] = '\0';
                    //8. 保存海康云存背景图片URL
                    nLen = strlen(document[JSONBKGURL].GetString());
                    pFeatureData->pBkgURL = new char[nLen + 1];
                    strcpy(pFeatureData->pBkgURL, document[JSONBKGURL].GetString());
                    pFeatureData->pBkgURL[nLen] = '\0';
                }

                nRet = m_pFeatureManage->AddFeature(pFeatureData);
                if (0 == nRet)
                {
                    if (!m_pMysqlOperation->InsertFeatureToDB((char*)m_pDeviceInfo->sLibID.c_str(), pFeatureData))
                    {
                        nRet = MysqlQueryFailed;
                    }
                }
                else
                {
                    delete pFeatureData;
                    pFeatureData = NULL;
                }
                //回应消息用到
                strcpy(pSubMessage->pFaceUUID, document[JSONFACEUUID].GetString());
            }
            else
            {
                nRet = JsonFormatError;
            }
        }
        else
        {
            if (document.HasMember(JSONFACEUUID) && document[JSONFACEUUID].IsString() && strlen(document[JSONFACEUUID].GetString()) < MAXLEN &&
                document.HasMember(JSONFEATURE) && document[JSONFEATURE].IsString() && strlen(document[JSONFEATURE].GetString()) < FEATURELEN &&
                strlen(document[JSONFEATURE].GetString()) > FEATUREMIXLEN &&
                document.HasMember(JSONDRIVE) && document[JSONDRIVE].IsString() && strlen(document[JSONDRIVE].GetString()) == 1 &&
                document.HasMember(JSONSERVERIP) && document[JSONSERVERIP].IsString() && strlen(document[JSONSERVERIP].GetString()) < MAXIPLEN)
            {
                LPKEYLIBFEATUREDATA pFeatureData = new KEYLIBFEATUREDATA;
                //1. 保存FaceUUID
                int nLen = strlen(document[JSONFACEUUID].GetString());
                pFeatureData->pFaceUUID = new char[nLen + 1];
                strcpy(pFeatureData->pFaceUUID, document[JSONFACEUUID].GetString());
                pFeatureData->pFaceUUID[nLen] = '\0';
                //2. 保存特征值
                strcpy(pSubMessage->pFeature, document[JSONFEATURE].GetString());
                //----解码特征值为二进制数据
                int nDLen = 0;
                string sDFeature = ZBase64::Decode(pSubMessage->pFeature, strlen(pSubMessage->pFeature), nDLen);
                pFeatureData->pFeature = new char[sDFeature.size()];
                memcpy(pFeatureData->pFeature, sDFeature.c_str(), sDFeature.size());
                pFeatureData->nFeatureLen = sDFeature.size();
                //4. 保存盘符
                strcpy(pFeatureData->pDisk, document[JSONDRIVE].GetString());
                //5. 保存图片保存服务器IP
                nLen = strlen(document[JSONSERVERIP].GetString());
                pFeatureData->pImageIP = new char[nLen + 1];
                strcpy(pFeatureData->pImageIP, document[JSONSERVERIP].GetString());
                pFeatureData->pImageIP[nLen] = '\0';

                //福州项目增加: 判断是否有FaceURL
                if (m_pDeviceInfo->nLoadURL &&
                    document.HasMember(JSONFACEURL) && document[JSONFACEURL].IsString() && strlen(document[JSONFACEURL].GetString()) < 2048)
                {
                    //6. 保存重点库图片URL
                    nLen = strlen(document[JSONFACEURL].GetString());
                    pFeatureData->pFaceURL = new char[nLen + 1];
                    strcpy(pFeatureData->pFaceURL, document[JSONFACEURL].GetString());
                    pFeatureData->pFaceURL[nLen] = '\0';
                }

                nRet = m_pFeatureManage->AddFeature(pFeatureData);
                if (0 == nRet)
                {
                    if (!m_pMysqlOperation->InsertFeatureToDB((char*)m_pDeviceInfo->sLibID.c_str(), pFeatureData))
                    {
                        m_pFeatureManage->DelFeature(pFeatureData->pFaceUUID);
                        nRet = MysqlQueryFailed;
                    }
                }
                else
                {
                    delete pFeatureData;
                    pFeatureData = NULL;
                }
                //回应消息用到
                strcpy(pSubMessage->pFaceUUID, document[JSONFACEUUID].GetString());
            }
            else
            {
                nRet = JsonFormatError;
            }
        }
    }
    else if (sCommand == COMMANDDEL)  //删除图片
    {
        pSubMessage->nTaskType = LIBDELFEATURE;
        if (document.HasMember(JSONFACEUUID) && document[JSONFACEUUID].IsString() && strlen(document[JSONFACEUUID].GetString()) < MAXLEN)
        {
            string sFaceUUID = document[JSONFACEUUID].GetString();
            nRet = m_pFeatureManage->DelFeature(sFaceUUID.c_str());
            if (0 == nRet)
            {
                if (!DelFeatureFromDB(sFaceUUID.c_str()))
                {
                    nRet = MysqlQueryFailed;
                }
            }
            //回应消息用到
            strcpy(pSubMessage->pFaceUUID, sFaceUUID.c_str());
        }
        else
        {
            nRet = JsonFormatError;
        }
    }
    else if (sCommand == COMMANDCLEAR)  //清空重点库
    {
        pSubMessage->nTaskType = LIBCLEARFEATURE;
        nRet = m_pFeatureManage->ClearKeyLib();
        if (0 == nRet)
        {
            if (!ClearKeyLibFromDB())
            {
                nRet = MysqlQueryFailed;
            }
        }
    }
    else if (sCommand == COMMANDSEARCH)//搜索图片
    {
        pSubMessage->nTaskType = LIBSEARCH;
        if (1 == m_pDeviceInfo->nServerType)
        {
            if (document.HasMember(JSONTASKID) && document[JSONTASKID].IsString() && strlen(document[JSONTASKID].GetString()) < MAXLEN &&
                document.HasMember(JSONSIMILAR) && document[JSONSIMILAR].IsInt() &&
                document.HasMember(JSONBEGINTIME) && document[JSONBEGINTIME].IsInt64() &&
                document.HasMember(JSONENDTIME) && document[JSONENDTIME].IsInt64() &&
                document.HasMember(JSONFACE) && document[JSONFACE].IsArray() && document[JSONFACE].Size() > 0)
            {
                LPSEARCHTASK pSearchTask = new SEARCHTASK;
                pSearchTask->nTaskType = LIBSEARCH;
                //保存订阅消息相关信息
                strcpy(pSearchTask->pHead, pSubMessage->pSource);
                strcpy(pSearchTask->pOperationType, pSubMessage->pOperationType);
                strcpy(pSearchTask->pSource, m_pDeviceInfo->sLibID.c_str());
                //保存任务ID
                strcpy(pSearchTask->pSearchTaskID, document[JSONTASKID].GetString());
                //保存搜索阈值
                pSearchTask->nScore = document[JSONSIMILAR].GetInt();
                //保存时间
                pSearchTask->nBeginTime = document[JSONBEGINTIME].GetInt64();
                if (pSearchTask->nBeginTime / 10000000000 == 0)
                {
                    pSearchTask->nBeginTime *= 1000;
                }
                pSearchTask->nEndTime = document[JSONENDTIME].GetInt64();
                if (pSearchTask->nEndTime / 10000000000 == 0)
                {
                    pSearchTask->nEndTime *= 1000;
                }
                
                //保存特征值
                for (int i = 0; i < document[JSONFACE].Size(); i++)
                {
                    if (document[JSONFACE][i].HasMember(JSONFACENO) && document[JSONFACE][i][JSONFACENO].IsInt() &&
                        document[JSONFACE][i].HasMember(JSONFACEFEATURE) && document[JSONFACE][i][JSONFACEFEATURE].IsString() &&
                        strlen(document[JSONFACE][i][JSONFACEFEATURE].GetString()) < FEATURELEN)
                    {
                        LPSEARCHFEATURE pSearchFeature = new SEARCHFEATURE;
                        int nDLen = 0;
                        string sDFeature = ZBase64::Decode(document[JSONFACE][i][JSONFACEFEATURE].GetString(),
                            strlen(document[JSONFACE][i][JSONFACEFEATURE].GetString()), nDLen);
                        pSearchFeature->pFeature = new char[sDFeature.size()];
                        memcpy(pSearchFeature->pFeature, sDFeature.c_str(), sDFeature.size());
                        pSearchFeature->nFeatureLen = sDFeature.size();
                        pSearchTask->vectorFeatureInfo.push_back(pSearchFeature);
                    }
                    else
                    {
                        nRet = JsonFormatError;
                        break;
                    }
                }
                //是否立即返回搜索结果(团伙分析服务调用)
                if (document.HasMember(JSONASYNCRETURN) && document[JSONASYNCRETURN].IsInt())
                {
                    pSearchTask->bAsyncReturn = document[JSONASYNCRETURN].GetInt();
                }

                if (pSearchTask->nBeginTime >= pSearchTask->nEndTime || pSearchTask->nBeginTime < UNIXBEGINTIME ||
                    pSearchTask->nEndTime < 0 || pSearchTask->nEndTime > UNIXENDTIME)
                {
                    nRet = SearchTimeWrong;
                }

                if (INVALIDERROR == nRet)
                {
                    m_pFeatureManage->AddSearchTask(pSearchTask);
                }
                else
                {
                    strcpy(pSubMessage->pSearchTaskID, pSearchTask->pSearchTaskID);
                    delete pSearchTask;
                    pSearchTask = NULL;
                }
            }
            else
            {
                nRet = JsonFormatError;
            }
        }
        else
        {
            if (document.HasMember(JSONTASKID) && document[JSONTASKID].IsString() && strlen(document[JSONTASKID].GetString()) < MAXLEN &&
                document.HasMember(JSONSIMILAR) && document[JSONSIMILAR].IsInt() &&
                document.HasMember(JSONFACE) && document[JSONFACE].IsArray() && document[JSONFACE].Size() > 0)
            {
                LPSEARCHTASK pSearchTask = new SEARCHTASK;
                pSearchTask->nTaskType = LIBSEARCH;
                //保存订阅消息相关信息
                strcpy(pSearchTask->pHead, pSubMessage->pSource);
                strcpy(pSearchTask->pOperationType, pSubMessage->pOperationType);
                strcpy(pSearchTask->pSource, m_pDeviceInfo->sLibID.c_str());
                //保存任务ID
                strcpy(pSearchTask->pSearchTaskID, document[JSONTASKID].GetString());
                //保存搜索阈值
                pSearchTask->nScore = document[JSONSIMILAR].GetInt();
                //保存特征值
                for (int i = 0; i < document[JSONFACE].Size(); i++)
                {
                    if (document[JSONFACE][i].HasMember(JSONFACENO) && document[JSONFACE][i][JSONFACENO].IsInt() &&
                        document[JSONFACE][i].HasMember(JSONFACEFEATURE) && document[JSONFACE][i][JSONFACEFEATURE].IsString() &&
                        strlen(document[JSONFACE][i][JSONFACEFEATURE].GetString()) < FEATURELEN)
                    {
                        LPSEARCHFEATURE pSearchFeature = new SEARCHFEATURE;
                        int nDLen = 0;
                        string sDFeature = ZBase64::Decode(document[JSONFACE][i][JSONFACEFEATURE].GetString(),
                            strlen(document[JSONFACE][i][JSONFACEFEATURE].GetString()), nDLen);
                        pSearchFeature->pFeature = new char[sDFeature.size()];
                        memcpy(pSearchFeature->pFeature, sDFeature.c_str(), sDFeature.size());
                        pSearchFeature->nFeatureLen = sDFeature.size();
                        pSearchTask->vectorFeatureInfo.push_back(pSearchFeature);
                    }
                    else
                    {
                        nRet = JsonFormatError;
                        break;
                    }
                }
                if (INVALIDERROR == nRet)
                {
                    m_pFeatureManage->AddSearchTask(pSearchTask);
                }
            }
            else
            {
                nRet = JsonFormatError;
            }
        }
    }
    else if (sCommand == COMMANDCAPTURE)
    {
        pSubMessage->nTaskType = LIBCAPTURE;
        if (document.HasMember(JSONTASKID) && document[JSONTASKID].IsString() && strlen(document[JSONTASKID].GetString()) < MAXLEN &&
            document.HasMember(JSONCAPTURESIZE) && document[JSONCAPTURESIZE].IsInt() &&
            document.HasMember(JSONBEGINTIME) && document[JSONBEGINTIME].IsInt64() &&
            document.HasMember(JSONENDTIME) && document[JSONENDTIME].IsInt64())
        {
            LPSEARCHTASK pSearchTask = new SEARCHTASK;
            pSearchTask->nTaskType = LIBCAPTURE;
            //保存订阅消息相关信息
            strcpy(pSearchTask->pHead, pSubMessage->pSource);
            strcpy(pSearchTask->pOperationType, pSubMessage->pOperationType);
            strcpy(pSearchTask->pSource, m_pDeviceInfo->sLibID.c_str());
            //保存任务ID
            strcpy(pSearchTask->pSearchTaskID, document[JSONTASKID].GetString());
            //保存返回抓拍最大数
            pSearchTask->nCaptureCount = document[JSONCAPTURESIZE].GetInt();
            //保存时间
            pSearchTask->nBeginTime = document[JSONBEGINTIME].GetInt64();
            pSearchTask->nEndTime = document[JSONENDTIME].GetInt64();
            //是否返回特征值
            if (document.HasMember(JSONRETURNFEATURE) && document[JSONRETURNFEATURE].IsInt())
            {
                pSearchTask->bFeature = document[JSONRETURNFEATURE].GetInt();
            }
            else
            {
                pSearchTask->bFeature = false;
            }

            if (pSearchTask->nBeginTime >= pSearchTask->nEndTime || pSearchTask->nBeginTime < UNIXBEGINTIME ||
                pSearchTask->nEndTime < 0 || pSearchTask->nEndTime > UNIXENDTIME)
            {
                nRet = SearchTimeWrong;
            }

            if (nRet == INVALIDERROR)
            {
                nRet = m_pFeatureManage->GetCaptureImageInfo(pSearchTask);
                if (INVALIDERROR == nRet)
                {
                    strcpy(pSubMessage->pHead, pSubMessage->pSource);
                    strcpy(pSubMessage->pSource, m_pDeviceInfo->sLibID.c_str());
                    SendResponseMsg(pSubMessage, nRet, pSearchTask);
                }
            }
            else
            {
                strcpy(pSubMessage->pSearchTaskID, pSearchTask->pSearchTaskID);
            }

            delete pSearchTask;
            pSearchTask = NULL;
        }
        else
        {
            nRet = JsonFormatError;
        }
    }
    else if (sCommand == COMMANDBUSINESSSUMMARY)    //频繁出入等统计
    {
        pSubMessage->nTaskType = LIBBUSINESS;
        if (document.HasMember(JSONTASKID)      && document[JSONTASKID].IsString()   && document[JSONTASKID].GetStringLength() < MAXLEN && //任务ID
            document.HasMember(JSONBEGINTIME)   && document[JSONBEGINTIME].IsInt64() &&   //开始时间
            document.HasMember(JSONENDTIME)     && document[JSONENDTIME].IsInt64()   &&   //结束时间
            document.HasMember(JSONSIMILAR)     && document[JSONSIMILAR].IsInt()     &&   //相似度
            document.HasMember(JSONFREQUENTLY)  && document[JSONFREQUENTLY].IsInt())      //频繁出入次数阈值
        {
            LPSEARCHTASK pSearchTask = new SEARCHTASK;
            pSearchTask->nTaskType = LIBBUSINESS;
            //保存订阅消息相关信息
            strcpy(pSearchTask->pHead, pSubMessage->pSource);
            strcpy(pSearchTask->pOperationType, pSubMessage->pOperationType);
            strcpy(pSearchTask->pSource, m_pDeviceInfo->sLibID.c_str());
            //保存任务ID
            strcpy(pSearchTask->pSearchTaskID, document[JSONTASKID].GetString());
            //保存时间
            pSearchTask->nBeginTime = document[JSONBEGINTIME].GetInt64();
            pSearchTask->nEndTime = document[JSONENDTIME].GetInt64();
            //保存相似度阈值
            pSearchTask->nScore = document[JSONSIMILAR].GetInt();
            //保存出入次数统计阈值
            pSearchTask->nFrequently = document[JSONFREQUENTLY].GetInt();
            if (document.HasMember(JSONNIGHTSTARTTIME)  && document[JSONNIGHTSTARTTIME].IsInt() &&
                document.HasMember(JSONNIGHTENDTIME)    && document[JSONNIGHTENDTIME].IsInt())
            {
                pSearchTask->bNightSearch = true;       //深夜出入
                pSearchTask->nNightStartTime = document[JSONNIGHTSTARTTIME].GetInt();
                pSearchTask->nNightEndTime =  document[JSONNIGHTENDTIME].GetInt();
                pSearchTask->nRangeNightTime = pSearchTask->nNightEndTime - pSearchTask->nNightStartTime > 0 ?
                                                ((pSearchTask->nNightEndTime - pSearchTask->nNightStartTime) * 3600 * 1000) :
                                                ((24 + pSearchTask->nNightEndTime - pSearchTask->nNightStartTime) * 3600 * 1000);
                printf("Night Continue Time: [%d].\n", pSearchTask->nRangeNightTime / 1000);
                if (pSearchTask->nNightStartTime == pSearchTask->nNightEndTime)
                {
                    nRet = JsonParamIllegal;
                }
            }
            if (pSearchTask->nBeginTime >= pSearchTask->nEndTime || pSearchTask->nBeginTime < UNIXBEGINTIME ||
                pSearchTask->nEndTime < 0 || pSearchTask->nEndTime > UNIXENDTIME)
            {
                nRet = SearchTimeWrong;
            }

            if (INVALIDERROR == nRet)
            {
                m_pFeatureManage->AddSearchTask(pSearchTask);
            }
            else
            {
                strcpy(pSubMessage->pSearchTaskID, pSearchTask->pSearchTaskID);
                delete pSearchTask;
                pSearchTask = NULL;
            }
        }
        else
        {
            nRet = JsonFormatError;
        }
    }
    else if (sCommand == COMMANDGET)
    {
        pSubMessage->nTaskType = LIBGET;
        if (document.HasMember(JSONFACEUUID) && document[JSONFACEUUID].IsString() && strlen(document[JSONFACEUUID].GetString()) < MAXLEN &&
            document.HasMember(JSONTIME) && document[JSONTIME].IsInt64() )
        {
            LPFACEURLINFO pFaceURLInfo = new FACEURLINFO;
            
            strcpy(pFaceURLInfo->pFaceUUID, document[JSONFACEUUID].GetString());
            pFaceURLInfo->nTime = document[JSONTIME].GetInt64();

            nRet = m_pFeatureManage->GetFaceURLInfo(pFaceURLInfo);
            if (INVALIDERROR == nRet)
            {
                strcpy(pSubMessage->pHead, pSubMessage->pSource);
                strcpy(pSubMessage->pSource, m_pDeviceInfo->sLibID.c_str());
                SendResponseMsg(pSubMessage, nRet, NULL, pFaceURLInfo);
            }

            delete pFaceURLInfo;
            pFaceURLInfo = NULL;
            return true;
        }
        else
        {
            nRet = JsonFormatError;
        }
    }
    else
    {
        printf("Command[%s] not Found!\n", sCommand.c_str());
        nRet = CommandNotFound;
    }

    if (INVALIDERROR != nRet || (sCommand != COMMANDADD && sCommand != COMMANDSEARCH && sCommand != COMMANDCAPTURE && sCommand != COMMANDBUSINESSSUMMARY))
    {
        strcpy(pSubMessage->pHead, pSubMessage->pSource);
        strcpy(pSubMessage->pSource, m_pDeviceInfo->sLibID.c_str());
        SendResponseMsg(pSubMessage, nRet);
    }
    return true;
}
//搜索任务结果回调
void CLibInfo::SearchTaskCallback(LPSEARCHTASK pSearchTask, void * pUser)
{
    CLibInfo * pThis = (CLibInfo *)pUser;
    LPSUBMESSAGE pSubMessage = new SUBMESSAGE;
    //发布订阅头信息
    strcpy(pSubMessage->pHead, pSearchTask->pHead);
    strcpy(pSubMessage->pOperationType, pSearchTask->pOperationType);
    strcpy(pSubMessage->pSource, pSearchTask->pSource);
    
    
    if (pSearchTask->nError == 0)
    {
        //搜索结果信息
        pSubMessage->nTaskType = pSearchTask->nTaskType;
        strcpy(pSubMessage->pSearchTaskID, pSearchTask->pSearchTaskID);
        strcpy(pSubMessage->pDeviceID, pThis->m_pDeviceInfo->sLibID.c_str());
        if (LIBSEARCH == pSubMessage->nTaskType)
        {
            if (1 == pThis->m_pDeviceInfo->nServerType && pSearchTask->bAsyncReturn)  //如果立即返回搜索结果(团伙分析服务搜索)
            {
                pThis->SendResponseMsg(pSubMessage, pSearchTask->nError, pSearchTask);
            }
            else
            {
                pSubMessage->nTotalNum = pSearchTask->nTotalCount;
                pSubMessage->nHighestScore = pSearchTask->nHighestScore;
                pSubMessage->nLatestTime = pSearchTask->nTime;

                if (pSearchTask->nTotalCount > 0)
                {
                    //将搜索结果写入Redis
                    pSearchTask->nError = pThis->InsertSearchResultToRedis(pSearchTask);
                }

                pThis->SendResponseMsg(pSubMessage, pSearchTask->nError, pSearchTask);
            }
        }
        else if (LIBBUSINESS == pSubMessage->nTaskType)
        {
#ifdef __WINDOWS__
            DWORD dwCurTime = GetTickCount();
            printf("业务统计[%s][%s]耗时: %dms.\n", pSearchTask->pSearchTaskID, pThis->m_pDeviceInfo->sLibID.c_str(), dwCurTime - pSearchTask->dwSearchTime);
#endif
            pThis->SendResponseMsg(pSubMessage, pSearchTask->nError, pSearchTask);
        }
    }
    else
    {
        pThis->SendResponseMsg(pSubMessage, pSearchTask->nError);
    }


    delete pSubMessage;
    pSubMessage = NULL;
    delete pSearchTask;
    pSearchTask = NULL;
    return;
}
//回应请求
void CLibInfo::SendResponseMsg(LPSUBMESSAGE pSubMessage, int nEvent, LPSEARCHTASK pSearchTask, LPFACEURLINFO pFaceURLInfo)
{
    int nPeopleNum = 0;
    char pErrorMsg[64] = { 0 };
    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType&allocator = document.GetAllocator();
    rapidjson::Value array(rapidjson::kArrayType);
    if (nEvent == 0)
    {
        document.AddMember("result", rapidjson::StringRef("success"), allocator);
        switch (pSubMessage->nTaskType)
        {
        case LIBADDFEATURE: case LIBDELFEATURE:
        {
            document.AddMember(JSONFACEUUID, rapidjson::StringRef(pSubMessage->pFaceUUID), allocator);
            break;
        }
        case LIBSEARCH:
        {
            document.AddMember(JSONTASKID, rapidjson::StringRef(pSubMessage->pSearchTaskID), allocator);
            document.AddMember(JSONDLIBID, rapidjson::StringRef(pSubMessage->pDeviceID), allocator);
            if (NULL != pSearchTask && pSearchTask->bAsyncReturn)  //如果立即返回搜索结果(团伙分析服务搜索)
            {
                MAPSEARCHRESULT::iterator it = pSearchTask->mapSearchTaskResult.begin();
                for (; it != pSearchTask->mapSearchTaskResult.end(); it++)
                {
                    rapidjson::Value object(rapidjson::kObjectType);
                    object.AddMember(JSONFACEUUID, rapidjson::StringRef(it->second->pFaceUUID), allocator);
                    object.AddMember(JSONSCORE, it->second->nScore, allocator);
                    object.AddMember(JSONTIME, it->second->nTime, allocator);
                    object.AddMember(JSONDRIVE, rapidjson::StringRef(it->second->pDisk), allocator);
                    object.AddMember(JSONSERVERIP, rapidjson::StringRef(it->second->pImageIP), allocator);
                    object.AddMember(JSONFACERECT, rapidjson::StringRef(it->second->pFaceRect), allocator);
                    object.AddMember(JSONFACEURL, rapidjson::StringRef(it->second->pFaceURL), allocator);
                    object.AddMember(JSONBKGURL, rapidjson::StringRef(it->second->pBkgURL), allocator);

                    array.PushBack(object, allocator);
                }
                document.AddMember("info", array, allocator);
            }
            else if (NULL != pSearchTask)
            {
                int nHitTwoMoreCount = 0;
                int nHitThreeMoreCount = 0;
                int nHitFourMoreCount = 0;
                int nHitFiveMoreCount = 0;
                MAPSEARCHRESULT::iterator it = pSearchTask->mapSearchTaskResult.begin();
                for (; it != pSearchTask->mapSearchTaskResult.end(); it++)
                {
                    switch (it->second->nHitCount)
                    {
                    case 2:
                        nHitTwoMoreCount++;
                        break;
                    case 3:
                        nHitTwoMoreCount++;
                        nHitThreeMoreCount++;
                        break;
                    case 4:
                        nHitTwoMoreCount++;
                        nHitThreeMoreCount++;
                        nHitFourMoreCount++;
                        break;
                    case 5:
                        nHitTwoMoreCount++;
                        nHitThreeMoreCount++;
                        nHitFourMoreCount++;
                        nHitFiveMoreCount++;
                        break;
                    default:
                        break;
                    }
                }
                document.AddMember(JSONSEARCHCOUNT, pSubMessage->nTotalNum, allocator);
                document.AddMember(JSONSEARCHTWOMORECOUNT, nHitTwoMoreCount, allocator);
                document.AddMember(JSONSEARCHTHREEMORECOUNT, nHitThreeMoreCount, allocator);
                document.AddMember(JSONSEARCHFOURMORECOUNT, nHitFourMoreCount, allocator);
                document.AddMember(JSONSEARCHFIVEMORECOUNT, nHitFiveMoreCount, allocator);
                document.AddMember(JSONSEARCHHIGHEST, pSubMessage->nHighestScore, allocator);
                document.AddMember(JSONSEARCHLATESTTIME, pSubMessage->nLatestTime, allocator);
            }
            break;
        }
        case LIBCAPTURE:
        {
            if (NULL != pSearchTask)
            {
                //任务ID
                document.AddMember(JSONTASKID, rapidjson::StringRef(pSearchTask->pSearchTaskID), allocator);
                MAPSEARCHRESULT::iterator it = pSearchTask->mapSearchTaskResult.begin();
                for (; it != pSearchTask->mapSearchTaskResult.end(); it++)
                {
                    rapidjson::Value object(rapidjson::kObjectType);
                    object.AddMember(JSONFACEUUID, rapidjson::StringRef(it->second->pFaceUUID), allocator);
                    object.AddMember(JSONDEVICEID, rapidjson::StringRef(m_pDeviceInfo->sLibID.c_str()), allocator);
                    object.AddMember(JSONDRIVE, rapidjson::StringRef(it->second->pDisk), allocator);
                    object.AddMember(JSONSERVERIP, rapidjson::StringRef(it->second->pImageIP), allocator);
                    object.AddMember(JSONFACERECT, rapidjson::StringRef(it->second->pFaceRect), allocator);
                    object.AddMember(JSONFACEURL, rapidjson::StringRef(it->second->pFaceURL), allocator);
                    object.AddMember(JSONBKGURL, rapidjson::StringRef(it->second->pBkgURL), allocator);
                    object.AddMember(JSONTIME, it->second->nTime, allocator);
                    if (pSearchTask->bFeature && NULL != it->second->pFeature)
                    {
                        object.AddMember(JSONFEATURE, rapidjson::StringRef(it->second->pFeature), allocator);
                    }

                    array.PushBack(object, allocator);
                }
                document.AddMember("info", array, allocator);
            }
            
            break;
        }
        case LIBBUSINESS:
        {
            document.AddMember(JSONTASKID, rapidjson::StringRef(pSubMessage->pSearchTaskID), allocator);
            document.AddMember(JSONDLIBID, rapidjson::StringRef(pSubMessage->pDeviceID), allocator);
            if (NULL != pSearchTask)
            {
                for (MAPBUSINESSRESULT::iterator itBusiness = pSearchTask->mapBusinessResult.begin();
                    itBusiness != pSearchTask->mapBusinessResult.end(); itBusiness++)
                {
                    rapidjson::Value object(rapidjson::kObjectType);
                    object.AddMember(JSONPERSONID, itBusiness->first, allocator);
                    object.AddMember(JSONFREQUENTLY, itBusiness->second->mapPersonInfo.size(), allocator);
                    rapidjson::Value arrayPerson(rapidjson::kArrayType);

                    for (MAPSEARCHRESULT::iterator itPerson = itBusiness->second->mapPersonInfo.begin();
                        itPerson != itBusiness->second->mapPersonInfo.end(); itPerson++)
                    {
                        rapidjson::Value objectPerson(rapidjson::kObjectType);
                        objectPerson.AddMember(JSONFACEUUID, rapidjson::StringRef(itPerson->second->pFaceUUID), allocator);
                        objectPerson.AddMember(JSONTIME, itPerson->second->nTime, allocator);
                        objectPerson.AddMember(JSONDRIVE, rapidjson::StringRef(itPerson->second->pDisk), allocator);
                        objectPerson.AddMember(JSONSERVERIP, rapidjson::StringRef(itPerson->second->pImageIP), allocator);
                        objectPerson.AddMember(JSONFACERECT, rapidjson::StringRef(itPerson->second->pFaceRect), allocator);
                        objectPerson.AddMember(JSONFACEURL, rapidjson::StringRef(itPerson->second->pFaceURL), allocator);
                        objectPerson.AddMember(JSONBKGURL, rapidjson::StringRef(itPerson->second->pBkgURL), allocator);
                        objectPerson.AddMember(JSONSCORE, itPerson->second->nScore, allocator);

                        arrayPerson.PushBack(objectPerson, allocator);
                    }
                    nPeopleNum++;
                    object.AddMember("data", arrayPerson, allocator);

                    array.PushBack(object, allocator);
                }
            }
            document.AddMember("info", array, allocator);


            break;
        }
        case LIBGET:
        {
            if (NULL != pFaceURLInfo)
            {
                document.AddMember(JSONFACEUUID, rapidjson::StringRef(pFaceURLInfo->pFaceUUID), allocator);
                document.AddMember(JSONFACERECT, rapidjson::StringRef(pFaceURLInfo->pFaceRect), allocator);
                document.AddMember(JSONFACEURL, rapidjson::StringRef(pFaceURLInfo->pFaceURL), allocator);
                document.AddMember(JSONBKGURL, rapidjson::StringRef(pFaceURLInfo->pBkgURL), allocator);

            }
        }
        default:
            break;
        }
    }
    else
    {
        GetErrorMsg((ErrorCode)nEvent, pErrorMsg);
        document.AddMember("result", rapidjson::StringRef("error"), allocator);
        if(NULL != pSubMessage && (pSubMessage->nTaskType == LIBADDFEATURE || pSubMessage->nTaskType == LIBDELFEATURE))
        {
            document.AddMember(JSONFACEUUID, rapidjson::StringRef(pSubMessage->pFaceUUID), allocator);
        }
        else if (NULL != pSubMessage && (pSubMessage->nTaskType == LIBBUSINESS || pSubMessage->nTaskType == LIBSEARCH || pSubMessage->nTaskType == LIBCAPTURE))
        {
            document.AddMember(JSONTASKID, rapidjson::StringRef(pSubMessage->pSearchTaskID), allocator);
        }
        document.AddMember("errorcode", nEvent, allocator);
        document.AddMember("errorMessage", rapidjson::StringRef(pErrorMsg), allocator);
    }
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    pSubMessage->sPubJsonValue = string(buffer.GetString());
    //发布回应消息
    if (pSubMessage->nTaskType != LIBADDFEATURE)
    {
        m_pZeromqManage->PubMessage(pSubMessage);
    }
    switch (pSubMessage->nTaskType)
    {
    case LIBADDFEATURE: case LIBDELFEATURE:
        //printf("Rep Client json:\n%s\n==============================\n", pSubMessage->sPubJsonValue.c_str());
        break;
    case LIBCAPTURE:
        break;
    case LIBSEARCH:
#ifdef __WINDOWS__
    {
        DWORD dwCurTime = GetTickCount();
        printf("Search [%s] Time: %dms.\n", m_pDeviceInfo->sLibID.c_str(), dwCurTime - pSearchTask->dwSearchTime);
    }
#endif
        printf("Search [%s] Rep Client json:\n%s\n==============================\n",m_pDeviceInfo->sLibID.c_str(), pSubMessage->sPubJsonValue.c_str());
        break;
    case LIBBUSINESS:
        printf("Checkpoint[%s][%s] Business Rep, json size: %d, People: %d.\n",
            m_pDeviceInfo->sLibID.c_str(), pSearchTask->pSearchTaskID, pSubMessage->sPubJsonValue.size(), nPeopleNum);
        break;
    case LIBGET:
        printf("Rep Get FaceURL Info:\n%s\n==============================\n", pSubMessage->sPubJsonValue.c_str());
        break;
    default:
        break;
    }
    return;
}

int CLibInfo::InsertSearchResultToRedis(LPSEARCHTASK pSearchTask)
{
    int nRet = INVALIDERROR;
    //1. 写FaceUUID Hash表
    char pTableName[MAXLEN * 2] = { 0 };
    char pBuf[MAXLEN] = { 0 };
#ifdef __WINDOWS__
    std::map<acl::string, acl::string> mapValues;
#else
    map<string, string> mapValues;
#endif
    MAPSEARCHRESULT::iterator it = pSearchTask->mapSearchTaskResult.begin();
    for (; it != pSearchTask->mapSearchTaskResult.end(); it++)
    {
        mapValues.clear();
        sprintf(pTableName, "%s.%s", pSearchTask->pSearchTaskID, it->second->pFaceUUID);
        if (1 == m_pDeviceInfo->nServerType)
        {
            mapValues.insert(make_pair(REDISDEVICEID, m_pDeviceInfo->sLibID.c_str()));
        }
        else if (2 == m_pDeviceInfo->nServerType)
        {
            int nLibID = 0;
            nLibID = atoi(m_pDeviceInfo->sLibID.c_str());
            sprintf(pBuf, "%d", nLibID);
            mapValues.insert(make_pair(REDISDEVICEID, pBuf));
        }
        mapValues.insert(make_pair(REDISFACEUUID, it->second->pFaceUUID));
        sprintf(pBuf, "%d", it->second->nScore);
        mapValues.insert(make_pair(REDISSCORE, pBuf));
        sprintf(pBuf, "%lld", it->second->nTime);
        mapValues.insert(make_pair(REDISTIME, pBuf));
        mapValues.insert(make_pair(REDISDRIVE, it->second->pDisk));
        mapValues.insert(make_pair(REDISIMAGEIP, it->second->pImageIP));
        sprintf(pBuf, "%d", it->second->nHitCount);
        mapValues.insert(make_pair(REDISHITCOUNT, pBuf));
        mapValues.insert(make_pair(REDISFACERECT, it->second->pFaceRect));
        mapValues.insert(make_pair(REDISFACEURL, it->second->pFaceURL));
        mapValues.insert(make_pair(REDISBKGURL, it->second->pBkgURL));

        if (!m_pRedisManage->InsertHashValue(pTableName, mapValues))
        {
            nRet = InsertRedisFailed;
            break;
        }
    }

    if (INVALIDERROR == nRet)
    {
        int nCount = pSearchTask->nTotalCount > pSearchTask->mapSearchTaskResult.size() ? pSearchTask->mapSearchTaskResult.size() : pSearchTask->nTotalCount;
        double * nScore = new double[nCount];
        double * nTime = NULL;
        const char * * pMember = new const char *[nCount];
        if (1 == m_pDeviceInfo->nServerType)
        {
            nTime = new double[nCount];
        }
        MAPSEARCHRESULT::iterator it = pSearchTask->mapSearchTaskResult.begin();
        for (int i = 0; i < nCount && it != pSearchTask->mapSearchTaskResult.end(); i++)
        {
            nScore[i] = it->second->nScore;
            pMember[i] = it->second->pFaceUUID;
            if (1 == m_pDeviceInfo->nServerType)
            {
                nTime[i] = it->second->nTime;
            }
            it++;
        }
        //插入TaskID.score
        sprintf(pTableName, "%s.score", pSearchTask->pSearchTaskID);
        m_pRedisManage->ZAddSortedSet(pTableName, nScore, pMember, nCount);
        if (1 == m_pDeviceInfo->nServerType)
        {
            //插入TaskID.time
            sprintf(pTableName, "%s.time", pSearchTask->pSearchTaskID);
            m_pRedisManage->ZAddSortedSet(pTableName, nTime, pMember, nCount);
            //插入TaskID.deviceid
            sprintf(pTableName, "%s.%s", pSearchTask->pSearchTaskID, m_pDeviceInfo->sLibID.c_str());
            m_pRedisManage->ZAddSortedSet(pTableName, nTime, pMember, nCount);
        }
        else if (2 == m_pDeviceInfo->nServerType)
        {
            //插入TaskID.LibID
            int nLibID = atoi(m_pDeviceInfo->sLibID.c_str());
            sprintf(pTableName, "%s.%d", pSearchTask->pSearchTaskID, nLibID);
            m_pRedisManage->ZAddSortedSet(pTableName, nScore, pMember, nCount);
            printf("redis %s.\n", pTableName);
        }
    }
    return nRet;
}
//获取错误码说明
int CLibInfo::GetErrorMsg(ErrorCode nError, char * pMsg)
{
    int nRet = 0;
    switch (nError)
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
    case InsertRedisFailed:
        strcpy(pMsg, "INSERT_REDIS_FAILED");
        break;
    default:
        nRet = -1;
        break;
    }

    return nRet;
}
string CLibInfo::ChangeSecondToTime(unsigned long long nSecond)
{
    time_t ctime = nSecond;
    tm *tTime = localtime(&ctime);
    char sTime[20];
    sprintf(sTime, "%04d-%02d-%02d %02d:%02d:%02d", tTime->tm_year + 1900, tTime->tm_mon + 1, tTime->tm_mday,
        tTime->tm_hour, tTime->tm_min, tTime->tm_sec);
    string RetTime = sTime;
    return sTime;
}
void CLibInfo::EnterMutex()
{
#ifdef __WINDOWS__
    EnterCriticalSection(&m_cs);
#else
    pthread_mutex_lock(&m_mutex);
#endif
}
void CLibInfo::LeaveMutex()
{
#ifdef __WINDOWS__
    LeaveCriticalSection(&m_cs);
#else
    pthread_mutex_unlock(&m_mutex);
#endif
}