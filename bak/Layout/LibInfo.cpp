#include "LibInfo.h"
#include "stdio.h"
#include "ZBase64.h"
#include "time.h"

CLibInfo::CLibInfo()
{
    memset(m_pServerIP, 0, sizeof(m_pServerIP));
    m_nSubPort = 0;
    m_nPubPort = 0;

    m_pFeatureManage = NULL;
    m_pSingleFeatureNode = NULL;
    m_pZeromqManage = NULL;
    m_pDeviceInfo = NULL;
    m_hThreadPubLibInfo = -1;
    m_bStopLib = false;
    pipe(m_nPipe);
}
CLibInfo::~CLibInfo()
{
    close(m_nPipe[0]);
    close(m_nPipe[1]);
}
//开始服务
bool CLibInfo::Start(LPDEVICEINFO pDeviceInfo, LPSINGLEFEATURENODE pSingleFeatureNode)
{
    m_pSingleFeatureNode = pSingleFeatureNode;
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
    if (NULL != m_pDeviceInfo)
    {
        delete m_pDeviceInfo;
        m_pDeviceInfo = NULL;
    }
    if (NULL != m_pSingleFeatureNode)
    {
        m_pSingleFeatureNode->mapSingleFeatureNode.clear();
        delete m_pSingleFeatureNode;
        m_pSingleFeatureNode = NULL;
    }
    mysql_close(&m_mysql);
    pthread_mutex_destroy(&m_cs);
    if (-1 != m_hThreadPubLibInfo)
    {
        pthread_join(m_hThreadPubLibInfo, NULL);
        printf("--Pub Lib Info Thread End.\n");
    }
    return true;
}
//初始化服务
bool CLibInfo::Init()
{
    printf("---------------\nLib [%s] Init...\n", m_pDeviceInfo->sLibID.c_str());
    //初始化临界区
    pthread_mutex_init(&m_cs, NULL);
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
        nRet = m_pFeatureManage->Init((char*)m_pDeviceInfo->sLibID.c_str(), m_pDeviceInfo->nMaxCount, m_pSingleFeatureNode);
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
    if (!GetKeyLibFeatureFromDB())
    {
        printf("***Warning: CKeyLibInfo::GetCheckpointFeatureFromDB Failed.\n");
        return false;
    }
    return true;
}
bool CLibInfo::GetKeyLibFeatureFromDB()
{
    char pSQL[SQLMAXLEN] = { 0 };
    int nRet = INVALIDERROR;

    //增加重点库特征值表
    sprintf(pSQL,
        "create table %s(faceuuid varchar(%d), feature blob(%d), ftlen int(10), time bigint(20), "
        "imagedisk varchar(1), imageserverip varchar(20))",
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
    else
    {
        //创建主键索引
        sprintf(pSQL, "ALTER TABLE %s ADD PRIMARY KEY(faceuuid)", m_pDeviceInfo->sLibID.c_str());
        nRet = mysql_query(&m_mysql, pSQL);
        if (nRet == 1)
        {
            const char * pErrorMsg = mysql_error(&m_mysql);
            printf("Excu SQL Failed[\"%s\"], SQL:\n%s\n", pErrorMsg, pSQL);
        }
    }
    


    //获取重点库特征值信息
    int nRead = 0;
    sprintf(pSQL, "select faceuuid, feature, ftlen, imagedisk, imageserverip from %s order by faceuuid", m_pDeviceInfo->sLibID.c_str());
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
                pKeyLibFeatureData->pDisk = pDisk[0];
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
    char pSubAdd[MAXLEN] = { 0 };
    sprintf(pSubAdd, "%sedit", m_pDeviceInfo->sLibID.c_str());
    m_pZeromqManage->AddSubMessage(pSubAdd);
    
    if (!m_pZeromqManage->InitPub(NULL, 0, (char*)m_pDeviceInfo->sPubServerIP.c_str(), m_pDeviceInfo->nSubServerPort))
    {
        printf("****Error: 初始化发布失败!");
        return false;
    }

    pthread_create(&m_hThreadPubLibInfo, NULL, PubLibInfoThread, (void*)this);
    printf("--Pub Lib Info Thread Start.\n");

    return true;
}
void * CLibInfo::PubLibInfoThread(void * pParam)
{
    CLibInfo * pThis = (CLibInfo *)pParam;
    pThis->PubLibInfoAction();
    return NULL;
}
void CLibInfo::PubLibInfoAction()
{
    usleep(1000*2000);

    fd_set fdsr;
    FD_ZERO(&fdsr);
    FD_SET(m_nPipe[0], &fdsr);
    struct timeval tv;
    

    LPSUBMESSAGE pSubMessage = new SUBMESSAGE;
    strcpy(pSubMessage->pHead, LAYOUTLIBINFO);
    strcpy(pSubMessage->pOperationType, "UpdateLibAddress");
    strcpy(pSubMessage->pSource, m_pDeviceInfo->sLibID.c_str());
    printf("Pub Lib[%s] Feature Address Info...\n", m_pDeviceInfo->sLibID.c_str());
    while (!m_bStopLib)
    {
        rapidjson::Document document;
        document.SetObject();
        rapidjson::Document::AllocatorType&allocator = document.GetAllocator();
        rapidjson::Value array(rapidjson::kArrayType);

        for (MAPKEYLIBINFO::iterator it = m_pSingleFeatureNode->mapSingleFeatureNode.begin();
            it != m_pSingleFeatureNode->mapSingleFeatureNode.end(); it++)
        {
            rapidjson::Value object(rapidjson::kObjectType);
            object.AddMember(JSONLIBINDEX, it->first, allocator);

            array.PushBack(object, allocator);
        }
        document.AddMember("libinfo", array, allocator);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        document.Accept(writer);
        pSubMessage->sPubJsonValue = buffer.GetString();

        m_pZeromqManage->PubMessage(pSubMessage);

        tv.tv_sec = 30;
        tv.tv_usec = 0;
        fd_set fdPipe = fdsr;
        if(0 != select(m_nPipe[0] + 1, &fdPipe, NULL, NULL, &tv)) //阻塞10s  
        {
            break;
        }
    }
    return;
}
//向重点库增加特征值写入DB
bool CLibInfo::InsertFeatureToDB(LPKEYLIBFEATUREDATA  pFeatureData)
{
    char pSQL[SQLMAXLEN] = { 0 };
    int nSQLLen = 0;

    sprintf(pSQL,
        "insert into %s(faceuuid, imagedisk, imageserverip, ftlen, feature) "
        "values('%s', '%c', '%s', %d, '",
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
    
    sprintf(pSQL,
        "delete from %s where faceuuid = '%s'",
        m_pDeviceInfo->sLibID.c_str(), pFaceUUID);
    
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
        "delete from %s", m_pDeviceInfo->sLibID.c_str());
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
    if (sCommand == COMMANDADD)        //增加图片
    {
        pSubMessage->nTaskType = LIBADDFEATURE;
        
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
            pFeatureData->pDisk = document[JSONDRIVE].GetString()[0];
            //5. 保存图片保存服务器IP
            nLen = strlen(document[JSONSERVERIP].GetString());
            pFeatureData->pImageIP = new char[nLen + 1];
            strcpy(pFeatureData->pImageIP, document[JSONSERVERIP].GetString());
            pFeatureData->pImageIP[nLen] = '\0';

            nRet = m_pFeatureManage->AddFeature(pFeatureData);
            if (0 == nRet)
            {
                pthread_mutex_lock(&m_cs);
                if (!InsertFeatureToDB(pFeatureData))
                {
                    m_pFeatureManage->DelFeature(pFeatureData->pFaceUUID);
                    nRet = MysqlQueryFailed;
                }
                pthread_mutex_unlock(&m_cs);
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
    else if (sCommand == COMMANDDEL)  //删除图片
    {
        pSubMessage->nTaskType = LIBDELFEATURE;
        if (document.HasMember(JSONFACDUUID) && document[JSONFACDUUID].IsString() && strlen(document[JSONFACDUUID].GetString()) < MAXLEN)
        {
            string sFaceUUID = document[JSONFACDUUID].GetString();
            nRet = m_pFeatureManage->DelFeature(sFaceUUID.c_str());
            if (0 == nRet)
            {
                pthread_mutex_lock(&m_cs);
                if (!DelFeatureFromDB(sFaceUUID.c_str()))
                {
                    nRet = MysqlQueryFailed;
                }
                pthread_mutex_unlock(&m_cs);
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
            pthread_mutex_lock(&m_cs);
            if (!ClearKeyLibFromDB())
            {
                nRet = MysqlQueryFailed;
            }
            pthread_mutex_unlock(&m_cs);
        }
    }
    else
    {
        nRet = CommandNotFound;
    }
    if (INVALIDERROR != nRet || sCommand != COMMANDSEARCH)
    {
        strcpy(pSubMessage->pHead, pSubMessage->pSource);
        strcpy(pSubMessage->pSource, m_pDeviceInfo->sLibID.c_str());
        SendResponseMsg(pSubMessage, nRet);
    }

    return true;
}
//回应请求
void CLibInfo::SendResponseMsg(LPSUBMESSAGE pSubMessage, int nEvent)
{
    char pErrorMsg[64] = { 0 };
    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType&allocator = document.GetAllocator();
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
    pSubMessage->sPubJsonValue = buffer.GetString();
    //发布回应消息
    m_pZeromqManage->PubMessage(pSubMessage);
    printf("Rep Client json:\n%s\n==============================\n", pSubMessage->sPubJsonValue.c_str());
    return;
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
