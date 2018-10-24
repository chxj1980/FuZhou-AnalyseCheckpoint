#ifndef NodeHandle_h
#include "FaceFeatureStruct.h"

class DBHandler
{
public :
	//	�������
	virtual int AppendNode(DateTimeStruct &lastDt, DateTimeStruct &newDt, IndexStruct &index,
		FeatureData *pFeature, bool handleFlag) = 0 ;

	//	��λ>=datatimeFind�ĵ�һʱ���� ������꣬�¡��졢ʱ�������һ�������ڣ������ȡ��һ����㡣��2015������û�С���ȡ16���һ�����ݡ�
	//	ListHeadNode : �б�ͷ���
	//	datatimeFind : ��λ��ʱ��
	//  existParentFlag : �ϼ��Ľ��ָ����Value�Ƿ����
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