{
  "targets": [
    {
      "target_name": "sbffi",
      "target_defaults": {},
      // "cflags!": [ "-fno-exceptions" ],
      // "cflags_cc!": [ "-fno-exceptions" ],
      "sources": [ "src/sbffi.cc" ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ],
    }
  ]
}
