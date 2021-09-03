#pragma once
#include "obs.hpp"

class MyOBSOutput {
    OBSService serviceStreaming;
    OBSEncoder audioStreaming;
    OBSEncoder videoStreaming;
    OBSOutput streamOutput;
    OBSSource source;

public:
    MyOBSOutput();
    ~MyOBSOutput() {}

    void SetKey(const char* key);
    void SetSource(obs_source_t* source);
    bool Start();
    void Stop();

private:
    bool CreateAudioEncoder();
    bool CreateVideoEncoder();
};

