{
  "targets": [
    {
      "target_name": "nabto_device",
      "include_dirs": [
        "<!(node -p \"require('node-addon-api').include_dir\")",
        "./native_libraries/include"
      ],
      "sources": [ "native_code/nabto_device.cc",
                  "native_code/node_nabto_device.cc",
                ],
      'link_settings': {
        'libraries': [
            '-lnabto_device',
        ],
        'library_dirs': [
            '../native_libraries/lib/linux-x86_64',
        ]
      },
      "copies":[
            {
                'destination': './build/Release',
                'files':[
                    './native_libraries/lib/linux-x86_64/libnabto_device.so',
                    './native_libraries/lib/linux-x86_64/libnabto_client.so'
                ]
            }
      ],
      "ldflags": [
        "-Wl,-rpath,./native_libraries/lib/linux-x86_64"
      ],
       'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ]
    }
  ]
}
