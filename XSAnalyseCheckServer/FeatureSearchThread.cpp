#include "stdafx.h"
#include "FeatureSearchThread.h"

CFeatureSearchThread::CFeatureSearchThread()
{
    m_nServerType = 1;
    m_pFaceFeaturedb = NULL;        //��������ֵ����
    m_pKeyLibFeatureInfo = NULL;    //�ص������ֵ����

    m_bStopTask = false;
    m_pSearchResult = NULL;
    m_nBeginTime = 0;
    m_nEndTime = 0;
#ifdef __WINDOWS__
    m_hThreadID = INVALID_HANDLE_VALUE;
    InitializeCriticalSection(&m_cs);
    m_hSearchEvent = CreateEvent(NULL, true, false, NULL);
#else
    m_hThreadID = -1;
    pthread_mutex_init(&m_mutex, NULL);
    pipe(m_nPipe);
#endif
}

CFeatureSearchThread::~CFeatureSearchThread()
{
#ifdef __WINDOWS__
    DeleteCriticalSection(&m_cs);
    CloseHandle(m_hSearchEvent);
#else
    pthread_mutex_destroy(&m_mutex);
    close(m_nPipe[0]);
    close(m_nPipe[1]);
#endif
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
#ifdef __WINDOWS__
    m_hThreadID = CreateThread(NULL, 0, SearchThread, this, NULL, 0);   //�����ȶ��߳�
#else
    pthread_create(&m_hThreadID, NULL, SearchThread, (void*)this);
#endif
    printf("--CFeatureSearchThread::Create Thread.\n");
    return true;
}
void CFeatureSearchThread::UnInit()
{
    m_bStopTask = true;
#ifdef __WINDOWS__
    SetEvent(m_hSearchEvent);
    WaitForSingleObject(m_hThreadID, INFINITE);
    Sleep(100);
#else
    write(m_nPipe[1], "1", 1);
    pthread_join(m_hThreadID, NULL);
    usleep(100 * 1000);
#endif
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
//�����ص������ֵ
int CFeatureSearchThread::AddFeature(LPKEYLIBFEATUREDATA pFeatureData)
{
    int nRet = INVALIDERROR;
    //�Ȳ���FaceUUID�Ƿ񼺴���, �����ظ����
    EnterMutex();
    //MAPKEYLIBFEATUREDATA::iterator itFeature = m_pKeyLibFeatureInfo->mapFeature.find(pFeatureData->pFaceUUID);
    //if (itFeature != m_pKeyLibFeatureInfo->mapFeature.end())    //FaceUUID������
    {
        //nRet = FaceUUIDAleadyExist;
        //printf("***Warning: CKeyLibFeatureManage::FaceUUID[%s] Aleady Exist.\n", pFeatureData->pFaceUUID);
    }
    //else
    {
        m_pKeyLibFeatureInfo->mapFeature.insert(make_pair(pFeatureData->pFaceUUID, pFeatureData));

        if (NULL == m_pKeyLibFeatureInfo->pHeadFeature) //��һ�β���
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
    LeaveMutex();
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
        EnterMutex();

        MAPKEYLIBFEATUREDATA::iterator itFeature = m_pKeyLibFeatureInfo->mapFeature.find(pFaceUUID);
        if (itFeature != m_pKeyLibFeatureInfo->mapFeature.end())
        {
            if (itFeature->second == m_pKeyLibFeatureInfo->pHeadFeature)   //��ɾ��ͷָ��
            {
                m_pKeyLibFeatureInfo->pHeadFeature = m_pKeyLibFeatureInfo->pHeadFeature->pNext;   //ͷָ��ָ����һ�����
                if (NULL == m_pKeyLibFeatureInfo->pHeadFeature)    //���ʱͷָ��Ϊ��, ��˵���޽��, βָ��Ҳ��Ϊ��
                {
                    m_pKeyLibFeatureInfo->pTailFeature = NULL;
                }
                else//��ʱͷָ�벻Ϊ��
                {
                    m_pKeyLibFeatureInfo->pHeadFeature->pPrevious = NULL;
                }
            }
            else if (itFeature->second == m_pKeyLibFeatureInfo->pTailFeature)   //��ɾ��βָ��
            {
                m_pKeyLibFeatureInfo->pTailFeature = itFeature->second->pPrevious; //ָ��ָ��ǰһ�����
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
        LeaveMutex();

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
        EnterMutex();

        while (NULL != m_pKeyLibFeatureInfo->pHeadFeature)
        {
            LPKEYLIBFEATUREDATA pFeatureData = m_pKeyLibFeatureInfo->pHeadFeature->pNext;
            delete m_pKeyLibFeatureInfo->pHeadFeature;
            m_pKeyLibFeatureInfo->pHeadFeature = pFeatureData;
        }
        m_pKeyLibFeatureInfo->mapFeature.clear();
        LeaveMutex();

    }
    return 0;
}
void CFeatureSearchThread::AddSearchTask(LPSEARCHTASK pSearchTask)
{
    EnterMutex();
    m_listTask.push_back(pSearchTask); 
#ifdef __WINDOWS__
    SetEvent(m_hSearchEvent);
#else
    write(m_nPipe[1], "1", 1);
#endif
    LeaveMutex();
    return;
}

#ifdef __WINDOWS__
DWORD WINAPI CFeatureSearchThread::SearchThread(void * pParam)
{
    CFeatureSearchThread * pThis = (CFeatureSearchThread *)pParam;
    pThis->SearchAction();
    return 0;
}
#else
void * CFeatureSearchThread::SearchThread(void * pParam)
{
    CFeatureSearchThread * pThis = (CFeatureSearchThread *)pParam;
    pThis->SearchAction();
    return NULL;
}
#endif
void CFeatureSearchThread::SearchAction()
{
#ifndef __WINDOWS__
    char pPipeRead[10] = { 0 };
#endif
    int nSearchNodeNum = 0;
    float dbScore = 0.0;
    DataIterator * pDataIterator = NULL;     //����ֵ��������
    LPSEARCHTASK pSearchTask = NULL;
    if (1 == m_nServerType)
    {
        pDataIterator = m_pFaceFeaturedb->CreateIterator();
    }
    while (!m_bStopTask)
    {
#ifdef __WINDOWS__
        if (WAIT_TIMEOUT == WaitForSingleObject(m_hSearchEvent, THREADWAITTIME))
        {
            continue;
        }
        ResetEvent(m_hSearchEvent);
#else
        read(m_nPipe[0], pPipeRead, 1);
#endif
        do 
        {
            EnterMutex();
            if (m_listTask.size() > 0)
            {
                pSearchTask = m_listTask.front();
                m_listTask.pop_front();
            }
            else
            {
                pSearchTask = NULL;
            }
            LeaveMutex();
            if (NULL != pSearchTask)
            {
                m_pSearchResult->Init();

                if (LIBSEARCH == pSearchTask->nTaskType)
                {
                    if (1 == m_nServerType)
                    {
                        int64_t nBeginTime = pSearchTask->nBeginTime > m_nBeginTime ? pSearchTask->nBeginTime : m_nBeginTime;
                        int64_t nEndTime = pSearchTask->nEndTime < m_nEndTime ? pSearchTask->nEndTime : m_nEndTime;
                        int nRet = pDataIterator->SetTimeRange(nBeginTime, nEndTime);
                        if (-2 != nRet && -3 != nRet)   //ָ��ʱ�����������ֵ���
                        {
                            FeatureData * pFeatureData = pDataIterator->First();

                            while (NULL != pFeatureData)
                            {
                                if (pSearchTask->bStopSearch)
                                {
                                    break;
                                }

                                LPSEARCHRESULT pSearchResult = NULL;
                                for (int i = 0 ; i < pSearchTask->vectorFeatureInfo.size(); i ++)
                                {
                                    int nScore = VerifyFeature((unsigned char*)pSearchTask->vectorFeatureInfo[i]->pFeature,
                                        (unsigned char*)pFeatureData->FeatureValue, pSearchTask->vectorFeatureInfo[i]->nFeatureLen, dbScore);
                                    if (nScore >= pSearchTask->nScore)  //ƥ�䷵�ط��� > ��ֵ, ����������
                                    {
                                        if (m_pSearchResult->nHighestScore < nScore)
                                        {
                                            m_pSearchResult->nHighestScore = nScore;  //����������߷���
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
                                            if (NULL != pFeatureData->pFaceURL && NULL != pFeatureData->pBkgURL && strlen(pFeatureData->pFaceURL) > 10)
                                            {
                                                strcpy(pSearchResult->pFaceURL, pFeatureData->pFaceURL);
                                                strcpy(pSearchResult->pBkgURL, pFeatureData->pBkgURL);
                                            }
                                            else
                                            {
                                                strcpy(pSearchResult->pFaceURL, "0");
                                                strcpy(pSearchResult->pBkgURL, "0");
                                            }
                                           
                                            pSearchResult->nHitCount = 1;
                                            m_pSearchResult->mapThreadSearchResult.insert(make_pair(pSearchResult->pFaceUUID, pSearchResult));
                                            m_pSearchResult->nLatestTime = pFeatureData->CollectTime;       //������������ʱ��
                                        }
                                        else
                                        {
                                            if (pSearchResult->nScore < nScore)
                                            {
                                                pSearchResult->nScore = nScore;  //��ǰ����ֵ���ж��, ȡ��߷�������
                                            }
                                            pSearchResult->nHitCount++;         //��ǰ����ֵ���д���
                                        }
                                    }
                                }
                                //��ȡ��һ������ֵ
                                pFeatureData = pDataIterator->Next();
                            }
                        }
                        //printf("Thread Search End,  Feature Number : %d, HitCount: %d\n", nSearchNodeNum, m_pSearchResult->mapThreadSearchResult.size());
                    }
                    else if (2 == m_nServerType)
                    {
                        nSearchNodeNum = 0;
                        LPKEYLIBFEATUREDATA pFeatureNode = m_pKeyLibFeatureInfo->pHeadFeature;
                        while (NULL != pFeatureNode)
                        {
                            if (pSearchTask->bStopSearch)
                            {
                                break;
                            }

                            LPSEARCHRESULT pSearchResult = NULL;
                            nSearchNodeNum++;
                            for (int i = 0; i < pSearchTask->vectorFeatureInfo.size(); i++)
                            {
                                int nScore = VerifyFeature((unsigned char*)pSearchTask->vectorFeatureInfo[i]->pFeature,
                                    (unsigned char*)pFeatureNode->pFeature, pSearchTask->vectorFeatureInfo[i]->nFeatureLen, dbScore);
                                if (nScore >= pSearchTask->nScore)  //ƥ�䷵�ط��� > ��ֵ, ����������
                                {
                                    if (m_pSearchResult->nHighestScore < nScore)
                                    {
                                        m_pSearchResult->nHighestScore = nScore;  //����������߷���
                                    }
                                    if (NULL == pSearchResult)
                                    {
                                        pSearchResult = new SEARCHRESULT;
                                        strcpy(pSearchResult->pFaceUUID, pFeatureNode->pFaceUUID);
                                        pSearchResult->nScore = nScore;
                                        strcpy(pSearchResult->pDisk, pFeatureNode->pDisk);
                                        strcpy(pSearchResult->pImageIP, pFeatureNode->pImageIP);
                                        strcpy(pSearchResult->pFaceRect, "0,0,0,0");
                                        if (NULL != pFeatureNode->pFaceURL && strlen(pFeatureNode->pFaceURL) > 10)
                                        {
                                            strcpy(pSearchResult->pFaceURL, pFeatureNode->pFaceURL);
                                            strcpy(pSearchResult->pBkgURL, pFeatureNode->pFaceURL);
                                        }
                                        else
                                        {
                                            strcpy(pSearchResult->pFaceURL, "0");
                                            strcpy(pSearchResult->pBkgURL, "0");
                                        }
                                            
                                        pSearchResult->nHitCount = 1;
                                        m_pSearchResult->mapThreadSearchResult.insert(make_pair(pSearchResult->pFaceUUID, pSearchResult));
                                    }
                                    else
                                    {
                                        if (pSearchResult->nScore < nScore)
                                        {
                                            pSearchResult->nScore = nScore;  //��ǰ����ֵ���ж��, ȡ��߷�������
                                        }
                                        pSearchResult->nHitCount++;         //��ǰ����ֵ���д���
                                    }
                                }
                            }
                            pFeatureNode = pFeatureNode->pNext;
                        }
                        printf("Search End,  Feature Number : %d, HitCount: %d\n", nSearchNodeNum, m_pSearchResult->mapThreadSearchResult.size());
                    }
                }
                else if (LIBBUSINESS == pSearchTask->nTaskType)     //ҵ��ͳ�Ƶ�
                {
                    int64_t nBeginTime = pSearchTask->pCurFeatureData->CollectTime > m_nBeginTime ? pSearchTask->pCurFeatureData->CollectTime : m_nBeginTime;
                    int64_t nEndTime = pSearchTask->nEndTime < m_nEndTime ? pSearchTask->nEndTime : m_nEndTime;

                    if (nBeginTime < nEndTime)
                    {
                        int nRet = pDataIterator->SetTimeRange(nBeginTime, nEndTime);
                        if (0 == nRet)   //ָ��ʱ�����������ֵ���
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
                                    (unsigned char*)pFeatureData->FeatureValue, pSearchTask->pCurFeatureData->FeatureValueLen, dbScore);
                                if (nScore >= pSearchTask->nScore &&    //ƥ�䷵�ط��� > ��ֵ, ����������
                                    pSearchTask->setAleadySearch.find(pFeatureData->FaceUUID) == pSearchTask->setAleadySearch.end())    //�鿴��ǰ����ֵ����Ƿ�ƥ���, ƥ������ټ�����
                                {
                                    LPSEARCHRESULT pSearchResult = new SEARCHRESULT;
                                    strcpy(pSearchResult->pFaceUUID, pFeatureData->FaceUUID);
                                    pSearchResult->nScore = nScore;
                                    pSearchResult->nTime = pFeatureData->CollectTime;
                                    strcpy(pSearchResult->pDisk, pFeatureData->pDisk);
                                    strcpy(pSearchResult->pImageIP, pFeatureData->pImageIP);
                                    strcpy(pSearchResult->pFaceRect, pFeatureData->pFaceRect);
                                    if (NULL != pFeatureData->pFaceURL && NULL != pFeatureData->pBkgURL && strlen(pFeatureData->pFaceURL) > 10)
                                    {
                                        strcpy(pSearchResult->pFaceURL, pFeatureData->pFaceURL);
                                        strcpy(pSearchResult->pBkgURL, pFeatureData->pBkgURL);
                                    }
                                    else
                                    {
                                        strcpy(pSearchResult->pFaceURL, "0");
                                        strcpy(pSearchResult->pBkgURL, "0");
                                    }
                                        
                                    m_pSearchResult->mapThreadSearchResult.insert(make_pair(pSearchResult->pFaceUUID, pSearchResult));
                                    if (strcmp(pSearchTask->pCurFeatureData->FaceUUID, pFeatureData->FaceUUID) != 0)
                                    {
                                        m_pSearchResult->setAleadySearchResult.insert(pSearchResult->pFaceUUID);
                                    }
                                }

                                //��ȡ��һ������ֵ
                                pFeatureData = pDataIterator->Next();
                            }
                        }
                    }
                    
                }

                //������ɺ�, �ص����
                m_pResultCallback(pSearchTask, m_pSearchResult, m_pUser);
            }
        } while (NULL != pSearchTask);
    }
    return ;
}
//����ֵ�ȶ�
int CFeatureSearchThread::VerifyFeature(unsigned char * pf1, unsigned char * pf2, int nLen, float & dbScore)
{
#ifdef __WINDOWS__
        m_FRSVerify.Verify(pf1, pf2, &dbScore);
    //if (1 == m_FRSVerify.Verify(pf1, pf2, &dbScore))
#else
    if (0 == st_feature_comp(pf1, pf2, nLen, &dbScore, 0))
#endif
        /*{
        }
        else
        {
            printf("***Warning: st_feature_comp Failed, Stop Search!\n");
            return -1;
        }*/
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
void CFeatureSearchThread::EnterMutex()
{
#ifdef __WINDOWS__
    EnterCriticalSection(&m_cs);
#else
    pthread_mutex_lock(&m_mutex);
#endif
}
void CFeatureSearchThread::LeaveMutex()
{
#ifdef __WINDOWS__
    LeaveCriticalSection(&m_cs);
#else
    pthread_mutex_unlock(&m_mutex);
#endif
}