#include "MyOBS.h"

#include "MyOBSOutput.h"
#include "obs.h"

#include <napi.h>
#include <uv.h>

const int kDefaultAudioSamples = 48000;
const speaker_layout kDefaultAudioSpeakers = SPEAKERS_STEREO;

using namespace Napi;

std::atomic_int MyOBS::RefCount(0);

static void OBSLog(int log_level, const char* msg, va_list args, void* param)
{
    // TODO: Redirect log to JavaScript via callback

    char bla[4096];
    vsnprintf(bla, 4095, msg, args);

    OutputDebugStringA(bla);
    OutputDebugStringA("\n");

    if (log_level < LOG_WARNING)
        __debugbreak();

    UNUSED_PARAMETER(param);
}

Napi::Object MyOBS::Init(Napi::Env env, Napi::Object exports) 
{
    base_set_log_handler(OBSLog, nullptr);

    Napi::Function func = DefineClass(
        env, "MyOBS", { 
            InstanceMethod("addMediaSource", &MyOBS::AddMediaSource),
            InstanceMethod("loadModules", &MyOBS::LoadModules),
            InstanceMethod("resetAudio", &MyOBS::ResetAudio),
            InstanceMethod("resetVideo", &MyOBS::ResetVideo),
            InstanceMethod("setOutputKey", &MyOBS::SetOutputKey),
            InstanceMethod("shutdown", &MyOBS::Shutdown),
            InstanceMethod("startup", &MyOBS::Startup),
            InstanceMethod("startStreaming", &MyOBS::StartStreaming),
            InstanceMethod("stopStreaming", &MyOBS::StopStreaming)
        });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("MyOBS", func);
    return exports;
}

Napi::Object MyOBS::NewInstance(Napi::Env env, Napi::Value arg)
{
    // TODO: How to support multiple instances
    if (RefCount.fetch_add(std::memory_order_relaxed) > 0) {
        NAPI_THROW(Error::New(env, "There can be only one"), Object::New(env));
    }

    Napi::EscapableHandleScope scope(env);
    Napi::Object obj = env.GetInstanceData<Napi::FunctionReference>()->New({ arg });
    return scope.Escape(napi_value(obj)).ToObject();
}

MyOBS::MyOBS(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<MyOBS>(info) 
{
}

MyOBS::~MyOBS()
{
    Shutdown();
}

void MyOBS::AddMediaSource(const Napi::CallbackInfo& info) 
{
    Napi::Env env = info.Env();
    if (info.Length() != 1 || !info[0].IsString()) {
        NAPI_THROW_VOID(Error::New(env, "Invalid Argument"));
    }

    obs_source_t* source = obs_source_create("ffmpeg_source", "Media Source", nullptr, nullptr);
    OBSData nd_settings = obs_source_get_settings(source);
    OBSData settings = obs_data_get_defaults(nd_settings);
    obs_data_apply(settings, nd_settings);

    std::string inputPath = info[0].As<String>();
    obs_data_set_string(settings, "local_file", inputPath.c_str());
    obs_source_update(source, settings);

    obs_data_release(settings);
    obs_data_release(nd_settings);

    rtmpOutput->SetSource(source);
}

void MyOBS::LoadModules(const Napi::CallbackInfo& info) 
{
    obs_load_all_modules();
    obs_log_loaded_modules();

    try {
        rtmpOutput = std::make_unique<MyOBSOutput>();
    }
    catch (const char* e) {
        NAPI_THROW_VOID(Error::New(info.Env(), e));
    }

    return;
}

Napi::Value MyOBS::ResetAudio(const Napi::CallbackInfo& info) 
{
    Napi::Env env = info.Env();
    if (info.Length() == 1 && !info[0].IsObject()) {
        NAPI_THROW(Error::New(env, "Invalid Argument"), Boolean::New(env, false));
    }

    struct obs_audio_info audioInfo;
    memset(&audioInfo, 0, sizeof(audioInfo));
    GetDefaultAudioInfo(audioInfo);

    if (info.Length() == 1) {
        Object audioParams = info[0].ToObject();
        if (audioParams.Has("samples_per_sec"))
            audioInfo.samples_per_sec = static_cast<Napi::Value>(audioParams["samples_per_sec"]).As<Number>();
        if (audioParams.Has("speakers"))    // TODO: Use a JavaScript enum instead of integer value
            audioInfo.speakers = (speaker_layout)static_cast<Napi::Value>(audioParams["speakers"]).As<Number>().Int32Value();
    }

    return Boolean::New(env, obs_reset_audio(&audioInfo));
}

Napi::Value MyOBS::ResetVideo(const Napi::CallbackInfo& info) 
{
  Napi::Env env = info.Env();
  if (info.Length() != 1 || !info[0].IsObject()) {
      NAPI_THROW(Error::New(env, "Invalid Argument"), Boolean::New(env, false));
  }

  struct obs_video_info videoInfo;
  memset(&videoInfo, 0, sizeof(videoInfo));
  GetDefaultVideoInfo(videoInfo);

  Object videoParams = info[0].ToObject();
  if (videoParams.Has("width"))
      videoInfo.output_width = static_cast<Napi::Value>(videoParams["width"]).As<Number>();
  if (videoParams.Has("height"))
      videoInfo.output_width = static_cast<Napi::Value>(videoParams["height"]).ToNumber();

  return Number::New(env, obs_reset_video(&videoInfo));
}

void MyOBS::SetOutputKey(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() != 1 || !info[0].IsString()) {
        NAPI_THROW_VOID(Error::New(env, "Invalid Argument"));
    }

    std::string keyValue = info[0].As<String>();
    rtmpOutput->SetKey(keyValue.c_str());
}

Napi::Value MyOBS::StartStreaming(const Napi::CallbackInfo& info)
{
    if (rtmpOutput)
        return Boolean::New(info.Env(), rtmpOutput->Start());

    return Boolean::New(info.Env(), false);
}

void MyOBS::Shutdown(const Napi::CallbackInfo& info)
{
    Shutdown();
}

Napi::Value MyOBS::Startup(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    std::string locale("en-US");

    if (info.Length() == 1) {
        if (!info[0].IsString()) {
            NAPI_THROW(Error::New(env, "Invalid Argument"), Boolean::New(env, false));
        }

        locale = info[0].As<String>();
    }

    return Boolean::New(env, obs_startup(locale.c_str(), nullptr, nullptr));
}

void MyOBS::StopStreaming(const Napi::CallbackInfo& info)
{
    if (rtmpOutput)
        rtmpOutput->Stop();
}

void MyOBS::GetDefaultAudioInfo(struct obs_audio_info& audioInfo) 
{
    audioInfo.samples_per_sec = kDefaultAudioSamples;
    audioInfo.speakers = kDefaultAudioSpeakers;
}

void MyOBS::GetDefaultVideoInfo(struct obs_video_info& videoInfo) 
{
    videoInfo.graphics_module = "libobs-d3d11";
    videoInfo.fps_num = 30000;
    videoInfo.fps_den = 1000;
    videoInfo.base_width = 1920;
    videoInfo.base_height = 1080;
    videoInfo.output_width = 1280;
    videoInfo.output_height = 720;
    videoInfo.output_format = VIDEO_FORMAT_NV12;
    videoInfo.adapter = 0;
    videoInfo.gpu_conversion = true;
    videoInfo.colorspace = VIDEO_CS_709;
    videoInfo.range = VIDEO_RANGE_PARTIAL;
    videoInfo.scale_type = OBS_SCALE_BICUBIC;
}

void MyOBS::Shutdown()
{
    if (rtmpOutput) {
        rtmpOutput->Stop();
        rtmpOutput.reset(nullptr);
    }

    obs_shutdown();

    blog(LOG_INFO, "Number of memory leaks: %ld", bnum_allocs());
}
