#include <cstdint>
#include <napi.h>
#include <dyncall.h>
#include <dynload.h>
#include <dyncall_callback.h>
#include <stdint.h>

#define call_types_except_void(V)\
  V(fn_type_bool, DCbool, dcCallBool, dcArgBool, dcbArgBool, B)\
  V(fn_type_char, DCchar, dcCallChar, dcArgChar, dcbArgChar, c)\
  V(fn_type_short, DCshort, dcCallShort, dcArgShort, dcbArgShort, s)\
  V(fn_type_int, DCint, dcCallInt, dcArgInt, dcbArgInt, i)\
  V(fn_type_long, DClong, dcCallLong, dcArgLong, dcbArgLong, j)\
  V(fn_type_long_long, DClonglong, dcCallLongLong, dcArgLongLong, dcbArgLongLong, l)\
  V(fn_type_float, DCfloat, dcCallFloat, dcArgFloat, dcbArgFloat, f)\
  V(fn_type_double, DCdouble, dcCallDouble, dcArgDouble, dcbArgDouble, d)\
  V(fn_type_pointer, DCpointer, dcCallPointer, dcArgPointer, dcbArgPointer, p)
// TODO struct is missing from this ^

#define callback_types_except_void(V)\
  call_types_except_void(V)\
  V(fn_type_u_char, DCuchar, _, _, dcbArgUChar, C)\
  V(fn_type_u_short, DCushort, _, _, dcbArgUShort, S)\
  V(fn_type_u_int, DCuint, _, _, dcbArgUInt, I)\
  V(fn_type_u_long, DCulong, _, _, dcbArgULong, J)\
  V(fn_type_u_long_long, DCulonglong, _, _, dcbArgULongLong, L)\

using namespace Napi;

namespace Sbffi {
  bool lossless = false;

  // dyncall_disgnature.h
  const char textSigValues[] = "BcsijlfdpvCSIJL";

  typedef enum fn_type {
    fn_type_bool,
    fn_type_char,
    fn_type_short,
    fn_type_int,
    fn_type_long,
    fn_type_long_long,
    fn_type_float,
    fn_type_double,
    fn_type_pointer,
    fn_type_void,
    fn_type_u_char,
    fn_type_u_short,
    fn_type_u_int,
    fn_type_u_long,
    fn_type_u_long_long,
  } fn_type;

  typedef struct fn_sig {
    void * fn;
    fn_type return_type;
    size_t argc;
    fn_type argv[];
  } fn_sig;

  typedef struct cb_sig {
    ThreadSafeFunction func;
    fn_type return_type;
    size_t ret_size;
    size_t args_size;
    size_t argc;
    uint8_t * callBackBuffer;
    fn_type argv[];
  } cb_sig;

  typedef struct cb_data {
    uint8_t * buf;
    size_t len;
    cb_sig * sig;
  } cb_data;

  class SbffiData {
    public:
      DCCallVM * vm;
      uint8_t * callBuffer;
      uint8_t * callBackBuffer;
      SbffiData();
      ~SbffiData();
  };

  class Sbffi : public Addon<Sbffi> {

    public:
      Sbffi(Env env, Object exports) {
        auto sizes = Object::New(env);
#define export_size(typ) sizes.Set(#typ, Number::New(env, sizeof(typ)))
        // These types could in theory be of different sizes on different systems.
        // We get their size here to make it available in JS.
        export_size(char);
        export_size(unsigned char);
        export_size(short);
        export_size(unsigned short);
        export_size(int);
        export_size(unsigned int);
        export_size(long);
        export_size(unsigned long);
        export_size(long long);
        export_size(unsigned long long);
        export_size(size_t);
        export_size(char *);
        export_size(bool);
        export_size(uint8_t);
        export_size(int8_t);
        export_size(uint16_t);
        export_size(int16_t);
        export_size(uint32_t);
        export_size(int32_t);
        export_size(uint64_t);
        export_size(int64_t);
        export_size(float);
        export_size(double);

        DefineAddon(exports, {
          InstanceValue("sizes", sizes, napi_enumerable),

          InstanceMethod("dlLoadLibrary", &Sbffi::JSDlLoadLibrary),
          InstanceMethod("dlFreeLibrary", &Sbffi::JSDlFreeLibrary),
          InstanceMethod("dlFindSymbol", &Sbffi::JSDlFindSymbol),

          InstanceMethod("setCallBuffer", &Sbffi::JSSetCallBuffer),
          InstanceMethod("addSignature", &Sbffi::JSAddSignature),
          InstanceMethod("call", &Sbffi::JSCall),
          InstanceMethod("getBufPtr", &Sbffi::JSGetBufferPtr),

          InstanceMethod("setCallBackBuffer", &Sbffi::JSSetCallBackBuffer),
          InstanceMethod("createCallback", &Sbffi::JSCreateCallback)
        });

        vm = dcNewCallVM(4096);
        dcMode(vm, DC_CALL_C_DEFAULT);
        dcReset(vm);
      }
    private:
      DCCallVM * vm;
      uint8_t * callBuffer;
      uint8_t * callBackBuffer;

      Value JSDlLoadLibrary(const CallbackInfo& info) {
        auto libName = info[0].As<String>().Utf8Value();
        DLLib * lib = dlLoadLibrary(libName.c_str());
        return BigInt::New(info.Env(), (uint64_t)lib);
      }

      Value JSDlFreeLibrary(const CallbackInfo& info) {
        auto lib = (DLLib *)info[0].As<BigInt>().Uint64Value(&lossless);
        dlFreeLibrary(lib);
        return info.Env().Undefined();
      }

      Value JSDlFindSymbol(const CallbackInfo& info) {
        DLLib * lib = NULL;
        if (!info[0].IsNull()) {
          lib = (DLLib *)info[0].As<BigInt>().Uint64Value(&lossless);
        }
        auto sym = dlFindSymbol(lib, info[1].As<String>().Utf8Value().c_str());
        return BigInt::New(info.Env(), (uint64_t)sym);
      }

      Value JSSetCallBuffer(const CallbackInfo& info) {
        callBuffer = info[0].As<Buffer<uint8_t>>().Data();
        return info.Env().Undefined();
      }

      Value JSAddSignature(const CallbackInfo& info) {
        void * fnPtr = (void*)info[0].As<BigInt>().Uint64Value(&lossless);
        uint32_t retTyp = info[1].As<Number>().Uint32Value();
        Array argv = info[2].As<Array>();
        uint32_t fnArgc = argv.Length();

        fn_sig * sig = (fn_sig *)malloc(sizeof(fn_sig) + (sizeof(fn_type) * fnArgc));
        sig->fn = fnPtr;
        sig->return_type = (fn_type)retTyp;
        sig->argc = fnArgc;
        for (uint32_t i = 0; i < fnArgc; i++) {
          sig->argv[i] = (fn_type)argv.Get(i).As<Number>().Uint32Value();
        }

        return BigInt::New(info.Env(), (uint64_t)sig);
      }

#define call_to_buf(_1, dcTyp, dcFn, _4, _5, _6) void call_##dcFn(DCCallVM * vm, DCpointer funcptr) {\
  dcTyp * retVal = (dcTyp*)(callBuffer + 8);\
  *retVal = dcFn(vm, funcptr);\
}
      call_types_except_void(call_to_buf)
      void call_dcCallVoid(DCCallVM * vm, DCpointer funcptr) {
        dcCallVoid(vm, funcptr);
      }

      Value JSCall(const CallbackInfo& info) {
        fn_sig * sig = *(fn_sig **)callBuffer;
        uint8_t * offset = callBuffer + sizeof(fn_sig *);
        void (Sbffi::*callFn)(DCCallVM *, DCpointer);

        switch (sig->return_type) {
          case fn_type_void:
            callFn = &Sbffi::call_dcCallVoid;
            break;
#define js_call_ret_case(enumTyp, typ, callFunc, _4, _5, _6) \
          case enumTyp: \
            callFn = &Sbffi::call_##callFunc;\
            offset += sizeof(typ);\
            break;
          call_types_except_void(js_call_ret_case)
          default:
            throw Error::New(info.Env(), "invalid types in signature");
        }

        for (size_t i = 0; i < sig->argc; i++) {
          fn_type typ = sig->argv[i];
          switch (typ) {
            case fn_type_void:
              throw Error::New(info.Env(), "cannot have void argument");
#define js_call_arg_case(enumTyp, argTyp, _3, argFunc, _5, _6) \
            case enumTyp:\
                         argFunc(vm, *(argTyp*)offset);\
              offset += sizeof(argTyp);\
              break;
              call_types_except_void(js_call_arg_case)
            default:
              throw Error::New(info.Env(), "invalid types in signature");
          }
        }

        (this->*callFn)(vm, (DCpointer)sig->fn);
        dcReset(vm);

        return info.Env().Undefined();
      }

      Value JSGetBufferPtr(const CallbackInfo& info) {
        return BigInt::New(info.Env(), (uint64_t)info[0].As<Buffer<uint8_t *>>().Data());
      }

      Value JSSetCallBackBuffer(const CallbackInfo& info) {
        callBackBuffer = info[0].As<Buffer<uint8_t>>().Data();
        return info.Env().Undefined();
      }


      static char cbHandler(DCCallback* cb, DCArgs* args, DCValue* result, void* userdata) {
        cb_sig * sig = (cb_sig *)userdata;

        // [ ...args, ret]
        uint8_t * tempCallBackBuffer = (uint8_t *)malloc(sig->args_size + sig->ret_size);
        uint8_t * offset = tempCallBackBuffer;

        for (uint32_t i = 0; i < sig->argc; i++) {
          switch (sig->argv[i]) {
            case fn_type_void:
              exit(1); // TODO handle this better
#define js_cb_handler_arg(enumTyp, typ, _3, _4, argFn, _6) \
            case enumTyp:\
              *(typ *)offset = argFn(args);\
              offset = offset + sizeof(typ);\
              break;
            callback_types_except_void(js_cb_handler_arg)
          }
        }

        auto call_js = []( Napi::Env env, Function jsCallback, cb_data * cbData ) {
          cb_sig * sig = cbData->sig;
          uint8_t * callBackBuffer = sig->callBackBuffer;
          uint8_t * tempBuf = cbData->buf;

          memcpy(callBackBuffer, tempBuf, cbData->len);
          Value result = jsCallback.Call( { } );

          if (sig->return_type != fn_type_void) {
            memcpy(cbData->buf + sig->args_size, callBackBuffer + sig->args_size, sig->ret_size);
          }
          free(cbData->buf);
          free(cbData);
          // This release should only actually be done after the _last_
          // invocation of the callback. Because we're doing it like this, it
          // the callback can only be called ONCE.
          sig->func.Release();
        };

        cb_data *data = (cb_data *)malloc(sizeof(cb_data));
        data->buf = tempCallBackBuffer;
        data->len = sig->args_size + sig->ret_size;
        data->sig = sig;
        sig->func.Acquire();
        sig->func.NonBlockingCall(data, call_js);
        sig->func.Release();

        switch(sig->return_type) {
          case fn_type_void:
            return 'v';
#define js_cb_return_type(enumTyp, typ, _3, _4, _5, sym) \
          case enumTyp:\
            result->sym = *(typ *)offset;\
            return textSigValues[enumTyp];
          callback_types_except_void(js_cb_return_type)
        }
      }

      size_t getTypeSize(fn_type typ) {
        switch(typ) {
          case fn_type_void: return 0;
#define get_size_type(enumTyp, typ, _3, _4, _5, _6)\
          case enumTyp: return sizeof(typ);
          callback_types_except_void(get_size_type)
        }
      }

      Value JSCreateCallback(const CallbackInfo& info) {
        uint32_t retTyp = info[0].As<Number>().Uint32Value();
        Array argv = info[1].As<Array>();
        uint32_t fnArgc = argv.Length();

        cb_sig * sig = (cb_sig *)malloc(sizeof(cb_sig) + (sizeof(fn_type) * fnArgc));

        char * textSig = (char *)malloc(sizeof(char) * (fnArgc + 2));
        sig->return_type = (fn_type)retTyp;
        sig->ret_size = getTypeSize((fn_type)retTyp);
        sig->argc = fnArgc;
        sig->args_size = 0;
        sig->callBackBuffer = callBackBuffer;
        for (uint32_t i = 0; i < fnArgc; i++) {
          uint32_t argTyp = argv.Get(i).As<Number>().Uint32Value();
          sig->argv[i] = (fn_type)argTyp;
          sig->args_size += getTypeSize((fn_type)argTyp);
          textSig[i] = textSigValues[argTyp];
        }
        textSig[fnArgc] = ')';
        textSig[fnArgc+1] = textSigValues[retTyp];

        Function cbFunc = info[2].As<Function>();
        sig->func = ThreadSafeFunction::New(info.Env(), cbFunc, "sbffi:callback", 0, 1, sig);

        // TODO Rather than creating a new ThreadSafeFunction per callback and
        // trying to unref them all correctly, try just keeping one alive, i.e.
        // with only one callback alive on the JS side, which then calls the
        // user-supplied callback. Some kind of Callback ID can be stored in
        // cb_sig which is passed around everywhere anyway. Then, we can
        // release the ThreadSafeFunction in the finalizer/destructor for this
        // addon.

        DCCallback * dcCb = dcbNewCallback(textSig, cbHandler, (void *)sig);

        return BigInt::New(info.Env(), (uint64_t)dcCb);
      }
  };

  NODE_API_ADDON(Sbffi)

}
