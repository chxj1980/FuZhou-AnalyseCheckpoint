#include "LibInfo.h"
#include "stdio.h"
#include "time.h"

CLibInfo::CLibInfo()
{
    m_pFeatureManage = NULL;
    m_pZeromqManage = NULL;
    m_pDeviceInfo = NULL;
    m_pRedisManage = NULL;
    m_hThreadPubStatus = -1;
    m_bStopLib = false;
    pipe(m_nPipe);
}
CLibInfo::~CLibInfo()
{
    close(m_nPipe[0]);
    close(m_nPipe[1]);
}
//开始服务
bool CLibInfo::Start(LPDEVICEINFO pDeviceInfo)
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
    if (!Init())
    {
        printf("***Warning: StartLibInfo[%s]::Init Failed.\n", m_pDeviceInfo->sLibID.c_str());
        return false;
    }
    return true;
}
//停止服务
bool CLibInfo::Stop()
{
    m_bStopLib = true;
    write(m_nPipe[1], "1", 1);
    if (NULL != m_pFeatureManage)
    {
        m_pFeatureManage->UnInit();
        delete m_pFeatureManage;
        m_pFeatureManage = NULL;
    }
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
    if (NULL != m_pDeviceInfo)
    {
        delete m_pDeviceInfo;
        m_pDeviceInfo = NULL;
    }
    mysql_close(&m_mysql);
    pthread_mutex_destroy(&m_mutex);

    if (-1 != m_hThreadPubStatus)
    {
        pthread_join(m_hThreadPubStatus, NULL);
        printf("--Pub Status Thread End.\n");
    }
    return true;
}
//初始化服务
bool CLibInfo::Init()
{
    printf("---------------\nLib [%s] Init...\n", m_pDeviceInfo->sLibID.c_str());
    //初始化临界区
    pthread_mutex_init(&m_mutex, NULL);
    if (!ConnectDB())
    {
        printf("***Warning: connect MySQL Failed.\n");
        return false;
    }
    if (!InitFeatureManage())
    {
        printf("***Warning: InitFeatureManage Failed.\n");
        return false;
    }
    //初始化Zeromq
    if (!InitZeromq())
    {
        return false;
    }
    if (!InitRedis())
    {
        return false;
    }
    pthread_create(&m_hThreadPubStatus, NULL, PubStatusThread, (void*)this);
    printf("--Pub Status Thread Start.\n");
    printf("---------------Lib [%s] Init End.\n", m_pDeviceInfo->sLibID.c_str());
    return true;
}
//连接本地数据库
bool CLibInfo::ConnectDB()
{
    mysql_init(&m_mysql);
    if (!mysql_real_connect(&m_mysql, m_pDeviceInfo->sDBIP.c_str(), m_pDeviceInfo->sDBUser.c_str(),
        m_pDeviceInfo->sDBPassword.c_str(), m_pDeviceInfo->sDBName.c_str(), m_pDeviceInfo->nDBPort, NULL, 0))
    {
        const char * pErrorMsg = mysql_error(&m_mysql);
        printf("%s\n", pErrorMsg);
        printf("***Warning: CLibInfo::mysql_real_connect Failed, Please Check MySQL Service is start!\n");
        printf("DBInfo: %s:%d:%s:%s:%s\n", m_pDeviceInfo->sDBIP.c_str(), m_pDeviceInfo->nDBPort,
            m_pDeviceInfo->sDBName.c_str(), m_pDeviceInfo->sDBUser.c_str(), m_pDeviceInfo->sDBPassword.c_str());
        return false;
    }
    else
    {
        printf("CLibInfo::Connect MySQL Success!\n");
    }

    return true;
}
//重连接本地数据库
bool CLibInfo::ReConnectDB()
{
    mysql_init(&m_mysql);
    if (!mysql_real_connect(&m_mysql, m_pDeviceInfo->sDBIP.c_str(), m_pDeviceInfo->sDBUser.c_str(),
        m_pDeviceInfo->sDBPassword.c_str(), m_pDeviceInfo->sDBName.c_str(), m_pDeviceInfo->nDBPort, NULL, 0))
    {
        printf("***Warning: CLibInfo::ReConnect MySQL Failed, Please Check MySQL Service is start!\n");
        return false;
    }
    else
    {
        printf("CLibInfo::ReConnect MySQL Success!\n");
    }

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
    sprintf(pSQL, "create table %s%s(faceuuid varchar(%d), feature blob(%d), ftlen int(10), "
        "time bigint(20), imagedisk varchar(1), imageserverip varchar(20), facerect varchar(20), UNIQUE INDEX index_faceuuid(faceuuid ASC))",
        CHECKPOINTPREFIX, m_pDeviceInfo->sLibID.c_str(), MAXLEN, FEATURELEN);
    nRet = mysql_query(&m_mysql, pSQL);
    if (nRet == 1)
    {
        const char * pErrorMsg = mysql_error(&m_mysql);
        if (string(pErrorMsg).find("already exists") == string::npos)    //表己存在则不返回错误
        {
            printf("Excu SQL Failed[\"%s\"], SQL:\n%s\n", pErrorMsg, pSQL);
            return false;
        }
    }

    //获取卡口特征值信息
    int nRead = 0;
    sprintf(pSQL, "select faceuuid, feature, ftlen, time, imagedisk, imageserverip, facerect from %s%s where ftlen > 1024  order by time",
        CHECKPOINTPREFIX, m_pDeviceInfo->sLibID.c_str());
    nRet = mysql_query(&m_mysql, pSQL);
    if (nRet == 1)
    {
        const char * pErrorMsg = mysql_error(&m_mysql);
        printf("***Warning: Excu SQL Failed[\"%s\"], SQL:\n%s\n", pErrorMsg, pSQL);
        return false;
    }
    MYSQL_RES *result = mysql_store_result(&m_mysql);
    if (NULL == result)
    {
        printf("Excu SQL Failed, SQL:\n%s\n", pSQL);
        return false;
    }
    int nRowCount = mysql_num_rows(result);
    printf("Checkpoint[%s] Record Total: %d.\n", m_pDeviceInfo->sLibID.c_str(), nRowCount);

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
                nCollectTime = strtol(row[3], NULL, 10);
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
                //保存特征值
                m_pFeatureManage->AddFeature(pFeatureData, true);

                /*nRead++;
                if (nRead % 100000 == 0)
                {
                    printf("Aleady Read: %d.\n", nRead);
                }*/
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
    sprintf(pSQL, 
        "create table %s(faceuuid varchar(%d), feature blob(%d), ftlen int(10), time bigint(20), "
        "imagedisk varchar(1), imageserverip varchar(20), UNIQUE INDEX index_faceuuid(faceuuid ASC))",
        m_pDeviceInfo->sLibID.c_str(), MAXLEN, FEATURELEN);
    nRet = mysql_query(&m_mysql, pSQL);
    if (nRet == 1)
    {
        const char * pErrorMsg = mysql_error(&m_mysql);
        if (string(pErrorMsg).find("already exists") == string::npos)    //表己存在则不返回错误
        {
            printf("Excu SQL Failed[\"%s\"], SQL:\n%s\n", pErrorMsg, pSQL);
            return false;
        }
    }

    //获取重点库特征值信息
    int nRead = 0;
    sprintf(pSQL, "select faceuuid, feature, ftlen, imagedisk, imageserverip from %s", m_pDeviceInfo->sLibID.c_str());
    nRet = mysql_query(&m_mysql, pSQL);
    if (nRet == 1)
    {
        const char * pErrorMsg = mysql_error(&m_mysql);
        printf("***Warning: Excu SQL Failed[\"%s\"], SQL:\n%s\n", pErrorMsg, pSQL);
        return false;
    }
    MYSQL_RES *result = mysql_store_result(&m_mysql);
    if (NULL == result)
    {
        printf("Excu SQL Failed, SQL:\n%s\n", pSQL);
        return false;
    }
    int nRowCount = mysql_num_rows(result);
    printf("Checkpoint[%s] Record Total: %d.\n", m_pDeviceInfo->sLibID.c_str(), nRowCount);

    if (nRowCount > 0)
    {
        MYSQL_ROW row = NULL;
        row = mysql_fetch_row(result);
        char pFaceUUID[MAXLEN] = { 0 };
        char pFeature[FEATURELEN] = { 0 };
        int nFtLen = 0;
        char pDisk[MAXLEN] = { 0 };
        char pImageServerIP[MAXLEN] = { 0 };
        while (NULL != row)
        {
            if (NULL == row[0] || NULL == row[1] || NULL == row[2] || NULL == row[3])
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

                //保存特征值
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
        m_pZeromqManage = new CZeromqManage;
    }
    
    if (!m_pZeromqManage->InitSub(NULL, 0, (char*)m_pDeviceInfo->sPubServerIP.c_str(), m_pDeviceInfo->nPubServerPort, ZeromqSubMsg, this, 1))
    {
        printf("****Error: 订阅[%s:%d:%s]失败!", (char*)m_pDeviceInfo->sPubServerIP.c_str(), m_pDeviceInfo->nPubServerPort, m_pDeviceInfo->sLibID.c_str());
        return false;
    }
    m_pZeromqManage->AddSubMessage((char*)m_pDeviceInfo->sLibID.c_str());
    string sSubAll = m_pDeviceInfo->nServerType == 1 ? SUBALLCHECKPOINT : SUBALLLIB;
    if (1 == m_pDeviceInfo->nServerType)
    {
        m_pZeromqManage->AddSubMessage(SUBALLCHECKPOINT);   //搜索所有卡口订阅命令
    }
    else if (2 == m_pDeviceInfo->nServerType)
    {
        m_pZeromqManage->AddSubMessage(SUBALLLIB);      //搜索所有核查库订阅命令

        char pSubAdd[MAXLEN] = { 0 };
        sprintf(pSubAdd, "lib.%s", m_pDeviceInfo->sLibID.c_str());
        m_pZeromqManage->AddSubMessage(pSubAdd);

        sprintf(pSubAdd, "%sedit", m_pDeviceInfo->sLibID.c_str());
        m_pZeromqManage->AddSubMessage(pSubAdd);        //订阅核查库特征值编辑(add, del, clear)命令(ex: 12.keylibedit)
    }
    
    if (!m_pZeromqManage->InitPub(NULL, 0, (char*)m_pDeviceInfo->sPubServerIP.c_str(), m_pDeviceInfo->nSubServerPort))
    {
        printf("****Error: 初始化发布失败!");
        return false;
    }

    return true;
}
bool CLibInfo::InitRedis()
{
    if (NULL == m_pRedisManage)
    {
        m_pRedisManage = new CRedisManage;
        if (!m_pRedisManage->InitRedis((char*)m_pDeviceInfo->sRedisIP.c_str(), m_pDeviceInfo->nRedisPort))
        {
            return false;
        }
    }
    return true;
}
void *  CLibInfo::PubStatusThread(void * pParam)
{
    CLibInfo * pThis = (CLibInfo *)pParam;
    pThis->PubStatusAction();
    return NULL;
}
void CLibInfo::PubStatusAction()
{
    sleep(2);

    bool bUpdatePastCount = false;  //是否己插入当天之前的每天采集特征值数量, 插入一次后不用再更新, 只需要更新当前日期
    char pTableName[MAXLEN * 2] = { 0 };
    char pKey[MAXLEN] = { 0 };
    char pValue[MAXLEN] = { 0 };

    fd_set fdsr;
    FD_ZERO(&fdsr);
    FD_SET(m_nPipe[0], &fdsr);
    struct timeval tv;

    LPSUBMESSAGE pSubMessage = new SUBMESSAGE;
    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType&allocator = document.GetAllocator();
    char pProgress[10] = { 0 };

    do
    {
        //发布卡口状态消息给检索服务, 用于检索任务超时检查
        m_pZeromqManage->PubMessage(DEVICESTATUS, (char *)m_pDeviceInfo->sLibID.c_str());

        if (1 == m_pDeviceInfo->nServerType)
        {
            //推送卡口总特征值数量和当天特征值数量到Redis, 用于web页面统计抓拍数量
            int nTotalCount = m_pFeatureManage->GetTotalNum();
            //int nDayCount = m_pFeatureManage->GetDayNum();
            
            map<string, string> mapValues;
            sprintf(pTableName, "%s.count", m_pDeviceInfo->sLibID.c_str());
            sprintf(pValue, "%d", nTotalCount);
            mapValues.insert(make_pair("totalcount", pValue));
            //sprintf(pValue, "%d", nDayCount);
            //mapValues.insert(make_pair("daycount", pValue));

            if (m_pFeatureManage->m_mapDayCount.size() > 0)
            {
                if (!bUpdatePastCount)
                {
                    for (map<int64_t, int>::iterator it = m_pFeatureManage->m_mapDayCount.begin();
                        it != m_pFeatureManage->m_mapDayCount.end(); it++)
                    {
                        sprintf(pKey, "%lld", it->first);
                        sprintf(pValue, "%d", it->second);
                        mapValues.insert(make_pair(pKey, pValue));
                        printf("[%s]%s %d\n", m_pDeviceInfo->sLibID.c_str(), ChangeSecondToTime(it->first / 1000).c_str(), it->second);
                    }
                    bUpdatePastCount = true;
                }
                else
                {
                    map<int64_t, int>::iterator it = m_pFeatureManage->m_mapDayCount.end();
                    it--;
                    sprintf(pKey, "%lld", it->first);
                    sprintf(pValue, "%d", it->second);
                    mapValues.insert(make_pair(pKey, pValue));
                    //printf("[%s]%s %d\n", m_pDeviceInfo->sLibID.c_str(), ChangeSecondToTime(it->first / 1000).c_str(), it->second);
                }
            }
            
            m_pRedisManage->InsertHashValue(pTableName, mapValues);
            //printf("[%s]all Record: %d, Current Day Record: %d\n", m_pDeviceInfo->sLibID.c_str(), nTotalCount, nDayCount);
        }


        //定时发送业务统计进度
        /*pthread_mutex_lock(&m_mutex);
        for (MAPSEARCHTASK::iterator it = m_mapBusinessTask.begin(); it != m_mapBusinessTask.end(); it++)
        {
            strcpy(pSubMessage->pHead, it->second->pHead);
            strcpy(pSubMessage->pOperationType, it->second->pOperationType);
            strcpy(pSubMessage->pSource, it->second->pSource);

            document.AddMember(JSONTASKID, rapidjson::StringRef(it->second->pSearchTaskID), allocator);
            sprintf(pProgress, "%d", it->second->nProgress);
            document.AddMember(JSONPROGRESS, rapidjson::StringRef(pProgress), allocator);

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            document.Accept(writer);
            pSubMessage->sPubJsonValue = string(buffer.GetString());
            printf("[%s]Pub TaskID[%s] Progress: %d.\n", pSubMessage->pSource, it->second->pSearchTaskID, it->second->nProgress);

            m_pZeromqManage->PubMessage(pSubMessage);
        }
        pthread_mutex_unlock(&m_mutex);*/

        //等待30s, 无信号则下一次循环, 有信号(只有需要退出线程时才有信号)则退出线程
        tv.tv_sec = 30;
        tv.tv_usec = 0;
        fd_set fdPipe = fdsr;
        if (0 != select(m_nPipe[0] + 1, &fdPipe, NULL, NULL, &tv)) //阻塞30s  
        {
            break;
        }
    }
    while (!m_bStopLib);
}
//向卡口增加特征值写入DB
bool CLibInfo::InsertFeatureToDB(FeatureData * pFeatureData)
{
    char pSQL[SQLMAXLEN] = { 0 };
    int nSQLLen = 0;

    sprintf(pSQL,
        "insert into %s%s(faceuuid, time, imagedisk, imageserverip, ftlen, facerect, feature) "
        "values('%s', %lld, '%s', '%s', %d, '%s', '",
        CHECKPOINTPREFIX, m_pDeviceInfo->sLibID.c_str(), pFeatureData->FaceUUID, pFeatureData->CollectTime, pFeatureData->pDisk,
        pFeatureData->pImageIP, pFeatureData->FeatureValueLen, pFeatureData->pFaceRect);
    nSQLLen = strlen(pSQL);
    nSQLLen += mysql_real_escape_string(&m_mysql, pSQL + nSQLLen, (const char*)pFeatureData->FeatureValue, pFeatureData->FeatureValueLen);
    memcpy(pSQL + nSQLLen, "')", 2);
    nSQLLen += 2;
    int nRet = mysql_real_query(&m_mysql, pSQL, nSQLLen);
    if (nRet == 1)
    {
        const char * pErrorMsg = mysql_error(&m_mysql);
        printf("Excu SQL Failed[\"%s\"], SQL:\n%s\n", pErrorMsg, pSQL);

        if (ReConnectDB())
        {
            int nRet = mysql_real_query(&m_mysql, pSQL, nSQLLen);
            if (nRet == 1)
            {
                const char * pErrorMsg = mysql_error(&m_mysql);
                printf("ErrorAgain: Excu SQL Failed[\"%s\"], SQL:\n%s\n", pErrorMsg, pSQL);

                return false;
            }
        }
        else
        {
            return false;
        }
    }
    return true;
}
//向重点库增加特征值写入DB
bool CLibInfo::InsertFeatureToDB(LPKEYLIBFEATUREDATA  pFeatureData)
{
    char pSQL[SQLMAXLEN] = { 0 };
    int nSQLLen = 0;

    sprintf(pSQL,
        "insert into %s(faceuuid, imagedisk, imageserverip, ftlen, feature) "
        "values('%s', '%s', '%s', %d, '",
        m_pDeviceInfo->sLibID.c_str(), pFeatureData->pFaceUUID, pFeatureData->pDisk,
        pFeatureData->pImageIP, pFeatureData->nFeatureLen);
    nSQLLen = strlen(pSQL);
    nSQLLen += mysql_real_escape_string(&m_mysql, pSQL + nSQLLen, (const char*)pFeatureData->pFeature, pFeatureData->nFeatureLen);
    memcpy(pSQL + nSQLLen, "')", 2);
    nSQLLen += 2;
    int nRet = mysql_real_query(&m_mysql, pSQL, nSQLLen);
    if (nRet == 1)
    {
        const char * pErrorMsg = mysql_error(&m_mysql);
        printf("Excu SQL Failed[\"%s\"], SQL:\n%s\n", pErrorMsg, pSQL);

        if (ReConnectDB())
        {
            int nRet = mysql_real_query(&m_mysql, pSQL, nSQLLen);
            if (nRet == 1)
            {
                const char * pErrorMsg = mysql_error(&m_mysql);
                printf("ErrorAgain: Excu SQL Failed[\"%s\"], SQL:\n%s\n", pErrorMsg, pSQL);

                return false;
            }
        }
        else
        {
            return false;
        }
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
            "delete from %s%s where faceuuid = '%s'",
            CHECKPOINTPREFIX, m_pDeviceInfo->sLibID.c_str(), pFaceUUID);
    }
    else
    {
        sprintf(pSQL,
            "delete from %s where faceuuid = '%s'",
            m_pDeviceInfo->sLibID.c_str(), pFaceUUID);
    }
    
    int nRet = mysql_query(&m_mysql, pSQL);
    if (1 == nRet)
    {
        const char * pErrorMsg = mysql_error(&m_mysql);
        printf("Excu SQL Failed[\"%s\"], SQL:\n%s\n", pErrorMsg, pSQL);
        return false;
    }
    return true;
}
bool CLibInfo::ClearKeyLibFromDB(bool bDel)
{
    char pSQL[SQLMAXLEN] = { 0 };
    int nSQLLen = 0;

    sprintf(pSQL, 
        "delete from %s",
        m_pDeviceInfo->sLibID.c_str());
    int nRet = mysql_query(&m_mysql, pSQL);
    if (1 == nRet)
    {
        const char * pErrorMsg = mysql_error(&m_mysql);
        printf("Excu SQL Failed[\"%s\"], SQL:\n%s\n", pErrorMsg, pSQL);
        return false;
    }

    if (bDel)
    {
        sprintf(pSQL,
            "delete from %s where libname = '%s'",
            LIBINFOTABLE, m_pDeviceInfo->sLibID.c_str());
        int nRet = mysql_query(&m_mysql, pSQL);
        if (1 == nRet)
        {
            const char * pErrorMsg = mysql_error(&m_mysql);
            printf("Excu SQL Failed[\"%s\"], SQL:\n%s\n", pErrorMsg, pSQL);
            return false;
        }
    }
    return true;
}
//Zeromq消息回调
void CLibInfo::ZeromqSubMsg(LPSUBMESSAGE pSubMessage, void * pUser)
{
    CLibInfo * pThis = (CLibInfo *)pUser;
    pThis->ParseZeromqJson(pSubMessage);
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
            SendResponseMsg(pSubMessage, JsonFormatError);
            return false;
        }
    }

    /*if (sCommand != COMMANDBUSINESSSUMMARY)
    {
        return false;
    }*/

    if (sCommand == COMMANDADD)        //增加图片
    {
        pSubMessage->nTaskType = LIBADDFEATURE;
        if (1 == m_pDeviceInfo->nServerType)
        {
            if (document.HasMember(JSONFACDUUID) && document[JSONFACDUUID].IsString() && strlen(document[JSONFACDUUID].GetString()) < MAXLEN       &&
                document.HasMember(JSONFEATURE) && document[JSONFEATURE].IsString() && strlen(document[JSONFEATURE].GetString()) < FEATURELEN    &&
                document.HasMember(JSONTIME) && document[JSONTIME].IsInt64() && strlen(document[JSONFEATURE].GetString()) > FEATUREMIXLEN &&
                document.HasMember(JSONDRIVE) && document[JSONDRIVE].IsString() && strlen(document[JSONDRIVE].GetString()) == 1 &&
                document.HasMember(JSONSERVERIP) && document[JSONSERVERIP].IsString() && strlen(document[JSONSERVERIP].GetString()) < MAXIPLEN     &&
                document.HasMember(JSONFACERECT) && document[JSONFACERECT].IsString() && strlen(document[JSONFACERECT].GetString()) < MAXIPLEN)
            {
                FeatureData * pFeatureData = new FeatureData;
                //1. 保存FaceUUID
                int nLen = strlen(document[JSONFACDUUID].GetString());
                pFeatureData->FaceUUID = new char[nLen + 1];
                strcpy(pFeatureData->FaceUUID, document[JSONFACDUUID].GetString());
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
                pFeatureData->CollectTime = document[JSONTIME].GetInt64();
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

                nRet = m_pFeatureManage->AddFeature(pFeatureData);
                if (0 == nRet)
                {
                    pthread_mutex_lock(&m_mutex);
                    if (!InsertFeatureToDB(pFeatureData))
                    {
                        nRet = MysqlQueryFailed;
                    }
                    pthread_mutex_unlock(&m_mutex);
                }
                else
                {
                    delete pFeatureData;
                    pFeatureData = NULL;
                }
                //回应消息用到
                strcpy(pSubMessage->pFaceUUID, document[JSONFACDUUID].GetString());
            }
            else
            {
                nRet = JsonFormatError;
            }
        }
        else
        {
            if (document.HasMember(JSONFACDUUID) && document[JSONFACDUUID].IsString() && strlen(document[JSONFACDUUID].GetString()) < MAXLEN &&
                document.HasMember(JSONFEATURE) && document[JSONFEATURE].IsString() && strlen(document[JSONFEATURE].GetString()) < FEATURELEN &&
                strlen(document[JSONFEATURE].GetString()) > FEATUREMIXLEN &&
                document.HasMember(JSONDRIVE) && document[JSONDRIVE].IsString() && strlen(document[JSONDRIVE].GetString()) == 1 &&
                document.HasMember(JSONSERVERIP) && document[JSONSERVERIP].IsString() && strlen(document[JSONSERVERIP].GetString()) < MAXIPLEN)
            {
                LPKEYLIBFEATUREDATA pFeatureData = new KEYLIBFEATUREDATA;
                //1. 保存FaceUUID
                int nLen = strlen(document[JSONFACDUUID].GetString());
                pFeatureData->pFaceUUID = new char[nLen + 1];
                strcpy(pFeatureData->pFaceUUID, document[JSONFACDUUID].GetString());
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

                nRet = m_pFeatureManage->AddFeature(pFeatureData);
                if (0 == nRet)
                {
                    pthread_mutex_lock(&m_mutex);
                    if (!InsertFeatureToDB(pFeatureData))
                    {
                        m_pFeatureManage->DelFeature(pFeatureData->pFaceUUID);
                        nRet = MysqlQueryFailed;
                    }
                    pthread_mutex_unlock(&m_mutex);
                }
                else
                {
                    delete pFeatureData;
                    pFeatureData = NULL;
                }
                //回应消息用到
                strcpy(pSubMessage->pFaceUUID, document[JSONFACDUUID].GetString());
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
        if (document.HasMember(JSONFACDUUID) && document[JSONFACDUUID].IsString() && strlen(document[JSONFACDUUID].GetString()) < MAXLEN)
        {
            string sFaceUUID = document[JSONFACDUUID].GetString();
            nRet = m_pFeatureManage->DelFeature(sFaceUUID.c_str());
            if (0 == nRet)
            {
                pthread_mutex_lock(&m_mutex);
                if (!DelFeatureFromDB(sFaceUUID.c_str()))
                {
                    nRet = MysqlQueryFailed;
                }
                pthread_mutex_unlock(&m_mutex);
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
            pthread_mutex_lock(&m_mutex);
            if (!ClearKeyLibFromDB())
            {
                nRet = MysqlQueryFailed;
            }
            pthread_mutex_unlock(&m_mutex);
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
                        pSearchTask->listFeatureInfo.push_back(pSearchFeature);
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
                        pSearchTask->listFeatureInfo.push_back(pSearchFeature);
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
                /*pthread_mutex_lock(&m_mutex);
                m_mapBusinessTask.insert(make_pair(pSearchTask->pSearchTaskID, pSearchTask));
                pthread_mutex_unlock(&m_mutex);*/
                m_pFeatureManage->AddSearchTask(pSearchTask);
            }
            else
            {
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
        nRet = CommandNotFound;
    }

    if (INVALIDERROR != nRet || (sCommand != COMMANDSEARCH && sCommand != COMMANDCAPTURE && sCommand != COMMANDBUSINESSSUMMARY))
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
                pThis->SendResponseMsg(pSubMessage, pSearchTask->nError);
            }
        }
        else if (LIBBUSINESS == pSubMessage->nTaskType)
        {
            pThis->SendResponseMsg(pSubMessage, pSearchTask->nError, pSearchTask);
        }
    }
    else
    {
        pThis->SendResponseMsg(pSubMessage, pSearchTask->nError);
    }
    /*pthread_mutex_lock(&pThis->m_mutex);
    pThis->m_mapBusinessTask.erase(pSearchTask->pSearchTaskID);
    pthread_mutex_unlock(&pThis->m_mutex);*/

    delete pSubMessage;
    pSubMessage = NULL;
    delete pSearchTask;
    pSearchTask = NULL;
    return;
}
//回应请求
void CLibInfo::SendResponseMsg(LPSUBMESSAGE pSubMessage, int nEvent, LPSEARCHTASK pSearchTask)
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
            document.AddMember(JSONFACDUUID, rapidjson::StringRef(pSubMessage->pFaceUUID), allocator);
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
                    object.AddMember(JSONFACDUUID, rapidjson::StringRef(it->second->pFaceUUID), allocator);
                    object.AddMember(JSONSCORE, it->second->nScore, allocator);
                    object.AddMember(JSONTIME, it->second->nTime, allocator);
                    object.AddMember(JSONDRIVE, rapidjson::StringRef(it->second->pDisk), allocator);
                    object.AddMember(JSONSERVERIP, rapidjson::StringRef(it->second->pImageIP), allocator);
                    object.AddMember(JSONFACERECT, rapidjson::StringRef(it->second->pFaceRect), allocator);

                    array.PushBack(object, allocator);
                }
                document.AddMember("info", array, allocator);
            }
            else
            {
                document.AddMember(JSONSEARCHCOUNT, pSubMessage->nTotalNum, allocator);
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
                    object.AddMember(JSONFACDUUID, rapidjson::StringRef(it->second->pFaceUUID), allocator);
                    object.AddMember(JSONDEVICEID, rapidjson::StringRef(m_pDeviceInfo->sLibID.c_str()), allocator);
                    object.AddMember(JSONDRIVE, rapidjson::StringRef(it->second->pDisk), allocator);
                    object.AddMember(JSONSERVERIP, rapidjson::StringRef(it->second->pImageIP), allocator);
                    object.AddMember(JSONFACERECT, rapidjson::StringRef(it->second->pFaceRect), allocator);
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
                        objectPerson.AddMember(JSONFACDUUID, rapidjson::StringRef(itPerson->second->pFaceUUID), allocator);
                        objectPerson.AddMember(JSONTIME, itPerson->second->nTime, allocator);
                        objectPerson.AddMember(JSONDRIVE, rapidjson::StringRef(itPerson->second->pDisk), allocator);
                        objectPerson.AddMember(JSONSERVERIP, rapidjson::StringRef(itPerson->second->pImageIP), allocator);
                        objectPerson.AddMember(JSONFACERECT, rapidjson::StringRef(itPerson->second->pFaceRect), allocator);
                        objectPerson.AddMember(JSONSCORE, itPerson->second->nScore, allocator);

                        arrayPerson.PushBack(objectPerson, allocator);
                        nPeopleNum++;
                    }
                    object.AddMember("data", arrayPerson, allocator);

                    array.PushBack(object, allocator);
                }
            }
            document.AddMember("info", array, allocator);


            break;
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
            document.AddMember(JSONFACDUUID, rapidjson::StringRef(pSubMessage->pFaceUUID), allocator);
        }
        document.AddMember("errorcode", nEvent, allocator);
        document.AddMember("errorMessage", rapidjson::StringRef(pErrorMsg), allocator);
    }
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    pSubMessage->sPubJsonValue = string(buffer.GetString());
    //发布回应消息
    m_pZeromqManage->PubMessage(pSubMessage);
    switch (pSubMessage->nTaskType)
    {
    case LIBADDFEATURE: case LIBDELFEATURE:
        //printf("Rep Client json:\n%s\n==============================\n", pSubMessage->sPubJsonValue.c_str());
        break;
    case LIBCAPTURE:
        break;
    case LIBSEARCH:
        printf("Rep Client json:\n%s\n==============================\n", pSubMessage->sPubJsonValue.c_str());
        break;
    case LIBBUSINESS:
        printf("Checkpoint[%s][%s] Business Rep, json size: %d, People: %d.\n",
            m_pDeviceInfo->sLibID.c_str(), pSearchTask->pSearchTaskID, pSubMessage->sPubJsonValue.size(), nPeopleNum);
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
    map<string, string> mapValues;
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

        if (!m_pRedisManage->InsertHashValue(pTableName, mapValues))
        {
            nRet = InsertRedisFailed;
            break;
        }

        /*if (!m_pRedisManage->InsertHashValue(pTableName, REDISDEVICEID, m_pDeviceInfo->sLibID.c_str(), m_pDeviceInfo->sLibID.size()))
        {
            nRet = InsertRedisFailed;
            break;
        }
        if (!m_pRedisManage->InsertHashValue(pTableName, REDISFACEUUID, it->second->pFaceUUID, strlen(it->second->pFaceUUID)))
        {
            nRet = InsertRedisFailed;
            break;
        }
        itoa(it->second->nScore, pBuf, 10);
        if (!m_pRedisManage->InsertHashValue(pTableName, REDISSCORE, pBuf, strlen(pBuf)))
        {
            nRet = InsertRedisFailed;
            break;
        }
        _i64toa(it->second->nTime, pBuf, 10);
        if (!m_pRedisManage->InsertHashValue(pTableName, REDISTIME, pBuf, strlen(pBuf)))
        {
            nRet = InsertRedisFailed;
            break;
        }
        if (!m_pRedisManage->InsertHashValue(pTableName, REDISDRIVE, it->second->pDisk, strlen(it->second->pDisk)))
        {
            nRet = InsertRedisFailed;
            break;
        }
        if (!m_pRedisManage->InsertHashValue(pTableName, REDISIMAGEIP, it->second->pImageIP, strlen(it->second->pImageIP)))
        {
            nRet = InsertRedisFailed;
            break;
        }
        itoa(it->second->nHitCount, pBuf, 10);
        if (!m_pRedisManage->InsertHashValue(pTableName, REDISHITCOUNT, pBuf, strlen(pBuf)))
        {
            nRet = InsertRedisFailed;
            break;
        }
        if (!m_pRedisManage->InsertHashValue(pTableName, REDISFACERECT, it->second->pFaceRect, strlen(it->second->pFaceRect)))
        {
            nRet = InsertRedisFailed;
            break;
        }*/
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
