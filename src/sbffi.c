#include <stdint.h>
#include <string.h>
#include <node_api.h>
#include <dlfcn.h>
#include <libtcc.h>
#include "sbffi_common.h"

#define tcc_dir CMAKE_CURRENT_SOURCE_DIR
#define str(x) #x

uint8_t * callBuffer;

char * argList [16] = {
  "arg0",
  "arg1",
  "arg2",
  "arg3",
  "arg4",
  "arg5",
  "arg6",
  "arg7",
  "arg8",
  "arg9",
  "arg10",
  "arg11",
  "arg12",
  "arg13",
  "arg14",
  "arg15"
};

void createFunctionString(char * dest, char * fnName, char * retType, uint32_t fnArgc, char ** fnArgv) {
  strcat(dest, "#include <stdint.h>\n");
  strcat(dest, retType);
  strcat(dest, " ");
  strcat(dest, fnName);
  strcat(dest, "(");
  for (uint32_t i = 0; i < fnArgc; i++) {
    strcat(dest, fnArgv[i]);
    strcat(dest, " ");
    strcat(dest, argList[i]);
    if (i != fnArgc - 1) {
      strcat(dest, ",");
    }
  }
  strcat(dest, ");\n");
  strcat(dest,
      "void sbffi_fn_call_"
      );
  strcat(dest, fnName);
  strcat(dest,
      "(uint8_t * callBuffer){\n"
      "uint8_t * cursor = callBuffer + sizeof("
  );
  strcat(dest, retType);
  strcat(dest, ");\n");
  for (uint32_t i = 0; i < fnArgc; i++) {
    strcat(dest, fnArgv[i]);
    strcat(dest, " ");
    strcat(dest, argList[i]);
    strcat(dest, " = *(");
    strcat(dest, fnArgv[i]);
    strcat(dest,
        " *)cursor;\n"
        "cursor += sizeof("
    );
    strcat(dest, fnArgv[i]);
    strcat(dest, ");\n");
  }
  strcat(dest, retType);
  strcat(dest, "* retVal = (");
  strcat(dest, retType);
  strcat(dest,
      "*)callBuffer;\n"
      "*retVal = "
  );
  strcat(dest, fnName);
  strcat(dest, "(");
  for (uint32_t i = 0; i < fnArgc; i++) {
    strcat(dest, argList[i]);
    if (i != fnArgc - 1) {
      strcat(dest, ",");
    }
  }
  strcat(dest, ");\n}");
}

uint8_t * createFunction(char * fnString, char * fnName, void * fnPtr) {
  TCCState * tccState = tcc_new();
  tcc_set_lib_path(tccState, TCC_DIR);
  tcc_set_output_type(tccState, TCC_OUTPUT_MEMORY);
  tcc_compile_string(tccState, fnString);
  tcc_add_symbol(tccState, fnName, fnPtr);
  void * mem = malloc(tcc_relocate(tccState, NULL));
  tcc_relocate(tccState, mem);
  char * symName = (char *)calloc(15 + strlen(fnName), sizeof(char));
  strcat(symName, "sbffi_fn_call_");
  strcat(symName, fnName);
  uint8_t * retVal = (uint8_t *)tcc_get_symbol(tccState, symName);
  free(symName);
  tcc_delete(tccState);
  return retVal;
}

napi_value js_setCallBuffer(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_get_args(1);
  size_t len;
  napi_call(napi_get_buffer_info(env, args[0], (void **)&callBuffer, &len));
  return NULL;
}

// createFunction(fnName, fnPtr, retType, [...argTypes])
napi_value js_createFunction(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_get_args(4);
  napi_get_string(fnName, args[0], 256);
  void * fnPtr;
  napi_get_ptr(fnPtr,args[1])
  napi_get_string(retTyp, args[2], 256)
  uint32_t fnArgc;
  napi_call(napi_get_array_length(env, args[3], &fnArgc));

  char ** fnArgv = (char **)malloc(sizeof(char *) * fnArgc);

  for (uint32_t i = 0; i < fnArgc; i++) {
    napi_value key;
    napi_call(napi_create_uint32(env, i, &key));
    napi_value val;
    napi_call(napi_get_property(env, args[3], key, &val));
    napi_get_string(argTyp, val, 256);
    fnArgv[i] = (char *)malloc(sizeof(char) * strlen(argTyp));
    memcpy(fnArgv[i], argTyp, strlen(argTyp));
  }

  char * fnSource = (char *)calloc(4096, sizeof(char));
  createFunctionString(fnSource, fnName, retTyp, fnArgc, fnArgv);
  for (uint32_t i = 0; i < fnArgc; i++) {
    free(fnArgv[i]);
  }
  free(fnArgv);

  uint8_t * newFnPtr = createFunction(fnSource, fnName, fnPtr);
  free(fnSource);

  napi_return_uint64((uint64_t)newFnPtr);
}

napi_value js_call(napi_env env, napi_callback_info info) {
  void * fnPtr = *(void**)callBuffer;
  void (*fnToCall)(uint8_t *) = (void (*)(uint8_t *))fnPtr;
  fnToCall(callBuffer + 8);
  return NULL;
}

napi_value js_loadLibrary(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_get_args(1)
  napi_get_string(libName, args[0], 256)
  void * libPtr = dlopen(libName, RTLD_NOW);
  napi_return_uint64((uint64_t)libPtr);
}

napi_value js_findSymbol(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_get_args(2)
  void * libPtr;
  napi_get_ptr(libPtr,args[0])
  napi_get_string(symName, args[1], 256)
  void * sym = dlsym(libPtr, symName);
  napi_return_uint64((uint64_t)sym)
}

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;
#define export_method(name, fn) {\
  napi_property_descriptor desc = { name, 0, fn, 0, 0, 0, napi_enumerable, 0 };\
  napi_call(napi_define_properties(env, exports, 1, &desc));\
  }

  export_method("call", js_call);
  export_method("setCallBuffer", js_setCallBuffer);
  export_method("createFunction", js_createFunction);
  export_method("loadLibrary", js_loadLibrary);
  export_method("findSymbol", js_findSymbol);

  napi_value sizes;
  napi_call(napi_create_object(env, &sizes));
#define export_size(typ) {\
  napi_value sizeof_val;\
  napi_call(napi_create_uint32(env, sizeof(typ), &sizeof_val));\
  napi_call(napi_set_named_property(env, sizes, #typ, sizeof_val));\
  }
  export_size(char)
  export_size(short)
  export_size(int)
  export_size(long)
  export_size(long long)
  export_size(size_t)
  export_size(char *)

  napi_call(napi_set_named_property(env, exports, "sizes", sizes));

  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)

