#ifndef FaceFeature_h

#include "FaceFeatureStruct.h"
#include "NodeHandler.h"

class FaceFeatureDB;



class DataIterator
{
public:
	DataIterator(FaceFeatureDB *pFacefeatureDb);
public:

	/* ����ʱ�䷶Χ
	
		begintime : ��ʼʱ��
		endtime   : ����ʱ��
		\����ֵ��0=�ɹ�;
						-1�������ʱ���(��ʼʱ����ڽ���ʱ��)
						-2: û�����ݡ� 
						-3: ʱ�䷶Χ��û������;			   
		��ע�����û�е��ã���ȫ��˳���ȡ��
		      �ڵ����˸ú�������Ҫ����Frist()���±�������ͬDataIterator����֮�䲻��Ӱ��
			  ����begintime��endtimeͬʱΪ0ʱ�����ʱ�䷶Χ��
	*/
	int SetTimeRange(int64_t begintime , int64_t endtime );

    /*
        ��ȡָ��ʱ�������ֵ�������, �ڵ���Frist֮����� 
    */
    int GetNodeCount();

	/*	��ȡ��һ������
		���������ʱ�䷶Χ������ʱ��ε�һ������
		����NULL��������
	*/
	FeatureData *First();

    /*	��ȡ���һ������
    ���������ʱ�䷶Χ������ʱ������һ������
    ����NULL��������
    */
    FeatureData *Last();

	/* ��ȡ��һ������

		�����ˣ�SetTimeRange��ʱ�䷶Χ��ʱ��������ؿա�
		û������ʱ�䷶Χʱ������Frist��������ʱ�������Զ������˽���ʱ��Ϊ��ǰ���һ�����ݵ�ʱ�䡣���������ݲ���next��Χ��.
	*/
	FeatureData *Next();
    /* ��ȡǰһ������

    �����ˣ�SetTimeRange��ʱ�䷶Χ��ʱ��������ؿա�
    û������ʱ�䷶Χʱ������Frist��������ʱ�������Զ������˽���ʱ��Ϊ��ǰ���һ�����ݵ�ʱ�䡣���������ݲ���next��Χ��.
    */
    FeatureData *Previous();

	
private:
	FaceFeatureDB *m_pFacefeatureDb;
	LeafNode *m_pCurrentNode;  //   ��ǰ���
	LeafNode *m_pSubHeadNode;  //	�ӽ���ͷ

	DateTimeStruct m_begintime; 
	DateTimeStruct m_endtime;

	bool m_IsSetTimeRange; //	�Ƿ�������ʱ�䷶Χ��־�� û�����ð�˳�������
			
	LeafNode *GetFristDataNode();
	TreeNode *GetFristYearNode();
	void ParseTime(DateTimeStruct &ts, int64_t time);
};


 class FaceFeatureDB
{
public:
	friend class DataIterator;
public:
	FaceFeatureDB(void);
	~FaceFeatureDB(void);
public:

	void InitDB();

	/* !������������
	   \������
		 \pFeature�ṹָ�룬�ⲿ���룬��FaceFeatureDB�������
	     \year,month,day,hour ����ʶ��ʱ�䡣
	   \����ֵ��
	      0:�ɹ���ʧ��Ϊ��ֵ��
	*/
	int AppendFaceFeatureData(FeatureData *pFeature);

	/*!��ȡ��¼����

	   \����ֵ��
	      ��¼������Ĭ��Ϊ0��
	*/
	unsigned int GetRecordNum(){return m_RecoredNum;};

    //��ȡ�����¼��
    unsigned int GetDayNum()
    {
        
        return m_indexStruct.dayNode == 0 ? 0 : m_indexStruct.dayNode->nCount;
    }

	/* !����������

		�ɵ������ͷ�,�����������ݵĶ���.
	*/
	DataIterator *CreateIterator();

	/* !��ȡ���ݵĵ�һ��ʱ������һ��ʱ��
		
		\����ֵ��ture = �ɹ�; false = û������
	*/
	bool   GetDataDate(int64_t &begintime, int64_t &endtime);

private:

	IndexStruct    m_indexStruct;
	DateTimeStruct m_lastDateTime;//���һ����¼��ʱ����Ϣ
	DateTimeStruct m_newDateTime; //����ļ�¼��ʱ����Ϣ
	DBHandler *m_pHandler;  //
	unsigned int m_RecoredNum; // ���ݼ�¼�ܸ���


private:
	// ��¼����
	void CountRecond(){m_RecoredNum++;};
	
};

#define FaceFeature_h
#endif