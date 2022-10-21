#include <iostream>
#include <mutex>
#include <fstream>
#include <nlohmann/json.hpp>


#include "faceid.h"
#include "nats/nats.h"
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"

using json = nlohmann::json;
json conf;

json score(natsMsg *msg) {
    json msg_json = json::parse(natsMsg_GetData(msg));
    char img;
    msg_json["img"].get_to(img);
    image_quality_t quality = get_quality_from_url(&img);

    json result_json;
    result_json["code"] = quality.code;
    result_json["quality"] = quality.quality;
    result_json["pose"] = quality.pose;

    return result_json;
}

json feature(natsMsg *msg) {
    json msg_json = json::parse(natsMsg_GetData(msg));

    std::string img;
    std::string method;
    msg_json["img"].get_to(img);

    char cFileName[img.length() + 1];
    strcpy(cFileName, img.c_str());

    //Get Info
    bool use_mask_recognizer = false;
    image_info_t info = {0};
    info = get_feature_from(cFileName, true, use_mask_recognizer);
    if (info.err_code == 0) {
        std::cout << "normal: " << info.normal[0] << std::endl;
        std::cout << "mask_normal: " << info.normal[1] << std::endl;
        std::cout << "feature: " << info.feature << std::endl;
    }
    /*char *fName;
    sprintf(fName, "feature_%s_%s.bin", model_type_c, model_ver_c);
    std::cout << "Export feature to file: " << fName << std::endl;*/
    FILE *fp = fopen("feature.bin", "wb+");
    fwrite(info.feature, 1, 1024, fp);
    fclose(fp);

    json result_json;
    result_json["code"] = 0;
    return result_json;
}

static void onMsg(natsConnection *nc, natsSubscription *sub, natsMsg *msg, void *closure) {
    printf("Received msg: %s - %.*s\n",
           natsMsg_GetSubject(msg),
           natsMsg_GetDataLength(msg),
           natsMsg_GetData(msg));

    std::string subject = natsMsg_GetSubject(msg);
    json result_json;

    if (subject == "inf.facelib.score") {
        result_json = score(msg);
    }
    if (subject == "inf.facelib.feature") {
        result_json = feature(msg);
    }


    auto result_str = to_string(result_json);
    natsConnection_PublishString(nc, natsMsg_GetReply(msg),
                                 result_str.c_str());

    delete &result_json;
    natsMsg_Destroy(msg);
}

int main() {
    std::string model_type;
    std::string model_ver;
    //Read config from file
    std::ifstream f("conf.json");
    conf = json::parse(f);

    conf["model_type"].get_to(model_type);
    conf["model_ver"].get_to(model_ver);

    const char *model_type_c = model_type.c_str();
    const char *model_ver_c = model_ver.c_str();

    //Establish connection with NATS server
    std::string nats_url;
    std::string nats_subj;
    conf["nats_url"].get_to(nats_url);
    conf["nats_subj"].get_to(nats_subj);
    const char *nats_url_c = nats_url.c_str();
    const char *nats_subj_c = nats_subj.c_str();

    natsConnection *conn = nullptr;
    natsSubscription *sub = nullptr;
    natsStatus s;

    std::cout << "Start connecting NATS !" << std::endl;
    s = natsConnection_ConnectTo(&conn, nats_url_c);

    if (s == NATS_OK)
        std::cout << "NATS successfully connected !" << std::endl;

    if (s == NATS_OK)
        s = natsConnection_Subscribe(&sub, conn, nats_subj_c, onMsg, nullptr);

    if (s == NATS_OK)
        std::cout << "NATS subject subscribed !" << std::endl;

    //Start loading model here .....
    std::cout << "Start loading model!" << std::endl;
    bool use_mask_recognizer = false;
    init_model(model_type_c, model_ver_c, use_mask_recognizer);
    std::cout << "Model loaded !" << std::endl;

    int flag;
    flag = std::cin.get();

    //Exit on flag=0
    if (flag == 0) {
        //Close NATS
        std::cout << "Clear NATS resources!" << std::endl;
        natsSubscription_Destroy(sub);
        natsConnection_Destroy(conn);
        nats_Close();
        return 0;
    }
    return 0;
}
