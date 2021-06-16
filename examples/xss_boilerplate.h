#include <jsapi.h>

// See 'boilerplate.cpp' for documentation.

namespace boilerplate {

extern const JSClassOps DefaultGlobalClassOps;

JSObject* CreateGlobal(JSContext* cx);

void ReportAndClearException(JSContext* cx);

bool RunExample(bool (*task)(JSContext*,const char*),const char* s, bool initSelfHosting);
void Finish();

}  // namespace boilerplate
