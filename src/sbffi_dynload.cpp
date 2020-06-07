#include "sbffi_dynload.hpp"

namespace sbffi {

Value js_dlLoadLibrary(const CallbackInfo& info) {
  std::string libNameStr = info[0].As<String>().Utf8Value();

  const char * libName = libNameStr.c_str();
  DLLib * libPtr = dlLoadLibrary(libName);
  return BigInt::New(info.Env(), (uint64_t)libPtr);
}

void js_dlFreeLibrary(const CallbackInfo& info) {
  bool lossless;
  DLLib * libPtr = (DLLib *)info[0].As<BigInt>().Uint64Value(&lossless);
  dlFreeLibrary(libPtr);
}

Value js_dlFindSymbol(const CallbackInfo& info) {
  bool lossless;
  DLLib * libPtr = (DLLib *)info[0].As<BigInt>().Uint64Value(&lossless);
  std::string symNameStr = info[1].As<String>().Utf8Value();
  const char * symName = symNameStr.c_str();
  void * sym = dlFindSymbol(libPtr, symName);
  return BigInt::New(info.Env(), (uint64_t)sym);
}

}
