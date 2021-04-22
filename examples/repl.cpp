#include <cassert>
#include <codecvt>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>

#include <jsapi.h>
#include <jsfriendapi.h>

#include <mozilla/Unused.h>

#include <js/CompilationAndEvaluation.h>
#include <js/Conversions.h>
#include <js/ErrorReport.h>
#include <js/Exception.h>
#include <js/Initialization.h>
#include <js/SourceText.h>
#include <js/Warnings.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "boilerplate.h"

/* This is a longer example that illustrates how to build a simple
 * REPL (Read-Eval-Print Loop). */

/* NOTE: This example assumes that it's okay to print UTF-8 encoded text to
 * stdout and stderr. On Linux and macOS this will usually be the case. On
 * Windows you may have to set your terminal's codepage to UTF-8. */

class ReplGlobal {
  bool m_shouldQuit : 1;

  ReplGlobal(void) : m_shouldQuit(false) {}

  static ReplGlobal* priv(JSObject* global) {
    auto* retval = static_cast<ReplGlobal*>(JS_GetPrivate(global));
    assert(retval);
    return retval;
  }

  static bool quit(JSContext* cx, unsigned argc, JS::Value* vp) {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    JS::RootedObject global(cx, JS::GetNonCCWObjectGlobal(&args.callee()));
    if (!global) return false;

    // Return an "uncatchable" exception, by returning false without setting an
    // exception to be pending. We distinguish it from any other uncatchable
    // that the JS engine might throw, by setting m_shouldQuit
    priv(global)->m_shouldQuit = true;
    js::StopDrainingJobQueue(cx);
    return false;
  }

  /* The class of the global object. */
  static constexpr JSClass klass = {"ReplGlobal",
                                    JSCLASS_GLOBAL_FLAGS | JSCLASS_HAS_PRIVATE,
                                    &JS::DefaultGlobalClassOps};

  static constexpr JSFunctionSpec functions[] = {
      JS_FN("quit", &ReplGlobal::quit, 0, 0), JS_FS_END};

 public:
  static JSObject* create(JSContext* cx);
  static void loop(JSContext* cx, JS::HandleObject global);
};
constexpr JSClass ReplGlobal::klass;
constexpr JSFunctionSpec ReplGlobal::functions[];

std::string FormatString(JSContext* cx, JS::HandleString string) {
  std::string buf = "\"";

  JS::UniqueChars chars(JS_EncodeStringToUTF8(cx, string));
  if (!chars) {
    JS_ClearPendingException(cx);
    return "[invalid string]";
  }

  buf += chars.get();
  buf += '"';
  return buf;
}

std::string FormatResult(JSContext* cx, JS::HandleValue value) {
  JS::RootedString str(cx);

  /* Special case format for strings */
  if (value.isString()) {
    str = value.toString();
    return FormatString(cx, str);
  }

  str = JS::ToString(cx, value);

  if (!str) {
    JS_ClearPendingException(cx);
    str = JS_ValueToSource(cx, value);
  }

  if (!str) {
    JS_ClearPendingException(cx);
    if (value.isObject()) {
      const JSClass* klass = JS_GetClass(&value.toObject());
      if (klass)
        str = JS_NewStringCopyZ(cx, klass->name);
      else
        return "[unknown object]";
    } else {
      return "[unknown non-object]";
    }
  }

  if (!str) {
    JS_ClearPendingException(cx);
    return "[invalid class]";
  }

//// dongguangli add this
///*
//JS::AutoValueArray<1> parseParams(cx);
//parseParams[0].setString(str);
//JS::AutoValueArray<3> vp(context);
//vp[2].set(*parseParams.begin());
//*/
//JS::RootedValueArray<1> parseParams(cx);
//parseParams[0].setString(str);
//JS::RootedValueArray<3> vp(cx);
//vp[2].set(*parseParams.begin());
//
//
////JS::Value *vp;
//reflect_parse(cx, parseParams.length(), vp.begin());
//
//JS::CallArgs args = JS::CallArgsFromVp(1, vp.begin());
//
//auto x = args.rval();
////JS::HandleValue y=  x;

//JS::RootedValue x = vp[0];
//printf("%s\n",typeid(x).name());
//JS_ReportOutOfMemory(cx);



  JS::UniqueChars bytes(JS_EncodeStringToUTF8(cx, str));
  if (!bytes) {
    JS_ClearPendingException(cx);
    return "[invalid string]";
  }

  return bytes.get();
}

JSObject* ReplGlobal::create(JSContext* cx) {
  JS::RealmOptions options;
  JS::RootedObject global(cx,
                          JS_NewGlobalObject(cx, &ReplGlobal::klass, nullptr,
                                             JS::FireOnNewGlobalHook, options));

  ReplGlobal* priv = new ReplGlobal();
  JS_SetPrivate(global, priv);

  // Define any extra global functions that we want in our environment.
  JSAutoRealm ar(cx, global);
  if (!JS_DefineFunctions(cx, global, ReplGlobal::functions)) return nullptr;

  return global;
}


bool test(JSContext* cx, const std::string& buffer, JS::HandleObject global){
// dongguangli add this
// define Reflect.parse
if (!JS_InitReflectParse(cx, global)) {
  std::cout << "error--\n" << std::endl;
}

// Get Reflect.parse value
JS::Rooted<JS::Value> Reflect(cx);
if (!JS_GetProperty(cx, global, "Reflect", &Reflect)) {
  std::cout << "error1\n" << std::endl;
}

//assert Reflect.isObject()

JS::Rooted<JSObject*> ReflectObj(cx, &Reflect.toObject());
JS::Rooted<JS::Value> Reflect_parse(cx);
if (!JS_GetProperty(cx, ReflectObj, "parse", &Reflect_parse)) {
  std::cout << "error2\n" << std::endl;
}

//TODO:
JS::Rooted<JS::Value> thisValue(cx);
//if (!args.computeThis(cx, &thisValue)){
//  std::cout << "error3\n" << std::endl;
//} 

//JS::Rooted<JS::Value> functionValue(cx, {callee function here});
const char *c = buffer.c_str();
JS::RootedString someString(cx, JS_NewStringCopyZ(cx, c));
JS::Rooted<JS::Value> arg0(cx);
arg0.setString(someString);
JS::Rooted<JS::Value> rval(cx);

//JS::Rooted<JSObject*> functionObject(cx, &functionValue.toObject());

JS::RootedVector<JS::Value> args(cx);
if (!args.append(arg0)) {
  std::cout << "error4\n" << std::endl;
}

if (!JS::Call(cx, Reflect, Reflect_parse, args, &rval)) {
  // Either the function throws, or the call fails for some other reason.
  std::cout << "error5\n" << std::endl;
  std::cout << rval.isObject() << std::endl;
}

// rval contains the returned value

// dongguangli end
}

bool EvalAndPrint(JSContext* cx, const std::string& buffer, unsigned lineno) {

  JS::CompileOptions options(cx);
  options.setFileAndLine("typein", lineno);

  JS::SourceText<mozilla::Utf8Unit> source;
  if (!source.init(cx, buffer.c_str(), buffer.size(),
                   JS::SourceOwnership::Borrowed)) {
    return false;
  }

  JS::RootedValue result(cx);
  if (!JS::Evaluate(cx, options, source, &result)) return false;

  JS_MaybeGC(cx);

  if (result.isUndefined()) return true;

  std::string display_str = FormatResult(cx, result);
  if (!display_str.empty()) std::cout << display_str << '\n';
  return true;
}

void ReplGlobal::loop(JSContext* cx, JS::HandleObject global) {
  bool eof = false;
  unsigned lineno = 1;
  do {
    // Accumulate lines until we get a 'compilable unit' - one that either
    // generates an error (before running out of source) or that compiles
    // cleanly.  This should be whenever we get a complete statement that
    // coincides with the end of a line.
    unsigned startline = lineno;
    std::string buffer;

    do {
      const char* prompt = startline == lineno ? "js> " : "... ";
      char* line = readline(prompt);
      if (!line) {
        eof = true;
        break;
      }
      if (line[0] != '\0') add_history(line);
      buffer += line;
      lineno++;
    } while (!JS_Utf8BufferIsCompilableUnit(cx, global, buffer.c_str(),
                                            buffer.length()));

    // dongguangli add this
    test(cx, buffer, global);
    // dongguangli

    if (!EvalAndPrint(cx, buffer, startline)) {
      if (!priv(global)->m_shouldQuit) {
        boilerplate::ReportAndClearException(cx);
      }
    }

    js::RunJobs(cx);
  } while (!eof && !priv(global)->m_shouldQuit);
}

static bool RunREPL(JSContext* cx) {
  // In order to use Promises in the REPL, we need a job queue to process
  // events after each line of input is processed.
  //
  // A more sophisticated embedding would schedule it's own tasks and use
  // JS::SetEnqueuePromiseJobCallback(), JS::SetGetIncumbentGlobalCallback(),
  // and JS::SetPromiseRejectionTrackerCallback().
  if (!js::UseInternalJobQueues(cx)) return false;

  // We must instantiate self-hosting *after* setting up job queue.
  if (!JS::InitSelfHostedCode(cx)) return false;

  JS::RootedObject global(cx, ReplGlobal::create(cx));
  if (!global) return false;

  JSAutoRealm ar(cx, global);

  JS::SetWarningReporter(cx, [](JSContext* cx, JSErrorReport* report) {
    JS::PrintError(cx, stderr, report, true);
  });

  ReplGlobal::loop(cx, global);

  std::cout << '\n';
  return true;
}

int main(int argc, const char* argv[]) {
  if (!boilerplate::RunExample(RunREPL, /* initSelfHosting = */ false))
    return 1;
  return 0;
}
