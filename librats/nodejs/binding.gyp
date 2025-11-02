{
  "targets": [
    {
      "target_name": "librats",
      "sources": [
        "src/librats_node.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../src",
        "../build2/src"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "xcode_settings": {
        "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
        "CLANG_CXX_LIBRARY": "libc++",
        "MACOSX_DEPLOYMENT_TARGET": "10.7"
      },
      "msvs_settings": {
        "VCCLCompilerTool": { "ExceptionHandling": 1 }
      },
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ],
      "conditions": [
        ["OS=='win'", {
          "libraries": [
            "../build2/lib/Debug/rats.lib",
            "ws2_32.lib",
            "iphlpapi.lib",
            "bcrypt.lib"
          ],
          "defines": [
            "WIN32_LEAN_AND_MEAN",
            "_WIN32_WINNT=0x0600"
          ]
        }],
        ["OS=='linux'", {
          "libraries": [
            "../build2/lib/librats.a"
          ],
          "libraries!": [
            "-undefined dynamic_lookup"
          ],
          "link_settings": {
            "libraries": [
              "-lpthread"
            ]
          }
        }],
        ["OS=='mac'", {
          "libraries": [
            "../build2/lib/librats.a"
          ],
          "link_settings": {
            "libraries": [
              "-lpthread"
            ]
          }
        }]
      ]
    }
  ]
}
