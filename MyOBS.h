#ifndef MYOBJECT_H
#define MYOBJECT_H

#include <napi.h>
#include <atomic>

struct obs_video_info;

class MyOBSOutput;

class MyOBS : public Napi::ObjectWrap<MyOBS> {
	static std::atomic_int RefCount;
	std::unique_ptr<MyOBSOutput> rtmpOutput;

public:
	static Napi::Object Init(Napi::Env env, Napi::Object exports);
	static Napi::Object NewInstance(Napi::Env env, Napi::Value arg);

	MyOBS(const Napi::CallbackInfo& info);
	~MyOBS();

 private:

	 void AddMediaSource(const Napi::CallbackInfo& info);
	 void LoadModules(const Napi::CallbackInfo& info);
	 Napi::Value ResetAudio(const Napi::CallbackInfo& info);
	 Napi::Value ResetVideo(const Napi::CallbackInfo& info);
	 void SetOutputKey(const Napi::CallbackInfo& info);
	 void Shutdown(const Napi::CallbackInfo& info);
	 Napi::Value Startup(const Napi::CallbackInfo& info);
	 Napi::Value StartStreaming(const Napi::CallbackInfo& info);
	 void StopStreaming(const Napi::CallbackInfo& info);

	 void GetDefaultAudioInfo(struct obs_audio_info& audioInfo);
	 void GetDefaultVideoInfo(struct obs_video_info& videoInfo);
	 void Shutdown();
};

#endif
