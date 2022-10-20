#ifndef LOMBO_H
#define LOMBO_H

#include <string>
#include <vector>
#ifdef WIN32
#include <time.h>
#include <windows.h>
#else
#include <sys/time.h>
#endif
#include "opencv2/core.hpp"

class Detection;
class Recognition;
class Pose;
class Data;
class Quality;


enum PROPERTY{
    DETECTION_SCORES,
    LANDMARKS,
    BOXES,
    RECOGNITION_EMBEDDINGS,
	MASK_RECOGNITION_EMBEDDINGS,
	DETECTION_TIME,
    RECOGNITION_TIME,
	MASK_RECOGNITION_TIME,
    SRC_POSE,
    POSE,
    SRC_QUALITY,
    QUALITY,

};


enum DATA_FLAG{
    NO_FACE,
    POSE_ERROR,
    QUALITY_ERROR,
    PASS
};

class Data {
    /* One Image */
public:
    explicit Data(): n(0), flg(DATA_FLAG::PASS) { };
    ~Data()=default;

    void print();
    void print_row(int row);
    cv::Mat get_property(PROPERTY property, int row=-1) const;

    int n;  // n face

    // detection
    cv::Mat detection_scores; // n x 1
    cv::Mat landmarks; // n x 10
    cv::Mat boxes;  // n x 4

    // recognition
    cv::Mat recognition_embeddings; // n x 512
	cv::Mat mask_recognition_embeddings; // n x 512 
    cv::Mat detection_time;  // 1 x 1
    cv::Mat recognition_time;  // 1 x 1
	cv::Mat mask_recognition_time; // 1 x 1 

    // pose
    cv::Mat src_pose;  // n x 3
    cv::Mat pose;  // n x 3

    //quality
    cv::Mat src_quality_score;
    cv::Mat quality_score;

    DATA_FLAG flg;
};


class Lombo {
public:
    Lombo(const std::string& detector_filename, const std::string& recognizer_filename, 
		  int& recognizer_fl, const unsigned long& sleep_usec=5000);
    Lombo(const std::string& detector_filename, const std::string& recognizer_filename,
          const std::string& pose_filename, const std::string& quality_filename, 
		  int& recognizer_fl, const unsigned long& sleep_usec=5000);
	Lombo(const std::string& detector_filename, const std::string& recognizer_filename, const std::string& mask_recognizer_filename, 
		  int& recognizer_fl, int& mask_recognizer_fl, const unsigned long& sleep_usec = 5000);
	Lombo(const std::string& detector_filename, const std::string& recognizer_filename, const std::string& mask_recognizer_filename,
		 const std::string& pose_filename, const std::string& quality_filename, 
		 int& recognizer_fl, int& mask_recognizer_fl, const unsigned long& sleep_usec = 5000);

	~Lombo();

    Data run(const cv::Mat& image);  // BGR
    std::vector<Data> run(const std::vector<cv::Mat>& images);  // BGR
    void set_yaw_pitch_roll(const float& yaw, const float& pitch, const float& roll);
	void set_quality(const float& quality);
	void set_max_faces(const int& n);
	void set_runner_flags(const bool& use_recognizer, const bool& use_pose_and_quality, const bool& use_mask_recognizer=false);

    // some tools
    // about cosine distance
    static void normalize(cv::Mat& x);
    static cv::Mat get_norm(const cv::Mat& x);
    static cv::Mat apply_norm(const cv::Mat& x, const cv::Mat& norm);
    static cv::Mat cosine_distance(const cv::Mat& register_embedding, const cv::Mat& test_embedding);
    static cv::Mat cosine_distance(const cv::Mat& register_embedding, const cv::Mat& register_norm,
                                   const cv::Mat& test_embedding, const cv::Mat& test_norm);
    static cv::Mat norm_and_cosine_distance(const cv::Mat& register_embedding, const cv::Mat& test_embedding);
    static void top_1(const cv::Mat& score, cv::Mat& top_score, cv::Mat& index);

    // about data
    static cv::Mat collect_data(std::vector<Data>& x, PROPERTY property, const int& row=-1);
    static cv::Mat concatenate(const cv::Mat& left, const cv::Mat& right, const int& axis);
    static cv::Mat apply_fl(const cv::Mat& src, const int& fl);
    static void mat_to_float(const cv::Mat& src, float*& dst, int& n);
    static void mat_to_char(const cv::Mat& src, const int& fl, signed char*& dst, int& n);
    static void mat_to_char(const cv::Mat& src, signed char*& dst, int& n);

    // other tools
    static float get_run_time(const timeval& start_time, const timeval& end_time);
    static float version();

    int recognition_float;
	int mask_recognition_float;
private:
    Detection* detector;
    Recognition* recognizer;
	Recognition* mask_recognizer;
    Pose* pose_estimator;
    Quality* quality_estimator;
	unsigned long sleep_us;
	bool recognizer_flg;
	bool mask_recognizer_flg;
	bool pose_and_quality_flg;
};

#endif
