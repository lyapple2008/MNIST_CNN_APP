#include <jni.h>
#include <string>
#include <algorithm>

#define PROTOBUF_USE_DLLS   1
#define CAFFE2_USE_LITE_PROTO   1

#include <caffe2/core/predictor.h>
#include <caffe2/core/operator.h>
#include <caffe2/core/timer.h>

#include "caffe2/core/init.h"
#include "caffe2/core/predictor.h"
#include "caffe2/core/workspace.h"
#include "caffe2/proto/caffe2.pb.h"
#include "caffe2/core/tensor.h"

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>

#define IMG_H   28
#define IMG_W   28
#define IMG_C   1
#define IMG_DATA_SIZE   IMG_H*IMG_W*IMG_C
#define INPUT_DATA_SIZE IMG_H*IMG_W
#define alog(...) __android_log_print(ANDROID_LOG_ERROR, "F8DEMO", __VA_ARGS__);

static caffe2::NetDef _initNet, _predictNet;
static caffe2::Predictor *_predictor;
static int raw_data[IMG_DATA_SIZE];
static float input_data[INPUT_DATA_SIZE];
static caffe2::Workspace ws;


void loadToNetDef(AAssetManager *mgr, caffe2::NetDef *net, const char *filename) {
    AAsset *asset = AAssetManager_open(mgr, filename, AASSET_MODE_BUFFER);
    assert(asset != nullptr);
    const void *data = AAsset_getBuffer(asset);
    assert(data != nullptr);
    off_t len = AAsset_getLength(asset);
    assert(len != 0);
    if (!net->ParseFromArray(data, len)) {
        alog("Couldn't parse net from data.\n");
    }
    AAsset_close(asset);
}

extern "C"
void Java_com_example_beyoung_handwrittendigit_MainActivity_initCaffe2(
        JNIEnv *env,
        jobject,
        jobject assetManager) {
    AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);
    alog("Attempting to load protobuf netdefs...");
    loadToNetDef(mgr, &_initNet, "mnist/init_net.pb");
    loadToNetDef(mgr, &_predictNet, "mnist/predict_net.pb");
    alog("done.");
    alog("Instantiating predictor...");
    _predictor = new caffe2::Predictor(_initNet, _predictNet);
    if (_predictor) {
        alog("done...");
    } else {
        alog("fail to instantiat predictor...");
    }
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_beyoung_handwrittendigit_MainActivity_recognitionFromCaffe2(
        JNIEnv *env,
        jobject,
        jint h, jint w, jintArray data) {
    if (!_predictor) {
        return env->NewStringUTF("Loading...");
    }

    jsize len = env->GetArrayLength(data);
    jint *img_data = env->GetIntArrayElements(data, 0);
    jint img_size = h * w;
    assert(img_size <= INPUT_DATA_SIZE);

    // convert rgb image to grey image and normalize to 0~1
    for (auto i = 0; i < h; ++i) {
        std::ostringstream stringStream;
        for (auto j = 0; j < w; ++j) {
            int color = img_data[i * w + j];
            //int red = ((color & 0x00FF0000) >> 16);
            //int green = ((color & 0x0000FF00) >> 8);
            //int blue = color & 0x000000FF;
            //float grey = red * 0.3 + green * 0.59 + blue * 0.11;
            float grey = 0.0;
            if (color != 0) {
                grey = 1.0;
            }
            input_data[i * w + j] = grey;
            //alog("%f", grey);
            //alog("%d", color);
            if (color != 0) {
                color = 1;
            }
            stringStream << color << " ";
        }
        //alog("\n");
        alog("%s", stringStream.str().c_str());
    }

    caffe2::TensorCPU input;
    input.Resize(std::vector<int>({1, IMG_C, IMG_H, IMG_W}));
    memcpy(input.mutable_data<float>(), input_data, INPUT_DATA_SIZE * sizeof(float));
    caffe2::Predictor::TensorVector input_vec{&input};
    caffe2::Predictor::TensorVector output_vec;
    _predictor->run(input_vec, &output_vec);

    constexpr int k = 3;
    float max[k] = {0};
    int max_index[k] = {0};
    // Find the top-k result manually
    if (output_vec.capacity() > 0) {
        for (auto output : output_vec) {
            for (auto i = 0; i < output->size(); ++i) {
                for (auto j = 0; j < k; ++j) {
                    if (output->template data<float>()[i] > max[j]) {
                        for (auto _j = k - 1; _j > j; --_j) {
                            max[_j - 1] = max[_j];
                            max_index[_j - 1] = max_index[_j];
                        }
                        max[j] = output->template data<float>()[i];
                        max_index[j] = i;
                        goto skip;
                    }
                }
                skip:;
            }
        }
    }

    std::ostringstream stringStream;
    for (auto j = 0; j < k; ++j) {
        stringStream << max_index[j] << ": " << max[j]*100 << "%\n";
    }

//    if (output_vec.capacity() > 0) {
//        for (auto output: output_vec) {
//            for (auto i = 0;i<output->size();++i) {
//                stringStream << output->template data<float>()[i] << "\n";
//            }
//        }
//    }

    return env->NewStringUTF(stringStream.str().c_str());
}

extern "C" JNIEXPORT jstring

JNICALL
Java_com_example_beyoung_handwrittendigit_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
