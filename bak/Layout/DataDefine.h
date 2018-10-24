#ifndef datadefine_h

#include <map>
#include <string>
#include <list>
#include <stdint.h>
#include <string.h>
using namespace std;

//数据库表名
#define LIBINFOTABLE        "layoutlibinfo"

#define COMMANDADDLIB "addlib"    //增加关联卡口或重点库
#define COMMANDDELLIB "dellib"    //删除关联卡口或重点库
#define COMMANDADD      "add"       //增加特征值命令
#define COMMANDDEL      "del"       //删除特征值命令
#define COMMANDCLEAR    "clear"     //清空特征值库命令
#define COMMANDSEARCH   "search"    //搜索特征值命令

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
#define LAYOUTLIBINFO    "layoutlibinfo"  //布控库信息发布

//Json串字段定义
#define JSONDLIBID "libid"              //卡口或重点库ID
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
#define JSONBEGINTIME   "startime"      //检索开始时间
#define JSONENDTIME     "endtime"       //检索结束时间
#define JSONSCORE       "score"         //检索阈值
#define JSONLIBINDEX    "index"         //布控库分片索引
#define JSONLAYOUTFACDUUID    "layoutfaceuuid"      //匹配中的布控特征值

#define JSONSEARCHCOUNT "count"         //检索结果数
#define JSONSEARCHHIGHEST "highest"     //检索最高分数
#define JSONSEARCHLATESTTIME "lasttime" //检索最新图片时间

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
    LibAleadyExist,         //库己存在
    LibNotExist,            //库不存在
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
    ADDLIB,             //增加布控库
    DELLIB,             //删除布控库
    LIBADDFEATURE,      //增加特征值
    LIBDELFEATURE,      //删除特征值
    LIBCLEARFEATURE,    //清空特征值
    LIBSEARCH,          //搜索特征值
};

typedef struct _DeviceInfo
{
    int nServerType;    //1: 卡口分析服务, 2: 检索分析服务, 3: 布控分析服务
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
    char pSubJsonValue[FEATURELEN * 10]; //订阅消息Json串
    string sPubJsonValue;       //发布消息Json串

    int nTaskType;              //任务类型

    char pFaceUUID[MAXLEN];     //特征值FaceUUID
    char pFeature[FEATURELEN];  //特征值
    char pDeviceID[MAXLEN];     //卡口ID
    int nImageTime;             //抓拍时间
    char pDisk[2];              //特征值保存磁盘
    char pImageIP[MAXIPLEN];    //特征值保存服务器IP
    char pFaceRect[MAXIPLEN];   //图片人脸坐标
    int nScore;                 //布控阈值
    int nIndex;                 //布控库特征值分片索引
    _SubMessage()
    {
        memset(pHead, 0, sizeof(pHead));
        memset(pOperationType, 0, sizeof(pOperationType));
        memset(pSource, 0, sizeof(pSource));
        memset(pSubJsonValue, 0, sizeof(pSubJsonValue));
        sPubJsonValue = "";
        nTaskType = INVALIDTASK;
        memset(pFaceUUID, 0, sizeof(pFaceUUID));
        memset(pFeature, 0, sizeof(pFeature));
        memset(pDeviceID, 0, sizeof(pDeviceID));
        nImageTime = 0;
        memset(pDisk, 0, sizeof(pDisk));
        memset(pImageIP, 0, sizeof(pImageIP));
        memset(pFaceRect, 0, sizeof(pFaceRect));
        nScore = 0;
        nIndex = 0;
    }
    void Init()
    {
        memset(pHead, 0, sizeof(pHead));
        memset(pOperationType, 0, sizeof(pOperationType));
        memset(pSource, 0, sizeof(pSource));
        memset(pSubJsonValue, 0, sizeof(pSubJsonValue));
        sPubJsonValue = "";
        nTaskType = INVALIDTASK;
        memset(pFaceUUID, 0, sizeof(pFaceUUID));
        memset(pFeature, 0, sizeof(pFeature));
        memset(pDeviceID, 0, sizeof(pDeviceID));
        nImageTime = 0;
        memset(pDisk, 0, sizeof(pDisk));
        memset(pImageIP, 0, sizeof(pImageIP));
        memset(pFaceRect, 0, sizeof(pFaceRect));
        nScore = 0;
        nIndex = 0;
    }
}SUBMESSAGE, *LPSUBMESSAGE;
typedef std::list<LPSUBMESSAGE> LISTSUBMESSAGE;
typedef void(*LPSubMessageCallback)(LPSUBMESSAGE pSubMessage, void * pUser);


//重点库特征值结点
typedef struct _KeyLibFeatureData
{
    char * pFaceUUID;           //特征值FaceUUID
    char * pFeature;            //特征值
    int nFeatureLen;			//解码后特征值长度
    char pDisk;
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
    char pLibID[MAXLEN];        //重点库名
    unsigned int nNum;          //特征值结点数量
    LPKEYLIBFEATUREDATA pHeadFeature; //特征值链表头
    LPKEYLIBFEATUREDATA pTailFeature; //特征值链表尾
    MAPKEYLIBFEATUREDATA mapFeature;  //保存FaceUUID对应特征值结点map
    _KeyLibInfo()
    {
        memset(pLibID, 0, sizeof(pLibID));
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
typedef list<LPKEYLIBINFO> LISTKEYLIBINFO;
typedef map<int, LPKEYLIBINFO> MAPKEYLIBINFO;


//单个重点库所有特征值分片结点信息
typedef struct _SingleFeatureNode
{
    MAPKEYLIBINFO mapSingleFeatureNode;
}SINGLEFEATURENODE, *LPSINGLEFEATURENODE;
typedef map<string, LPSINGLEFEATURENODE> MAPSINGLEFEATURENODE;

//所有重点库所有特征值分片结点信息
typedef struct _AllFeatureNodeInfo
{
    MAPSINGLEFEATURENODE mapAllFeatureNodeInfo;
}ALLFEATURENODEINFO, *LPALLFEATURENODEINFO;

#define datadefine_h
#endif


