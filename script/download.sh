LIB_URL="https://seeiner-cloud-storage.oss-cn-hangzhou.aliyuncs.com/facelib-lib.tar.gz"
MODELS_URL="https://seeiner-cloud-storage.oss-cn-hangzhou.aliyuncs.com/facelib-models.tar.gz"
echo "Downloading libs"
wget $LIB_URL
tar -zxvf facelib-lib.tar.gz
echo "Downloading models"
wget $MODELS_URL
tar -zxvf facelib-models.tar.gz
echo "Finished"
