#ifndef MySQLOperation_h

#include <mysql/mysql.h>
#include "DataDefine.h"
#ifdef __WINDOWS__
#pragma comment( lib, "libmysql.lib" )
#else
#include <unistd.h>
#include <stdio.h>
#endif
class CMySQLOperation
{
public:
    CMySQLOperation(int nLoadURL = 1);
    ~CMySQLOperation();
public:
    bool ConnectDB(const char * pIP, const char * pUser, const char * pPassword, const char * pDBName, int nPort);
    bool DisConnectDB();
    //执行SQL语句
    bool QueryCommand(char * pSQL);
    //取出查询语句结果
    bool GetQueryResult(char * pSQL, MYSQL_RES * &pResult);

    //向卡口增加特征值写入DB
    bool InsertFeatureToDB(char * pLibID, FeatureData * pFeatureData);
    //向重点库增加特征值写入DB
    bool InsertFeatureToDB(char * pLibID, LPKEYLIBFEATUREDATA pFeatureData);

    void EnterMutex();
    void LeaveMutex();
private:
    MYSQL m_mysql;     //连接MySQL数据库
    int m_nLoadURL;     //是否写URL到DB
#ifdef __WINDOWS__
    CRITICAL_SECTION m_cs;
#else
    pthread_mutex_t m_mutex;            //临界区
#endif
};


#define MySQLOperation_h
#endif