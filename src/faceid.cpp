#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"

#ifdef WIN32
#define NOMINMAX
#endif

#include <time.h>

#ifdef WIN32
#include <winsock2.h>
#pragma comment (lib, "ws2_32.lib")
#include <windows.h>
#else

#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#endif

#ifdef WIN32
int gettimeofday(struct timeval *tp, void *tzp)
{
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;
    GetLocalTime(&wtm);
    tm.tm_year = wtm.wYear - 1900;
    tm.tm_mon = wtm.wMonth - 1;
    tm.tm_mday = wtm.wDay;
    tm.tm_hour = wtm.wHour;
    tm.tm_min = wtm.wMinute;
    tm.tm_sec = wtm.wSecond;
    tm.tm_isdst = -1;
    clock = mktime(&tm);
    tp->tv_sec = clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;
    return (0);
}
#endif

#include <iostream>
#include <mutex>
#include "lombo.h"
#include "base64.h"
#include "faceid.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdlib.h>
#include <pthread.h>

#ifdef __cplusplus
}
#endif /* __cplusplus */

#define FA_FEAT_SIZE 1024
#define FA_KPTS_SIZE (sizeof(float)*10)
#define FA_NORM_SIZE (sizeof(float)*2)
#define FA_DATA_SIZE (FA_FEAT_SIZE + FA_KPTS_SIZE + FA_NORM_SIZE)
#define DET_MIN_IMG_PIXEL 64

#define FA_MODELS_PATH_LEN 512

static long long get_systime_us(void);

static long long get_systime_us(void) {
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return (long long) tv.tv_sec * 1000000 + tv.tv_usec;
}

typedef struct recognizer_fl_param_s {
    const char *chip_version;
    const char *model_version;
    int recognizer_fl;
    int mask_recognizer_fl;
} recognizer_fl_param_t;

static recognizer_fl_param_t recognizer_fl_params[] = {
        //80x
        {"80x", "v1", 7, 1},
        {"80x", "v2", 7, 1},
        {"80x", "v3", 7, 1},

        //81x
        {"81x", "v1", 7, 1},
        {"81x", "v2", 7, 1},
        {"81x", "v3", 7, 1},
        {"81x", "v4", 7, 1},

        //82x
        {"82x", "v1", 7, 1},
        {"82x", "v2", 7, 1},
        {"82x", "v3", 7, -6},
};

static void show_support_version_list() {
    int size = sizeof(recognizer_fl_params) / sizeof(recognizer_fl_params[0]);
    std::cout << "算法版本与设备端SDK对应关系，查看models目录下说明。" << std::endl;
    for (int i = 0; i < size; i++) {
        recognizer_fl_param_t *pstrecognizer_fl_param = &recognizer_fl_params[i];
        std::cout << "芯片型号: " << pstrecognizer_fl_param->chip_version << " 算法版本: "
                  << pstrecognizer_fl_param->model_version << std::endl;
    }
    std::cout << std::endl;
}

static int get_recognizer_fl_param_t(const char *chip_version, const char *model_version, int *recognizer_fl,
                                     int *mask_recognizer_fl) {
    int size = sizeof(recognizer_fl_params) / sizeof(recognizer_fl_params[0]);

    *recognizer_fl = 0;
    *mask_recognizer_fl = 0;

    for (int i = 0; i < size; i++) {
        recognizer_fl_param_t *pstrecognizer_fl_param = &recognizer_fl_params[i];
        if (0 == strcmp(pstrecognizer_fl_param->chip_version, chip_version)) {
            if (0 == strcmp(pstrecognizer_fl_param->model_version, model_version)) {
                *recognizer_fl = pstrecognizer_fl_param->recognizer_fl;
                *mask_recognizer_fl = pstrecognizer_fl_param->mask_recognizer_fl;
                return 0;
            }
        }
    }
    return -1;
}

static pthread_mutex_t g_mutex;

static Lombo *lombo;

static Lombo *creatLombo(const char *chip_version, const char *model_version, const bool &use_mask_recognizer) {
    const char *faceidsdk_path = getenv("FACEIDSDKPATH");
    if (faceidsdk_path == NULL) {
        faceidsdk_path = ".";
    }
    char detection_file_c[FA_MODELS_PATH_LEN] = {0};
    char recognition_file_c[FA_MODELS_PATH_LEN] = {0};
    char mask_recognition_file_c[FA_MODELS_PATH_LEN] = {0};
    char pose_file_c[FA_MODELS_PATH_LEN] = {0};
    char quality_file_c[FA_MODELS_PATH_LEN] = {0};

    snprintf(detection_file_c, FA_MODELS_PATH_LEN - 1, "%s/models/%s/%s/detection_final.lb", faceidsdk_path,
             chip_version, model_version);
    snprintf(recognition_file_c, FA_MODELS_PATH_LEN - 1, "%s/models/%s/%s/recognition_final.lb", faceidsdk_path,
             chip_version, model_version);
    snprintf(mask_recognition_file_c, FA_MODELS_PATH_LEN - 1, "%s/models/%s/%s/recognition_mask_final.lb",
             faceidsdk_path, chip_version, model_version);
    snprintf(pose_file_c, FA_MODELS_PATH_LEN - 1, "%s/models/%s/%s/pose_final.lb", faceidsdk_path, chip_version,
             model_version);
    snprintf(quality_file_c, FA_MODELS_PATH_LEN - 1, "%s/models/%s/%s/quality_final.lb", faceidsdk_path, chip_version,
             model_version);

    std::string detection_file = std::string(detection_file_c);
    std::string recognition_file = std::string(recognition_file_c);
    std::string mask_recognition_file = std::string(mask_recognition_file_c);
    std::string pose_file = std::string(pose_file_c);
    std::string quality_file = std::string(quality_file_c);

    std::cout << detection_file << std::endl;
    std::cout << recognition_file << std::endl;
    if (use_mask_recognizer) {
        std::cout << mask_recognition_file_c << std::endl;
    }
    std::cout << pose_file << std::endl;
    std::cout << quality_file << std::endl;

    int recognizer_fl = 0; // no mask
    int mask_recognizer_fl = 0; // mask
    get_recognizer_fl_param_t(chip_version, model_version, &recognizer_fl, &mask_recognizer_fl);

    assert(recognizer_fl != 0 || mask_recognizer_fl != 0);
    std::cout << "recognizer_fl: " << recognizer_fl << " mask_recognizer_fl: " << mask_recognizer_fl
              << " use_mask_recognizer: " << use_mask_recognizer << std::endl;

    Lombo *tmp = NULL;
    if (use_mask_recognizer) {
        tmp = new Lombo(detection_file, recognition_file, mask_recognition_file, pose_file, quality_file, recognizer_fl,
                        mask_recognizer_fl, 0);
    } else {
        tmp = new Lombo(detection_file, recognition_file, pose_file, quality_file, recognizer_fl, 0);
    }

    if (!tmp) {
        std::cout << "Lombo NULL" << std::endl;
        return NULL;
    }
    tmp->set_max_faces(1);
    tmp->set_quality(0);
    tmp->set_yaw_pitch_roll(180, 180, 180);
    std::cout << "Lombo ok" << std::endl;
    return tmp;
}

void init_model(const char *chip_version, const char *model_version, const bool &use_mask_recognizer) {

    std::cout << "Lombo init chip_version: " << chip_version << " model_version: " << model_version << std::endl;
    lombo = creatLombo(chip_version, model_version, use_mask_recognizer);
    pthread_mutex_init(&g_mutex, NULL);
}

std::vector<Data> get_feature(std::string &filename, const bool &use_recognizer, const bool &use_pose_and_quality,
                              const bool &use_mask_recognizer,
                              int *code) {

    //long long start, end, time;

    std::vector<Data> dst;
    //start = get_systime_us();
    cv::Mat img = cv::imread(filename);
    //cv::Mat img = cv::imread("/home/ai/gocode/src/github.com/tzpeng/ggin/test/test.sjpg");
    if (img.empty()) {
        std::cout << filename << std::endl;
        *code = IMAGE_INFO_ERR_CODE_IMG_DECODE;
        return dst;
    }
    //std::cout << "img row:" << img.rows << " col:" << img.cols << std::endl;
    if (std::min(img.rows, img.cols) < DET_MIN_IMG_PIXEL) {
        std::cout << filename << std::endl;
        *code = IMAGE_INFO_ERR_CODE_SMALL_FACE;
        return dst;
    }
    //end = get_systime_us();
    //time = end - start;
    //std::cout << "read us:" << time << std::endl;

    // detection & recognition
    //std::cout << filename << std::endl;
    pthread_mutex_lock(&g_mutex);
    //start = get_systime_us();
    lombo->set_runner_flags(use_recognizer, use_pose_and_quality, use_mask_recognizer); //reg, post, mask_reg
    Data result = lombo->run(img);
    //end = get_systime_us();
    pthread_mutex_unlock(&g_mutex);
    //std::cout << filename << std::endl;
    //result.print();
    //time = end - start;
    //std::cout << "run us:" << time << std::endl;
    if (result.flg == PASS) {
        if (result.n == 1) {
            *code = IMAGE_INFO_ERR_CODE_SUCCESS;
            dst.push_back(result);
        } else if (result.n > 1) {
            *code = IMAGE_INFO_ERR_CODE_MULTI_FACE;
        }
    } else if (result.flg == NO_FACE) {
        *code = IMAGE_INFO_ERR_CODE_NO_FACE;
    }
    //std::cout << "flg: " << result.flg << std::endl;
    //std::cout << "flg1: " << result.n << std::endl;
    //std::cout << "flg2: " << PASS << std::endl;

    return dst;
}

void gen_features_and_normal(const cv::Mat &embedding, signed char *&features, float *&normal) {
    int array_norm_n = embedding.rows * 2;
    int array_feature_n = embedding.rows * 1024;
    cv::Mat fix_point_embeddings = Lombo::apply_fl(embedding, 7);
    //std::cout << "reg" << fix_point_embeddings << std::endl;
    cv::Mat norm = Lombo::get_norm(fix_point_embeddings);
    cv::Mat zero_norm = norm * 0;
    cv::Mat comb_norm_data = Lombo::concatenate(norm, zero_norm, 1);
    cv::Mat zero_emb = fix_point_embeddings * 0;
    cv::Mat comb_emb_data = Lombo::concatenate(fix_point_embeddings, zero_emb, 1);
    Lombo::mat_to_float(comb_norm_data, normal, array_norm_n);
    Lombo::mat_to_char(comb_emb_data, features, array_feature_n);
}

void gen_mask_features_and_normal(const cv::Mat &embedding, signed char *&features, float *&normal) {
    int array_norm_n = embedding.rows * 2;
    int array_feature_n = embedding.rows * 1024;
    cv::Mat fix_point_embeddings = Lombo::apply_fl(embedding, 1);
    //std::cout << "mask reg" << fix_point_embeddings << std::endl;
    cv::Mat norm = Lombo::get_norm(fix_point_embeddings);
    //std::cout << "mask morm" << norm << std::endl;
    cv::Mat zero_norm = norm * 0;
    cv::Mat comb_norm_data = Lombo::concatenate(norm, zero_norm, 1);
    cv::Mat zero_emb = fix_point_embeddings * 0;
    cv::Mat comb_emb_data = Lombo::concatenate(fix_point_embeddings, zero_emb, 1);
    Lombo::mat_to_float(comb_norm_data, normal, array_norm_n);
    Lombo::mat_to_char(comb_emb_data, features, array_feature_n);
}

image_info_t get_feature_from(char *filename, bool use_recognizer, bool use_mask_recognizer) {
    image_info_t image_info;
    memset(&image_info, 0, sizeof(image_info_t));

    std::string image = filename;
    int code = -1;
    bool _use_recognizer = use_recognizer;
    bool _use_pose_and_quality = false;
    bool _use_mask_recognizer = use_mask_recognizer;
    std::vector<Data> reg_feat_result = get_feature(image, _use_recognizer, _use_pose_and_quality, _use_mask_recognizer,
                                                    &code);
    //std::cout << "size :" << reg_feat_result.size() << std::endl;
    if (reg_feat_result.size() == 1) {
        signed char *__feature = new signed char[1024]();
        signed char *__mask_feature = new signed char[1024]();
        float *__normal = new float[2]();
        float *__mask_normal = new float[2]();
        char *feature = image_info.feature;
        char *mask_feature = image_info.feature + 512;
        float *norm_feature = image_info.norm_feature;

        cv::Mat register_embeddings = Lombo::collect_data(reg_feat_result, PROPERTY::RECOGNITION_EMBEDDINGS);
        //std::cout << register_embeddings <<std::endl;
        gen_features_and_normal(register_embeddings, __feature, __normal);

        if (use_mask_recognizer) {
            cv::Mat mask_register_embeddings = Lombo::collect_data(reg_feat_result,
                                                                   PROPERTY::MASK_RECOGNITION_EMBEDDINGS);
            //std::cout << mask_register_embeddings <<std::endl;
            gen_mask_features_and_normal(mask_register_embeddings, __mask_feature, __mask_normal);
        }

        //for (int i=0; i < 2; i++) {
        //	std::cout << "normal: " << __normal[i] <<std::endl;
        //}
        image_info.normal[0] = __normal[0];

        //for (int i=0; i < 2; i++) {
        //	std::cout << "mask_normal: " << __mask_normal[i] <<std::endl;
        //}
        image_info.normal[1] = __mask_normal[0];

        memcpy(feature, __feature, 512);
        memcpy(mask_feature, __mask_feature, 512);

        Lombo::normalize(register_embeddings);
        //std::cout << "register_embeddings: " << register_embeddings << std::endl;
        memcpy(norm_feature, (float *) register_embeddings.data, 512 * sizeof(float));

        delete[] __feature;
        delete[] __mask_feature;
        delete[] __normal;
        delete[] __mask_normal;
        //FILE *fp = fopen("test.bin", "wb+");
        //fwrite(features, 1, FA_FEAT_SIZE, fp);
        //fclose(fp);
    }

    //for (int i = 0; i < FA_FEAT_SIZE; i++) {
    //	signed char x = (signed char)image_info.feature[i];
    //	printf("%d ", x);
    //}
    //printf("\n");

    image_info.err_code = code;
    std::cout << "err_code: " << image_info.err_code << std::endl;

    return image_info;
}

std::vector<Data> get_quality(std::string &filename, int *code) {

    //long long start, end, time;

    std::vector<Data> dst;
    //start = get_systime_us();
    cv::Mat img = cv::imread(filename);
    //cv::Mat img = cv::imread("/home/ai/gocode/src/github.com/tzpeng/ggin/test/test.sjpg");
    if (img.empty()) {
        //std::cout << filename << std::endl;
        *code = IMAGE_INFO_ERR_CODE_IMG_DECODE;
        return dst;
    }
    //std::cout << "img row:" << img.rows << " col:" << img.cols << std::endl;
    if (std::min(img.rows, img.cols) < DET_MIN_IMG_PIXEL) {
        //std::cout << filename << std::endl;
        *code = IMAGE_INFO_ERR_CODE_SMALL_FACE;
        return dst;
    }
    //end = get_systime_us();
    //time = end - start;
    //std::cout << "read us:" << time << std::endl;

    // detection & recognition
    //std::cout << filename << std::endl;
    pthread_mutex_lock(&g_mutex);
    //start = get_systime_us();
    lombo->set_runner_flags(false, true, false); //reg, post, mask_reg
    Data result = lombo->run(img);
    //end = get_systime_us();
    pthread_mutex_unlock(&g_mutex);
    //std::cout << filename << std::endl;
    //result.print();
    //time = end - start;
    //std::cout << "run us:" << time << std::endl;
    if (result.flg == PASS) {
        if (result.n == 1) {
            *code = IMAGE_INFO_ERR_CODE_SUCCESS;
            dst.push_back(result);
        } else if (result.n > 1) {
            *code = IMAGE_INFO_ERR_CODE_MULTI_FACE;
        }
    } else if (result.flg == NO_FACE) {
        *code = IMAGE_INFO_ERR_CODE_NO_FACE;
    }
    //std::cout << "flg: " << result.flg << std::endl;
    //std::cout << "flg1: " << result.n << std::endl;
    //std::cout << "flg2: " << PASS << std::endl;

    return dst;
}

image_quality_t get_quality_from(char *filename) {
    image_quality_t info;
    memset(&info, 0, sizeof(image_quality_t));

    std::string image = filename;
    int code = -1;
    std::vector<Data> result = get_quality(image, &code);
    //std::cout << "code :" << code << std::endl;
    //std::cout << "size :" << reg_feat_result.size() << std::endl;
    info.code = code;
    if (result.size() == 1) {
        cv::Mat pose = Lombo::collect_data(result, PROPERTY::POSE);
        int rows = pose.rows;
        int cols = pose.cols;
        //std::cout << "row" << rows << "cols" << cols << std::endl;
        //std::cout << "pose" << pose << std::endl;
        for (int i = 0; i < rows; i++) {
            float *data = pose.ptr<float>(i);
            for (int j = 0; j < cols; j++) {
                //std::cout << "data[j] " << data[j] << std::endl;
                info.pose[j] = data[j];

            }
        }

        cv::Mat quality = Lombo::collect_data(result, PROPERTY::QUALITY);
        rows = quality.rows;
        cols = quality.cols;
        //std::cout << "row" << rows << "cols" << cols << std::endl;
        //std::cout << "quality" << quality << std::endl;
        for (int i = 0; i < rows; i++) {
            float *data = quality.ptr<float>(i);
            for (int j = 0; j < cols; j++) {
                //std::cout << "data[j] " << data[j] << std::endl;
                info.quality = data[j];
            }
        }
    }
    return info;
}

float feature_compare(int reg_total, char *reg_feat, char *feat, int mask = 0) {
    int total = reg_total;
    char *face_reg_feat = reg_feat;
    char *face_feat = feat;

    cv::Mat embedding1(total, 512, CV_32F);
    cv::Mat embedding2(1, 512, CV_32F);

    int step = mask ? 512 : 0;
    float scale = mask ? 2.0f : 128.0f;

    //reg_feat
    for (int i = 0; i < total; i++) {
        float *data = embedding1.ptr<float>(i);
        for (int j = step; j < 512 + step; j++) {
            signed char x = (signed char) face_reg_feat[j];
            //printf("[%d ", x);
            float _x = (float) (x / scale);
            //printf("%f] ", _x);
            data[j - step] = _x;
        }
        face_reg_feat += 1024;
    }

    //to be compare face feat
    for (int i = 0; i < 1; i++) {
        float *data = embedding2.ptr<float>(i);
        for (int j = step; j < 512 + step; j++) {
            signed char x = (signed char) face_feat[j];
            float _x = (float) (x / scale);
            data[j - step] = _x;
        }
    }

    //std::cout << "embedding1" << embedding1 << std::endl;
    //std::cout << "embedding2" << embedding2 << std::endl;

    cv::Mat similarity = Lombo::norm_and_cosine_distance(embedding1, embedding2);
    //std::cout << "similarity: " << similarity << std::endl;

    double maxValue;
    minMaxLoc(similarity, NULL, &maxValue, NULL, NULL);

    return (float) maxValue;
}

typedef struct feature_db_s {
    feature_db_s() {
        for (int i = 0; i < FEATURE_MAX_DB_INDEX; i++) {
            total[i] = 0;
            names[i] = NULL;

            //score > result->thr face
            thr_idxs[i] = NULL;
            thr_scores[i] = NULL;
            thr_names[i] = NULL;
        }
    }

    cv::Mat db[FEATURE_MAX_DB_INDEX];
    std::mutex m_mutex[FEATURE_MAX_DB_INDEX];
    //reg face total
    int total[FEATURE_MAX_DB_INDEX];
    reg_name_t *names[FEATURE_MAX_DB_INDEX];

    //score > result->thr face
    reg_idx_t *thr_idxs[FEATURE_MAX_DB_INDEX];
    reg_score_t *thr_scores[FEATURE_MAX_DB_INDEX];
    reg_name_t *thr_names[FEATURE_MAX_DB_INDEX];

} feature_db_t;

static feature_db_t feature_db;

int feature_db_register(int db_index, int reg_total, float *reg_feat) {
    std::unique_lock<std::mutex> lock(feature_db.m_mutex[db_index]);

    int total = reg_total;
    float *face_reg_feat = reg_feat;

    if (db_index < 0 || db_index >= FEATURE_MAX_DB_INDEX) {
        std::cout << "invalid db_index: " << db_index << std::endl;
        return -1;
    }

    feature_db.db[db_index] = cv::Mat(total, 1024, CV_32FC1, face_reg_feat);
    feature_db.db[db_index] = feature_db.db[db_index].colRange(0, 512);

    return 0;
}

float feature_db_compare(int db_index, float *feat) {
    std::unique_lock<std::mutex> lock(feature_db.m_mutex[db_index]);

    if (db_index < 0 || db_index >= FEATURE_MAX_DB_INDEX) {
        std::cout << "invalid db_index: " << db_index << std::endl;
        return 0.0f;
    }

    cv::Mat embedding2(1, 1024, CV_32FC1, feat);
    embedding2 = embedding2.colRange(0, 512);

    cv::Mat similarity = Lombo::cosine_distance(feature_db.db[db_index], embedding2);

    double maxValue;
    minMaxLoc(similarity, NULL, &maxValue, NULL, NULL);

    return (float) maxValue;
}

int feature_db_register_and_norm(int db_index, int reg_total, char *reg_feat) {
    std::unique_lock<std::mutex> lock(feature_db.m_mutex[db_index]);

    int total = reg_total;
    char *face_reg_feat = reg_feat;

    if (db_index < 0 || db_index >= FEATURE_MAX_DB_INDEX) {
        std::cout << "invalid db_index: " << db_index << std::endl;
        return -1;
    }

    feature_db.db[db_index] = cv::Mat(total, 512, CV_32FC1);
    cv::Mat buffer(total, 1024, CV_8SC1, face_reg_feat);
    buffer.convertTo(feature_db.db[db_index], CV_32FC1);
    feature_db.db[db_index] = feature_db.db[db_index].colRange(0, 512);
    // norm reg face feature feature_db.db[db_index]
    long long start, end, time;
    start = get_systime_us();
    Lombo::normalize(feature_db.db[db_index]);
    end = get_systime_us();
    time = end - start;
    std::cout << "norm us:" << time << std::endl;
    return 0;
}

float feature_db_compare_and_norm(int db_index, char *feat) {
    std::unique_lock<std::mutex> lock(feature_db.m_mutex[db_index]);

    cv::Mat embedding2(1, 512, CV_32F);

    if (db_index < 0 || db_index >= FEATURE_MAX_DB_INDEX) {
        std::cout << "invalid db_index: " << db_index << std::endl;
        return 0.0f;
    }

    cv::Mat buffer(1, 1024, CV_8SC1, feat);
    buffer.convertTo(embedding2, CV_32FC1);
    embedding2 = embedding2.colRange(0, 512);

    //norm to do compare face feature embedding2
    Lombo::normalize(embedding2);
    cv::Mat similarity = Lombo::cosine_distance(feature_db.db[db_index], embedding2);

    double maxValue;
    minMaxLoc(similarity, NULL, &maxValue, NULL, NULL);

    return (float) maxValue;
}

static int feature_db_register_thr_data_st(int db_index, int reg_total, reg_data_t *reg_info) {
    int total = reg_total;
    reg_name_t *face_reg_names = reg_info->names;

    feature_db.total[db_index] = total;

    //names
    if (feature_db.names[db_index]) {
        free(feature_db.names[db_index]);
    }
    feature_db.names[db_index] = (reg_name_t *) calloc(total, sizeof(reg_name_t));
    if (NULL == feature_db.names[db_index]) {
        std::cout << "calloc names err db_index: " << db_index << std::endl;
        return -1;
    }

    //thr_idxs
    if (feature_db.thr_idxs[db_index]) {
        free(feature_db.thr_idxs[db_index]);
    }
    feature_db.thr_idxs[db_index] = (reg_idx_t *) calloc(total, sizeof(reg_idx_t));
    if (NULL == feature_db.thr_idxs[db_index]) {
        std::cout << "calloc thr_idxs err db_index: " << db_index << std::endl;
        return -1;
    }

    //thr_scores
    if (feature_db.thr_scores[db_index]) {
        free(feature_db.thr_scores[db_index]);
    }
    feature_db.thr_scores[db_index] = (reg_score_t *) calloc(total, sizeof(reg_score_t));
    if (NULL == feature_db.thr_scores[db_index]) {
        std::cout << "calloc thr_scores err db_index: " << db_index << std::endl;
        return -1;
    }

    //thr_names
    if (feature_db.thr_names[db_index]) {
        free(feature_db.thr_names[db_index]);
    }
    feature_db.thr_names[db_index] = (reg_name_t *) calloc(total, sizeof(reg_name_t));
    if (NULL == feature_db.thr_names[db_index]) {
        std::cout << "calloc thr_names err db_index: " << db_index << std::endl;
        return -1;
    }

    reg_name_t *name = feature_db.names[db_index];
    reg_name_t *face_reg_name = face_reg_names;
    for (int i = 0; i < total; i++) {
        memcpy(&name[i], &face_reg_name[i], sizeof(reg_name_t));
    }
    return 0;
}

static void feature_db_compare_thr_data_st(cv::Mat &similarity, int db_index, cmp_result_t *result) {
    //find score > result->thr face
    //similarity rows = reg_total
    float score = 0.0f;
    int i = 0, j = 0;
    reg_name_t *reg_names = feature_db.names[db_index];
    reg_idx_t *thr_idxs = feature_db.thr_idxs[db_index];
    reg_name_t *thr_names = feature_db.thr_names[db_index];
    reg_score_t *thr_scores = feature_db.thr_scores[db_index];
    for (i = 0, j = 0; i < similarity.rows; i++) {
        score = similarity.at<float>(i);
        if (score >= result->thr) {
            thr_idxs[j].idx = i;
            thr_scores[j].score = score;
            memcpy(&thr_names[j], &reg_names[i], sizeof(reg_name_t));
            j++;
        }
    }
    result->thr_cnt = j;
    result->thr_idxs = feature_db.thr_idxs[db_index];
    result->thr_names = feature_db.thr_names[db_index];
    result->thr_scores = feature_db.thr_scores[db_index];
}

int feature_db_register_st(int db_index, int reg_total, reg_data_t *reg_info) {
    int ret = -1;
    float *face_reg_feat = reg_info->norm_features->norm_feature;

    ret = feature_db_register(db_index, reg_total, face_reg_feat);
    if (ret) {
        return -1;
    }

    ret = feature_db_register_thr_data_st(db_index, reg_total, reg_info);
    if (ret) {
        return -1;
    }

    return 0;
}

int feature_db_compare_st(int db_index, float *feat, cmp_result_t *result) {
    std::unique_lock<std::mutex> lock(feature_db.m_mutex[db_index]);

    if (db_index < 0 || db_index >= FEATURE_MAX_DB_INDEX) {
        std::cout << "invalid db_index: " << db_index << std::endl;
        return -1;
    }

    cv::Mat embedding2(1, 1024, CV_32FC1, feat);
    embedding2 = embedding2.colRange(0, 512);

    cv::Mat similarity = Lombo::cosine_distance(feature_db.db[db_index], embedding2);

    double maxValue = 0.0f;
    cv::Point idx; //x = 0, y = idx
    minMaxLoc(similarity, NULL, &maxValue, NULL, &idx);

    result->idx = idx.y;
    result->score = (float) maxValue;
    reg_name_t *names = feature_db.names[db_index];
    memcpy(result->name, &names[idx.y], sizeof(char) * 64);

    if (result->thr > 0.0f) {
        feature_db_compare_thr_data_st(similarity, db_index, result);
    }
    return 0;
}

int feature_db_register_and_norm_st(int db_index, int reg_total, reg_data_t *reg_info) {
    int ret = -1;
    char *face_reg_feature = reg_info->features->feature;
    ret = feature_db_register_and_norm(db_index, reg_total, face_reg_feature);
    if (ret) {
        return -1;
    }

    ret = feature_db_register_thr_data_st(db_index, reg_total, reg_info);
    if (ret) {
        return -1;
    }

    return 0;
}

int feature_db_compare_and_norm_st(int db_index, char *feat, cmp_result_t *result) {
    std::unique_lock<std::mutex> lock(feature_db.m_mutex[db_index]);

    cv::Mat embedding2(1, 512, CV_32F);

    if (db_index < 0 || db_index >= FEATURE_MAX_DB_INDEX) {
        std::cout << "invalid db_index: " << db_index << std::endl;
        return -1;
    }

    cv::Mat buffer(1, 1024, CV_8SC1, feat);
    buffer.convertTo(embedding2, CV_32FC1);
    embedding2 = embedding2.colRange(0, 512);

    //norm to do compare face feature embedding2
    Lombo::normalize(embedding2);
    cv::Mat similarity = Lombo::cosine_distance(feature_db.db[db_index], embedding2);

    double maxValue;
    cv::Point idx; //x = 0, y = idx
    minMaxLoc(similarity, NULL, &maxValue, NULL, &idx);

    result->idx = idx.y;
    result->score = (float) maxValue;
    reg_name_t *names = feature_db.names[db_index];
    memcpy(result->name, &names[idx.y], sizeof(char) * 64);

    if (result->thr > 0.0f) {
        feature_db_compare_thr_data_st(similarity, db_index, result);
    }
    return 0;
}

#if 1

static int http_method(std::string host, std::string request) {
    std::cout << request << std::endl;

    int sockfd;
    struct sockaddr_in address;
    memset(&address, 0, sizeof(struct sockaddr_in));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_port = htons(80);
    address.sin_addr.s_addr = inet_addr(host.c_str());

    if (connect(sockfd, (struct sockaddr *) &address, sizeof(address))) {
        std::cout << "connect faild" << std::endl;
        return -1;
    }
    std::cout << "connect successed" << std::endl;

    int sc = send(sockfd, request.c_str(), request.size(), 0);

    std::cout << "request.size(): " << request.size() << std::endl;
    std::cout << "sc: " << sc << std::endl;

    char buf[4096] = {0};
    int offset = 0;
    int rc;
    int timeout = 1000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(int));
    while ((rc = recv(sockfd, buf + offset, 1024, 0)) > 0) {
        offset += rc;
        std::cout << "offset: " << offset << std::endl;
    }
#ifdef WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif
    buf[offset] = '\0';
    std::cout << buf << std::endl;
    return 0;
}

static int http_method_post(std::string host, std::string url, std::string body) {
    std::stringstream stream;
    stream << "POST ";
    stream << url;
    stream << " HTTP/1.1\r\n";
    stream << "Host: " << host << "\r\n";
    stream << "Content-Length:" << body.size() << "\r\n";
    stream << "Content-Type: application/json\r\n";
    stream << "\r\n";
    stream << body.c_str();
    return http_method(host, stream.str());
}

int compare_faces(int compare_type, const char *feature, int len) {
#ifdef WIN32
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif

    char *base64 = (char *) calloc(sizeof(char) * len * 4 / 3 + 4, 1);
    base64_encode((const unsigned char *) feature, len, base64);

    std::string host = "192.168.1.168";
    //std::string url = "/v1/device_time/get_time_info";
    //std::string body = "{\"action\":\"get_time_info\"}";

    std::string url = "/v1/smart_event/compare_faces";
    std::stringstream body;
    std::string feature_base64 = std::string(base64);

    body << "{\"action\": \"compare_faces\", \"data\": {\"type\": ";
    body << compare_type;
    body << ", \"base64\": \"";
    body << feature_base64;
    body << "\"}}";
    http_method_post(host, url, body.str());

#ifdef WIN32
    WSACleanup();
#endif
    return 0;
}

int file_read(char *file_path, char *buf, int buf_len) {
    FILE *file_fp = NULL;
    int read_cnt = 0;
    if (NULL == file_path) {
        printf("%s:file_path is null\n", __func__);
        return -1;
    }
    if (NULL == buf) {
        printf("%s:buf is null\n", __func__);
        return -1;
    }
    if (buf_len <= 0) {
        printf("%s:buf_len is %d\n", __func__, buf_len);
        return -1;
    }
    printf("%s\n", file_path);
    file_fp = fopen(file_path, "rb");
    if (file_fp == NULL) {
        printf("%s: open %s failed\n", __func__, file_path);
        return -1;
    }
    read_cnt = fread(buf, 1, buf_len, file_fp);
    fclose(file_fp);

    return read_cnt;
}

#define TEST_FAETURE_SIZE (20000)

int main_test(int argc, char **argv) {

#ifdef WIN32
    system("chcp 65001");
#endif //WIN32
    long long start, end, time;
    int ret = 0;
    float score;

    if (argc == 1) {
        show_support_version_list();
        std::cout << "人脸特征值提取: " << std::endl;
        std::cout << "         运行:  ./faceidtest xxxx.jpeg 81x v4" << std::endl;
        std::cout << std::endl;
        std::cout << "人脸特征值比对: " << std::endl;
        std::cout << "         运行:  ./faceidtest xxxx.bin xxxx.bin" << std::endl;
        std::cout << std::endl;
    }

    start = get_systime_us();

    if (argc == 4) {
        bool use_mask_recognizer = false;
        init_model(argv[2], argv[3], use_mask_recognizer);

        image_info_t info = {0};
        info = get_feature_from(argv[1], true, use_mask_recognizer);
        if (info.err_code == 0) {
            std::cout << "normal: " << info.normal[0] << std::endl;
            std::cout << "mask_normal: " << info.normal[1] << std::endl;
            std::cout << "feature: " << info.feature << std::endl;
            std::cout << "save feature.bin" << std::endl;
            FILE *fp = fopen("feature.bin", "wb+");
            fwrite(info.feature, 1, 1024, fp);
            //send to device for compare feature
            //compare_faces(1, info.feature, 1024);
            fclose(fp);
        }
    }

    if (argc == 3) {
        char feature[1024] = {0};
        char device_feature[1024] = {0};
        file_read(argv[1], feature, 1024);
        file_read(argv[2], device_feature, 1024);
        score = feature_compare(1, feature, device_feature);
        std::cout << "feature_compare score: " << score << std::endl;
    }

    end = get_systime_us();
    time = end - start;
    //std::cout << "spend time us:" << time << std::endl;

#ifdef WIN32
    system("pause");
#endif //WIN32

    return 0;
}

#endif

