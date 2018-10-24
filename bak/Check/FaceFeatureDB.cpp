#include "FaceFeatureDB.h"
#include <stdio.h>
#include <time.h>

FaceFeatureDB::FaceFeatureDB(void)
{
}


FaceFeatureDB::~FaceFeatureDB(void)
{
	//�����������
}
void FaceFeatureDB::InitDB()
{
	if(m_indexStruct.HeadNode != NULL)
    {
        printf("Error:�����ظ���ʼ��!\n" );
    }

	m_RecoredNum = 0 ;

	m_indexStruct.HeadNode = new TreeNode();
	m_indexStruct.dataHeadNode = new LeafNode();
	m_indexStruct.dataNode = m_indexStruct.dataHeadNode;

	
	m_pHandler = new YearHandler(); 
	m_pHandler->SetSuccessor(new MonthHandler())->SetSuccessor(new DayHandler())->SetSuccessor(new HourHandler())->SetSuccessor(new DataHandler())->SetSuccessor((DBHandler *)0);
}


int FaceFeatureDB::AppendFaceFeatureData(FeatureData *pFeature)
{
	
	if(m_indexStruct.HeadNode == NULL)
    {
        printf("Error:δ��ʼ��!\n" );
    }

    int64_t nCollectTime = pFeature->CollectTime / 1000;
	struct tm  *t = localtime((time_t*)&nCollectTime);
	m_newDateTime.year  = t->tm_year + 1900;
	m_newDateTime.month = t->tm_mon + 1 ; //0-11
	m_newDateTime.day   = t->tm_mday; //1-31
	m_newDateTime.hour  = t->tm_hour; //0-23
	m_newDateTime.collectTime = pFeature->CollectTime;

	CountRecond();
	return m_pHandler->AppendNode(m_lastDateTime, m_newDateTime, m_indexStruct, pFeature, false);
}

DataIterator *FaceFeatureDB::CreateIterator()
{
	return new DataIterator(this);
}

bool FaceFeatureDB::GetDataDate(int64_t &begintime, int64_t &endtime)
{
    if(m_indexStruct.HeadNode == NULL)
    {
        printf("Error:δ��ʼ��!\n" );
    }

	//	��ȡ��һ�����ݽ��
	LeafNode *fristLeafNode = m_indexStruct.dataHeadNode->nextNode;

	if (fristLeafNode == NULL)
	{
		return false;
	}

	
	begintime = fristLeafNode->featureData->CollectTime;
	endtime   = m_lastDateTime.collectTime;  //���һ����¼��ʱ��
	return true;
}

/*
			DataIterator����
*/


DataIterator::DataIterator(FaceFeatureDB *pFacefeatureDb)
{
	m_pFacefeatureDb = pFacefeatureDb;
	m_pCurrentNode = NULL;
	m_IsSetTimeRange = false;
	m_begintime.collectTime = 0 ; 
	m_endtime.collectTime = 0;
}


int DataIterator::SetTimeRange(int64_t begintimeFind , int64_t endtimeFind )
{
	//	���ʱ�䷶Χ
	if (begintimeFind == 0 && endtimeFind == 0)
	{
		m_IsSetTimeRange = false;
		return 0;
	}

	//	�����Ƿ�����
	if (begintimeFind > endtimeFind )
	{
		printf("����ʱ�������ڿ�ʼʱ�䣬�ϲ�������ʱ�䷶Χ\n" );
		return -1;	//-1�������ʱ���(��ʼʱ����ڽ���ʱ��)
	}

	int64_t begintimeFact ;
    int64_t endtimeFact;

	//	�Ƿ�������
	if( m_pFacefeatureDb->GetRecordNum() <=0 || !m_pFacefeatureDb->GetDataDate(begintimeFact, endtimeFact))
	{
		return -2; // -2: û��ʱ�䷶Χ�ڵ����ݡ�
	}

	//	ʱ�䷶Χ���Ƿ�������
	if (begintimeFind > endtimeFact || endtimeFind < begintimeFact)
	{
		return -3;//-3: ʱ�䷶Χ��û������;
	}

	//	����ʱ��

	//	������ʼʱ�䣬�Ա���ȷ��λ��ʼʱ��
	if (begintimeFind < begintimeFact)
	{
		ParseTime(m_begintime, begintimeFact);
	}else
	{
		ParseTime(m_begintime, begintimeFind);
	}

	//	��������ʱ�䣬�Ա㱣֤����������ȷ����
	if (endtimeFind > endtimeFact)
	{
		ParseTime(m_endtime, endtimeFact);
	}else
	{
		ParseTime(m_endtime, endtimeFind);
	}
	
	
	m_IsSetTimeRange = true;

	return 0;
}

int DataIterator::GetNodeCount()
{
    LeafNode * pCurrentNode = m_pCurrentNode;  //��ǰ������Ϊ�������ӽ��
    LeafNode * pSubHeadNode = m_pSubHeadNode;

    int nCount = 0;
    FeatureData *pdata = pCurrentNode->featureData;
    while (pdata != NULL)
    {
        nCount++;
        pdata = NULL;
        if (pCurrentNode->childNode != 0)
        {
            pCurrentNode = pCurrentNode->childNode;
            pdata = pCurrentNode->featureData;
        }
        //	��������Ҳ�ͬʱ����ֵܽ�㡣
        else if (pSubHeadNode->nextNode != 0)
        {
            pSubHeadNode = pSubHeadNode->nextNode;
            pCurrentNode = pSubHeadNode;
            pdata = pCurrentNode->featureData;
        }

        if (NULL == pdata || pdata->CollectTime > m_endtime.collectTime)
        {
            break;
        }
    }
    return nCount;
}

FeatureData *DataIterator::First()
{
	if( m_pFacefeatureDb->GetRecordNum()<=0)
	{
		return 0;
	}
	
	//	�ӵ�һ�����ݽ�㿪ʼ����
	if (m_IsSetTimeRange==false)
	{
        int64_t begintimeFact ; //��ʹ��
        int64_t endtimeFact;

		//	�޶�����ʱ��
		m_pFacefeatureDb->GetDataDate(begintimeFact, endtimeFact);
		m_endtime.collectTime = endtimeFact;

		m_pCurrentNode = GetFristDataNode();	
		m_pSubHeadNode = m_pCurrentNode;
		return m_pCurrentNode->featureData;
	}

	//	�ҵ���һ�����ڿ�ʼʱ������ݽ��

	TreeNode *fristYearNode = GetFristYearNode();
	LeafNode *leafNode = m_pFacefeatureDb->m_pHandler->LocalTimeNode(fristYearNode, m_begintime , true);

	if (!leafNode)
	{
		return 0;
	}

	m_pCurrentNode = leafNode;  //��ǰ������Ϊ�������ӽ��
    m_pSubHeadNode = m_pCurrentNode;
    while (m_pSubHeadNode->ParentNode != NULL)    //m_pSubHeadNode��λ���ӽ������������
    {
        m_pSubHeadNode = m_pSubHeadNode->ParentNode;
    }
	return m_pCurrentNode->featureData;

}

FeatureData *DataIterator::Next()
{

	FeatureData *pdata = NULL;
	//	�Ⱥ������ͬһʱ��ĺ��ӽ�㡣
	if (m_pCurrentNode->childNode!=0)
	{
		m_pCurrentNode = m_pCurrentNode->childNode;
		pdata =  m_pCurrentNode->featureData;
    }
	//	��������Ҳ�ͬʱ����ֵܽ�㡣
	else if (m_pSubHeadNode->nextNode !=0)
	{
		m_pSubHeadNode = m_pSubHeadNode->nextNode;
		m_pCurrentNode = m_pSubHeadNode;
		pdata =  m_pCurrentNode->featureData;
	}

	if (pdata && pdata->CollectTime <= m_endtime.collectTime)
	{
		return pdata;
	}
	return 0;

}
FeatureData *DataIterator::Last()
{
    if (m_pFacefeatureDb->GetRecordNum() <= 0)
    {
        return 0;
    }

    //	�ӵ�һ�����ݽ�㿪ʼ����
    if (m_IsSetTimeRange == false)
    {
        int64_t begintimeFact; //��ʹ��
        int64_t endtimeFact;

        //	�޶�����ʱ��
        m_pFacefeatureDb->GetDataDate(begintimeFact, endtimeFact);
        m_endtime.collectTime = endtimeFact;

        m_pCurrentNode = GetFristDataNode();
        m_pSubHeadNode = m_pCurrentNode;
        return m_pCurrentNode->featureData;
    }

    //	�ҵ���һ��С�ڽ���ʱ������ݽ��

    TreeNode *fristYearNode = GetFristYearNode();
    LeafNode *leafNode = m_pFacefeatureDb->m_pHandler->LocalTimeNode(fristYearNode, m_endtime, true);

    if (!leafNode)
    {
        return 0;
    }

    m_pCurrentNode = leafNode;
    return m_pCurrentNode->featureData;

}

FeatureData *DataIterator::Previous()
{

    FeatureData *pdata = NULL;
    //	�Ⱥ������ͬһʱ��ĸ���㡣
    if (m_pCurrentNode->ParentNode != 0)
    {
        m_pCurrentNode = m_pCurrentNode->ParentNode;
        pdata = m_pCurrentNode->featureData;
    }
    //	��������Ҳ�ͬʱ����ֵܽ�㡣
    else if (m_pCurrentNode->previousNode != 0)
    {
        m_pCurrentNode = m_pCurrentNode->previousNode;
        while (m_pCurrentNode->childNode != NULL)   //���ҵ���, ��λ��ͬһ������һ�����, �ٴӺ���ǰȡ
        {
            m_pCurrentNode = m_pCurrentNode->childNode;
        }
        pdata = m_pCurrentNode->featureData;
    }

    if (pdata && pdata->CollectTime >= m_begintime.collectTime)
    {
        return pdata;
    }
    return 0;

}
LeafNode *DataIterator::GetFristDataNode()
{
	return m_pFacefeatureDb->m_indexStruct.dataHeadNode->nextNode;
}
TreeNode *DataIterator::GetFristYearNode()
{
	return m_pFacefeatureDb->m_indexStruct.HeadNode->nextNode;
}
void DataIterator::ParseTime(DateTimeStruct &ts, int64_t time)
{
    int64_t nTime = time / 1000;
	struct tm  *t = localtime((time_t*)&nTime) ;
	ts.year  = t->tm_year + 1900;
	ts.month = t->tm_mon + 1; //0-11
	ts.day   = t->tm_mday; //1-31
	ts.hour  = t->tm_hour; //0-23
	ts.collectTime = time;
}


