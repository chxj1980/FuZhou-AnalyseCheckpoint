#ifndef FaceFeatureStruct_h
#include <stdint.h>
/*
	****数据内存结构图 详参见《内存结构图.vsdx》

	****IndexStruct结构指针 详见《IndexStruct指针说明.vsdx》

	by. lc 2017-01-05
*/
typedef struct _FeatureData
{
    char *FaceUUID;					//	图片FaceUUID
    unsigned char *FeatureValue;	//	特征值
	unsigned short FeatureValueLen; //	特征值数据长度
	int64_t CollectTime ;			//	采集时间
    char pDisk[2];                  //  保存盘符
    char * pImageIP;                //  采集服务地址(图片保存服务器IP)
    char * pFaceRect;               //  人脸图片坐标位置
    ~_FeatureData()
    {
        delete FaceUUID;
        delete FeatureValue;
        delete pImageIP;
        delete pFaceRect;
    }
}FeatureData;


typedef struct _TreeNode
{
	unsigned short value;
    int nCount;             //特征值结点数量
	_TreeNode *nextNode;    // 兄弟	结点
	void *childNode;        // 孩子结点 ,此处用void*的目的是，因为时间结点下面是 LeafNode结点。

	_TreeNode()
	{
        nCount = 0;
		nextNode = 0 ;
		childNode = 0 ;
		value = 0 ;
	}
}TreeNode;

typedef struct _LeafNode
{
	FeatureData *featureData;
    _LeafNode *nextNode;     //	下一个叶结点
    _LeafNode *previousNode; //	上一个叶结点
	_LeafNode *childNode;	 // 子结点（同一时间的人脸）
    _LeafNode * ParentNode;

	_LeafNode()
	{
		featureData = 0 ;
		nextNode = 0;
        previousNode = 0;
		childNode = 0;
        ParentNode = 0;
	}
}LeafNode;


/*!当前结构的时间信息 
	
		year,month,day,houre 用于插入数据时，判断是否需要增加新的treenode.
		LastCollectTime 用于插入数据时，判断与上次的datanode是否为同一时间。时间相同时，
						插入到同一时间的数据队列中；时间不相同时重新建立数据队列。
*/
typedef struct _DateTimeStruct
{
	unsigned short year; 
	unsigned char month; // 0-11
	unsigned char day;	 // 1-31
	unsigned char hour;  // 0-23
    int64_t collectTime;   //采集时间，用于判断数据结点的插入位置。

	_DateTimeStruct(){
		year = 0 ;
		month = 0;
		day = 1;
		hour = 0;
		collectTime = 0;
	}
}DateTimeStruct;



typedef struct _IndexStruct
{
	TreeNode *HeadNode;  // 头指针 

	/*当前记录结点记录的指针信息。 */
	TreeNode* yearNode;
	TreeNode* monthNode;
	TreeNode* dayNode;
	TreeNode* hourNode;

	LeafNode* dataNode;	   //数据的结点的队列头。同一时间的人脸在一个队列中。
	LeafNode* listEndNode; //当前数据结点中的最后一个结点。

	LeafNode *dataHeadNode; // 数据头结点。

	_IndexStruct()
	{
		
		yearNode = 0;
		monthNode = 0 ;
		dayNode = 0;
		hourNode = 0 ;

		dataNode = 0 ;
		listEndNode = 0;
		HeadNode = 0;
		dataHeadNode = 0;
	}
}IndexStruct;

#define FaceFeatureStruct_h
#endif