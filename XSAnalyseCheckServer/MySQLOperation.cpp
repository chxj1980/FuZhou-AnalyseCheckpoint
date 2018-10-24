#include "stdafx.h"
#include "MySQLOperation.h"


CMySQLOperation::CMySQLOperation(int nLoadURL)
{
    mysql_library_init(0, 0, 0);
    m_nLoadURL = nLoadURL;
#ifdef __WINDOWS__
    InitializeCriticalSection(&m_cs);
#else
    pthread_mutex_init(&m_mutex, NULL);
#endif
}


CMySQLOperation::~CMySQLOperation()
{
#ifdef __WINDOWS__
    DeleteCriticalSection(&m_cs);
#else
    pthread_mutex_destroy(&m_mutex);
#endif
}
bool CMySQLOperation::ConnectDB(const char * pIP, const char * pUser, const char * pPassword, const char * pDBName, int nPort)
{
    mysql_init(&m_mysql);
    char value = 1;
    mysql_options(&m_mysql, MYSQL_OPT_RECONNECT, (char *)&value);
    if (!mysql_real_connect(&m_mysql, pIP, pUser, pPassword, pDBName, nPort, NULL, 0))
    {
        const char * pErrorMsg = mysql_error(&m_mysql);
        printf("%s\n", pErrorMsg);
        printf("***Warning: CAnalyseServer::mysql_real_connect Failed, Please Check MySQL Service is start!\n");
        printf("DBInfo: %s:%s:%s:%s:%d\n", pIP, pUser, pPassword, pDBName, nPort);
        return false;
    }
    else
    {
        printf("CAnalyseServer::Connect MySQL Success!\n");
    }
    return true;
}
bool CMySQLOperation::DisConnectDB()
{
    mysql_close(&m_mysql);
    return true;
}
bool CMySQLOperation::QueryCommand(char * pSQL)
{
    bool bRet = false;
    EnterMutex();
    mysql_ping(&m_mysql);
    if (1 == mysql_query(&m_mysql, pSQL))
    {
        const char * pErrorMsg = mysql_error(&m_mysql);
        if (string(pErrorMsg).find("already exists") == string::npos)    //表己存在则不返回错误
        {
            printf("****Error: Excu SQL Failed, SQL:\n%s\nError: %s\n", pSQL, pErrorMsg);
        }
        else
        {
            bRet = true;
        }
    }
    else
    {
        bRet = true;
    }
    LeaveMutex();
    return bRet;
}
bool CMySQLOperation::GetQueryResult(char * pSQL, MYSQL_RES * &pResult)
{
    bool bRet = false;
    EnterMutex();
    mysql_ping(&m_mysql);
    if (1 == mysql_query(&m_mysql, pSQL))
    {
        const char * pErrorMsg = mysql_error(&m_mysql);
        printf("****Error: Excu SQL Failed, SQL:\n%s\nError: %s\n", pSQL, pErrorMsg);
    }
    else
    {
        pResult = mysql_store_result(&m_mysql);
        if (NULL == pResult)
        {
            const char * pErrorMsg = mysql_error(&m_mysql);
            printf("****Error: mysql_store_result Failed, SQL:\n%s\n", pSQL);
        }
        else
        {
            bRet = true;
        }
    }
    LeaveMutex();
    return bRet;
}
//向卡口增加特征值写入DB
bool CMySQLOperation::InsertFeatureToDB(char * pLibID, FeatureData * pFeatureData)
{
    bool bRet = true;
    char pSQL[SQLMAXLEN] = { 0 };
    int nSQLLen = 0;

    EnterMutex();
    if (m_nLoadURL)
    {
        sprintf(pSQL,
            "insert into `%s%s`(faceuuid, time, imagedisk, imageserverip, ftlen, facerect, face_url, bkg_url, feature) "
            "values('%s', %lld, '%s', '%s', %d, '%s', '%s', '%s', '",
            CHECKPOINTPREFIX, pLibID, pFeatureData->FaceUUID, pFeatureData->CollectTime, pFeatureData->pDisk,
            pFeatureData->pImageIP, pFeatureData->FeatureValueLen, pFeatureData->pFaceRect, pFeatureData->pFaceURL, pFeatureData->pBkgURL);
    }
    else
    {
        sprintf(pSQL,
            "insert into `%s%s`(faceuuid, time, imagedisk, imageserverip, ftlen, facerect, feature) "
            "values('%s', %lld, '%s', '%s', %d, '%s', '",
            CHECKPOINTPREFIX, pLibID, pFeatureData->FaceUUID, pFeatureData->CollectTime, pFeatureData->pDisk,
            pFeatureData->pImageIP, pFeatureData->FeatureValueLen, pFeatureData->pFaceRect);
    }
    
    nSQLLen = strlen(pSQL);
    nSQLLen += mysql_real_escape_string(&m_mysql, pSQL + nSQLLen, (const char*)pFeatureData->FeatureValue, pFeatureData->FeatureValueLen);
    memcpy(pSQL + nSQLLen, "')", 2);
    nSQLLen += 2;

    mysql_ping(&m_mysql);
    int nRet = mysql_real_query(&m_mysql, pSQL, nSQLLen);
    if (nRet == 1)
    {
        const char * pErrorMsg = mysql_error(&m_mysql);
        //printf("Excu SQL Failed[\"%s\"], SQL:\n%s\n", pErrorMsg, pSQL);
        printf("****Error: InsertFeatureToCheckpoint DB Excu SQL Failed[\"%s\"]\n", pErrorMsg);
        bRet = false;
    }
    LeaveMutex();
    return bRet;
}
//向重点库增加特征值写入DB
bool CMySQLOperation::InsertFeatureToDB(char * pLibID, LPKEYLIBFEATUREDATA  pFeatureData)
{
    bool bRet = true;
    char pSQL[SQLMAXLEN] = { 0 };
    int nSQLLen = 0;

    EnterMutex();
    if (m_nLoadURL)
    {
        sprintf(pSQL,
            "insert into %s(faceuuid, imagedisk, imageserverip, ftlen, face_url, feature) "
            "values('%s', '%s', '%s', %d, '%s', '",
            pLibID, pFeatureData->pFaceUUID, pFeatureData->pDisk,
            pFeatureData->pImageIP, pFeatureData->nFeatureLen, pFeatureData->pFaceURL);
    }
    else
    {
        sprintf(pSQL,
            "insert into %s(faceuuid, imagedisk, imageserverip, ftlen, feature) "
            "values('%s', '%s', '%s', %d, '%s', '",
            pLibID, pFeatureData->pFaceUUID, pFeatureData->pDisk,
            pFeatureData->pImageIP, pFeatureData->nFeatureLen);
    }
    
    nSQLLen = strlen(pSQL);
    nSQLLen += mysql_real_escape_string(&m_mysql, pSQL + nSQLLen, (const char*)pFeatureData->pFeature, pFeatureData->nFeatureLen);
    memcpy(pSQL + nSQLLen, "')", 2);
    nSQLLen += 2;

    mysql_ping(&m_mysql);
    int nRet = mysql_real_query(&m_mysql, pSQL, nSQLLen);
    if (nRet == 1)
    {
        const char * pErrorMsg = mysql_error(&m_mysql);
        printf("****Error: InsertFeatureToKeylib DB Excu SQL Failed[\"%s\"], SQL:\n%s\n", pErrorMsg, pSQL);
        bRet = false;
    }
    LeaveMutex();
    return bRet;
}
void CMySQLOperation::EnterMutex()
{
#ifdef __WINDOWS__
    EnterCriticalSection(&m_cs);
#else
    pthread_mutex_lock(&m_mutex);
#endif
}
void CMySQLOperation::LeaveMutex()
{
#ifdef __WINDOWS__
    LeaveCriticalSection(&m_cs);
#else
    pthread_mutex_unlock(&m_mutex);
#endif
}