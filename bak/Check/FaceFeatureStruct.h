#ifndef FaceFeatureStruct_h
#include <stdint.h>
/*
	****�����ڴ�ṹͼ ��μ����ڴ�ṹͼ.vsdx��

	****IndexStruct�ṹָ�� �����IndexStructָ��˵��.vsdx��

	by. lc 2017-01-05
*/
typedef struct _FeatureData
{
    char *FaceUUID;					//	ͼƬFaceUUID
    unsigned char *FeatureValue;	//	����ֵ
	unsigned short FeatureValueLen; //	����ֵ���ݳ���
	int64_t CollectTime ;			//	�ɼ�ʱ��
    char pDisk[2];                  //  �����̷�
    char * pImageIP;                //  �ɼ������ַ(ͼƬ���������IP)
    char * pFaceRect;               //  ����ͼƬ����λ��
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
    int nCount;             //����ֵ�������
	_TreeNode *nextNode;    // �ֵ�	���
	void *childNode;        // ���ӽ�� ,�˴���void*��Ŀ���ǣ���Ϊʱ���������� LeafNode��㡣

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
    _LeafNode *nextNode;     //	��һ��Ҷ���
    _LeafNode *previousNode; //	��һ��Ҷ���
	_LeafNode *childNode;	 // �ӽ�㣨ͬһʱ���������
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


/*!��ǰ�ṹ��ʱ����Ϣ 
	
		year,month,day,houre ���ڲ�������ʱ���ж��Ƿ���Ҫ�����µ�treenode.
		LastCollectTime ���ڲ�������ʱ���ж����ϴε�datanode�Ƿ�Ϊͬһʱ�䡣ʱ����ͬʱ��
						���뵽ͬһʱ������ݶ����У�ʱ�䲻��ͬʱ���½������ݶ��С�
*/
typedef struct _DateTimeStruct
{
	unsigned short year; 
	unsigned char month; // 0-11
	unsigned char day;	 // 1-31
	unsigned char hour;  // 0-23
    int64_t collectTime;   //�ɼ�ʱ�䣬�����ж����ݽ��Ĳ���λ�á�

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
	TreeNode *HeadNode;  // ͷָ�� 

	/*��ǰ��¼����¼��ָ����Ϣ�� */
	TreeNode* yearNode;
	TreeNode* monthNode;
	TreeNode* dayNode;
	TreeNode* hourNode;

	LeafNode* dataNode;	   //���ݵĽ��Ķ���ͷ��ͬһʱ���������һ�������С�
	LeafNode* listEndNode; //��ǰ���ݽ���е����һ����㡣

	LeafNode *dataHeadNode; // ����ͷ��㡣

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