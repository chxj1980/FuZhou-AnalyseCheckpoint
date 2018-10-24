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
//���ݿ����
#define LIBINFOTABLE        "checklibinfo"
#define CHECKPOINTPREFIX    "checkpoint"
#define ONEDAYTIME      (24 * 3600 * 1000)

#define COMMANDADDLIB   "addlib"    //���ӹ������ڻ��ص��
#define COMMANDDELLIB   "dellib"    //ɾ���������ڻ��ص��
#define COMMANDADD      "add"       //��������ֵ����
#define COMMANDDEL      "del"       //ɾ������ֵ����
#define COMMANDCLEAR    "clear"     //�������ֵ������
#define COMMANDSEARCH   "search"    //��������ֵ����
#define COMMANDCAPTURE  "capture"   //��ȡץ��ͼƬ
#define COMMANDBUSINESSSUMMARY "business.summary"   //Ƶ�������ͳ��

//Redis Hash���ֶζ���
#define REDISDEVICEID   "libid"     //����ID���ص��ID
#define REDISFACEUUID   "faceuuid"  //faceuuid
#define REDISSCORE      "score"     //����
#define REDISTIME       "time"      //ʱ��
#define REDISDRIVE      "drive"     //�̷�
#define REDISIMAGEIP    "imageip"   //ͼƬ���������IP
#define REDISHITCOUNT   "hitcount"  //ͼƬ���д���
#define REDISFACERECT   "facerect"  //��������
#define HASHOVERTIME    1800        //���ݱ���ʱ��

#define SUBALLCHECKPOINT    "allcheckpoints"    //�������п���
#define SUBALLLIB           "allfocuslib"       //���������ص��

#define DEVICESTATUS    "libstatus"  //����״̬����

//Json���ֶζ���
#define JSONDLIBID      "libid"         //���ڻ��ص��ID
#define JSONLIBTYPE     "type"          //�ص������, 2: �˲��, 3: ���ؿ�
//����
#define JSONDEVICEID    "checkpoint"    //����ID
#define JSONFACDUUID    "faceuuid"      //����ֵFaceUUID
#define JSONFACERECT    "facerect"      //����ͼƬ����
#define JSONFEATURE     "feature"       //����ֵ
#define JSONTIME        "imagetime"     //ץ��ʱ��
#define JSONDRIVE       "imagedisk"     //ͼƬ�����ڲɼ������ϵ������̷�
#define JSONSERVERIP    "imageip"       //�ɼ�����IP

#define JSONTASKID      "taskid"        //��������ID
#define JSONFACE        "faces"         //��������ֵ����
#define JSONFACENO      "faceno"
#define JSONFACEFEATURE "feature"
#define JSONBEGINTIME   "starttime"     //������ʼʱ��
#define JSONENDTIME     "endtime"       //��������ʱ��
#define JSONSIMILAR     "similar"       //������ֵ
#define JSONASYNCRETURN "asyncreturn"   //�Ƿ����������������(�Ż��������)
#define JSONSCORE       "score"         //��������

#define JSONSEARCHCOUNT     "count"         //���������
#define JSONSEARCHHIGHEST   "highest"       //������߷���
#define JSONSEARCHLATESTTIME "lasttime"     //��������ͼƬʱ��
#define JSONCAPTURESIZE     "size"          //��ȡץ��ͼƬ�����
#define JSONRETURNFEATURE   "feature"       //ץ���Ƿ񷵻�����ֵ

//Ƶ�������ͳ��
#define JSONNIGHTSTARTTIME  "nightstarttime"    //ҹ��ʼʱ��
#define JSONNIGHTENDTIME    "nightendtime"      //ҹ�����ʱ��
#define JSONFREQUENTLY      "frequency"         //ͳ�ƴ�����ֵ
#define JSONPERSONID        "person"            //ͳ����ԱID
#define JSONPROGRESS        "progress"          //ҵ��ͳ�ƽ��ȷ���

#define SQLMAXLEN       1024 * 4    //SQL�����󳤶�
#define MAXIPLEN        20          //IP, FaceRect��󳤶�
#define MAXLEN          36          //FaceUUID, LibName, DeviceID��󳤶�
#define FEATURELEN      1024 * 4    //Feature��󳤶�
#define FEATUREMIXLEN   500         //Feature��̳���, С�ڴ˳��ȶ�Ϊ���Ϸ�
#define RESOURCEMAXNUM  150         //���HTTP��Ϣ������, �������޷�������д���, ֱ�ӷ���ʧ��
#define THREADNUM       8           //�߳���
#define THREADWAITTIME  5           //�̵߳ȴ�ʱ��(ms)
#define FEATURESEARCHWAIT   2000    //���߳������ȴ���ʱʱ��

enum ErrorCode
{
    INVALIDERROR = 0,       //�޴���
    ServerNotInit = 12001,  //������δ��ʼ�����
    DBAleadyExist,          //�⼺����
    DBNotExist,             //�ⲻ����
    FaceUUIDAleadyExist,    //FaceUUID������
    FaceUUIDNotExist,       //FaceUUID������
    ParamIllegal,           //�����Ƿ�
    NewFailed,              //new�����ڴ�ʧ��
    JsonFormatError,        //json����ʽ����
    CommandNotFound,        //��֧�ֵ�����
    HttpMsgUpperLimit,      //http��Ϣ������������������
    PthreadMutexInitFailed, //�ٽ�����ʼ��ʧ��
    FeatureNumOverMax,      //������������ֵ��������
    JsonParamIllegal,       //Json����ֵ�Ƿ�
    MysqlQueryFailed,       //Mysql����ִ��ʧ��.
    ParamLenOverMax,        //��������̫��
    LibAleadyExist,         //���ڼ�����
    LibNotExist,            //���ڲ�����
    CheckpointInitFailed,   //�������ʼ��ʧ��
    VerifyFailed,           //����ֵ�ȶ�ʧ��, ʧ��һ�κ󼴲��ٱȶ�
    HttpSocketBindFailed,   //http�˿ڰ�ʧ��
    CreateTableFailed,      //�����ݿⴴ����ʧ��
    SearchTimeWrong,        //�����ʱ���(��ʼʱ����ڽ���ʱ��)
    SearchNoDataOnTime,      //ָ��ʱ�����û������
    AddFeatureToCheckpointFailed,    //��������ֵ������DB����
    SocketInitFailed,        //�����ʼ��ʧ��
    InsertRedisFailed       //���ݲ���Redis����
};

//��������
enum TASKTYPE
{
    INVALIDTASK,
    ADDLIB,              //�����ص��򿨿�
    DELLIB,              //ɾ���ص��򿨿�
    LIBADDFEATURE,       //�򿨿���������ֵ
    LIBDELFEATURE,       //�ӿ���ɾ������ֵ
    LIBCLEARFEATURE,     //�ӿ����������ֵ
    LIBSEARCH,           //��ʱ��, ������������ֵ
    LIBCAPTURE,          //��ѯָ��ʱ�����ץ��ͼƬ
    LIBBUSINESS          //Ƶ������, ��ҹ����, ���ҹ����ҵ��ͳ��
};

typedef struct _DeviceInfo
{
    int nServerType;    //1: ���ڷ�������, 2: �ص���������
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
    char pHead[MAXLEN];         //������Ϣͷ
    char pOperationType[MAXLEN];//������Ϣ��������
    char pSource[MAXLEN];       //������ϢԴ
    char pSubJsonValue[FEATURELEN * 10];       //������ϢJson��
    string sPubJsonValue;       //������ϢJson��

    char pDeviceID[MAXLEN];     //����ID
    char pFaceUUID[MAXLEN];     //����ֵFaceUUID
    char pFeature[FEATURELEN];  //����ֵ

    TASKTYPE nTaskType;         //��������
    char pSearchTaskID[MAXLEN]; //��������ID
    int nHighestScore;          //������߷���
    int64_t nLatestTime;   //��������ʱ��
    unsigned int nTotalNum;     //��������
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


//�ص������ֵ���
typedef struct _KeyLibFeatureData
{
    char * pFaceUUID;           //����ֵFaceUUID
    char * pFeature;            //����ֵ
    int nFeatureLen;			//���������ֵ����
    char pDisk[2];
    char * pImageIP;
    _KeyLibFeatureData * pNext;       //��һ�����
    _KeyLibFeatureData * pPrevious;   //ǰһ�����
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

//�ص����
typedef struct _KeyLibInfo
{
    unsigned int nNum;          //����ֵ�������
    LPKEYLIBFEATUREDATA pHeadFeature; //����ֵ����ͷ
    LPKEYLIBFEATUREDATA pTailFeature; //����ֵ����β
    MAPKEYLIBFEATUREDATA mapFeature;  //����FaceUUID��Ӧ����ֵ���map
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
    char * pFeature;                //����ֵ
    int nFeatureLen;                //����ֵ����
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
    char pFaceUUID[MAXLEN];     //����ֵFaceUUID
    int64_t nTime;              //ץ��ʱ��
    int nScore;                 //���з���(�������ȡ���)
    char pDisk[2];              //�̷�
    char pImageIP[MAXLEN];      //ͼƬ������IP
    char pFaceRect[MAXLEN];     //����ͼƬλ�ò���("111,458,233,543")
    int nHitCount;              //��ǰFaceUUID���д���
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
    TASKTYPE nTaskType;             //��������
    char pHead[MAXLEN];             //����������Ϣͷ
    char pOperationType[MAXLEN];    //����������Ϣ��������
    char pSource[MAXLEN];           //����������ϢԴ

    char pSearchTaskID[MAXLEN];     //��������ID
    //DWORD dwSearchTime;             //������ʼʱ��, �����ж������Ƿ�ʱ
    bool bStopSearch;               //�Ƿ�ֹͣ����(���ڳ�ʱ��ֹͣ����)
    int64_t nBeginTime;             //������ʼʱ��
    int64_t nEndTime;               //��������ʱ��
    int nCaptureCount;              //ץ�ķ��������
    bool bFeature;                  //��ȡץ�������Ƿ񷵻�����ֵ
    int nScore;                     //������ֵ
    LISTSEARCHFEATURE listFeatureInfo;      //����Ŀ������ֵ��Ϣ(���Զ���ͼƬһ�����, ȡ����, ��ֻҪ���ж��Ŀ���е�һ��Ҳ���ؽ��)

    int nSearchThreadNum;           //�м����߳̽��յ���������
    int nHighestScore;              //������߷���
    int64_t nTime;                  //��������ʱ��
    int nTotalCount;                //�����ܴ���(ͬһ����ֵ���������һ��)
    int nError;                     //������
    MAPSEARCHRESULT mapSearchTaskResult;    //�������(�ۺ��������߳��������)
    bool bAsyncReturn;              //�Ƿ����������������(�Ż��������)

    int nFrequently;                //�������ͳ����ֵ
    int64_t nTimeFeatureCount;          //ָ��ʱ���������ֵ�������
    int64_t nFinishSearchCount;         //���������ֵ�����������, �����������
    int nProgress;                  //ҵ����ɽ���
    bool bNightSearch;              //�Ƿ�ֻͳ����ҹ����
    int64_t tFirstNightTime;        //��ʼʱ�������ĵ�һ����ҹ��ʼʱ��
    int nNightStartTime;            //ҹ��ʼʱ��(24Сʱ��, ��ҹ����ͳ��ʹ��)
    int nNightEndTime;              //ҹ�����ʱ��(24Сʱ��, ��ҹ����ͳ��ʹ��)
    int nRangeNightTime;            //��ҹʱ��γ���ʱ��(ms) (nNightEndTime - nNightStartTime) * 3600 * 1000
    DataIterator * pDataIterator;   //����ֵ��ȡָ��
    FeatureData * pCurFeatureData;  //Ƶ������ͳ�Ƶ��ĵ�ǰ����ֵ���
    MAPBUSINESSRESULT mapBusinessResult;    //Ƶ�������ͳ�ƽ��
    set<string> setAleadySearch;    //����������ֵ���FaceUUID����
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
    unsigned int nHighestScore;     //������߷���
    int64_t nLatestTime;            //�����������ʱ��
    set<string> setAleadySearchResult;      //������ͼƬfaceuuid
    MAPSEARCHRESULT mapThreadSearchResult;  //�������
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


