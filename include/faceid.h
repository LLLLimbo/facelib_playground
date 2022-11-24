#ifndef __FACEID__
#define __FACEID__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define IMAGE_INFO_ERR_CODE_SUCCESS 0
#define IMAGE_INFO_ERR_CODE_IMG_DECODE 1
#define IMAGE_INFO_ERR_CODE_SMALL_FACE 1
#define IMAGE_INFO_ERR_CODE_NO_FACE 2
#define IMAGE_INFO_ERR_CODE_MULTI_FACE 3

typedef struct image_quality_s {
	int code; /* 错误码 */
	float pose[3]; /* 人脸角度 yaw pitch roll */
	float quality; /* 人脸质量 */
} image_quality_t;

typedef struct image_info_s {
	int err_code; /* 错误码 */
	char feature[1024]; /* 人脸特征值：前512非口罩，后512口罩 */
	float normal[2];	/* 人脸特征值模值：[0]非口罩，[1]512口罩 */
	float norm_feature[1024]; /* 归一化好的人脸特征值 */
} image_info_t;

/*
* @fn init_model
* @brief 初始化算法模型
* @param[in] chip_version chip 版本：["80x", "81x", "82x" ...]
*            model_version 算法模型版本：["v1", "v2", "v3" ...]
*            use_mask_recognizer 是否使用口罩识别模型：[ture false]
* @return    void
*/
void init_model(const char *chip_version, const char *model_version, const bool& use_mask_recognizer);

/*
* @fn get_quality_from
* @brief 获取人脸图片质量
* @param[in] filename 人脸图片路径
*
* @return    image_quality_t
*/
image_quality_t get_quality_from(char *filename);

/*
* @fn get_quality_from
* @brief 获取人脸特征值
* @param[in] filename 人脸图片路径
*            use_recognizer 提取非口罩特征值: [true false]
*            use_mask_recognizer 提取口罩特征值: [true false]
* @return    image_info_t
*/
image_info_t get_feature_from(char *filename, bool use_recognizer, bool use_mask_recognizer);
image_info_t get_feature_from_url(char *url, bool use_recognizer, bool use_mask_recognizer);

/*
* @fn feature_compare
* @brief 人脸特征值比对（1：N）
* @param[in] reg_total 底库中人脸特征值个数
*            reg_feat 底库中人脸特征值指针
*            feat 待比较的人脸特征值指针
*            mask 待比较的人脸特征值是否是口罩特征值
* @return    float 比对分数
*/
float feature_compare(int reg_total, char *reg_feat, char *feat, int mask);

/*******以下的接口是对上述的接口，根据测试的要求再次封装，客户根据自身的业务自行实现 start*********/
#define FEATURE_MAX_DB_INDEX (100) /* 最大特征底库index号 0 - 99 */

/*
* @fn feature_db_register
* @brief 用于注册比较特征库，这个接口传的特征值，是归一化好的人脸特征值 (norm_feature)
* @param[in] db_index 注册底库id,
*            reg_total 注册底库个数
*            reg_feat 注册底库特征值数据
* @return    0: 成功 -1：失败
* @note      使用归一化好的人脸特征值，是注册时调用的时间会相对减少，自测机器 20万600ms
*/
float feature_db_compare(int db_index, float *feat);
int feature_db_register(int db_index, int reg_total, float *reg_feat);

/*
* @fn feature_db_register_and_norm
* @brief 用于注册比较特征库，这个接口传的特征值，是小机端人脸特征值 (feature)
* @param[in] db_index 注册底库id,
*            reg_total 注册底库个数
*            reg_feat 注册底库特征值数据
* @return    0: 成功 -1：失败
* @note      使用小机端人脸特征值，是注册时调用的时间会相对增加，自测机器 20万 2000ms，
*            因为会对小机端人脸特征值归一化
*/
int feature_db_register_and_norm(int db_index, int reg_total, char *reg_feat);
float feature_db_compare_and_norm(int db_index, char *feat);

/*
* feature_db_register 对比 feature_db_register_and_norm
* feature_db_register           优点：注册底库，速度比较快，缺点：数据库中需要保存两份人脸特征值，一个是小机端的，一个是pc端归一化后的(norm_feature)
* feature_db_register_and_norm  优点：只需要保存一份小机端的特性值， 缺点：注册底库较慢
*/

image_quality_t get_quality_from_url(char *url);

const char* string_to_hex(const char *str, char *hex, size_t maxlen);

typedef struct reg_feat_s {
	char feature[1024];
} reg_feat_t;

typedef struct reg_normfeat_s {
	float norm_feature[1024];
} reg_normfeat_t;

typedef struct reg_name_s {
	char name[64];
} reg_name_t;

typedef struct reg_data_s {
	reg_feat_t *features;           /* 对应 feature_db_register_and_norm_st 接口， 未归一化的特征值 */
	reg_normfeat_t *norm_features;  /* 对应 feature_db_register_st 接口，特征值已进行归一化处理了 */

	reg_name_t *names; /* reg user name */
} reg_data_t;

typedef struct reg_idx_s {
	int idx;
} reg_idx_t;

typedef struct reg_score_s {
	float score;
} reg_score_t;

typedef struct cmp_result_s {
	int idx;
	float score;
	char name[64];

	/* 配置thr > 0.0f, 返回大于阀值的人脸信息 */
	float thr;
	int thr_cnt;
	reg_idx_t *thr_idxs;
	reg_score_t *thr_scores;
	reg_name_t *thr_names;
} cmp_result_t;

/*
* 带有st后缀的接口支持将人名信息注册到底库
*/
int feature_db_register_st(int db_index, int reg_total, reg_data_t *reg_info);
int feature_db_compare_st(int db_index, float *feat, cmp_result_t *result);

int feature_db_register_and_norm_st(int db_index, int reg_total, reg_data_t *reg_info);
int feature_db_compare_and_norm_st(int db_index, char *feat, cmp_result_t *result);

/*******以下的接口是对上述的接口，根据测试的要求再次封装，客户根据自身的业务自行实现 end*********/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
