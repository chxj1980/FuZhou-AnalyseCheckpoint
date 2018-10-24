#include "NodeHandler.h"
#include <time.h>
/************************************************************************/
/*                   年                                                 */
/************************************************************************/
int YearHandler::AppendNode(DateTimeStruct &lastDt, DateTimeStruct &newDt, IndexStruct &index, FeatureData *pFeature, bool handleFlag) 
{

	if (lastDt.year == newDt.year)
	{
		return m_pHandler->AppendNode(lastDt, newDt, index, pFeature, false);
	}


	TreeNode* yearNode = new TreeNode();
	yearNode->value = newDt.year; 
	if (index.yearNode == 0)
	{
		index.HeadNode->nextNode = yearNode; //第一个年结点
	}else
	{
		index.yearNode->nextNode = yearNode;
	}
	index.yearNode = yearNode;
	index.monthNode = 0; //新建的年结点，还没有月结点
	return  m_pHandler->AppendNode(lastDt, newDt, index, pFeature, true);;
}


 LeafNode* YearHandler::LocalTimeNode(TreeNode *ListHeadNode,  DateTimeStruct datatime,  bool existParentFlag)
 {
	
	if (ListHeadNode==0)
	{
		return 0;
	}

	for (; ListHeadNode!=0 ; ListHeadNode = ListHeadNode->nextNode)
	{
		if (datatime.year <= ListHeadNode->value)
		{
			//第三个参数，判断是否缺失。
			return m_pHandler->LocalTimeNode((TreeNode*)ListHeadNode->childNode, datatime, datatime.year == ListHeadNode->value);
		}
	}
	
	return 0;
 }

/************************************************************************/
/*                   月                                                 */
/************************************************************************/
int MonthHandler::AppendNode(DateTimeStruct &lastDt, DateTimeStruct &newDt, IndexStruct &index, FeatureData *pFeature, bool handleFlag) 
{
	if (handleFlag == false && lastDt.month == newDt.month )   
	{
		return m_pHandler->AppendNode(lastDt, newDt, index, pFeature, false);
	}

	//年份变了。月，天，时，都要新建。

	TreeNode *monthNode = new TreeNode();
	monthNode->value = newDt.month;

	if(index.monthNode == 0)
	{
		index.yearNode->childNode = monthNode; //第一个月结点
	}else
	{
		index.monthNode->nextNode = monthNode;
	}
	index.monthNode = monthNode;
	index.dayNode = 0;

	return m_pHandler->AppendNode(lastDt, newDt, index, pFeature, true);

}
LeafNode* MonthHandler::LocalTimeNode(TreeNode *ListHeadNode,  DateTimeStruct datatime,  bool existParentFlag)
{
	if (ListHeadNode==0)
	{
		return 0;
	}

	if (!existParentFlag)
	{
		return m_pHandler->LocalTimeNode((TreeNode*)ListHeadNode->childNode, datatime, false);
	}

	TreeNode * lastNode = ListHeadNode;
	for (; ListHeadNode!=0 ; ListHeadNode = ListHeadNode->nextNode)
	{
		if ( datatime.month <= ListHeadNode->value)
		{
			return m_pHandler->LocalTimeNode((TreeNode*)ListHeadNode->childNode, datatime, datatime.month == ListHeadNode->value);
		}
		lastNode = ListHeadNode;
	}

	return m_pHandler->LocalTimeNode((TreeNode*)lastNode->childNode, datatime, false);
}

/************************************************************************/
/*                   日                                                 */
/************************************************************************/
int DayHandler::AppendNode(DateTimeStruct &lastDt, DateTimeStruct &newDt,  
	IndexStruct &index, FeatureData *pFeature, bool handleFlag) 
{
	if (handleFlag == false && lastDt.day == newDt.day)
	{
		return m_pHandler->AppendNode(lastDt, newDt, index, pFeature, false);
	}

	TreeNode *dayNode = new TreeNode();
	dayNode->value = newDt.day;

	if (index.dayNode == 0 )
	{
		index.monthNode->childNode = dayNode; //
	}else
	{
		index.dayNode->nextNode = dayNode;
	}

	index.dayNode = dayNode;
	index.hourNode = 0 ;

	return m_pHandler->AppendNode(lastDt, newDt, index, pFeature, true);
}

LeafNode* DayHandler::LocalTimeNode(TreeNode *ListHeadNode,  DateTimeStruct datatime,  bool existParentFlag)
{
	if (ListHeadNode==0)
	{
		return 0;
	}

	if (!existParentFlag)
	{
		return m_pHandler->LocalTimeNode((TreeNode*)ListHeadNode->childNode, datatime, false);
	}

	TreeNode * lastNode = ListHeadNode;

	for (; ListHeadNode!=0 ; ListHeadNode = ListHeadNode->nextNode)
	{
		if (datatime.day <=ListHeadNode->value)
		{
			return m_pHandler->LocalTimeNode((TreeNode*)ListHeadNode->childNode, datatime, datatime.day == ListHeadNode->value);
		}
	}

	return m_pHandler->LocalTimeNode((TreeNode*)lastNode->childNode, datatime, false);
}
/************************************************************************/
/*                   时                                                 */
/************************************************************************/
int HourHandler::AppendNode(DateTimeStruct &lastDt, DateTimeStruct &newDt,  
	IndexStruct &index, FeatureData *pFeature, bool handleFlag) 
{
	if (handleFlag == false && lastDt.hour == newDt.hour)
	{
		return m_pHandler->AppendNode(lastDt, newDt, index, pFeature, false);
	}

	TreeNode *hourNode = new TreeNode();
	hourNode->value = newDt.hour;

	if (index.hourNode == 0 )
	{
		index.dayNode->childNode = hourNode; //
	}else
	{
		index.hourNode->nextNode = hourNode;
	}

	index.hourNode = hourNode;
	index.listEndNode = 0;

	return m_pHandler->AppendNode(lastDt, newDt, index, pFeature, true);
}

LeafNode* HourHandler::LocalTimeNode(TreeNode *ListHeadNode,  DateTimeStruct datatime,  bool existParentFlag)
{
	if (ListHeadNode==0)
	{
		return 0;
	}

	if (!existParentFlag)
	{
		return m_pHandler->LocalTimeNode((TreeNode*)ListHeadNode->childNode, datatime, false);
	}

	TreeNode * lastNode = ListHeadNode;

	for (; ListHeadNode!=0 ; ListHeadNode = ListHeadNode->nextNode)
	{
		if (datatime.hour <= ListHeadNode->value)
		{
			return m_pHandler->LocalTimeNode((TreeNode*)ListHeadNode->childNode, datatime, datatime.hour == ListHeadNode->value);
		}
	}
	return m_pHandler->LocalTimeNode((TreeNode*)lastNode->childNode, datatime, false);;
}
/************************************************************************/
/*                   数据                                               */
/************************************************************************/
int DataHandler::AppendNode(DateTimeStruct &lastDt, DateTimeStruct &newDt,  
	IndexStruct &index, FeatureData *pFeature, bool handleFlag) 
{
	LeafNode *leafNode = new LeafNode();
	leafNode->featureData = pFeature;
	
	//	同一时间（秒精度）的数据
	if (handleFlag == false && (lastDt.collectTime / 1000) == (newDt.collectTime / 1000))
	{
		index.listEndNode->childNode = leafNode;
        leafNode->ParentNode = index.listEndNode;
        index.listEndNode = leafNode;
	// 同一小时的数据 新增
	}else	if (handleFlag == false && index.dataNode != 0)
	{
		index.dataNode->nextNode = 	leafNode;
        leafNode->previousNode = index.dataNode;
		index.dataNode = leafNode;
		index.listEndNode = leafNode;
	// 不同小时的数据
	}else if (handleFlag == true && index.dataNode != 0)
	{
		index.hourNode->childNode = leafNode; //与父结点连接
		index.dataNode->nextNode = 	leafNode; //所有数据结点连接
        leafNode->previousNode = index.dataNode;
		index.dataNode = leafNode;  
		index.listEndNode = leafNode;
	}
	
	lastDt = newDt;

    index.yearNode->nCount++;
    index.monthNode->nCount++;
    index.dayNode->nCount++;
    index.hourNode->nCount++;

	return 0;
}


LeafNode* DataHandler::LocalTimeNode(TreeNode *ListHeadNode,  DateTimeStruct datatime,  bool existParentFlag)
{
	if (ListHeadNode == 0)
	{
		return 0;
	}
#ifdef _DEBUG
    //int64_t time = ( (LeafNode *)ListHeadNode)->featureData->CollectTime;
	//// 把分和秒置为0,转为该小时下的第1秒.  
	//struct tm  *t = localtime(&time);
	//t->tm_min = 0;		
	//t->tm_sec = 0; 
	//time = mktime(t); 
#endif

	for (LeafNode *leafNode = (LeafNode *)ListHeadNode; leafNode != 0 ; leafNode = leafNode->nextNode)
	{
        for (LeafNode * cNode = leafNode; cNode != NULL; cNode = cNode->childNode)
        {
            if (datatime.collectTime <= cNode->featureData->CollectTime)
            {
                return cNode;
            }
        }
		
#ifdef DEBUG
		#define HOUR 3600
		BOOST_ASSERT(leafNode->featureData->CollectTime - time < HOUR  && "与设计背离，最坏遍历不超过1小时" );
#endif
	}
	return 0;
}