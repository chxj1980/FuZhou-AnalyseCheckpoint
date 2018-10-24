#pragma once

#ifdef WIN32
#ifdef XSIGHT_DLL_EXPORTS
#define XS_ENCRYPT_Dll __declspec(dllexport)
#else
#define XS_ENCRYPT_Dll __declspec(dllimport)
#endif
#endif


extern "C"
{
	//提供一组默认公用的秘钥进行加解密

	//pOutPublicKey:输出公钥
	//pOutPrivateKey:输出私钥
	//bDefault:是否是一组默认公用的秘钥
	//Key长度不大于2*1024
	XS_ENCRYPT_Dll bool XSGetKey(char *pOutPublicKey,char *pOutPrivateKey,bool bDefault);

	//加密
	//pInSource:明文
	//pOutSource:输出加密后的串
	//pPublicKey:秘钥,传入空时为默认秘钥
	XS_ENCRYPT_Dll bool XSEncrypt(const char *pInSource, char *pOutSource, const char *pPublicKey);

	//解密
	//pInSource:加密的串
	//pOutSource:输出解密后的串
	//pPrivateKey:秘钥,传入空时为默认秘钥
	XS_ENCRYPT_Dll bool XSDecrypt(const char *pInSource, char *pOutSource, const char *pPrivateKey);
}
