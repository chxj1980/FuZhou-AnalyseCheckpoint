#ifndef FeatureManage_h

#include "DataDefine.h"
#include <time.h>
#include <stdio.h>

class CFeatureManage
{
public:
    CFeatureManage(void);
    ~CFeatureManage(void);
public:
    int Init(char * pDeviceID, int nMaxCount, LPSINGLEFEATURENODE pSingleFeatureNode);
    bool UnInit();
    //增加特征值, bInit: 是否在初始化时增加特征值数据, 是则不需要再反向copy解码后特征值到pFeature
    //重点库增加特征值
    int AddFeature(LPKEYLIBFEATUREDATA pFeatureData, bool bInit = false);
    //删除特征值
    int DelFeature(const char * pFaceUUID);
    //清空重点库
    int ClearKeyLib();
    unsigned int GetTotalNum();
    
private:
    LISTKEYLIBINFO m_listKeyLibInfo;  //特征值库, 每2.5w(由配置文件指定)条数据即分段处理
    LPSINGLEFEATURENODE m_pSingleFeatureNode;
    int m_nFeatureIndex;            //重点库内部分片索引
    char m_pLibName[MAXLEN];
    int m_nMaxCount;
};

#define FeatureManage_h
#endif