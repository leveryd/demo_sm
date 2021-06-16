#include <cassert>
#include <codecvt>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>
#include <sys/time.h>

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

#include "xss_boilerplate.h"
#include <mutex>

// dongguangli add this
extern "C"{
#include <unistd.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
// end

/* This is a longer example that illustrates how to build a simple
 * REPL (Read-Eval-Print Loop). */

/* NOTE: This example assumes that it's okay to print UTF-8 encoded text to
 * stdout and stderr. On Linux and macOS this will usually be the case. On
 * Windows you may have to set your terminal's codepage to UTF-8. */

class ReplGlobal {
    bool m_shouldQuit : 1;

    ReplGlobal(void) : m_shouldQuit(false) {}

    /* The class of the global object. */
    static constexpr JSClass klass = {"ReplGlobal",
        JSCLASS_GLOBAL_FLAGS | JSCLASS_HAS_PRIVATE,
        &JS::DefaultGlobalClassOps};

    //static constexpr JSFunctionSpec functions[];

    public:
    static JSObject* create(JSContext* cx);
    static void loop(JSContext* cx, JS::HandleObject global);
};
constexpr JSClass ReplGlobal::klass;
//constexpr JSFunctionSpec ReplGlobal::functions[];


JSObject* ReplGlobal::create(JSContext* cx) {
    JS::RealmOptions options;
    JS::RootedObject global(cx,
            JS_NewGlobalObject(cx, &ReplGlobal::klass, nullptr,
                JS::FireOnNewGlobalHook, options));

    //ReplGlobal* priv = new ReplGlobal();
    //JS_SetPrivate(global, priv);

    // Define any extra global functions that we want in our environment.
    //JSAutoRealm ar(cx, global);
    //if (!JS_DefineFunctions(cx, global, ReplGlobal::functions)) return nullptr;

    return global;
}


std::once_flag flag3;
JS::Rooted<JS::Value>* Reflect;
JS::Rooted<JSObject*>* ReflectObj;
JS::Rooted<JS::Value>* Reflect_parse;


bool test(JSContext* cx, const char* s, JS::HandleObject global){
    // dongguangli add this
    // define Reflect.parse
    std::call_once(flag3, [](JSContext* cx, JS::HandleObject global){
            if (!JS_InitReflectParse(cx, global)) {
            std::cout << "error--\n" << std::endl;
            }

            // Get Reflect.parse value
            Reflect = new JS::Rooted<JS::Value>(cx);
            if (!JS_GetProperty(cx, global, "Reflect", Reflect)) {
            std::cout << "error1\n" << std::endl;
            }

            //assert Reflect.isObject()

            ReflectObj = new JS::Rooted<JSObject*>(cx, &Reflect->toObject());

            Reflect_parse = new JS::Rooted<JS::Value>(cx);
            if (!JS_GetProperty(cx, *ReflectObj, "parse", Reflect_parse)) {
            std::cout << "error2\n" << std::endl;
            }

            }, cx, global);



    JS::RootedString someString(cx, JS_NewStringCopyZ(cx, s));
    JS::Rooted<JS::Value> arg0(cx);
    arg0.setString(someString);
    JS::Rooted<JS::Value> rval(cx);


    JS::RootedVector<JS::Value> args(cx);
    if (!args.append(arg0)) {
        std::cout << "error4\n" << std::endl;
    }


    if (!JS::Call(cx, *Reflect, *Reflect_parse, args, &rval)) {
        // Either the function throws, or the call fails for some other reason.
        std::cout << "error5\n" << std::endl;
        std::cout << rval.isObject() << std::endl;
    }

    // rval contains the returned value
    if (rval.toBoolean() == true){
        //printf("found xss\n");
        return true;
    }

    // dongguangli end
    return false;
}


std::once_flag flag2;
JS::RootedObject* global;

static bool RunREPL(JSContext* cx, const char* s) {
    // In order to use Promises in the REPL, we need a job queue to process
    // events after each line of input is processed.
    //
    // A more sophisticated embedding would schedule it's own tasks and use
    // JS::SetEnqueuePromiseJobCallback(), JS::SetGetIncumbentGlobalCallback(),
    // and JS::SetPromiseRejectionTrackerCallback().

    //if (!js::UseInternalJobQueues(cx)) return false;

    // We must instantiate self-hosting *after* setting up job queue.
    //if (!JS::InitSelfHostedCode(cx)) return false;

    //std::cerr << "xxxx1\n";
    std::call_once(flag2, [](JSContext* cx){
            //static JS::RootedObject global(cx, ReplGlobal::create(cx));
            global = new JS::RootedObject(cx, ReplGlobal::create(cx));


            JS::SetWarningReporter(cx, [](JSContext* cx, JSErrorReport* report) {
                    JS::PrintError(cx, stderr, report, true);
                    });
            }, cx);
    if (!global){
        //std::cerr << "global create error\n";
        return false;
    }   
    JSAutoRealm ar(cx, *global);

    //std::cerr << "xxxx2\n";

    //std::cerr << "xxxx3\n";

    return test(cx,s, *global);
    //js::RunJobs(cx);

}

void unit_test(){
    #define LEN 1000

    char s[][LEN] = {
        "delete alert(1);",
        "alert(1n)",
        "let:e:alert(1);",
        "yield:-!alert(1)-(1);",
        "(async function*(){})['constructor']('alert(1)')().next();",
        "alert?.(1);",
        "window.alert(1)",
        "function a(){alert();} a();",
        "a=alert;a(1)",
        "1?alert(1):2;",
        "x=1?alert(1):2;x()",
        "document.location=\"javascript:alert(1)\""
    };
    printf("test bad...\n");
    for(int i=0;i<sizeof(s)/LEN;i++){
        if(boilerplate::RunExample(RunREPL, s[i], /* initSelfHosting = */ true)==false){
            printf("%s\n", s[i]);
        }
    }
    boilerplate::Finish();
}

int main(int argc, char* argv[]) {
    int c=0;
    float benchmarkTime=0;
    char* payload;
    char* opt="d:p:th";
    bool test=false;

    while ((c = getopt(argc, (char **)argv, opt)) != -1){
        switch(c){
            case 'd':
                benchmarkTime = std::stof(optarg);
                break;
            case 'p':
                payload = optarg;
                break;
            case 't':
                test = true;
                break;
            case 'h':
            default:
                printf("usage\n");
                exit(0);
        }
    }

    if(test){
        unit_test();
        exit(0);
    }
    if(benchmarkTime > 0){

        printf("benchmark:%f\n", benchmarkTime);

        // test
        float run_time = 0;
        float count = 0;

        auto t_start = time(NULL);
        auto t_end = time(NULL);
        while (false) {
            boilerplate::RunExample(RunREPL,payload, /* initSelfHosting = */ true);
            count++;

            if (argc == 2){
                t_end = time(NULL);
                if (difftime(t_end,t_start) >= run_time){
                    printf("benchmark:%f\n", count);
                    return 0;
                }
            }
        }
        boilerplate::Finish();
        exit(0);
    }

    if (payload){
        printf("payload: %s\n", payload);
        if(boilerplate::RunExample(RunREPL, payload, /* initSelfHosting = */ true)){
            printf("found xss!\n");
        }
        boilerplate::Finish();
        exit(0);
    } 

    //boilerplate::RunExample(RunREPL,"yield:-!console.log()-(1);", /* initSelfHosting = */ true);

    //if (!boilerplate::RunExample(RunREPL,"true", /* initSelfHosting = */ true))
    //  return 1;
    return 0;
}

static int isxss(lua_State *L) {
    const char *s = luaL_checkstring(L, 1);
    if(strcmp(s, "debug")==0) {
        lua_pushboolean(L, true);
    } else {
        if (!boilerplate::RunExample(RunREPL, s, /* initSelfHosting = */ true)){
            lua_pushboolean(L, false);
        } else {
            lua_pushboolean(L, false);
        }
        boilerplate::Finish();
    }
    return 1;
}

// luaopen_add 需要对应着 luaopen_模块名
extern "C" int luaopen_xss(lua_State *L) {
    luaL_Reg luaLoadFun[] = {
        {"isxss", isxss},
        {NULL, NULL}
    };
    // luaL_newlib(L, luaLoadFun);   // Lua5.3版本
    luaL_register(L, "xss", luaLoadFun);  // Lua5.1版本的注册方法
    return 1;
}
