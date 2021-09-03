#include "MyOBSOutput.h"

#include "obs.h"

const int kDefaultVideoBitrate = 2500;
const int kDefaultAudioBitrate = 160;

MyOBSOutput::MyOBSOutput()
{
    // Create audio and video encoders
    obs_data_t* h264Settings = nullptr;
    obs_data_t* aacSettings = nullptr;

    if (CreateVideoEncoder()) {
        h264Settings = obs_data_create();
        obs_data_set_int(h264Settings, "bitrate", kDefaultVideoBitrate);
        obs_data_set_string(h264Settings, "rate_control", "CBR");
        obs_data_set_string(h264Settings, "preset", "veryfast");
        obs_data_set_int(h264Settings, "keyint_sec", 2);
        obs_encoder_update(videoStreaming, h264Settings);
    }
    else {
        throw "Failed to create h264 streaming encoder";
    }

    if (CreateAudioEncoder()) {
        aacSettings = obs_data_create();
        obs_data_set_int(aacSettings, "bitrate", 32);
        obs_data_set_string(aacSettings, "rate_control", "CBR");
        obs_data_set_int(aacSettings, "bitrate", kDefaultAudioBitrate);
        obs_encoder_update(audioStreaming, aacSettings);
    }

    // Create Twitch service with server URL + streamkey
    obs_data_t* serviceSettings = obs_data_create();
    obs_data_set_bool(serviceSettings, "bwtest", false);
    obs_data_set_default_string(serviceSettings, "server", "auto");
    obs_data_set_default_string(serviceSettings, "service", "Twitch");

    serviceStreaming = obs_service_create("rtmp_common", "default_service", serviceSettings, nullptr);
    obs_service_release(serviceStreaming);
    obs_data_release(serviceSettings);

    obs_service_apply_encoder_settings(serviceStreaming, h264Settings, aacSettings);
    obs_data_release(h264Settings);
    obs_data_release(aacSettings);

    obs_encoder_set_video(videoStreaming, obs_get_video());
    obs_encoder_set_audio(audioStreaming, obs_get_audio());

    // Create an RTMP output
    const char* type = obs_service_get_output_type(serviceStreaming);
    streamOutput = obs_output_create(type, "simple_stream", nullptr, nullptr);

    if (!streamOutput) {
        blog(LOG_WARNING,
            "Creation of stream output type '%s' "
            "failed!",
            type);
        
        throw "Failed to create output";
    }

    obs_output_release(streamOutput);

    // Set the encoders and the service to the output
    obs_output_set_video_encoder(streamOutput, videoStreaming);
    obs_output_set_audio_encoder(streamOutput, audioStreaming, 0);
    obs_output_set_service(streamOutput, serviceStreaming);
}

void MyOBSOutput::SetKey(const char* key)
{
    obs_data_t* settings = obs_service_get_settings(serviceStreaming);
    obs_data_set_default_string(settings, "key", key);
    obs_service_update(serviceStreaming, settings);
    obs_data_release(settings);
}

void MyOBSOutput::SetSource(obs_source_t* source)
{
    this->source = source;
    obs_source_release(source);
    obs_set_output_source(0, source);
}

bool MyOBSOutput::CreateAudioEncoder()
{
    audioStreaming = obs_audio_encoder_create("ffmpeg_aac", "simple_aac", nullptr, 0, nullptr);

    if (audioStreaming) {
        obs_encoder_release(audioStreaming);
        return true;
    }

    return false;
}

bool MyOBSOutput::CreateVideoEncoder()
{
    videoStreaming = obs_video_encoder_create("obs_x264", "simple_h264_stream", nullptr, nullptr);

    if (videoStreaming) {
        obs_encoder_release(videoStreaming);
        return true;
    }

    return false;
}

bool MyOBSOutput::Start()
{
    return obs_output_start(streamOutput);
}

void MyOBSOutput::Stop()
{
    if (streamOutput)
        obs_output_stop(streamOutput);
}

