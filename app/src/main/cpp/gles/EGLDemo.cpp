//
// Created by Administrator on 2018/5/30 0030.
//

#include <jni.h>
#include <android/log.h>
#include <pthread.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "EGLDemo.h"
#include "Triangle.h"
#include "Circle.h"
#include "Square.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#define LOG_TAG "Shape"
#define LOGI(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

void *startThreadCallback(void *arg);

EGLDemo::EGLDemo(ANativeWindow *window) : mWindow(window), mEGLCore(new EGLCore()), mStartThread(0),
                                      mIsRendering(false) {
    pthread_mutex_init(&mMutex, nullptr);
    pthread_cond_init(&mCondition, nullptr);
}

void EGLDemo::start() {
    if (mWindow == nullptr || mWidth == 0 || mHeight == 0) {
        LOGE("not configured, cannot start");
        return;
    }
    pthread_create(&mStartThread, nullptr, startThreadCallback, (void *) this);
}

void *startThreadCallback(void *arg) {
    EGLDemo *shape = (EGLDemo *) arg;
    if (shape->doInit()) {
        shape->renderLoop();
        shape->doStop();
    }
    return 0;
}

bool EGLDemo::doInit() {
    if (!mEGLCore->buildContext(mWindow)) {
        LOGE("buildContext failed");
        return false;
    }

    return true;
}

void EGLDemo::renderLoop() {
    mIsRendering = true;
    LOGI("renderLoop started");
    while (mIsRendering) {
        pthread_mutex_lock(&mMutex);

        doDraw();
        pthread_cond_wait(&mCondition, &mMutex);

        pthread_mutex_unlock(&mMutex);
    }
    LOGI("renderLoop ended");
}

void EGLDemo::doStop() {
    glDeleteProgram(mProgram);
    mEGLCore->release();
}

void EGLDemo::draw() {
    pthread_mutex_lock(&mMutex);
    mIsRendering = true;
    pthread_cond_signal(&mCondition);
    pthread_mutex_unlock(&mMutex);
}

void EGLDemo::stop() {
    pthread_mutex_lock(&mMutex);
    mIsRendering = false;
    pthread_cond_signal(&mCondition);
    pthread_mutex_unlock(&mMutex);

    pthread_join(mStartThread, 0);
}

EGLDemo::~EGLDemo() {
    pthread_mutex_destroy(&mMutex);
    pthread_cond_destroy(&mCondition);

    if (mWindow) {
        ANativeWindow_release(mWindow);
        mWindow = nullptr;
    }

    if (mEGLCore) {
        delete mEGLCore;
        mEGLCore = nullptr;
    }
}

EGLDemo *shape = nullptr;

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_ShapeActivity__1init(JNIEnv *env, jclass type,
                                                              jobject surface, jint width,
                                                              jint height, jint shapeType) {
    if (shape) {
        delete shape;
    }
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    shape = new Square(window);
    shape->resize(width, height);
    shape->start();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_ShapeActivity__1draw(JNIEnv *env, jclass type) {
    if (shape == nullptr) {
        LOGE("draw error, shape is null");
        return;
    }
    shape->draw();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_steven_avgraphics_activity_gles_ShapeActivity__1release(JNIEnv *env, jclass type) {
    if (shape) {
        shape->stop();
        delete shape;
        shape = nullptr;
    }
}
#pragma clang diagnostic pop