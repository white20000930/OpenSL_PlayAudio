#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/log.h>
#include <jni.h>
#include <string>
#include <sys/stat.h>

#define LOG_TAG "MyNativeCode"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class AudioOpenslesRenderer {
public:
  AudioOpenslesRenderer() noexcept;
  ~AudioOpenslesRenderer() noexcept = default;

  void Create(int sampleRate, int channels, int bitsPerSample) noexcept;
  void Destroy() noexcept;
  void Play() noexcept;

private:
  void InitializeEngine() noexcept;
  void InitializePlayer(int sampleRate, int channels,
                        int bitsPerSample) noexcept;
  static void BufferQueueCallback(SLAndroidSimpleBufferQueueItf bq,
                                  void *context) noexcept;
  void ProcessAudioBuffer() noexcept;

public:
  FILE *file = nullptr;

private:
  SLObjectItf engineObject = nullptr;
  SLEngineItf engineEngine = nullptr;
  SLObjectItf outputMixObject = nullptr;
  SLObjectItf bqPlayerObject = nullptr;
  SLPlayItf bqPlayerPlay = nullptr;
  SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue = nullptr;
};

AudioOpenslesRenderer::AudioOpenslesRenderer() noexcept {}

void AudioOpenslesRenderer::Create(int sampleRate, int channels,
                                   int bitsPerSample) noexcept {
  InitializeEngine();
  InitializePlayer(sampleRate, channels, bitsPerSample);
  LOGI("Audio renderer created");
}

void AudioOpenslesRenderer::Destroy() noexcept {}
void AudioOpenslesRenderer::InitializeEngine() noexcept {
  SLresult result =
      slCreateEngine(&engineObject, 0, nullptr, 0, nullptr, nullptr);
  if (SL_RESULT_SUCCESS != result) {
    LOGE("slCreateEngine failed: %d", result);
    return;
  }

  result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
  if (SL_RESULT_SUCCESS != result) {
    LOGE("RealizeEngine failed: %d", result);
    return;
  }

  result =
      (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
  if (SL_RESULT_SUCCESS != result) {
    LOGE("EngineGetInterface failed: %d", result);
    return;
  }

  result = (*engineEngine)
               ->CreateOutputMix(engineEngine, &outputMixObject, 0, nullptr,
                                 nullptr);
  if (SL_RESULT_SUCCESS != result) {
    LOGE("CreateOutputMix failed: %d", result);
    return;
  }

  result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
  if (SL_RESULT_SUCCESS != result) {
    LOGE("RealizeOutputMix failed: %d", result);
    return;
  }
}

void AudioOpenslesRenderer::InitializePlayer(int sampleRate, int channels,
                                             int bitsPerSample) noexcept {
  SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {
      SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
  SLDataFormat_PCM format_pcm = {
      SL_DATAFORMAT_PCM,                              // 是pcm格式的
      2,                                              // 两声道
      SL_SAMPLINGRATE_44_1,                           // 每秒的采样率
      SL_PCMSAMPLEFORMAT_FIXED_16,                    // 每个采样的位数
      SL_PCMSAMPLEFORMAT_FIXED_16,                    // 和位数一样就行
      SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, // 立体声(左前和右前)
      SL_BYTEORDER_LITTLEENDIAN,                      // 播放结束的标志
  };

  SLDataSource audioSrc = {&loc_bufq, &format_pcm};
  SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX,
                                        outputMixObject};
  SLDataSink audioSnk = {&loc_outmix, nullptr};

  const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
  const SLboolean req[1] = {SL_BOOLEAN_TRUE};

  SLresult result = (*engineEngine)
                        ->CreateAudioPlayer(engineEngine, &bqPlayerObject,
                                            &audioSrc, &audioSnk, 1, ids, req);
  if (SL_RESULT_SUCCESS != result) {
    LOGE("CreateAudioPlayer failed: %d", result);
    return;
  }

  result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
  if (SL_RESULT_SUCCESS != result) {
    LOGE("RealizeAudioPlayer failed: %d", result);
    return;
  }

  result = (*bqPlayerObject)
               ->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
  if (SL_RESULT_SUCCESS != result) {
    LOGE("GetInterface SL_IID_PLAY failed: %d", result);
    return;
  }

  result = (*bqPlayerObject)
               ->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                              &bqPlayerBufferQueue);
  if (SL_RESULT_SUCCESS != result) {
    LOGE("GetInterface SL_IID_ANDROIDSIMPLEBUFFERQUEUE failed: %d", result);
    return;
  }

  result =
      (*bqPlayerBufferQueue)
          ->RegisterCallback(bqPlayerBufferQueue, BufferQueueCallback, this);
  if (SL_RESULT_SUCCESS != result) {
    LOGE("RegisterCallback failed: %d", result);
    return;
  }
}

void AudioOpenslesRenderer::Play() noexcept {
  if (bqPlayerPlay != nullptr) {
    (*bqPlayerPlay)
        ->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING); // 启动播放
    BufferQueueCallback(bqPlayerBufferQueue, this); // 手动激活回调函数
  }
}

void AudioOpenslesRenderer::BufferQueueCallback(
    SLAndroidSimpleBufferQueueItf bq, void *context) noexcept {
  AudioOpenslesRenderer *renderer =
      static_cast<AudioOpenslesRenderer *>(context);
  renderer->ProcessAudioBuffer();
  LOGI("BufferQueueCallback回调函数");
}

void AudioOpenslesRenderer::ProcessAudioBuffer() noexcept {
  if (file == nullptr)
    return;

  // char *buffer = new char[44100 * 2 * 2]; // 修改：根据音频规格调整缓冲区大小
  char buffer[44100 * 2 * 2];
  size_t bytesRead = fread(buffer, 1, 44100 * 2 * 2, file);
  if (bytesRead > 0) {
    (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, buffer, bytesRead);
    LOGI("Enqueue %d bytes", bytesRead);
  } else {
    LOGI("End of audio stream");
    fclose(file);
    file = nullptr;
  }

  // 获取当前播放位置
  SLmillisecond position;
  (*bqPlayerPlay)->GetPosition(bqPlayerPlay, &position);
  LOGI("当前播放位置：%d", position);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_audiorender_1opensles_MainActivity_PlayAudio(
    JNIEnv *env, jobject /* this */, jstring url) {

  AudioOpenslesRenderer *renderer = new AudioOpenslesRenderer();
  const char *nativeString = env->GetStringUTFChars(url, 0);
  LOGI("url: %s", nativeString);
  renderer->file = fopen(nativeString, "rb");
  if (renderer->file == nullptr) {
    LOGE("Failed to open file");
    return;
  }

  renderer->Create(44100, 2, 16);
  renderer->Play();
  LOGI("Audio playback started");

  env->ReleaseStringUTFChars(url, nativeString);
}
