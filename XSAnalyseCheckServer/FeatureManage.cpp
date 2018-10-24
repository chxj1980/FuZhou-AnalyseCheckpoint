#include "stdafx.h"
#include "FeatureManage.h"

CFeatureManage::CFeatureManage(void)
{
    m_bStopTask = false;
    m_nServerType = 1;
#ifdef __WINDOWS__
    InitializeCriticalSection(&m_csTask);
    InitializeCriticalSection(&m_csWait);
    InitializeCriticalSection(&m_csBusiness);
    m_hBusinessEvent = CreateEvent(NULL, true, false, NULL);
    m_hSearchEvent = CreateEvent(NULL, true, false, NULL);
    m_hWaitEvent = CreateEvent(NULL, true, false, NULL);
#else
    pthread_mutex_init(&m_csTask, NULL);
    pthread_mutex_init(&m_csWait, NULL);
    pthread_mutex_init(&m_csBusiness, NULL);
    //管道
    pipe(m_nPipeBusiness);
    pipe(m_nPipeSearch);
    pipe(m_nPipeWait);
#endif
    m_pFaceFeaturedb = NULL;
}
CFeatureManage::~CFeatureManage(void)
{
#ifdef __WINDOWS__
    DeleteCriticalSection(&m_csTask);
    DeleteCriticalSection(&m_csWait);
    DeleteCriticalSection(&m_csBusiness);
    CloseHandle(m_hBusinessEvent);
    CloseHandle(m_hSearchEvent);
    CloseHandle(m_hWaitEvent);
#else
    pthread_mutex_destroy(&m_csTask);
    pthread_mutex_destroy(&m_csWait);
    pthread_mutex_destroy(&m_csBusiness);
    close(m_nPipeBusiness[0]);
    close(m_nPipeBusiness[1]);
    close(m_nPipeSearch[0]);
    close(m_nPipeSearch[1]);
    close(m_nPipeWait[0]);
    close(m_nPipeWait[1]);
#endif
}
int CFeatureManage::Init(char * pDeviceID, int nServerType, int nMaxCount, LPSearchTaskCallback pTaskCallback, void * pUser)
{
    //保存参数
    strcpy(m_pLibName, pDeviceID);
    m_nServerType = nServerType;
    m_nMaxCount = nMaxCount;
    m_pTaskCallback = pTaskCallback;
    m_pUser = pUser;

    if (1 == m_nServerType)
    {
        m_pFaceFeaturedb = new FaceFeatureDB();
        m_pFaceFeaturedb->InitDB();
    }

    //开启搜索线程
#ifdef __WINDOWS__
    m_ThreadIDSearch = CreateThread(NULL, 0, LibTaskThread, this, NULL, 0);   //搜索比对线程
    m_ThreadIDWait = CreateThread(NULL, 0, LibSearchWaitThread, this, NULL, 0);   //搜索等待线程
    m_ThreadIDBusiness = CreateThread(NULL, 0, LibBusinessThread, this, NULL, 0);   //业务模型线程
#else
    pthread_create(&m_ThreadIDSearch, NULL, LibTaskThread, (void*)this);
    pthread_create(&m_ThreadIDWait, NULL, LibSearchWaitThread, (void*)this);
    pthread_create(&m_ThreadIDBusiness, NULL, LibBusinessThread, (void*)this);
#endif
    printf("CFeatureManage::Lib[%s] Search Thread Begin.\n", pDeviceID);

    return true;
}
bool CFeatureManage::UnInit()
{
    m_bStopTask = true;

#ifdef __WINDOWS__
    SetEvent(m_hBusinessEvent);
    SetEvent(m_hSearchEvent);
    SetEvent(m_hWaitEvent);
    Sleep(100); //100 ms
    WaitForSingleObject(m_ThreadIDSearch, INFINITE);
    printf("--CFeatureManage::Checkpoint[%s] Search Thread end.\n", m_pLibName);
    WaitForSingleObject(m_ThreadIDWait, INFINITE);
    printf("--CFeatureManage::Checkpoint[%s] Wait Thread end.\n", m_pLibName);
    WaitForSingleObject(m_ThreadIDBusiness, INFINITE);
    printf("--CFeatureManage::Checkpoint[%s] Business Thread end.\n", m_pLibName);
#else
    write(m_nPipeBusiness[1], "1", 1);
    write(m_nPipeSearch[1], "1", 1);
    write(m_nPipeWait[1], "1", 1);
    usleep(100 * 1000); //100 ms
    pthread_join(m_ThreadIDSearch, NULL);
    printf("--CFeatureManage::Checkpoint[%s] Search Thread end.\n", m_pLibName);
    pthread_join(m_ThreadIDWait, NULL);
    printf("--CFeatureManage::Checkpoint[%s] Wait Thread end.\n", m_pLibName);
    pthread_join(m_ThreadIDBusiness, NULL);
    printf("--CFeatureManage::Checkpoint[%s] Business Thread end.\n", m_pLibName);
#endif

    EnterSearchMutex();
    while(m_listTask.size() > 0)
    {
        delete m_listTask.front();
        m_listTask.pop_front();
    }
    LeaveSearchMutex();

    EnterWaitMutex();
    while (m_listWait.size() > 0)
    {
        delete m_listWait.front();
        m_listWait.pop_front();
    }
    LeaveWaitMutex();

    EnterBusinessMutex();
    while (m_listBusiness.size() > 0)
    {
        delete m_listBusiness.front();
        m_listBusiness.pop_front();
    }
    LeaveBusinessMutex();

    while (m_listFeatureSearch.size() > 0)
    {
        m_listFeatureSearch.front()->pFeatureSearchThread->UnInit();
        delete m_listFeatureSearch.front();
        m_listFeatureSearch.pop_front();
    }
    if (NULL != m_pFaceFeaturedb)
    {
        delete m_pFaceFeaturedb;
        m_pFaceFeaturedb = NULL;
    }
    return true;
}
//增加卡口库特征值
int CFeatureManage::AddFeature(FeatureData * pFeatureData, bool bInit)
{
    int nRet = INVALIDERROR;
    //增加特征值结点
    LPFEATURETHREADINFO pFeatureThreadInfo = NULL;
    if (m_listFeatureSearch.size() > 0 && m_listFeatureSearch.back()->nFeatureCount < m_nMaxCount)
    {
        pFeatureThreadInfo = m_listFeatureSearch.back();
    }
    else
    {
        pFeatureThreadInfo = new FEATURETHREADINFO;
        pFeatureThreadInfo->pFeatureSearchThread = new CFeatureSearchThread;
        pFeatureThreadInfo->pFeatureSearchThread->Init(m_nServerType, ThreadSearchCallback, this, m_pFaceFeaturedb);
        m_listFeatureSearch.push_back(pFeatureThreadInfo);
        pFeatureThreadInfo->nBeginTime = pFeatureData->CollectTime;
        pFeatureThreadInfo->pFeatureSearchThread->SetCheckpointBeginTime(pFeatureData->CollectTime);
    }

    nRet = m_pFaceFeaturedb->AppendFaceFeatureData(pFeatureData);
    if (nRet != 0)
    {
        printf("***Warning: Add Feature To Checkpoint[%s] Failed, FaceUUID[%s], Return[%d].\n", m_pLibName, pFeatureData->FaceUUID, nRet);
        nRet = AddFeatureToCheckpointFailed;
    }
    else
    {
        if (!bInit)
        {
            //printf("Add Feature To Checkpoint[%s] Success, FaceUUID[%s].\n", m_pLibName, pFeatureData->FaceUUID);
            m_nTotalCount++;
        }
        pFeatureThreadInfo->nEndTime = pFeatureData->CollectTime;
        pFeatureThreadInfo->pFeatureSearchThread->SetCheckpointEndTime(pFeatureData->CollectTime);
        pFeatureThreadInfo->nFeatureCount++;


        //统计每天特征值数量更新
        int64_t nTime = pFeatureData->CollectTime - (pFeatureData->CollectTime % ONEDAYTIME) - 8 * 3600 * 1000;
        map<int64_t, int>::iterator it = m_mapDayCount.find(nTime);
        if (it != m_mapDayCount.end())
        {
            it->second++;
        }
        else
        {
            m_mapDayCount.insert(make_pair(nTime, 1));
        }
    }
    return (ErrorCode)nRet;
}
int CFeatureManage::AddFeature(LPKEYLIBFEATUREDATA pFeatureData, bool bInit)
{
    int nRet = INVALIDERROR;
    //增加特征值结点
    LPFEATURETHREADINFO pFeatureThreadInfo = NULL;
    if (m_listFeatureSearch.size() > 0 && m_listFeatureSearch.back()->nFeatureCount < m_nMaxCount)
    {
        pFeatureThreadInfo = m_listFeatureSearch.back();
    }
    else
    {
        pFeatureThreadInfo = new FEATURETHREADINFO;
        pFeatureThreadInfo->pFeatureSearchThread = new CFeatureSearchThread;
        pFeatureThreadInfo->pFeatureSearchThread->Init(m_nServerType, ThreadSearchCallback, this);
        m_listFeatureSearch.push_back(pFeatureThreadInfo);
    }

    nRet = pFeatureThreadInfo->pFeatureSearchThread->AddFeature(pFeatureData);

    if (nRet != 0)
    {
        printf("***Warning: Add Feature To Lib[%s] Failed, FaceUUID[%s], Return[%d].\n", m_pLibName, pFeatureData->pFaceUUID, nRet);
        nRet = AddFeatureToCheckpointFailed;
    }
    else
    {
        if (!bInit)
        {
            printf("Add Feature To Lib[%s] Success, FaceUUID[%s].\n", m_pLibName, pFeatureData->pFaceUUID);
        }
        pFeatureThreadInfo->nFeatureCount++;
        m_nTotalCount++;
    }
    return (ErrorCode)nRet;
}
//删除特征值
int CFeatureManage::DelFeature(const char * pFaceUUID)
{
    int nRet = FaceUUIDNotExist;
    LISTFEATURETHREADINFO::iterator it = m_listFeatureSearch.begin();
    for (; it != m_listFeatureSearch.end(); it++)
    {
        if (INVALIDERROR == (*it)->pFeatureSearchThread->DelFeature(pFaceUUID))
        {
            nRet = INVALIDERROR;
            (*it)->nFeatureCount--;
            break;
        }
    }
    
    return nRet;
}
//清空特征值
int CFeatureManage::ClearKeyLib()
{
    LISTFEATURETHREADINFO::iterator it = m_listFeatureSearch.begin();
    for (; it != m_listFeatureSearch.end(); it++)
    {
        (*it)->pFeatureSearchThread->ClearKeyLib();
        (*it)->nFeatureCount = 0;
    }
    return 0;
}
//获取特征值总数量
unsigned int CFeatureManage::GetTotalNum()
{
    return m_nTotalCount;
}
//获取当天特征值数量
unsigned int CFeatureManage::GetDayNum()
{
    if (1 == m_nServerType)
    { 
        return m_pFaceFeaturedb->GetDayNum();;
    }
    else
    {
        return 0;
    }
}
bool CFeatureManage::GetDataDate(unsigned long long &begintime, unsigned long long &endtime)
{
    LISTFEATURETHREADINFO::iterator it = m_listFeatureSearch.begin();
    if (1 == m_nServerType)
    {
        for (; it != m_listFeatureSearch.end(); it++)
        {
            if (m_listFeatureSearch.begin() == it)
            {
                begintime = (*it)->nBeginTime / 1000;
            }
            endtime = (*it)->nEndTime / 1000;
        }
    }
    else
    {
        begintime = 0;
        endtime = 0;
    }
    return true;
}
//返回抓拍图片
int CFeatureManage::GetCaptureImageInfo(LPSEARCHTASK pSearchTask)
{
    int nRet = 0;
    int nGetAleady = 0;
    if (1 == m_nServerType && m_listFeatureSearch.size() > 0)
    {
        //printf("GetCaptureImage[%s][%d][%s - %s]\n", m_pLibName, pSearchTask->nCaptureCount,
            //ChangeSecondToTime(pSearchTask->nBeginTime / 1000).c_str(), ChangeSecondToTime(pSearchTask->nEndTime / 1000).c_str());
        DataIterator * pDataIterator = NULL;     //特征值查找索引
        pDataIterator = m_pFaceFeaturedb->CreateIterator();
        int nRet = pDataIterator->SetTimeRange(pSearchTask->nBeginTime, pSearchTask->nEndTime);
        FeatureData * pFeatureData = pDataIterator->Last();
        while (NULL != pFeatureData)
        {
            if (pFeatureData->CollectTime > pSearchTask->nEndTime)
            {
                pFeatureData = pDataIterator->Previous();
            }
            else if (pFeatureData->CollectTime < pSearchTask->nBeginTime)
            {
                pFeatureData = NULL;
                delete pDataIterator;
                pDataIterator = NULL;
                break;
            }
            else
            {
                break;
            }
        }
        while (NULL != pFeatureData)
        {
            LPSEARCHRESULT pSearchResult = new SEARCHRESULT;
            memset(pSearchResult, 0, sizeof(SEARCHRESULT));
            strcpy(pSearchResult->pFaceUUID, pFeatureData->FaceUUID);
            pSearchResult->nTime = pFeatureData->CollectTime;
            strcpy(pSearchResult->pDisk, pFeatureData->pDisk);
            strcpy(pSearchResult->pImageIP, pFeatureData->pImageIP);
            strcpy(pSearchResult->pFaceRect, pFeatureData->pFaceRect);
            if (NULL != pFeatureData->pFaceURL && NULL != pFeatureData->pBkgURL)
            {
                strcpy(pSearchResult->pFaceURL, pFeatureData->pFaceURL);
                strcpy(pSearchResult->pBkgURL, pFeatureData->pBkgURL);
            }
            else
            {
                strcpy(pSearchResult->pFaceURL, "0");
                strcpy(pSearchResult->pBkgURL, "0");
            }
            
            if (pSearchTask->bFeature)
            {
                string sFeature = ZBase64::Encode(pFeatureData->FeatureValue, pFeatureData->FeatureValueLen);
                pSearchResult->pFeature = new char[sFeature.size() + 1];
                strcpy(pSearchResult->pFeature, sFeature.c_str());
                pSearchResult->pFeature[sFeature.size()] = '\0';
            }
            pSearchTask->mapSearchTaskResult.insert(make_pair(pSearchResult->pFaceUUID, pSearchResult));

            nGetAleady++;
            if (nGetAleady >= pSearchTask->nCaptureCount)
            {
                delete pDataIterator;
                pDataIterator = NULL;
                break;
            }


            pFeatureData = pDataIterator->Previous();
        }

        if (NULL != pDataIterator)
        {
            delete pDataIterator;
            pDataIterator = NULL;
        }

        printf("Get [%s] Capture Picture: %d.\n", m_pLibName, nGetAleady);
    }
    
    return nRet;
}
//根据faceUUID获取具体图片对应URL信息(提供给Kafka使用)
int CFeatureManage::GetFaceURLInfo(LPFACEURLINFO pFaceURLInfo)
{
    int nRet = 0;
    bool bFind = false;
    if (1 == m_nServerType && m_listFeatureSearch.size() > 0)
    {
        //printf("GetFaceURLInfo[%s][%s]\n", m_pLibName, ChangeSecondToTime(pFaceURLInfo->nTime / 1000).c_str());
        DataIterator * pDataIterator = NULL;     //特征值查找索引
        pDataIterator = m_pFaceFeaturedb->CreateIterator();
        int nRet = pDataIterator->SetTimeRange(pFaceURLInfo->nTime - 60000, pFaceURLInfo->nTime + 60000);   
        FeatureData * pFeatureData = pDataIterator->Last();
        
        while (NULL != pFeatureData)
        {
            if (strcmp(pFeatureData->FaceUUID, pFaceURLInfo->pFaceUUID) != 0)
            {
                pFeatureData = pDataIterator->Previous();
                continue;
            }
            else
            {
                strcpy(pFaceURLInfo->pDisk, pFeatureData->pDisk);
                strcpy(pFaceURLInfo->pImageIP, pFeatureData->pImageIP);
                strcpy(pFaceURLInfo->pFaceRect, pFeatureData->pFaceRect);
                if (NULL != pFeatureData->pFaceURL && NULL != pFeatureData->pBkgURL)
                {
                    strcpy(pFaceURLInfo->pFaceURL, pFeatureData->pFaceURL);
                    strcpy(pFaceURLInfo->pBkgURL, pFeatureData->pBkgURL);
                }
                else
                {
                    strcpy(pFaceURLInfo->pFaceURL, "0");
                    strcpy(pFaceURLInfo->pBkgURL, "0");
                }

                bFind = true;
                break;
            }
        }

        if (NULL != pDataIterator)
        {
            delete pDataIterator;
            pDataIterator = NULL;
        }
    }
    if (!bFind)
    {
        printf("***Warning: Get [%s][%s] URLInfo Failed.\n", m_pLibName, pFaceURLInfo->pFaceUUID);
        nRet = GetFaceURLFailed;
    }
    else
    {
        printf("Get [%s][%s] URLInfo Success.\n", m_pLibName, pFaceURLInfo->pFaceUUID);
    }

    return nRet;
}
//增加搜索任务, 在搜索线程里具体执行
void CFeatureManage::AddSearchTask(LPSEARCHTASK pSearchTask)
{
    if(m_bStopTask)
    {
        return;
    }
    if (LIBSEARCH == pSearchTask->nTaskType)
    {
        if (1 == m_nServerType)
        {
            printf("AddSearchTask[%s][%s-%s].\n", m_pLibName,
                ChangeSecondToTime(pSearchTask->nBeginTime / 1000).c_str(), ChangeSecondToTime(pSearchTask->nEndTime / 1000).c_str());
        }
        else if (2 == m_nServerType)
        {
            printf("CFeatureManage::AddSearchTask::Lib[%s].\n", m_pLibName);
        }

        EnterSearchMutex();
        m_listTask.push_back(pSearchTask);
#ifdef __WINDOWS__
        SetEvent(m_hSearchEvent);
        pSearchTask->dwSearchTime = GetTickCount();
#else
        write(m_nPipeSearch[1], "1", 1);
#endif
        LeaveSearchMutex();
        
    }
    else if (LIBBUSINESS == pSearchTask->nTaskType)
    {
        if (1 == m_nServerType)
        {
            printf("AddBusinessTask[%s][%s-%s][Frequency: %d][similar: %d][NightSearch: %s].\n", m_pLibName,
                ChangeSecondToTime(pSearchTask->nBeginTime / 1000).c_str(), ChangeSecondToTime(pSearchTask->nEndTime / 1000).c_str(),
                pSearchTask->nFrequently, pSearchTask->nScore, pSearchTask->bNightSearch ? "Y" : "N");
        }

        EnterBusinessMutex();
        m_listBusiness.push_back(pSearchTask);
#ifdef __WINDOWS__
        SetEvent(m_hBusinessEvent);
        pSearchTask->dwSearchTime = GetTickCount();
#else
        write(m_nPipeBusiness[1], "1", 1);
#endif
        LeaveBusinessMutex();
        
    }
    

    return;
}
//搜索线程
#ifdef __WINDOWS__
DWORD WINAPI CFeatureManage::LibTaskThread(void * pParam)
{
    CFeatureManage * pThis = (CFeatureManage *)pParam;
    pThis->LibTaskAction();
    return 0;
}
#else
void * CFeatureManage::LibTaskThread(void * pParam)
{
    CFeatureManage * pThis = (CFeatureManage *)pParam;
    pThis->LibTaskAction();
    return NULL;
}
#endif
void CFeatureManage::LibTaskAction()
{
    char pPipeRead[2] = { 0 };
    LPSEARCHTASK pTask = NULL;
    while(!m_bStopTask)
    {
#ifdef __WINDOWS__
        if (WAIT_TIMEOUT == WaitForSingleObject(m_hSearchEvent, THREADWAITTIME))
        {
            continue;
        }
        ResetEvent(m_hSearchEvent);
#else
        read(m_nPipeSearch[0], pPipeRead, 1);
#endif
        do
        {
            EnterSearchMutex();
            if (m_listTask.size() > 0)
            {
                pTask = m_listTask.front();
                m_listTask.pop_front();
            }
            else
            {
                pTask = NULL;
            }
            LeaveSearchMutex();

            bool bStartSearch = false;
            if (NULL != pTask)
            {
                if (1 == m_nServerType)
                {
                    //给线程分配搜索任务
                    LISTFEATURETHREADINFO::iterator it = m_listFeatureSearch.begin();
                    for (; it != m_listFeatureSearch.end(); it++)
                    {
                        if ((*it)->nBeginTime <= pTask->nEndTime && (*it)->nEndTime >= pTask->nBeginTime)
                        {
                            EnterSearchMutex();
                            pTask->nSearchThreadNum++;
                            LeaveSearchMutex();
                            if (!bStartSearch)
                            {
                                bStartSearch = true;
                                EnterWaitMutex();
                                m_listWait.push_back(pTask);
                                LeaveWaitMutex();
                            }
                            (*it)->pFeatureSearchThread->AddSearchTask(pTask);
                        }
                    }
                }
                else if (2 == m_nServerType)
                {
                    LISTFEATURETHREADINFO::iterator it = m_listFeatureSearch.begin();
                    for (; it != m_listFeatureSearch.end(); it++)
                    {
                        EnterSearchMutex();
                        pTask->nSearchThreadNum++;
                        LeaveSearchMutex();
                        if (!bStartSearch)
                        {
                            bStartSearch = true;
                            EnterWaitMutex();
                            m_listWait.push_back(pTask);
                            LeaveWaitMutex();
                        }
                        (*it)->pFeatureSearchThread->AddSearchTask(pTask);
                    }
                }

                if (!bStartSearch)
                {
                    m_pTaskCallback(pTask, m_pUser);
                }
            }
        } while (NULL != pTask);
    }

    return;
}
//搜索等待线程
#ifdef __WINDOWS__
DWORD WINAPI CFeatureManage::LibSearchWaitThread(void * pParam)
{
    CFeatureManage * pThis = (CFeatureManage *)pParam;
    pThis->LibSearchWaitAction();
    return 0;
}
#else
void * CFeatureManage::LibSearchWaitThread(void * pParam)
{
    CFeatureManage * pThis = (CFeatureManage *)pParam;
    pThis->LibSearchWaitAction();
    return NULL;
}
#endif
void CFeatureManage::LibSearchWaitAction()
{
    char pPipeRead[10] = { 0 };
    while (!m_bStopTask)
    {
#ifdef __WINDOWS__
        if (WAIT_TIMEOUT == WaitForSingleObject(m_hWaitEvent, THREADWAITTIME))
        {
            continue;
        }
        ResetEvent(m_hWaitEvent);
#else
        read(m_nPipeWait[0], pPipeRead, 1);
#endif

        EnterWaitMutex();
        LISTSEARCHTASK::iterator itWait = m_listWait.begin();
        while (itWait != m_listWait.end())
        {
            if ((*itWait)->nSearchThreadNum == 0)
            {
                m_pTaskCallback(*itWait, m_pUser);
                itWait = m_listWait.erase(itWait);
            }
            else
            {
                itWait++;
            }
        }
        LeaveWaitMutex();
    }
    return;
}
void CFeatureManage::ThreadSearchCallback(LPSEARCHTASK pSearchTask, LPTHREADSEARCHRESULT pThreadSearchResult, void * pUser)
{
    CFeatureManage * pThis = (CFeatureManage *)pUser;
    if (LIBSEARCH == pSearchTask->nTaskType)
    {
        pThis->EnterSearchMutex();
        pSearchTask->nHighestScore = pSearchTask->nHighestScore > pThreadSearchResult->nHighestScore ? pSearchTask->nHighestScore : pThreadSearchResult->nHighestScore;
        pSearchTask->nTime = pSearchTask->nTime > pThreadSearchResult->nLatestTime ? pSearchTask->nTime : pThreadSearchResult->nLatestTime;
        pSearchTask->mapSearchTaskResult.insert(pThreadSearchResult->mapThreadSearchResult.begin(), pThreadSearchResult->mapThreadSearchResult.end());
        pSearchTask->nTotalCount = pSearchTask->mapSearchTaskResult.size();
        pSearchTask->nSearchThreadNum--;
        if (0 == pSearchTask->nSearchThreadNum)
        {
#ifdef __WINDOWS__
            SetEvent(pThis->m_hWaitEvent);
#else
            write(pThis->m_nPipeWait[1], "1", 1);
#endif
        }
        pThis->LeaveSearchMutex();
    }
    else if (LIBBUSINESS == pSearchTask->nTaskType)
    {
        pThis->EnterBusinessMutex();
        pSearchTask->nSearchThreadNum--;
        MAPBUSINESSRESULT::iterator it = pSearchTask->mapBusinessResult.find(pSearchTask->nPersonID);
        if (it == pSearchTask->mapBusinessResult.end())
        {
            pThis->LeaveBusinessMutex();
            return;
        }
        it->second->mapPersonInfo.insert(pThreadSearchResult->mapThreadSearchResult.begin(), pThreadSearchResult->mapThreadSearchResult.end());
        pSearchTask->setAleadySearch.insert(pThreadSearchResult->setAleadySearchResult.begin(), pThreadSearchResult->setAleadySearchResult.end());

        if (0 == pSearchTask->nSearchThreadNum)
        {
            //printf("结点[%s]频繁出入次数: %d.\n", pSearchTask->pCurFeatureData->FaceUUID, it->second->mapPersonInfo.size());
            //当前图片检索结果小于统计阈值, 删除
            if (it->second->mapPersonInfo.size() < pSearchTask->nFrequently)
            {
                delete it->second;
                pSearchTask->mapBusinessResult.erase(pSearchTask->nPersonID);
            }

            pSearchTask->setAleadySearch.erase(pSearchTask->pCurFeatureData->FaceUUID);
            pSearchTask->pCurFeatureData = pSearchTask->pDataIterator->Next();
            pSearchTask->nFinishSearchCount++;
            pThis->m_listBusiness.push_back(pSearchTask);
#ifdef __WINDOWS__
            SetEvent(pThis->m_hBusinessEvent);
#else
            write(pThis->m_nPipeBusiness[1], "1", 1);
#endif
        }
        pThis->LeaveBusinessMutex();
    }
    
    return;
}
//频繁出入等业务线程
#ifdef __WINDOWS__
DWORD WINAPI CFeatureManage::LibBusinessThread(void * pParam)
{
    CFeatureManage * pThis = (CFeatureManage *)pParam;
    pThis->LibBusinessAction();
    return 0;
}
#else
void * CFeatureManage::LibBusinessThread(void * pParam)
{
    CFeatureManage * pThis = (CFeatureManage *)pParam;
    pThis->LibBusinessAction();
    return NULL;
}
#endif
void CFeatureManage::LibBusinessAction()
{
    LPSEARCHTASK pBusinessTask = NULL;
    char pPipeRead[10] = { 0 };
    while (!m_bStopTask)
    {
#ifdef __WINDOWS__
        if (WAIT_TIMEOUT == WaitForSingleObject(m_hBusinessEvent, THREADWAITTIME))
        {
            continue;
        }
        ResetEvent(m_hBusinessEvent);
#else
        read(m_nPipeBusiness[0], pPipeRead, 1);
#endif
        do 
        {
            EnterBusinessMutex();
            if (m_listBusiness.size() > 0)
            {
                pBusinessTask = m_listBusiness.front();
                m_listBusiness.pop_front();
            }
            else
            {
                pBusinessTask = NULL;
            }
            LeaveBusinessMutex();

            if (NULL != pBusinessTask)
            {
                int nRet = INVALIDERROR;

                //获取指定时间段特征值
                if (NULL == pBusinessTask->pDataIterator)     
                {
                    if (1 == m_nServerType && m_listFeatureSearch.size() > 0)
                    {
                        pBusinessTask->pDataIterator = m_pFaceFeaturedb->CreateIterator();

                        //如果搜索深夜出入, 则先获取当前开始时间所属的深夜开始时间, 或下一个深夜开始时间
                        pBusinessTask->tFirstNightTime = pBusinessTask->nBeginTime;
                        if (pBusinessTask->bNightSearch)
                        {
                            CheckNightTime(pBusinessTask->nNightStartTime, pBusinessTask->nNightEndTime, pBusinessTask->tFirstNightTime);
                            printf("[%s]Night Search BeginTime: %s.\n", m_pLibName, ChangeSecondToTime(pBusinessTask->tFirstNightTime / 1000).c_str());
                        }

                        //开始索引时间
                        int64_t nIndexTime = pBusinessTask->tFirstNightTime < pBusinessTask->nBeginTime ? pBusinessTask->nBeginTime : pBusinessTask->tFirstNightTime;
                        nRet = pBusinessTask->pDataIterator->SetTimeRange(nIndexTime, pBusinessTask->nEndTime);
                        if (nRet < 0)
                        {
                            pBusinessTask->nError = SearchNoDataOnTime;
                        }
                        else
                        {
                            pBusinessTask->pCurFeatureData = pBusinessTask->pDataIterator->First();    //保存第一个特征值结点, 搜索起点
                            if (NULL != pBusinessTask->pCurFeatureData)
                            {
                                printf("[%s] First Time[%s].\n", m_pLibName, ChangeSecondToTime(pBusinessTask->pCurFeatureData->CollectTime / 1000).c_str());
                                //获取指定时候段特征值结点数量
                                pBusinessTask->nTimeFeatureCount = pBusinessTask->pDataIterator->GetNodeCount();
                                printf("[%s] Specify Time Feature Count: %d\n", m_pLibName, pBusinessTask->nTimeFeatureCount);
                                if (1 == pBusinessTask->nTimeFeatureCount)
                                {
                                    pBusinessTask->nError = SearchNoDataOnTime;
                                }
                            }
                            else
                            {
                                pBusinessTask->nError = SearchNoDataOnTime;
                            }
                        }
                    }
                    else
                    {
                        pBusinessTask->nError = SearchNoDataOnTime;
                    }

                    if (INVALIDERROR != pBusinessTask->nError)
                    {
                        pBusinessTask->nError = INVALIDERROR;
                        m_pTaskCallback(pBusinessTask, m_pUser);
                        continue;
                    }
                }

                //当前特征值结点不为空, 且当前特征值结点时间小于业务结束时间, 则继续搜索, 否则回调结果
                if (NULL != pBusinessTask->pCurFeatureData && pBusinessTask->pCurFeatureData->CollectTime < pBusinessTask->nEndTime)
                {
                    bool bStartSearch = false;
                    //如搜索深夜出入, 则定位到深夜结点
                    if (pBusinessTask->bNightSearch)
                    {
                        while (NULL != pBusinessTask->pCurFeatureData &&
                            (pBusinessTask->setAleadySearch.find(pBusinessTask->pCurFeatureData->FaceUUID) != pBusinessTask->setAleadySearch.end() || //已经检索出来的图片不用再参与检索
                            (pBusinessTask->pCurFeatureData->CollectTime - pBusinessTask->tFirstNightTime) % ONEDAYTIME > pBusinessTask->nRangeNightTime)) //非深夜结点数据, 重新定位到深夜结点
                        {
                            if (pBusinessTask->setAleadySearch.find(pBusinessTask->pCurFeatureData->FaceUUID) != pBusinessTask->setAleadySearch.end())
                            {
                                //从己保存的搜索过的特征值结点set里删除, 之后不会再搜索到
                                //pBusinessTask->setAleadySearch.erase(pBusinessTask->pCurFeatureData->FaceUUID);
                                pBusinessTask->pCurFeatureData = pBusinessTask->pDataIterator->Next();
                                pBusinessTask->nFinishSearchCount++;
                            }
                            else  //如果特征值结点时间 - 夜晚开始时间 > 夜晚持续时间, 则说明己超出夜晚范围, 重新定位;
                            {
                                //当前特征值结点处于从开始时间算起第几天
                                int64_t nIndexDay = (pBusinessTask->pCurFeatureData->CollectTime - pBusinessTask->tFirstNightTime) / ONEDAYTIME + 1;
                                if (pBusinessTask->tFirstNightTime + nIndexDay * ONEDAYTIME < pBusinessTask->nEndTime)
                                {
                                    //重新定位特征值结点
                                    int nRet = pBusinessTask->pDataIterator->SetTimeRange(pBusinessTask->tFirstNightTime + nIndexDay * ONEDAYTIME, pBusinessTask->nEndTime);
                                    printf("[%s][%s] ReIndex Night Search Time: %s.\n", m_pLibName, pBusinessTask->pSearchTaskID, 
                                        ChangeSecondToTime((pBusinessTask->tFirstNightTime + nIndexDay * ONEDAYTIME) / 1000).c_str());
                                    if (nRet < 0)
                                    {
                                        pBusinessTask->pCurFeatureData = NULL;
                                    }
                                    else
                                    {
                                        pBusinessTask->pCurFeatureData = pBusinessTask->pDataIterator->First();
                                    }
                                }
                                else
                                {
                                    pBusinessTask->pCurFeatureData = NULL;
                                }
                            }
                        }
                    }
                    else
                    {
                        //已经检索出来的图片不用再参与检索
                        while (NULL != pBusinessTask->pCurFeatureData &&
                            pBusinessTask->setAleadySearch.find(pBusinessTask->pCurFeatureData->FaceUUID) != pBusinessTask->setAleadySearch.end())
                        {
                            //从己保存的搜索过的特征值结点set里删除, 之后不会再搜索到
                            //pBusinessTask->setAleadySearch.erase(pBusinessTask->pCurFeatureData->FaceUUID);
                            pBusinessTask->pCurFeatureData = pBusinessTask->pDataIterator->Next();
                            pBusinessTask->nFinishSearchCount++;
                        }
                    }

                    if (NULL == pBusinessTask->pCurFeatureData || pBusinessTask->pCurFeatureData->CollectTime > pBusinessTask->nEndTime)
                    {
                        m_pTaskCallback(pBusinessTask, m_pUser);
                    }
                    else
                    {
                        
                        if (!pBusinessTask->bNightSearch)  //非统计深夜出入时, 计算进度
                        {
                            int64_t nProgress = 100 - (((pBusinessTask->nTimeFeatureCount - pBusinessTask->nFinishSearchCount) *
                                (pBusinessTask->nTimeFeatureCount - pBusinessTask->nFinishSearchCount - 1) * 100) /
                                (pBusinessTask->nTimeFeatureCount * (pBusinessTask->nTimeFeatureCount - 1)) +
                                ((pBusinessTask->nTimeFeatureCount - pBusinessTask->nFinishSearchCount) * 100 / pBusinessTask->nTimeFeatureCount)) / 2;
                            if (nProgress % 10 == 0)
                            {
                                if (nProgress != pBusinessTask->nProgress)
                                {
                                    printf("[%s][%s]Business Progress: %lld%, Finish: %d.\n",
                                        m_pLibName, pBusinessTask->pSearchTaskID, nProgress, pBusinessTask->nFinishSearchCount);
                                    pBusinessTask->nProgress = nProgress;
                                }
                            }
                        }
                        
                        LISTFEATURETHREADINFO::iterator it = m_listFeatureSearch.begin();
                        for (; it != m_listFeatureSearch.end(); it++)
                        {
                            //以当前特征值结点时间为开始时间分配搜索任务
                            if ((*it)->nBeginTime <= pBusinessTask->nEndTime && (*it)->nEndTime >= pBusinessTask->pCurFeatureData->CollectTime)
                            {
                                (*it)->pFeatureSearchThread->AddSearchTask(pBusinessTask);
                                if (!bStartSearch)
                                {
                                    //有线程分配搜索任务
                                    bStartSearch = true;
                                    //为当前特征值结点创建person插入, 以结点特征值指针地址为PersonID
                                    LPBUSINESSRESULT pBusinessResult = new BUSINESSRESULT;
                                    pBusinessTask->nPersonID++;
                                    pBusinessTask->mapBusinessResult.insert(make_pair(pBusinessTask->nPersonID, pBusinessResult));
                                }
                                EnterBusinessMutex();
                                pBusinessTask->nSearchThreadNum++;
                                LeaveBusinessMutex();
                            }
                        }
                        if (!bStartSearch)  //如果没有分配搜索任务, 则回调结果
                        {
                            m_pTaskCallback(pBusinessTask, m_pUser);
                        }
                    }
                }
                else
                {
                    m_pTaskCallback(pBusinessTask, m_pUser);
                }
            }
        } while (NULL != pBusinessTask);

        //Sleep(5);
    }
}
bool CFeatureManage::CheckNightTime(int nNightStartTime, int nNightEndTime, int64_t &nTime)
{
    bool bRet = true;
    time_t ctime = nTime / 1000;
    tm *tTime = localtime(&ctime);
    if (nNightEndTime > nNightStartTime)
    {
        if (tTime->tm_hour < nNightStartTime || tTime->tm_hour > nNightEndTime)
        {
            tTime->tm_mday = tTime->tm_hour < nNightStartTime ? tTime->tm_mday : tTime->tm_mday + 1;
            bRet = false;
        }
    }
    else
    {
        if (tTime->tm_hour < nNightStartTime && tTime->tm_hour > nNightEndTime)
        {
            bRet = false;
        }
        else
        {
            tTime->tm_mday = tTime->tm_hour > nNightStartTime ? tTime->tm_mday : tTime->tm_mday - 1;
        }
    }
    tTime->tm_hour = nNightStartTime;
    tTime->tm_min = 0;
    tTime->tm_sec = 0;
    nTime = mktime(tTime) * 1000;
    return bRet;
}
string CFeatureManage::ChangeSecondToTime(unsigned long long nSecond)
{
    time_t ctime = nSecond;
    tm *tTime = localtime(&ctime);
    char sTime[20];
    sprintf(sTime, "%04d-%02d-%02d %02d:%02d:%02d", tTime->tm_year+1900, tTime->tm_mon+1, tTime->tm_mday,
        tTime->tm_hour, tTime->tm_min, tTime->tm_sec);
    string RetTime = sTime;
    return sTime;
}
void CFeatureManage::EnterSearchMutex()
{
#ifdef __WINDOWS__
    EnterCriticalSection(&m_csTask);
#else
    pthread_mutex_lock(&m_csTask);
#endif
}
void CFeatureManage::LeaveSearchMutex()
{
#ifdef __WINDOWS__
    LeaveCriticalSection(&m_csTask);
#else
    pthread_mutex_unlock(&m_csTask);
#endif
}
void CFeatureManage::EnterWaitMutex()
{
#ifdef __WINDOWS__
    EnterCriticalSection(&m_csWait);
#else
    pthread_mutex_lock(&m_csWait);
#endif
}
void CFeatureManage::LeaveWaitMutex()
{
#ifdef __WINDOWS__
    LeaveCriticalSection(&m_csWait);
#else
    pthread_mutex_unlock(&m_csWait);
#endif
}
void CFeatureManage::EnterBusinessMutex()
{
#ifdef __WINDOWS__
    EnterCriticalSection(&m_csBusiness);
#else
    pthread_mutex_lock(&m_csBusiness);
#endif
}
void CFeatureManage::LeaveBusinessMutex()
{
#ifdef __WINDOWS__
    LeaveCriticalSection(&m_csBusiness);
#else
    pthread_mutex_unlock(&m_csBusiness);
#endif
}