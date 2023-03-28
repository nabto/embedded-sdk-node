{
  "variables": {
    "libPath": "<!(pwd)/build/Release"
  },
  "targets": [
    {
      "target_name": "nabto_device",
      "include_dirs": [
        "<!(node -p \"require('node-addon-api').include_dir\")",
        "./native_libraries/include"
      ],
      "sources": [ "native_code/nabto_device.cc",
                  "native_code/node_nabto_device.cc",
                  "native_code/coap.cc",
                ],
      'link_settings': {
        'libraries': [
            '-lnabto_device',
        ],
        'library_dirs': [
          '<@(libPath)'
        ]
      },
      "copies":[
            {
                'destination': './build/Release',
                'files':[
                    './native_libraries/lib/linux-x86_64/libnabto_device.so',
                ]
            }
      ],
      "ldflags": [
        "-Wl,-rpath,<@(libPath)"
      ],
       'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ]
    }
  ]
}
