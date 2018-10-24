#include "FaceFeatureDB.h"
#include <stdio.h>
#include <time.h>

FaceFeatureDB::FaceFeatureDB(void)
{
}


FaceFeatureDB::~FaceFeatureDB(void)
{
	//清除所有数据
}
void FaceFeatureDB::InitDB()
{
	if(m_indexStruct.HeadNode != NULL)
    {
        printf("Error:不能重复初始化!\n" );
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
        printf("Error:未初始化!\n" );
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
        printf("Error:未初始化!\n" );
    }

	//	获取第一个数据结点
	LeafNode *fristLeafNode = m_indexStruct.dataHeadNode->nextNode;

	if (fristLeafNode == NULL)
	{
		return false;
	}

	
	begintime = fristLeafNode->featureData->CollectTime;
	endtime   = m_lastDateTime.collectTime;  //最后一条记录的时间
	return true;
}

/*
			DataIterator迭代
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
	//	清除时间范围
	if (begintimeFind == 0 && endtimeFind == 0)
	{
		m_IsSetTimeRange = false;
		return 0;
	}

	//	参数是否正常
	if (begintimeFind > endtimeFind )
	{
		printf("结束时间必须大于开始时间，上层必须控制时间范围\n" );
		return -1;	//-1：错误的时间段(开始时间大于结束时间)
	}

	int64_t begintimeFact ;
    int64_t endtimeFact;

	//	是否有数据
	if( m_pFacefeatureDb->GetRecordNum() <=0 || !m_pFacefeatureDb->GetDataDate(begintimeFact, endtimeFact))
	{
		return -2; // -2: 没有时间范围内的数据。
	}

	//	时间范围内是否有数据
	if (begintimeFind > endtimeFact || endtimeFind < begintimeFact)
	{
		return -3;//-3: 时间范围内没有数据;
	}

	//	调整时间

	//	调整开始时间，以便正确定位开始时间
	if (begintimeFind < begintimeFact)
	{
		ParseTime(m_begintime, begintimeFact);
	}else
	{
		ParseTime(m_begintime, begintimeFind);
	}

	//	调整结束时间，以便保证遍历数据正确结束
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
    LeafNode * pCurrentNode = m_pCurrentNode;  //当前结点可能为秒结点后的子结点
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
        //	再竖向查找不同时间的兄弟结点。
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
	
	//	从第一个数据结点开始遍历
	if (m_IsSetTimeRange==false)
	{
        int64_t begintimeFact ; //不使用
        int64_t endtimeFact;

		//	限定结束时间
		m_pFacefeatureDb->GetDataDate(begintimeFact, endtimeFact);
		m_endtime.collectTime = endtimeFact;

		m_pCurrentNode = GetFristDataNode();	
		m_pSubHeadNode = m_pCurrentNode;
		return m_pCurrentNode->featureData;
	}

	//	找到第一个大于开始时间的数据结点

	TreeNode *fristYearNode = GetFristYearNode();
	LeafNode *leafNode = m_pFacefeatureDb->m_pHandler->LocalTimeNode(fristYearNode, m_begintime , true);

	if (!leafNode)
	{
		return 0;
	}

	m_pCurrentNode = leafNode;  //当前结点可能为秒结点后的子结点
    m_pSubHeadNode = m_pCurrentNode;
    while (m_pSubHeadNode->ParentNode != NULL)    //m_pSubHeadNode定位到子结点所属的秒结点
    {
        m_pSubHeadNode = m_pSubHeadNode->ParentNode;
    }
	return m_pCurrentNode->featureData;

}

FeatureData *DataIterator::Next()
{

	FeatureData *pdata = NULL;
	//	先横向查找同一时间的孩子结点。
	if (m_pCurrentNode->childNode!=0)
	{
		m_pCurrentNode = m_pCurrentNode->childNode;
		pdata =  m_pCurrentNode->featureData;
    }
	//	再竖向查找不同时间的兄弟结点。
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

    //	从第一个数据结点开始遍历
    if (m_IsSetTimeRange == false)
    {
        int64_t begintimeFact; //不使用
        int64_t endtimeFact;

        //	限定结束时间
        m_pFacefeatureDb->GetDataDate(begintimeFact, endtimeFact);
        m_endtime.collectTime = endtimeFact;

        m_pCurrentNode = GetFristDataNode();
        m_pSubHeadNode = m_pCurrentNode;
        return m_pCurrentNode->featureData;
    }

    //	找到第一个小于结束时间的数据结点

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
    //	先横向查找同一时间的父结点。
    if (m_pCurrentNode->ParentNode != 0)
    {
        m_pCurrentNode = m_pCurrentNode->ParentNode;
        pdata = m_pCurrentNode->featureData;
    }
    //	再竖向查找不同时间的兄弟结点。
    else if (m_pCurrentNode->previousNode != 0)
    {
        m_pCurrentNode = m_pCurrentNode->previousNode;
        while (m_pCurrentNode->childNode != NULL)   //查找到后, 定位到同一秒的最后一个结点, 再从后往前取
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


