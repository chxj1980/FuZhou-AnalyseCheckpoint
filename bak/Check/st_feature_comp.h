/*********************************************************************
 * Copyright (C), 2016, Sensetime Tech. Co., Ltd.
 * file name  : st_feature_comp.h
 * version    : v1.0
 * create date: 2016-09-27
 * creator    : ruanjiabin@sensetime.com
 * description: for partners
 * modify log : ruanjiabin@sensetime 2017-03-23 add version
 ********************************************************************/
#ifndef SRC_ST_FEATURE_COMP_H_
#define SRC_ST_FEATURE_COMP_H_

#define SDK_VERSION "1.0.4"

#ifdef __cplusplus
extern "C" {
#endif

enum EStRetCode {
    RET_CODE_OK = 0, // OK
    RET_CODE_UNSUPPORTED_MODEL_VERSION = 1, //unsupported model version
    RET_CODE_FEATURE_LENGTH_ERROR = 2, //data length error (maybe incorrect data)
};

/**
 * Compare two features (for model 25301, 23904, and more). Performance: < 0.5s/10million
 * @param [in] f1 binary feature(with 12bytes head)
 * @param [in] f2 binary feature(with 12bytes head)
 * @param [in] len feature size including the 12bytes head
 * @param [out] sim similarity
 * @param [in] verbose 0-don't print anything, 1-print verbose infomation when error occur
 * @return 0-OK, else parameter error. see enum EStRetCode
 */
int st_feature_comp(const void *f1, const void *f2, int len, float *sim, int verbose);

const char *get_version();

#ifdef __cplusplus
}
#endif

#endif /* SRC_ST_FEATURE_COMP_H_ */
