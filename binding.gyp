{
  "targets": [
    {
      "target_name": "sbffi",
      "target_defaults": {},
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "sources": [
        "src/sbffi.cc",
        "deps/dyncall/dyncall/dyncall_api.c",
        "deps/dyncall/dyncall/dyncall_vector.c",
        "deps/dyncall/dyncall/dyncall_callvm.c",
        "deps/dyncall/dyncall/dyncall_callvm_base.c",
        "deps/dyncall/dyncall/dyncall_callf.c",
        "deps/dyncall/dyncall/dyncall_struct.c",
        "deps/dyncall/dynload/dynload.c",
        "deps/dyncall/dynload/dynload_syms.c",
        "deps/dyncall/dyncallback/dyncall_thunk.c",
        "deps/dyncall/dyncallback/dyncall_alloc_wx.c",
        "deps/dyncall/dyncallback/dyncall_args.c",
        "deps/dyncall/dyncallback/dyncall_callback.c",
        ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "deps/dyncall/dyncall",
        "deps/dyncall/dynload",
        "deps/dyncall/dyncallback"
      ],
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ],
      'conditions': [
        ['OS=="win"', {
          'conditions': [
            ['target_arch=="ia32"', {
              'sources': [
                'deps/dyncall/dyncall/dyncall_call_x86_generic_masm.asm',
                'deps/dyncall/dyncallback/dyncall_callback_x86_masm.asm'
              ]
            }, {
              'sources': [
                'deps/dyncall/dyncall/dyncall_call_x64_generic_masm.asm',
                'deps/dyncall/dyncallback/dyncall_callback_x64_masm.asm'
              ]
            }]
          ]
        }, {
          'sources': [
            'deps/dyncall/dyncall/dyncall_call.S',
            'deps/dyncall/dyncallback/dyncall_callback_arch.S'
          ]
        }]
      ]
    }
  ]
}
