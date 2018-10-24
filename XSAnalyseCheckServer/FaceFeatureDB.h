#ifndef FaceFeature_h

#include "FaceFeatureStruct.h"
#include "NodeHandler.h"

class FaceFeatureDB;



class DataIterator
{
public:
	DataIterator(FaceFeatureDB *pFacefeatureDb);
public:

	/* 设置时间范围
	
		begintime : 开始时间
		endtime   : 结束时间
		\返回值：0=成功;
						-1：错误的时间段(开始时间大于结束时间)
						-2: 没有数据。 
						-3: 时间范围内没有数据;			   
		！注：如果没有调用，则全局顺序获取。
		      在调用了该函数后，需要调用Frist()重新遍历。不同DataIterator对象之间不受影响
			  参数begintime和endtime同时为0时，清除时间范围。
	*/
	int SetTimeRange(int64_t begintime , int64_t endtime );

    /*
        获取指定时间段特征值结点总数, 在调用Frist之后调用 
    */
    int GetNodeCount();

	/*	获取第一条数据
		如果设置了时间范围，返回时间段第一条数据
		返回NULL，无数据
	*/
	FeatureData *First();

    /*	获取最后一条数据
    如果设置了时间范围，返回时间段最后一条数据
    返回NULL，无数据
    */
    FeatureData *Last();

	/* 获取下一条数据

		设置了（SetTimeRange）时间范围，时间结束返回空。
		没有设置时间范围时，调用Frist（）函数时，程序自动设置了结束时间为当前最后一条数据的时间。新增的数据不在next范围内.
	*/
	FeatureData *Next();
    /* 获取前一条数据

    设置了（SetTimeRange）时间范围，时间结束返回空。
    没有设置时间范围时，调用Frist（）函数时，程序自动设置了结束时间为当前最后一条数据的时间。新增的数据不在next范围内.
    */
    FeatureData *Previous();

	
private:
	FaceFeatureDB *m_pFacefeatureDb;
	LeafNode *m_pCurrentNode;  //   当前结点
	LeafNode *m_pSubHeadNode;  //	子结点队头

	DateTimeStruct m_begintime; 
	DateTimeStruct m_endtime;

	bool m_IsSetTimeRange; //	是否设置了时间范围标志。 没有设置按顺序遍历。
			
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

	/* !增加特征数据
	   \参数：
		 \pFeature结构指针，外部申请，由FaceFeatureDB对象管理。
	     \year,month,day,hour 人脸识别时间。
	   \返回值：
	      0:成功，失败为负值。
	*/
	int AppendFaceFeatureData(FeatureData *pFeature);

	/*!获取记录总数

	   \返回值：
	      记录总数。默认为0。
	*/
	unsigned int GetRecordNum(){return m_RecoredNum;};

    //获取当天记录数
    unsigned int GetDayNum()
    {
        
        return m_indexStruct.dayNode == 0 ? 0 : m_indexStruct.dayNode->nCount;
    }

	/* !创建迭代器

		由调用者释放,用来遍历数据的对象.
	*/
	DataIterator *CreateIterator();

	/* !获取数据的第一条时间和最后一条时间
		
		\返回值，ture = 成功; false = 没有数据
	*/
	bool   GetDataDate(int64_t &begintime, int64_t &endtime);

private:

	IndexStruct    m_indexStruct;
	DateTimeStruct m_lastDateTime;//最后一条记录的时间信息
	DateTimeStruct m_newDateTime; //插入的记录的时间信息
	DBHandler *m_pHandler;  //
	unsigned int m_RecoredNum; // 数据记录总个数


private:
	// 记录计数
	void CountRecond(){m_RecoredNum++;};
	
};

#define FaceFeature_h
#endif