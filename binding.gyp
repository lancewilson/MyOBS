{
  "targets": [
    {
      "target_name": "my-obs-addon",
      "cflags!": [ "-fexceptions" ],
      "cflags_cc!": [ "-fexceptions" ],
      "sources": [ "my-obs-addon.cpp", "MyOBS.cpp", "MyOBSOutput.cpp" ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "include", "../../obs-studio/libobs"
      ],
      "libraries": [
        "../../../obs-studio/build/libobs/Debug/obs.lib"
      ],
      "defines": [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ],
    }
  ]
}
