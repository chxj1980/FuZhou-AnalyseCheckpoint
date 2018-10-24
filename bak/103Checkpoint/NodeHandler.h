#ifndef NodeHandle_h
#include "FaceFeatureStruct.h"

class DBHandler
{
public :
	//	添加数据
	virtual int AppendNode(DateTimeStruct &lastDt, DateTimeStruct &newDt, IndexStruct &index,
		FeatureData *pFeature, bool handleFlag) = 0 ;

	//	定位>=datatimeFind的第一时间结点 ，如果年，月、天、时、如果有一个不存在，则后续取第一个结点。如2015的数据没有。则取16年第一个数据。
	//	ListHeadNode : 列表头结点
	//	datatimeFind : 定位的时间
	//  existParentFlag : 上级的结点指定的Value是否存在
	virtual LeafNode* LocalTimeNode(TreeNode *ListHeadNode, DateTimeStruct datatimeFind, bool existParentFlag) = 0;
public :
	DBHandler *SetSuccessor(DBHandler *pHandler) {m_pHandler = pHandler; return pHandler;}
	DBHandler *GetSuccessor(){return m_pHandler;}
protected:
	DBHandler *m_pHandler;
};


class YearHandler : public  DBHandler
{
public :
	virtual int AppendNode(DateTimeStruct &lastDt, DateTimeStruct &newDt,  
		IndexStruct &index, FeatureData *pFeature, bool handleFlag) ;

	virtual LeafNode* LocalTimeNode(TreeNode *parentNode, DateTimeStruct datatime, bool existParentFlag);
};

class MonthHandler : public DBHandler
{
public :
	virtual int AppendNode(DateTimeStruct &lastDt, DateTimeStruct &newDt,  
		IndexStruct &index, FeatureData *pFeature, bool handleFlag) ;

	virtual LeafNode* LocalTimeNode(TreeNode *parentNode, DateTimeStruct datatime, bool existParentFlag);
};

class DayHandler : public DBHandler
{
public :
	virtual int AppendNode(DateTimeStruct &lastDt, DateTimeStruct &newDt,  
		IndexStruct &index, FeatureData *pFeature, bool handleFlag) ;

	virtual LeafNode* LocalTimeNode(TreeNode *parentNode, DateTimeStruct datatime, bool existParentFlag);
};


class HourHandler : public DBHandler
{
public :
	virtual int AppendNode(DateTimeStruct &lastDt, DateTimeStruct &newDt,  
		IndexStruct &index, FeatureData *pFeature, bool handleFlag) ;
	
	virtual LeafNode* LocalTimeNode(TreeNode *parentNode, DateTimeStruct datatime,  bool existParentFlag);

};


class DataHandler : public DBHandler
{
public :
	virtual int AppendNode(DateTimeStruct &lastDt, DateTimeStruct &newDt,  
		IndexStruct &index, FeatureData *pFeature, bool handleFlag) ;

	virtual LeafNode* LocalTimeNode(TreeNode *parentNode, DateTimeStruct datatime,  bool existParentFlag);
};

#define NodeHandle_h
#endif