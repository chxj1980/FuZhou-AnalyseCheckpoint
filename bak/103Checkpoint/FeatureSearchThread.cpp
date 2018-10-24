#include "FeatureSearchThread.h"

CFeatureSearchThread::CFeatureSearchThread()
{
    m_nServerType = 1;
    m_pFaceFeaturedb = NULL;        //卡口特征值保存
    m_pKeyLibFeatureInfo = NULL;    //重点库特征值保存

    m_hThreadID = -1;
    m_bStopTask = false;
    m_pSearchResult = NULL;
    m_nBeginTime = 0;
    m_nEndTime = 0;
    pthread_mutex_init(&m_mutex, NULL);
}

CFeatureSearchThread::~CFeatureSearchThread()
{
    pthread_mutex_destroy(&m_mutex);
}
bool CFeatureSearchThread::Init(int nServerType, LPThreadSearchCallback pCallback, void * pUser, FaceFeatureDB * pFaceFeatureDB)
{
    m_nServerType = nServerType;
    m_pResultCallback = pCallback;
    m_pUser = pUser;

    m_pSearchResult = new THREADSEARCHRESULT;

    if (1 == m_nServerType)
    {
        m_pFaceFeaturedb = pFaceFeatureDB;
    }
    else if (2 == m_nServerType)
    {
        m_pKeyLibFeatureInfo = new KEYLIBINFO;
    }
    pipe(m_nPipe);
    pthread_create(&m_hThreadID, NULL, SearchThread, (void*)this);
    printf("--CFeatureSearchThread::Create Thread.\n");
    return true;
}
void CFeatureSearchThread::UnInit()
{
    m_bStopTask = true;
    write(m_nPipe[1], "1", 1);
    usleep(100 * 1000);
    pthread_join(m_hThreadID, NULL);
    printf("--CFeatureSearchThread::End Thread.\n");
    
    if (NULL != m_pSearchResult)
    {
        delete m_pSearchResult;
        m_pSearchResult = NULL;
    }
    if (2 == m_nServerType)
    {
        if (NULL != m_pKeyLibFeatureInfo)
        {
            delete m_pKeyLibFeatureInfo;
            m_pKeyLibFeatureInfo = NULL;
        }
    }
    close(m_nPipe[0]);
    close(m_nPipe[1]);
    return;
}
void CFeatureSearchThread::SetCheckpointBeginTime(int64_t nBeginTime)
{
    m_nBeginTime = nBeginTime;
}
void CFeatureSearchThread::SetCheckpointEndTime(int64_t nEndTime)
{
    m_nEndTime = nEndTime;
}
//增加重点库特征值
int CFeatureSearchThread::AddFeature(LPKEYLIBFEATUREDATA pFeatureData)
{
    int nRet = INVALIDERROR;
    //先查找FaceUUID是否己存在, 不能重复入库
    pthread_mutex_lock(&m_mutex);
    //MAPKEYLIBFEATUREDATA::iterator itFeature = m_pKeyLibFeatureInfo->mapFeature.find(pFeatureData->pFaceUUID);
    //if (itFeature != m_pKeyLibFeatureInfo->mapFeature.end())    //FaceUUID己存在
    {
        //nRet = FaceUUIDAleadyExist;
        //printf("***Warning: CKeyLibFeatureManage::FaceUUID[%s] Aleady Exist.\n", pFeatureData->pFaceUUID);
    }
    //else
    {
        m_pKeyLibFeatureInfo->mapFeature.insert(make_pair(pFeatureData->pFaceUUID, pFeatureData));

        if (NULL == m_pKeyLibFeatureInfo->pHeadFeature) //第一次插入
        {
            m_pKeyLibFeatureInfo->pHeadFeature = pFeatureData;
            m_pKeyLibFeatureInfo->pTailFeature = pFeatureData;
        }
        else
        {
            m_pKeyLibFeatureInfo->pTailFeature->pNext = pFeatureData;
            pFeatureData->pPrevious = m_pKeyLibFeatureInfo->pTailFeature;
            m_pKeyLibFeatureInfo->pTailFeature = pFeatureData;
        }

        m_pKeyLibFeatureInfo->nNum++;
    }
    pthread_mutex_unlock(&m_mutex);

    return nRet;

}
int CFeatureSearchThread::DelFeature(const char * pFaceUUID)
{
    int nRet = INVALIDERROR;
    if (1 == m_nServerType)
    {

    }
    else if (2 == m_nServerType && NULL != m_pKeyLibFeatureInfo)
    {
        pthread_mutex_lock(&m_mutex);
        MAPKEYLIBFEATUREDATA::iterator itFeature = m_pKeyLibFeatureInfo->mapFeature.find(pFaceUUID);
        if (itFeature != m_pKeyLibFeatureInfo->mapFeature.end())
        {
            if (itFeature->second == m_pKeyLibFeatureInfo->pHeadFeature)   //如删除头指针
            {
                m_pKeyLibFeatureInfo->pHeadFeature = m_pKeyLibFeatureInfo->pHeadFeature->pNext;   //头指针指向下一个结点
                if (NULL == m_pKeyLibFeatureInfo->pHeadFeature)    //如此时头指针为空, 则说明无结点, 尾指针也置为空
                {
                    m_pKeyLibFeatureInfo->pTailFeature = NULL;
                }
                else//此时头指针不为空
                {
                    m_pKeyLibFeatureInfo->pHeadFeature->pPrevious = NULL;
                }
            }
            else if (itFeature->second == m_pKeyLibFeatureInfo->pTailFeature)   //如删除尾指针
            {
                m_pKeyLibFeatureInfo->pTailFeature = itFeature->second->pPrevious; //指针指向前一个结点
                m_pKeyLibFeatureInfo->pTailFeature->pNext = NULL;
            }
            else
            {
                itFeature->second->pPrevious->pNext = itFeature->second->pNext;
                itFeature->second->pNext->pPrevious = itFeature->second->pPrevious;
            }
            delete itFeature->second;
            m_pKeyLibFeatureInfo->mapFeature.erase(itFeature);
            m_pKeyLibFeatureInfo->nNum--;
            nRet = INVALIDERROR;
        }
        else
        {
            nRet = FaceUUIDNotExist;
        }
        pthread_mutex_unlock(&m_mutex);
    }
    return nRet;
}
int CFeatureSearchThread::ClearKeyLib()
{
    if (1 == m_nServerType)
    {

    }
    else if (2 == m_nServerType && NULL != m_pKeyLibFeatureInfo)
    {
        pthread_mutex_lock(&m_mutex);
        while (NULL != m_pKeyLibFeatureInfo->pHeadFeature)
        {
            LPKEYLIBFEATUREDATA pFeatureData = m_pKeyLibFeatureInfo->pHeadFeature->pNext;
            delete m_pKeyLibFeatureInfo->pHeadFeature;
            m_pKeyLibFeatureInfo->pHeadFeature = pFeatureData;
        }
        m_pKeyLibFeatureInfo->mapFeature.clear();
        pthread_mutex_unlock(&m_mutex);
    }
    return 0;
}
void CFeatureSearchThread::AddSearchTask(LPSEARCHTASK pSearchTask)
{
    pthread_mutex_lock(&m_mutex);
    m_listTask.push_back(pSearchTask); 
    write(m_nPipe[1], "1", 1);
    pthread_mutex_unlock(&m_mutex);
}
void * CFeatureSearchThread::SearchThread(void * pParam)
{
    CFeatureSearchThread * pThis = (CFeatureSearchThread *)pParam;
    pThis->SearchAction();
    return NULL;
}
void CFeatureSearchThread::SearchAction()
{
    char pPipeRead[10] = { 0 };
    int nSearchNodeNum = 0;
    DataIterator * pDataIterator = NULL;     //特征值查找索引
    LPSEARCHTASK pSearchTask = NULL;
    if (1 == m_nServerType)
    {
        pDataIterator = m_pFaceFeaturedb->CreateIterator();
    }
    while (!m_bStopTask)
    {
        read(m_nPipe[0], pPipeRead, 1);
        do 
        {
            pthread_mutex_lock(&m_mutex);
            if (m_listTask.size() > 0)
            {
                pSearchTask = m_listTask.front();
                m_listTask.pop_front();
            }
            else
            {
                pSearchTask = NULL;
            }
            pthread_mutex_unlock(&m_mutex);
            if (NULL != pSearchTask)
            {
                m_pSearchResult->Init();

                if (LIBSEARCH == pSearchTask->nTaskType)
                {
                    nSearchNodeNum = 0;
                    if (1 == m_nServerType)
                    {   
                        

                        int64_t nBeginTime = pSearchTask->nBeginTime > m_nBeginTime ? pSearchTask->nBeginTime : m_nBeginTime;
                        int64_t nEndTime = pSearchTask->nEndTime < m_nEndTime ? pSearchTask->nEndTime : m_nEndTime;
                        int nRet = pDataIterator->SetTimeRange(nBeginTime, nEndTime);
                        if (-2 != nRet && -3 != nRet)   //指定时间段内有特征值结点
                        {
                            FeatureData * pFeatureData = pDataIterator->First();
                            while (NULL != pFeatureData)
                            {
                                if (pSearchTask->bStopSearch)
                                {
                                    break;
                                }

                                nSearchNodeNum++;
                                LPSEARCHRESULT pSearchResult = NULL;
                                for (LISTSEARCHFEATURE::iterator it = pSearchTask->listFeatureInfo.begin();
                                    it != pSearchTask->listFeatureInfo.end(); it++)
                                {
                                    int nScore = VerifyFeature((unsigned char*)(*it)->pFeature,
                                        (unsigned char*)pFeatureData->FeatureValue, (*it)->nFeatureLen);
                                    if (nScore >= pSearchTask->nScore)  //匹配返回分数 > 阈值, 保存检索结果
                                    {
                                        if (m_pSearchResult->nHighestScore < nScore)
                                        {
                                            m_pSearchResult->nHighestScore = nScore;  //卡口命中最高分数
                                        }
                                        if (NULL == pSearchResult)
                                        {
                                            pSearchResult = new SEARCHRESULT;
                                            strcpy(pSearchResult->pFaceUUID, pFeatureData->FaceUUID);
                                            pSearchResult->nScore = nScore;
                                            pSearchResult->nTime = pFeatureData->CollectTime;
                                            strcpy(pSearchResult->pDisk, pFeatureData->pDisk);
                                            strcpy(pSearchResult->pImageIP, pFeatureData->pImageIP);
                                            strcpy(pSearchResult->pFaceRect, pFeatureData->pFaceRect);
                                            pSearchResult->nHitCount = 1;
                                            m_pSearchResult->mapThreadSearchResult.insert(make_pair(pSearchResult->pFaceUUID, pSearchResult));
                                            m_pSearchResult->nLatestTime = pFeatureData->CollectTime;       //卡口命中最新时间
                                        }
                                        else
                                        {
                                            if (pSearchResult->nScore < nScore)
                                            {
                                                pSearchResult->nScore = nScore;  //当前特征值命中多次, 取最高分数保存
                                            }
                                            pSearchResult->nHitCount++;         //当前特征值命中次数
                                        }
                                    }
                                }
                                //获取下一个特征值
                                pFeatureData = pDataIterator->Next();
                            }
                        }

                        //printf("Thread Search End,  Feature Number : %d, HitCount: %d\n", nSearchNodeNum, m_pSearchResult->mapThreadSearchResult.size());
                    }
                    else if (2 == m_nServerType)
                    {
                        LPKEYLIBFEATUREDATA pFeatureNode = m_pKeyLibFeatureInfo->pHeadFeature;
                        while (NULL != pFeatureNode)
                        {
                            if (pSearchTask->bStopSearch)
                            {
                                break;
                            }

                            LPSEARCHRESULT pSearchResult = NULL;
                            nSearchNodeNum++;
                            for (LISTSEARCHFEATURE::iterator it = pSearchTask->listFeatureInfo.begin();
                                it != pSearchTask->listFeatureInfo.end(); it++)
                            {
                                int nScore = VerifyFeature((unsigned char*)(*it)->pFeature,
                                    (unsigned char*)pFeatureNode->pFeature, (*it)->nFeatureLen);
                                if (nScore >= pSearchTask->nScore)  //匹配返回分数 > 阈值, 保存检索结果
                                {
                                    if (m_pSearchResult->nHighestScore < nScore)
                                    {
                                        m_pSearchResult->nHighestScore = nScore;  //卡口命中最高分数
                                    }
                                    if (NULL == pSearchResult)
                                    {
                                        pSearchResult = new SEARCHRESULT;
                                        strcpy(pSearchResult->pFaceUUID, pFeatureNode->pFaceUUID);
                                        pSearchResult->nScore = nScore;
                                        strcpy(pSearchResult->pDisk, pFeatureNode->pDisk);
                                        strcpy(pSearchResult->pImageIP, pFeatureNode->pImageIP);
                                        strcpy(pSearchResult->pFaceRect, "0,0,0,0");
                                        pSearchResult->nHitCount = 1;
                                        m_pSearchResult->mapThreadSearchResult.insert(make_pair(pSearchResult->pFaceUUID, pSearchResult));
                                    }
                                    else
                                    {
                                        if (pSearchResult->nScore < nScore)
                                        {
                                            pSearchResult->nScore = nScore;  //当前特征值命中多次, 取最高分数保存
                                        }
                                        pSearchResult->nHitCount++;         //当前特征值命中次数
                                    }
                                }
                            }
                            pFeatureNode = pFeatureNode->pNext;
                        }
                        printf("Search End,  Feature Number : %d, HitCount: %d\n", nSearchNodeNum, m_pSearchResult->mapThreadSearchResult.size());
                    }
                }
                else if (LIBBUSINESS == pSearchTask->nTaskType)     //业务统计等
                {
                    int64_t nBeginTime = pSearchTask->pCurFeatureData->CollectTime > m_nBeginTime ? pSearchTask->pCurFeatureData->CollectTime : m_nBeginTime;
                    int64_t nEndTime = pSearchTask->nEndTime < m_nEndTime ? pSearchTask->nEndTime : m_nEndTime;

                    if (nBeginTime < nEndTime)
                    {
                        int nRet = pDataIterator->SetTimeRange(nBeginTime, nEndTime);
                        if (0 == nRet)   //指定时间段内有特征值结点
                        {
                            FeatureData * pFeatureData = pDataIterator->First();
                            while (NULL != pFeatureData)
                            {
                                if (pSearchTask->bNightSearch)
                                {
                                    while (NULL != pFeatureData &&
                                        (pFeatureData->CollectTime - pSearchTask->tFirstNightTime) % ONEDAYTIME > pSearchTask->nRangeNightTime)
                                    {
                                        pFeatureData = pDataIterator->Next();
                                    }
                                    if (NULL == pFeatureData)
                                    {
                                        break;
                                    }
                                }

                                int nScore = VerifyFeature((unsigned char*)pSearchTask->pCurFeatureData->FeatureValue,
                                    (unsigned char*)pFeatureData->FeatureValue, pSearchTask->pCurFeatureData->FeatureValueLen);
                                if (nScore >= pSearchTask->nScore &&    //匹配返回分数 > 阈值, 保存检索结果
                                    pSearchTask->setAleadySearch.find(pFeatureData->FaceUUID) == pSearchTask->setAleadySearch.end())    //查看当前特征值结点是否己匹配过, 匹配过则不再计入结果
                                {
                                    LPSEARCHRESULT pSearchResult = new SEARCHRESULT;
                                    strcpy(pSearchResult->pFaceUUID, pFeatureData->FaceUUID);
                                    pSearchResult->nScore = nScore;
                                    pSearchResult->nTime = pFeatureData->CollectTime;
                                    strcpy(pSearchResult->pDisk, pFeatureData->pDisk);
                                    strcpy(pSearchResult->pImageIP, pFeatureData->pImageIP);
                                    strcpy(pSearchResult->pFaceRect, pFeatureData->pFaceRect);
                                    m_pSearchResult->mapThreadSearchResult.insert(make_pair(pSearchResult->pFaceUUID, pSearchResult));
                                    if (strcmp(pSearchTask->pCurFeatureData->FaceUUID, pFeatureData->FaceUUID) != 0)
                                    {
                                        m_pSearchResult->setAleadySearchResult.insert(pSearchResult->pFaceUUID);
                                    }
                                }

                                //获取下一个特征值
                                pFeatureData = pDataIterator->Next();
                            }
                        }
                    }
                    
                }

                //搜索完成后, 回调结果
                m_pResultCallback(pSearchTask, m_pSearchResult, m_pUser);
            }
        } while (NULL != pSearchTask);
        
        //usleep(THREADWAITTIME * 1000);
    }
    return ;
}
//特征值比对
int CFeatureSearchThread::VerifyFeature(unsigned char * pf1, unsigned char * pf2, int nLen)
{
    float dbScore = 0.0;
    if (0 == st_feature_comp(pf1, pf2, nLen, &dbScore, 0))
    {
    }
    else
    {
        printf("***Warning: FRSFeatureMatch Failed, Stop Search!\n");
        return -1;
    }
    //printf("Verify Score : %lf,  %d.\n", dbScore, int(dbScore * 100));
    return int(dbScore * 100);
}
bool CFeatureSearchThread::CheckNightTime(int nNightStartTime, int nNightEndTime, int64_t &nTime)
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
string CFeatureSearchThread::ChangeSecondToTime(unsigned long long nSecond)
{
    time_t ctime = nSecond;
    tm *tTime = localtime(&ctime);
    char sTime[20];
    sprintf(sTime, "%04d-%02d-%02d %02d:%02d:%02d", tTime->tm_year + 1900, tTime->tm_mon + 1, tTime->tm_mday,
        tTime->tm_hour, tTime->tm_min, tTime->tm_sec);
    string RetTime = sTime;
    return sTime;
}