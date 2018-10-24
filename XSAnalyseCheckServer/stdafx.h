// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//
#ifndef stdafx_h



#ifdef __WINDOWS__

#define WIN32_LEAN_AND_MEAN		// 从 Windows 头中排除极少使用的资料
#include <stdio.h>
#include <tchar.h>
#include <Windows.h>

#pragma warning(disable:4996)
#pragma warning(disable:4099)
#pragma warning(disable:4244)
#pragma warning(disable:4003)
#pragma warning(disable:4267)
#pragma warning(disable:4018)
#pragma warning(disable:4267)

#include "AnalyseServer.h"
#include "FeatureManage.h"
#include "DataDefine.h"
#include "FaceFeatureDB.h"
#include "FaceFeatureStruct.h"
#include "NodeHandler.h"
#include "ConfigRead.h"
#include "ZBase64.h"
#include "LogRecorder.h"

#pragma comment( lib, "libzmq-v100-mt-gd-4_0_4.lib" )
#endif




#define stdafx_h
#endif
