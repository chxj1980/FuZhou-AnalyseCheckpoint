#include "FeatureManage.h"

CFeatureManage::CFeatureManage(void)
{
    m_bStopTask = false;
    m_nServerType = 1;
    pthread_mutex_init(&m_csTask, NULL);
    pthread_mutex_init(&m_csWait, NULL);
    pthread_mutex_init(&m_csBusiness, NULL);
    m_pFaceFeaturedb = NULL;
}
CFeatureManage::~CFeatureManage(void)
{
    pthread_mutex_destroy(&m_csTask);
    pthread_mutex_destroy(&m_csWait);
    pthread_mutex_destroy(&m_csBusiness);
}
int CFeatureManage::Init(char * pDeviceID, int nServerType, int nMaxCount, LPSearchTaskCallback pTaskCallback, void * pUser)
{
    //�������
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

    //�ܵ�
    pipe(m_nPipeBusiness);
    pipe(m_nPipeSearch);
    pipe(m_nPipeWait);

    //���������߳�
    pthread_create(&m_ThreadIDSearch, NULL, LibTaskThread, (void*)this);
    pthread_create(&m_ThreadIDWait, NULL, LibSearchWaitThread, (void*)this);
    pthread_create(&m_ThreadIDBusiness, NULL, LibBusinessThread, (void*)this);
    printf("CFeatureManage::Lib[%s] Search Thread Begin.\n", pDeviceID);

    return true;
}
bool CFeatureManage::UnInit()
{
    m_bStopTask = true;
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

    pthread_mutex_lock(&m_csTask);
    while(m_listTask.size() > 0)
    {
        delete m_listTask.front();
        m_listTask.pop_front();
    }
    pthread_mutex_unlock(&m_csTask);

    pthread_mutex_lock(&m_csWait);
    while (m_listWait.size() > 0)
    {
        delete m_listWait.front();
        m_listWait.pop_front();
    }
    pthread_mutex_unlock(&m_csWait);

    pthread_mutex_lock(&m_csBusiness);
    while (m_listBusiness.size() > 0)
    {
        delete m_listBusiness.front();
        m_listBusiness.pop_front();
    }
    pthread_mutex_unlock(&m_csBusiness);

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
    close(m_nPipeBusiness[0]);
    close(m_nPipeBusiness[1]);
    close(m_nPipeSearch[0]);
    close(m_nPipeSearch[1]);
    close(m_nPipeWait[0]);
    close(m_nPipeWait[1]);
    return true;
}
//���ӿ��ڿ�����ֵ
int CFeatureManage::AddFeature(FeatureData * pFeatureData, bool bInit)
{
    int nRet = INVALIDERROR;
    //��������ֵ���
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
            //printf("[%s]Add Feature FaceUUID[%s].\n", m_pLibName, pFeatureData->FaceUUID);
        }
        pFeatureThreadInfo->nEndTime = pFeatureData->CollectTime;
        pFeatureThreadInfo->pFeatureSearchThread->SetCheckpointEndTime(pFeatureData->CollectTime);
        pFeatureThreadInfo->nFeatureCount++;

        //ͳ��ÿ������ֵ��������
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
    //��������ֵ���
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
    }
    return (ErrorCode)nRet;
}

//ɾ������ֵ
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
unsigned int CFeatureManage::GetTotalNum()
{
    int nTotal = 0;
    if (1 == m_nServerType)
    {
        nTotal = m_pFaceFeaturedb->GetRecordNum();;
    }
    else
    {
        for (LISTFEATURETHREADINFO::iterator it = m_listFeatureSearch.begin();
            it != m_listFeatureSearch.end(); it++)
        {
            nTotal += (*it)->nFeatureCount;
        }
    }
    return nTotal;
}
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
//����ץ��ͼƬ
int CFeatureManage::GetCaptureImageInfo(LPSEARCHTASK pSearchTask)
{
    int nRet = 0;
    int nGetAleady = 0;
    if (1 == m_nServerType && m_listFeatureSearch.size() > 0)
    {
        //printf("GetCaptureImage[%s][%d][%s - %s]\n", m_pLibName, pSearchTask->nCaptureCount,
            //ChangeSecondToTime(pSearchTask->nBeginTime / 1000).c_str(), ChangeSecondToTime(pSearchTask->nEndTime / 1000).c_str());
        DataIterator * pDataIterator = NULL;     //����ֵ��������
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

        printf("Aleady Get [%s] Capture Picture: %d.\n", m_pLibName, nGetAleady);
    }
    
    return nRet;
}
//������������, �������߳������ִ��
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
		pthread_mutex_lock(&m_csTask);
        m_listTask.push_back(pSearchTask);
        write(m_nPipeSearch[1], "1", 1);
        pthread_mutex_unlock(&m_csTask);
    }
    else if (LIBBUSINESS == pSearchTask->nTaskType)
    {
        
        if (1 == m_nServerType)
        {
            printf("AddBusinessTask[%s][%s-%s][Frequency: %d][similar: %d][NightSearch: %s].\n", m_pLibName,
                ChangeSecondToTime(pSearchTask->nBeginTime / 1000).c_str(), ChangeSecondToTime(pSearchTask->nEndTime / 1000).c_str(), 
                pSearchTask->nFrequently, pSearchTask->nScore, pSearchTask->bNightSearch ? "Y" : "N");
        }
		pthread_mutex_lock(&m_csBusiness);
        m_listBusiness.push_back(pSearchTask);
        write(m_nPipeBusiness[1], "1", 1);
        pthread_mutex_unlock(&m_csBusiness);
    }
    

    return;
}
//�����߳�
void * CFeatureManage::LibTaskThread(void * pParam)
{
    CFeatureManage * pThis = (CFeatureManage *)pParam;
    pThis->LibTaskAction();
    return NULL;
}
void CFeatureManage::LibTaskAction()
{
    char pPipeRead[2] = { 0 };
    LPSEARCHTASK pTask = NULL;
    while(!m_bStopTask)
    {
        read(m_nPipeSearch[0], pPipeRead, 1);
        do
        {
            pthread_mutex_lock(&m_csTask);
            if (m_listTask.size() > 0)
            {
                pTask = m_listTask.front();
                m_listTask.pop_front();
            }
            else
            {
                pTask = NULL;
            }
            pthread_mutex_unlock(&m_csTask);

            bool bStartSearch = false;
            if (NULL != pTask)
            {
                if (1 == m_nServerType)
                {
                    //���̷߳�����������
                    LISTFEATURETHREADINFO::iterator it = m_listFeatureSearch.begin();
                    for (; it != m_listFeatureSearch.end(); it++)
                    {
                        if ((*it)->nBeginTime <= pTask->nEndTime && (*it)->nEndTime >= pTask->nBeginTime)
                        {
                            pthread_mutex_lock(&m_csTask);
                            pTask->nSearchThreadNum++;
                            pthread_mutex_unlock(&m_csTask);
                            if (!bStartSearch)
                            {
                                bStartSearch = true;
                                pthread_mutex_lock(&m_csWait);
                                m_listWait.push_back(pTask);
                                pthread_mutex_unlock(&m_csWait);
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
                        pthread_mutex_lock(&m_csTask);
                        pTask->nSearchThreadNum++;
                        pthread_mutex_unlock(&m_csTask);
                        if (!bStartSearch)
                        {
                            bStartSearch = true;
                            pthread_mutex_lock(&m_csWait);
                            m_listWait.push_back(pTask);
                            pthread_mutex_unlock(&m_csWait);
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
//�����ȴ��߳�
void * CFeatureManage::LibSearchWaitThread(void * pParam)
{
    CFeatureManage * pThis = (CFeatureManage *)pParam;
    pThis->LibSearchWaitAction();
    return NULL;
}
void CFeatureManage::LibSearchWaitAction()
{
    char pPipeRead[10] = { 0 };
    while (!m_bStopTask)
    {
        read(m_nPipeWait[0], pPipeRead, 1);
        pthread_mutex_lock(&m_csWait);
        LISTSEARCHTASK::iterator itWait = m_listWait.begin();
        while(itWait != m_listWait.end())
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
        pthread_mutex_unlock(&m_csWait);
    }
}
void CFeatureManage::ThreadSearchCallback(LPSEARCHTASK pSearchTask, LPTHREADSEARCHRESULT pThreadSearchResult, void * pUser)
{
    CFeatureManage * pThis = (CFeatureManage *)pUser;
    if (LIBSEARCH == pSearchTask->nTaskType)
    {
        pthread_mutex_lock(&pThis->m_csTask);
        pSearchTask->nHighestScore = pSearchTask->nHighestScore > pThreadSearchResult->nHighestScore ? pSearchTask->nHighestScore : pThreadSearchResult->nHighestScore;
        pSearchTask->nTime = pSearchTask->nTime > pThreadSearchResult->nLatestTime ? pSearchTask->nTime : pThreadSearchResult->nLatestTime;
        pSearchTask->mapSearchTaskResult.insert(pThreadSearchResult->mapThreadSearchResult.begin(), pThreadSearchResult->mapThreadSearchResult.end());
        pSearchTask->nTotalCount = pSearchTask->mapSearchTaskResult.size();
        pSearchTask->nSearchThreadNum--;
        if (0 == pSearchTask->nSearchThreadNum)
        {
            write(pThis->m_nPipeWait[1], "1", 1);
        }
        pthread_mutex_unlock(&pThis->m_csTask);
    }
    else if (LIBBUSINESS == pSearchTask->nTaskType)
    {
        pthread_mutex_lock(&pThis->m_csBusiness);
        pSearchTask->nSearchThreadNum--;
        MAPBUSINESSRESULT::iterator it = pSearchTask->mapBusinessResult.find(pSearchTask->nPersonID);
        if (it == pSearchTask->mapBusinessResult.end())
        {
            pthread_mutex_unlock(&pThis->m_csBusiness);
            return;
        }
        it->second->mapPersonInfo.insert(pThreadSearchResult->mapThreadSearchResult.begin(), pThreadSearchResult->mapThreadSearchResult.end());
        pSearchTask->setAleadySearch.insert(pThreadSearchResult->setAleadySearchResult.begin(), pThreadSearchResult->setAleadySearchResult.end());

        if (0 == pSearchTask->nSearchThreadNum)
        {
            //printf("���[%s]Ƶ���������: %d.\n", pSearchTask->pCurFeatureData->FaceUUID, it->second->mapPersonInfo.size());
            //��ǰͼƬ�������С��ͳ����ֵ, ɾ��
            if (it->second->mapPersonInfo.size() < pSearchTask->nFrequently)
            {
                delete it->second;
                pSearchTask->mapBusinessResult.erase(pSearchTask->nPersonID);
            }

            pSearchTask->setAleadySearch.erase(pSearchTask->pCurFeatureData->FaceUUID);
            pSearchTask->pCurFeatureData = pSearchTask->pDataIterator->Next();
            pSearchTask->nFinishSearchCount++;
            pThis->m_listBusiness.push_back(pSearchTask);
            write(pThis->m_nPipeBusiness[1], "1", 1);
        }
        pthread_mutex_unlock(&pThis->m_csBusiness);
    }
    
    return;
}
//Ƶ�������ҵ���߳�
void * CFeatureManage::LibBusinessThread(void * pParam)
{
    CFeatureManage * pThis = (CFeatureManage *)pParam;
    pThis->LibBusinessAction();
    return NULL;
}
void CFeatureManage::LibBusinessAction()
{
    LPSEARCHTASK pBusinessTask = NULL;
    char pPipeRead[10] = { 0 };
    while (!m_bStopTask)
    {
        read(m_nPipeBusiness[0], pPipeRead, 1);
        do 
        {
            pthread_mutex_lock(&m_csBusiness);
            if (m_listBusiness.size() > 0)
            {
                pBusinessTask = m_listBusiness.front();
                m_listBusiness.pop_front();
            }
            else
            {
                pBusinessTask = NULL;
            }
            pthread_mutex_unlock(&m_csBusiness);

            if (NULL != pBusinessTask)
            {
                int nRet = INVALIDERROR;
                //��ȡָ��ʱ�������ֵ
                if (NULL == pBusinessTask->pDataIterator)     
                {
                    if (1 == m_nServerType && m_listFeatureSearch.size() > 0)
                    {
                        pBusinessTask->pDataIterator = m_pFaceFeaturedb->CreateIterator();

                        //���������ҹ����, ���Ȼ�ȡ��ǰ��ʼʱ����������ҹ��ʼʱ��, ����һ����ҹ��ʼʱ��
                        pBusinessTask->tFirstNightTime = pBusinessTask->nBeginTime;
                        if (pBusinessTask->bNightSearch)
                        {
                            CheckNightTime(pBusinessTask->nNightStartTime, pBusinessTask->nNightEndTime, pBusinessTask->tFirstNightTime);
                            printf("[%s]Night Search BeginTime: %s.\n", m_pLibName, ChangeSecondToTime(pBusinessTask->tFirstNightTime / 1000).c_str());
                        }

                        //��ʼ����ʱ��
                        int64_t nIndexTime = pBusinessTask->tFirstNightTime < pBusinessTask->nBeginTime ? pBusinessTask->nBeginTime : pBusinessTask->tFirstNightTime;
                        nRet = pBusinessTask->pDataIterator->SetTimeRange(nIndexTime, pBusinessTask->nEndTime);
                        if (nRet < 0)
                        {
                            pBusinessTask->nError = SearchNoDataOnTime;
                        }
                        else
                        {
                            pBusinessTask->pCurFeatureData = pBusinessTask->pDataIterator->First();    //�����һ������ֵ���, �������
                            if (NULL != pBusinessTask->pCurFeatureData)
                            {
                                printf("[%s] First Time[%s].\n", m_pLibName, ChangeSecondToTime(pBusinessTask->pCurFeatureData->CollectTime / 1000).c_str());
                                //��ȡָ��ʱ�������ֵ�������
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

                //��ǰ����ֵ��㲻Ϊ��, �ҵ�ǰ����ֵ���ʱ��С��ҵ�����ʱ��, ���������, ����ص����
                if (NULL != pBusinessTask->pCurFeatureData && pBusinessTask->pCurFeatureData->CollectTime < pBusinessTask->nEndTime)
                {
                    bool bStartSearch = false;
                    //��������ҹ����, ��λ����ҹ���
                    if (pBusinessTask->bNightSearch)
                    {
                        while (NULL != pBusinessTask->pCurFeatureData &&
                            (pBusinessTask->setAleadySearch.find(pBusinessTask->pCurFeatureData->FaceUUID) != pBusinessTask->setAleadySearch.end() || //�Ѿ�����������ͼƬ�����ٲ������
                            (pBusinessTask->pCurFeatureData->CollectTime - pBusinessTask->tFirstNightTime) % ONEDAYTIME > pBusinessTask->nRangeNightTime)) //����ҹ�������, ���¶�λ����ҹ���
                        {
                            if (pBusinessTask->setAleadySearch.find(pBusinessTask->pCurFeatureData->FaceUUID) != pBusinessTask->setAleadySearch.end())
                            {
                                //�Ӽ������������������ֵ���set��ɾ��, ֮�󲻻���������
                                //pBusinessTask->setAleadySearch.erase(pBusinessTask->pCurFeatureData->FaceUUID);
                                pBusinessTask->pCurFeatureData = pBusinessTask->pDataIterator->Next();
                                pBusinessTask->nFinishSearchCount++;
                            }
                            else  //�������ֵ���ʱ�� - ҹ��ʼʱ�� > ҹ�����ʱ��, ��˵��������ҹ��Χ, ���¶�λ;
                            {
                                //��ǰ����ֵ��㴦�ڴӿ�ʼʱ������ڼ���
                                int64_t nIndexDay = (pBusinessTask->pCurFeatureData->CollectTime - pBusinessTask->tFirstNightTime) / ONEDAYTIME + 1;
                                if (pBusinessTask->tFirstNightTime + nIndexDay * ONEDAYTIME < pBusinessTask->nEndTime)
                                {
                                    //���¶�λ����ֵ���
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
                        //�Ѿ�����������ͼƬ�����ٲ������
                        while (NULL != pBusinessTask->pCurFeatureData &&
                            pBusinessTask->setAleadySearch.find(pBusinessTask->pCurFeatureData->FaceUUID) != pBusinessTask->setAleadySearch.end())
                        {
                            //�Ӽ������������������ֵ���set��ɾ��, ֮�󲻻���������
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
                        if (!pBusinessTask->bNightSearch)  //��ͳ����ҹ����ʱ, �������
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
                            //�Ե�ǰ����ֵ���ʱ��Ϊ��ʼʱ�������������
                            if ((*it)->nBeginTime <= pBusinessTask->nEndTime && (*it)->nEndTime >= pBusinessTask->pCurFeatureData->CollectTime)
                            {
                                (*it)->pFeatureSearchThread->AddSearchTask(pBusinessTask);
                                if (!bStartSearch)
                                {
                                    //���̷߳�����������
                                    bStartSearch = true;
                                    //Ϊ��ǰ����ֵ��㴴��person����, �Խ������ֵָ���ַΪPersonID
                                    LPBUSINESSRESULT pBusinessResult = new BUSINESSRESULT;
                                    pBusinessTask->nPersonID++;
                                    pBusinessTask->mapBusinessResult.insert(make_pair(pBusinessTask->nPersonID, pBusinessResult));
                                }
                                pthread_mutex_lock(&m_csBusiness);
                                pBusinessTask->nSearchThreadNum++;
                                pthread_mutex_unlock(&m_csBusiness);
                            }
                        }
                        if (!bStartSearch)  //���û�з�����������, ��ص����
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

        //usleep(2 * 1000);
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
