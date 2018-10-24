#ifndef datadefine_h

#include <map>
#include <string>
#include <string.h>
#include <list>
#include <set>
#include <stdint.h>
#include "FaceFeatureDB.h"
using namespace std;

#define UNIXBEGINTIME   946656000000    //2000-01-01 00:00:00
#define UNIXENDTIME   32503651200000    //3000-01-01 00:00:00
//数据库表名
#define LIBINFOTABLE        "checklibinfo"
#define CHECKPOINTPREFIX    "checkpoint"
#define ONEDAYTIME      (24 * 3600 * 1000)

#define COMMANDADDLIB   "addlib"    //增加关联卡口或重点库
#define COMMANDDELLIB   "dellib"    //删除关联卡口或重点库
#define COMMANDADD      "add"       //增加特征值命令
#define COMMANDDEL      "del"       //删除特征值命令
#define COMMANDCLEAR    "clear"     //清空特征值库命令
#define COMMANDSEARCH   "search"    //搜索特征值命令
#define COMMANDCAPTURE  "capture"   //获取抓拍图片
#define COMMANDBUSINESSSUMMARY "business.summary"   //频繁出入等统计

//Redis Hash表字段定义
#define REDISDEVICEID   "libid"     //卡口ID或重点库ID
#define REDISFACEUUID   "faceuuid"  //faceuuid
#define REDISSCORE      "score"     //分数
#define REDISTIME       "time"      //时间
#define REDISDRIVE      "drive"     //盘符
#define REDISIMAGEIP    "imageip"   //图片保存服务器IP
#define REDISHITCOUNT   "hitcount"  //图片命中次数
#define REDISFACERECT   "facerect"  //人脸坐标
#define HASHOVERTIME    1800        //数据保存时间

#define SUBALLCHECKPOINT    "allcheckpoints"    //订阅所有卡口
#define SUBALLLIB           "allfocuslib"       //订阅所有重点库

#define DEVICESTATUS    "libstatus"  //卡口状态发布

//Json串字段定义
#define JSONDLIBID      "libid"         //卡口或重点库ID
#define JSONLIBTYPE     "type"          //重点库类型, 2: 核查库, 3: 布控库
//卡口
#define JSONDEVICEID    "checkpoint"    //卡口ID
#define JSONFACDUUID    "faceuuid"      //特征值FaceUUID
#define JSONFACERECT    "facerect"      //人脸图片坐标
#define JSONFEATURE     "feature"       //特征值
#define JSONTIME        "imagetime"     //抓拍时间
#define JSONDRIVE       "imagedisk"     //图片保存在采集服务上的驱动盘符
#define JSONSERVERIP    "imageip"       //采集服务IP

#define JSONTASKID      "taskid"        //检索任务ID
#define JSONFACE        "faces"         //检索特征值数组
#define JSONFACENO      "faceno"
#define JSONFACEFEATURE "feature"
#define JSONBEGINTIME   "starttime"     //检索开始时间
#define JSONENDTIME     "endtime"       //检索结束时间
#define JSONSIMILAR     "similar"       //检索阈值
#define JSONASYNCRETURN "asyncreturn"   //是否立即返回搜索结果(团伙分析服务)
#define JSONSCORE       "score"         //检索分数

#define JSONSEARCHCOUNT     "count"         //检索结果数
#define JSONSEARCHHIGHEST   "highest"       //检索最高分数
#define JSONSEARCHLATESTTIME "lasttime"     //检索最新图片时间
#define JSONCAPTURESIZE     "size"          //获取抓拍图片最大数
#define JSONRETURNFEATURE   "feature"       //抓拍是否返回特征值

//频繁出入等统计
#define JSONNIGHTSTARTTIME  "nightstarttime"    //夜晚开始时间
#define JSONNIGHTENDTIME    "nightendtime"      //夜晚结束时间
#define JSONFREQUENTLY      "frequency"         //统计次数阈值
#define JSONPERSONID        "person"            //统计人员ID
#define JSONPROGRESS        "progress"          //业务统计进度返回

#define SQLMAXLEN       1024 * 4    //SQL语句最大长度
#define MAXIPLEN        20          //IP, FaceRect最大长度
#define MAXLEN          36          //FaceUUID, LibName, DeviceID最大长度
#define FEATURELEN      1024 * 4    //Feature最大长度
#define FEATUREMIXLEN   500         //Feature最短长度, 小于此长度定为不合法
#define RESOURCEMAXNUM  150         //最大HTTP消息队列数, 超过后将无法塞入队列处理, 直接返回失败
#define THREADNUM       8           //线程数
#define THREADWAITTIME  5           //线程等待时间(ms)
#define FEATURESEARCHWAIT   2000    //多线程搜索等待超时时间

enum ErrorCode
{
    INVALIDERROR = 0,       //无错误
    ServerNotInit = 12001,  //服务尚未初始化完成
    DBAleadyExist,          //库己存在
    DBNotExist,             //库不存在
    FaceUUIDAleadyExist,    //FaceUUID己存在
    FaceUUIDNotExist,       //FaceUUID不存在
    ParamIllegal,           //参数非法
    NewFailed,              //new申请内存失败
    JsonFormatError,        //json串格式错误
    CommandNotFound,        //不支持的命令
    HttpMsgUpperLimit,      //http消息待处理数量己达上限
    PthreadMutexInitFailed, //临界区初始化失败
    FeatureNumOverMax,      //批量增加特征值数量超标
    JsonParamIllegal,       //Json串有值非法
    MysqlQueryFailed,       //Mysql操作执行失败.
    ParamLenOverMax,        //参数长度太长
    LibAleadyExist,         //卡口己存在
    LibNotExist,            //卡口不存在
    CheckpointInitFailed,   //卡口类初始化失败
    VerifyFailed,           //特征值比对失败, 失败一次后即不再比对
    HttpSocketBindFailed,   //http端口绑定失败
    CreateTableFailed,      //在数据库创建表失败
    SearchTimeWrong,        //错误的时间段(开始时间大于结束时间)
    SearchNoDataOnTime,      //指定时间段内没有数据
    AddFeatureToCheckpointFailed,    //增加特征值到卡口DB错误
    SocketInitFailed,        //网络初始化失败
    InsertRedisFailed       //数据插入Redis错误
};

//任务类型
enum TASKTYPE
{
    INVALIDTASK,
    ADDLIB,              //增加重点库或卡口
    DELLIB,              //删除重点库或卡口
    LIBADDFEATURE,       //向卡口增加特征值
    LIBDELFEATURE,       //从卡口删除特征值
    LIBCLEARFEATURE,     //从卡口清空特征值
    LIBSEARCH,           //按时间, 卡口搜索特征值
    LIBCAPTURE,          //查询指定时间段内抓拍图片
    LIBBUSINESS          //频繁出入, 深夜出入, 昼伏夜出等业务统计
};

typedef struct _DeviceInfo
{
    int nServerType;    //1: 卡口分析服务, 2: 重点库分析服务
    string sLibID;
    int nMaxCount;
    string sDBIP;
    int nDBPort;
    string sDBName;
    string sDBUser;
    string sDBPassword;
    string sPubServerIP;
    int nPubServerPort;
    int nSubServerPort;
    string sRedisIP;
    int nRedisPort;
}DEVICEINFO, *LPDEVICEINFO;

typedef struct _SubMessage
{
    char pHead[MAXLEN];         //订阅消息头
    char pOperationType[MAXLEN];//订阅消息操作类型
    char pSource[MAXLEN];       //订阅消息源
    char pSubJsonValue[FEATURELEN * 10];       //订阅消息Json串
    string sPubJsonValue;       //发布消息Json串

    char pDeviceID[MAXLEN];     //卡口ID
    char pFaceUUID[MAXLEN];     //特征值FaceUUID
    char pFeature[FEATURELEN];  //特征值

    TASKTYPE nTaskType;         //任务类型
    char pSearchTaskID[MAXLEN]; //检索任务ID
    int nHighestScore;          //检索最高分数
    int64_t nLatestTime;   //检索最新时间
    unsigned int nTotalNum;     //检索总数
    _SubMessage()
    {
        memset(pHead, 0, sizeof(pHead));
        memset(pOperationType, 0, sizeof(pOperationType));
        memset(pSource, 0, sizeof(pSource));
        memset(pSubJsonValue, 0, sizeof(pSubJsonValue));
        sPubJsonValue = "";
        memset(pDeviceID, 0, sizeof(pDeviceID));
        memset(pFaceUUID, 0, sizeof(pFaceUUID));
        memset(pFeature, 0, sizeof(pFeature));
        nTaskType = INVALIDTASK;
        memset(pSearchTaskID, 0, sizeof(pSearchTaskID));
        nHighestScore = 0;
        nLatestTime = 0;
        nTotalNum = 0;
    }
    void Init()
    {
        memset(pHead, 0, sizeof(pHead));
        memset(pOperationType, 0, sizeof(pOperationType));
        memset(pSource, 0, sizeof(pSource));
        memset(pSubJsonValue, 0, sizeof(pSubJsonValue));
        sPubJsonValue = "";
        memset(pDeviceID, 0, sizeof(pDeviceID));
        memset(pFaceUUID, 0, sizeof(pFaceUUID));
        memset(pFeature, 0, sizeof(pFeature));
        nTaskType = INVALIDTASK;
        memset(pSearchTaskID, 0, sizeof(pSearchTaskID));
        nHighestScore = 0;
        nLatestTime = 0;
        nTotalNum = 0;
    }
}SUBMESSAGE, *LPSUBMESSAGE;
typedef std::list<LPSUBMESSAGE> LISTSUBMESSAGE;
typedef void(*LPSubMessageCallback)(LPSUBMESSAGE pSubMessage, void * pUser);


//重点库特征值结点
typedef struct _KeyLibFeatureData
{
    char * pFaceUUID;           //特征值FaceUUID
    char * pFeature;            //特征值
    int nFeatureLen;			//解码后特征值升度
    char pDisk[2];
    char * pImageIP;
    _KeyLibFeatureData * pNext;       //后一个结点
    _KeyLibFeatureData * pPrevious;   //前一个结点
    _KeyLibFeatureData()
    {
        pFaceUUID = NULL;
        pFeature = NULL;
        pNext = NULL;
        pPrevious = NULL;
    }
    ~_KeyLibFeatureData()
    {
        if (NULL != pFaceUUID)
        {
            delete[]pFaceUUID;
            pFaceUUID = NULL;
        }
        if (NULL != pFeature)
        {
            delete[]pFeature;
            pFeature = NULL;
        }
        if (NULL != pImageIP)
        {
            delete[]pImageIP;
            pImageIP = NULL;
        }
        pNext = NULL;
        pPrevious = NULL;
    }
}KEYLIBFEATUREDATA, *LPKEYLIBFEATUREDATA;
typedef map<string, LPKEYLIBFEATUREDATA> MAPKEYLIBFEATUREDATA;

//重点库结点
typedef struct _KeyLibInfo
{
    unsigned int nNum;          //特征值结点数量
    LPKEYLIBFEATUREDATA pHeadFeature; //特征值链表头
    LPKEYLIBFEATUREDATA pTailFeature; //特征值链表尾
    MAPKEYLIBFEATUREDATA mapFeature;  //保存FaceUUID对应特征值结点map
    _KeyLibInfo()
    {
        nNum = 0;
        pHeadFeature = NULL;
        pTailFeature = NULL;
        mapFeature.clear();
    }
    ~_KeyLibInfo()
    {
        nNum = 0;
        while (NULL != pHeadFeature)
        {
            LPKEYLIBFEATUREDATA pFeatureData = pHeadFeature->pNext;
            delete pHeadFeature;
            pHeadFeature = pFeatureData;
        }
        mapFeature.clear();
    }
}KEYLIBINFO, *LPKEYLIBINFO;

typedef struct _SearchFeature
{
    char * pFeature;                //特征值
    int nFeatureLen;                //特征值长度
    _SearchFeature()
    {
        pFeature = NULL;
    }
    ~_SearchFeature()
    {
        if (NULL != pFeature)
        {
            delete []pFeature;
            pFeature = NULL;
        }
    }
}SEARCHFEATURE, *LPSEARCHFEATURE;
typedef list<LPSEARCHFEATURE> LISTSEARCHFEATURE;

typedef struct _SearchResult
{
    char pFaceUUID[MAXLEN];     //特征值FaceUUID
    int64_t nTime;              //抓拍时间
    int nScore;                 //命中分数(多次命中取最高)
    char pDisk[2];              //盘符
    char pImageIP[MAXLEN];      //图片服务器IP
    char pFaceRect[MAXLEN];     //人脸图片位置参数("111,458,233,543")
    int nHitCount;              //当前FaceUUID命中次数
    char * pFeature;
    _SearchResult()
    {
        memset(pFaceUUID, 0, sizeof(pFaceUUID));
        memset(pImageIP, 0, sizeof(pImageIP));
        memset(pFaceRect, 0, sizeof(pFaceRect));
        memset(pDisk, 0, sizeof(pDisk));
        nTime = 0;
        nScore = 0;
        nHitCount = 0;
        pFeature = NULL;
    }
    ~_SearchResult()
    {
        if (NULL != pFeature)
        {
            delete []pFeature;
            pFeature = NULL;
        }
    }
}SEARCHRESULT, *LPSEARCHRESULT;
typedef map<string, LPSEARCHRESULT> MAPSEARCHRESULT;

typedef struct _BusinessResult
{
    int nPersonID;
    MAPSEARCHRESULT mapPersonInfo;
    _BusinessResult()
    {
        nPersonID = 0;
    }
    ~_BusinessResult()
    {
        while (mapPersonInfo.size() > 0)
        {
            delete mapPersonInfo.begin()->second;
            mapPersonInfo.erase(mapPersonInfo.begin());
        }
    }
}BUSINESSRESULT, *LPBUSINESSRESULT;
typedef map<int64_t, LPBUSINESSRESULT> MAPBUSINESSRESULT;
typedef struct _SearchTask
{
    TASKTYPE nTaskType;             //任务类型
    char pHead[MAXLEN];             //发布订阅消息头
    char pOperationType[MAXLEN];    //发布订阅消息操作类型
    char pSource[MAXLEN];           //发布订阅消息源

    char pSearchTaskID[MAXLEN];     //检索任务ID
    //DWORD dwSearchTime;             //搜索开始时间, 用于判断搜索是否超时
    bool bStopSearch;               //是否停止搜索(用于超时后停止搜索)
    int64_t nBeginTime;             //检索开始时间
    int64_t nEndTime;               //检索结束时间
    int nCaptureCount;              //抓拍返回最大结果
    bool bFeature;                  //获取抓拍数据是否返回特征值
    int nScore;                     //检索阈值
    LISTSEARCHFEATURE listFeatureInfo;      //检索目标特征值信息(可以多张图片一起检索, 取并集, 即只要命中多个目标中的一个也返回结果)

    int nSearchThreadNum;           //有几个线程接收到搜索任务
    int nHighestScore;              //命中最高分数
    int64_t nTime;                  //命中最新时间
    int nTotalCount;                //命中总次数(同一特征值多次命中算一次)
    int nError;                     //错误码
    MAPSEARCHRESULT mapSearchTaskResult;    //检索结果(综合所有子线程搜索结果)
    bool bAsyncReturn;              //是否立即返回搜索结果(团伙分析服务)

    int nFrequently;                //出入次数统计阈值
    int64_t nTimeFeatureCount;          //指定时间段内特征值结点数量
    int64_t nFinishSearchCount;         //己完成特征值结点搜索数量, 用来计算进度
    int nProgress;                  //业务完成进度
    bool bNightSearch;              //是否只统计深夜出入
    int64_t tFirstNightTime;        //开始时间所属的第一个深夜起始时间
    int nNightStartTime;            //夜晚开始时间(24小时制, 深夜出入统计使用)
    int nNightEndTime;              //夜晚结束时间(24小时制, 深夜出入统计使用)
    int nRangeNightTime;            //深夜时间段持续时间(ms) (nNightEndTime - nNightStartTime) * 3600 * 1000
    DataIterator * pDataIterator;   //特征值获取指针
    FeatureData * pCurFeatureData;  //频繁出入统计到的当前特征值结点
    MAPBUSINESSRESULT mapBusinessResult;    //频繁出入等统计结果
    set<string> setAleadySearch;    //己检索特征值结点FaceUUID集合
    _SearchTask()
    {
        nError = INVALIDERROR;
        nCaptureCount = 0;
        bFeature = false;
        nHighestScore = 0;
        nTotalCount = 0;
        nTime = 0;
        nSearchThreadNum = 0;
        bAsyncReturn = false;

        nFrequently = 0;
        bNightSearch = false;
        nNightStartTime = 0;
        nNightEndTime = 0;
        bStopSearch = false;
        //dwSearchTime = 0;
        pCurFeatureData = NULL;
        pDataIterator = NULL;
        nTimeFeatureCount = 0;
        nFinishSearchCount = 0;
        nProgress = 0;
        tFirstNightTime = 0;
    }
    ~_SearchTask()
    {
        while (listFeatureInfo.size() > 0)
        {
            delete listFeatureInfo.front();
            listFeatureInfo.pop_front();
        }
        while (mapSearchTaskResult.size() > 0)
        {
            delete mapSearchTaskResult.begin()->second;
            mapSearchTaskResult.erase(mapSearchTaskResult.begin());
        }
        while (mapBusinessResult.size() > 0)
        {
            delete mapBusinessResult.begin()->second;
            mapBusinessResult.erase(mapBusinessResult.begin());
        }
        if (NULL != pDataIterator)
        {
            delete pDataIterator;
            pDataIterator = NULL;
        }
        setAleadySearch.clear();
    }
}SEARCHTASK, *LPSEARCHTASK;
typedef list<LPSEARCHTASK> LISTSEARCHTASK;
typedef map<string, LPSEARCHTASK> MAPSEARCHTASK;
typedef void (*LPSearchTaskCallback)(LPSEARCHTASK pSearchTask, void * pUser);

typedef struct _ThreadSearchResult
{
    unsigned int nHighestScore;     //搜索最高分数
    int64_t nLatestTime;            //搜索结果最新时间
    set<string> setAleadySearchResult;      //己搜索图片faceuuid
    MAPSEARCHRESULT mapThreadSearchResult;  //检索结果
    _ThreadSearchResult()
    {
        nHighestScore = 0;
        nLatestTime = 0;
    }
    void Init()
    {
        nHighestScore = 0;
        nLatestTime = 0;
        setAleadySearchResult.clear();
        mapThreadSearchResult.clear();
    }
}THREADSEARCHRESULT, *LPTHREADSEARCHRESULT;

#define datadefine_h
#endif


