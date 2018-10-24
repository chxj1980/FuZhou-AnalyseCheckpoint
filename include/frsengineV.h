#pragma once

#ifdef FRS_EXPORTS
#define FRS_API __declspec(dllexport)
#else
#define FRS_API __declspec(dllimport)
#endif // FRS_EXPORTS

#include <string>

namespace NeoFaceV
{
	class FRS_API CFaceInfo
	{
	public:
		CFaceInfo(void);
		~CFaceInfo(void);
		int AddFace(POINT &, POINT &);
		// bmp: 位图头部信息; dat:位图数据
		int FindFace(BITMAPINFO const &bmp, void const *dat);
		int FindFace(unsigned char const *dat, int width, int height);
		RECT GetFace(void);
		int GetFaceIndex(void);
		// 获取图片中的人脸数量
		int GetFaceMax(void);
		float GetFaceQualityScore(void);
		float GetFaceScore(void);
		RECT GetHead(void);
		int GetHeight(void);
		POINT GetLeftEye(void);
		POINT GetRightEye(void);
		int GetWidth(void);
		void Init(void);
		CFaceInfo & operator=(CFaceInfo const &);
		//选择图片中的人脸,index为序号，从0开始，如果国片中有多张人脸，必须设置
		int SetFaceIndex(int index);
		int SetImage(BITMAPINFO const &bmp, void const *dat);
		int SetImage(unsigned char const *, int, int);
		int SetLeftEye(POINT const &);
		int SetRightEye(POINT const &);
		// 设置算法模式，可选1或6，只是对图片进人脸检测可用模式6，其它都用模式1，
		int SetParamAlgorithm(int value);
		// 设置最大两眼间像素
		int SetParamEyesMaxWidth(int value);
		// 设置最小两眼间像素
		int SetParamEyesMinWidth(int value);
		int SetParamLog(std::string, std::string);
		// 设置最多检测人脸数量
		int SetParamMaxFace(int value);
		// 设置人脸可信度
		int SetParamReliability(float value);
	};
	class FRS_API CFaceAttribute
	{
	public:
		CFaceAttribute(void);
		~CFaceAttribute(void);
		float GetFacePan(CFaceInfo *info);
		float GetFaceRoll(CFaceInfo *info);
		float GetFaceTilt(CFaceInfo *info);
		// 人脸正面质量，综合了人脸角度的质量评判，比CFaceInfo中的质量要可靠些
		float GetFrontalFaceScore(CFaceInfo *info);
		CFaceAttribute& operator == (CFaceAttribute const &);
		int SetParamLog(std::string, std::string);
	};
	class FRS_API CFaceFeature
	{
	public:
		CFaceFeature(void);
		~CFaceFeature(void);
		// 计算人脸特征信息
		int CalcFeature(CFaceInfo *info);
		void FreeAlignment(float *);
		// 释放人脸特征值内存
		void FreeSerializeData(unsigned char *feature);
		int GetAlignment(float **feature, int *length, CFaceInfo *info);
		void Init(void);
		CFaceFeature& operator =(CFaceFeature const &);
		// 在计算人脸特征信息（CalcFeature）后，将人脸特征信息序列化为特征值
		int Serialize(unsigned char **feature, long *length, CFaceInfo *info);
		int SetParamLog(std::string, std::string);

	};
	class FRS_API CVerify
	{
	public:
		CVerify(void);
		~CVerify(void);
		CVerify & operator=(CVerify const &);
		int SetParamLog(std::string, std::string);
		int Verify(unsigned char *feature1, unsigned char *feature2, float *score);
	};
	FRS_API char const * * GetVersion(void);
	// mode必须为29
	FRS_API int Initialize(int mode);
	// mode必须为29
	FRS_API int Terminate(int mode);
}
