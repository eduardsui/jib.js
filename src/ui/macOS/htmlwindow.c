#include <CoreFoundation/CoreFoundation.h>
#include <objc/objc.h>
#include <objc/objc-runtime.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <objc/NSObjCRuntime.h>
#define _DARWIN_BETTER_REALPATH
#include <mach-o/dyld.h>
#include "../htmlwindow.h"

#define objc_msgSend    ((id (*)(id, SEL, ...))objc_msgSend)

static id pool;
static id app;

Class AppDelClass;
Class WebViewClass;

static ui_idle_event idle_event = NULL;
static void *idle_userdata = NULL;
static ui_trigger_event callback_event = NULL;
static ui_tray_event event_tray_event = NULL;
static int window_count = 0;
static int content_set = 0;
static int gui_lock = 0;
static ui_event ui_callbacks[UI_EVENTS];
static void *ui_data[UI_EVENTS];
static ui_idle_event scheduled_event[UI_SCHEDULER_SIZE];
static void *scheduled_data[UI_SCHEDULER_SIZE];
static int schedule_count = 0;

void app_run_iterate_once(id app);

enum {
    NSBorderlessWindowMask      = 0,
    NSTitledWindowMask          = 1 << 0,
    NSClosableWindowMask        = 1 << 1,
    NSMiniaturizableWindowMask  = 1 << 2,
    NSResizableWindowMask       = 1 << 3,
};

typedef struct edworkPoint {
    double x;
    double y;
} edworkPoint;

typedef struct edworkSize {
    double width;
    double height;
} edworkSize;

typedef struct edworkRect {
    edworkPoint origin;
    edworkSize size;
} edworkRect;

typedef struct AppDel {
    Class isa;    
    id window;
} AppDelegate;

id get_base_url() {
    char path[1032];
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    memset(path, 0, sizeof(buffer));
    memcpy(path, "file://", 7);
    uint32_t size = 1024;
    _NSGetExecutablePath(buffer, &size);
    if (!realpath(buffer, path + 7))
        memcpy(path + 7, buffer, size);

    int len = strlen(path) - 1;
    while (path[len] != '/')
        len --;
    path[len + 1] = 0;

    CFStringRef base_str = CFStringCreateWithCString(NULL, path, kCFStringEncodingMacRoman);

    id baseURL = objc_msgSend((id)objc_getClass("NSURL"), sel_getUid("URLWithString:"), base_str);
    if (base_str)
        CFRelease(base_str);
    return baseURL;
}

void *ui_window(const char *title, const char *body) {
    id window = objc_msgSend((id)objc_getClass("NSWindow"), sel_getUid("alloc"));
    window = objc_msgSend(window, sel_getUid("initWithContentRect:styleMask:backing:defer:"), (edworkRect){0, 0, 1200, 750}, (NSTitledWindowMask | NSClosableWindowMask | NSResizableWindowMask | NSMiniaturizableWindowMask), 0, false);
    objc_msgSend(window, sel_getUid("center"));

    if (title) {
        CFStringRef title_str = CFStringCreateWithCString(NULL, title, kCFStringEncodingMacRoman);
        objc_msgSend(window, sel_getUid("setTitle:"), title_str);
        // if (title_str)
        //     CFRelease(title_str);
    }
    objc_msgSend(window, sel_getUid("setDelegate:"), objc_msgSend(app, sel_getUid("delegate")));
    
    id view = objc_msgSend(objc_msgSend((id)objc_getClass("WKWebView"), sel_getUid("alloc")), sel_getUid("init"));

    objc_msgSend(view, sel_getUid("setNavigationDelegate:"), objc_msgSend(app, sel_getUid("delegate")));
    
    objc_msgSend(view, sel_getUid("setAllowsBackForwardNavigationGestures:"), TRUE);

    if (body) {
        int len = strlen(body);
        const char script[] = "<script>window.external={\"notify\":function(){window.location.href=\"ui:event\";}};</script>";
        char *body_with_script = (char *)malloc(len + sizeof(script) + 1);
        if (body_with_script) {
            memcpy(body_with_script, body, len);
            memcpy(body_with_script + len, script, sizeof(script));
            // already null terminated ... kind of useless
            body_with_script[len + sizeof(script)] = 0;
            CFStringRef body_str = CFStringCreateWithCString(NULL, body_with_script, kCFStringEncodingMacRoman);
            
            id baseURL = get_base_url();
            objc_msgSend(view, sel_getUid("loadHTMLString:baseURL:"), body_str, baseURL);
            content_set ++;
            // if (body_str)
            //     CFRelease(body_str);
            // if (baseURL)
            //     CFRelease(baseURL);

            free(body_with_script);
        }
    }
    objc_msgSend(window, sel_getUid("setContentView:"), view);
    objc_msgSend(window, sel_getUid("becomeFirstResponder"));
    objc_msgSend(window, sel_getUid("makeKeyAndOrderFront:"), objc_msgSend(app, sel_getUid("delegate")));

    window_count ++;

    if (ui_callbacks[UI_EVENT_WINDOW_CREATE])
        ui_callbacks[UI_EVENT_WINDOW_CREATE](window, ui_data[UI_EVENT_WINDOW_CREATE]);

    return window;
}

void ui_window_set_content(void *window, const char *body) {
    CFStringRef body_str = CFStringCreateWithCString(NULL, body ? body : "", kCFStringEncodingMacRoman);
    id baseURL = get_base_url();

    objc_msgSend(objc_msgSend(window, sel_getUid("contentView")), sel_getUid("loadHTMLString:baseURL:"), body_str, baseURL);
    content_set ++;
    // if (baseURL)
    //     CFRelease(baseURL);
    // if (body_str)
    //     CFRelease(body_str);
}

void ui_window_close(void *window) {
    if (!window)
        return;

    objc_msgSend(window, sel_getUid("close"));
}

void ui_window_maximize(void *window) {
    if (!window)
        return;

    objc_msgSend(window, sel_getUid("performZoom:"), NULL);
}

void ui_window_minimize(void *window) {
    if (!window)
        return;

    objc_msgSend(window, sel_getUid("miniaturize:"), NULL);
}

void ui_window_restore(void *window) {
    if (!window)
        return;

    objc_msgSend(window, sel_getUid("deminiaturize:"), NULL);
}

void ui_window_top(void *window) {
    if (app)
        objc_msgSend(app, sel_getUid("activateIgnoringOtherApps:"), YES);
    if (window)
        objc_msgSend(window, sel_getUid("setCollectionBehavior:"), 1 << 0);
}

BOOL AppDel_applicationDidUpdate(AppDelegate *self, SEL _cmd, id notification) {
    if (idle_event)
        idle_event(idle_userdata);

    if (schedule_count > 0) {
        int i;
        for (i = 0; i < UI_SCHEDULER_SIZE; i ++) {
            if (scheduled_event[i]) {
                scheduled_event[i](scheduled_data[i]);
                scheduled_event[i] = NULL;
                scheduled_data[i] = NULL;
            } else
                break;
        }
        schedule_count = 0;
    }
    return YES;
}

BOOL AppDel_didFinishLaunching(AppDelegate *self, SEL _cmd, id notification) {
    return YES;
}

BOOL AppDel_willTerminate(AppDelegate *self, SEL _cmd, id notification) {
    app = NULL;
    if (ui_callbacks[UI_EVENT_LOOP_EXIT])
        ui_callbacks[UI_EVENT_LOOP_EXIT](NULL, ui_data[UI_EVENT_LOOP_EXIT]);
    return YES;
}

BOOL AppDel_applicationShouldHandleReopen(AppDelegate *self, SEL _cmd, ...) {
    if (event_tray_event) {
        event_tray_event(NULL);
        return NO;
    }
    return YES;
}

void windowWillClose(id self, SEL _sel, id notification) {
    // avoid double call (not sure why this is called twice)
    objc_msgSend(objc_msgSend(notification, sel_getUid("object")), sel_getUid("setDelegate:"), NULL);
    if (ui_callbacks[UI_EVENT_WINDOW_CLOSE])
        ui_callbacks[UI_EVENT_WINDOW_CLOSE](objc_msgSend(notification, sel_getUid("object")), ui_data[UI_EVENT_WINDOW_CLOSE]);
    if (window_count)
        window_count --;
    if ((window_count <= 0) && (!gui_lock))
        ui_app_quit();
}

char *CFStringCopyUTF8String(CFStringRef aString) {
    if (aString == NULL)
        return NULL;

    CFIndex length = CFStringGetLength(aString);
    CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
    char *buffer = (char *)malloc(maxSize);
    if (CFStringGetCString(aString, buffer, maxSize, kCFStringEncodingUTF8))
        return buffer;
    free(buffer); // If we failed
    return NULL;
}

void WebView_decidePolicyFor(id self, SEL _cmd, id webView, id response, void (^decisionHandler)(int)) {
    char *url = CFStringCopyUTF8String((CFStringRef)objc_msgSend(objc_msgSend(objc_msgSend(response, sel_getUid("request")), sel_getUid("URL")), sel_getUid("scheme")));
    if ((url) && (!strcmp(url, "ui"))) {
        if (callback_event)
            callback_event(objc_msgSend(webView, sel_getUid("window")));
        decisionHandler(0);
        ui_free_string(url);
        return;
    }

    ui_free_string(url);
    decisionHandler(1);
}

static void CreateAppDelegate() {
    AppDelClass = objc_allocateClassPair((Class)objc_getClass("NSObject"), "AppDelegate", 0);
    class_addMethod(AppDelClass, sel_getUid("applicationDidFinishLaunching:"), (IMP)AppDel_didFinishLaunching, "i@:@");
    class_addMethod(AppDelClass, sel_getUid("applicationWillTerminate:"), (IMP)AppDel_willTerminate, "i@:@");
    class_addMethod(AppDelClass, sel_getUid("applicationDidUpdate:"), (IMP)AppDel_applicationDidUpdate, "i@:@");
    class_addMethod(AppDelClass, sel_getUid("applicationShouldHandleReopen:hasVisibleWindows:"), (IMP)AppDel_applicationShouldHandleReopen, "B@:");
    Protocol *protocol = objc_getProtocol("WKNavigationDelegate");
    assert(protocol);
    class_addProtocol(AppDelClass, protocol);
    class_addProtocol(AppDelClass, objc_getProtocol("NSApplicationDelegate"));
    class_addProtocol(AppDelClass, objc_getProtocol("NSWindowDelegate"));
    class_addMethod(AppDelClass, sel_getUid("webView:decidePolicyForNavigationAction:decisionHandler:"), (IMP)WebView_decidePolicyFor, "v@:@@?");
    
    class_addMethod(AppDelClass, sel_getUid("windowWillClose:"), (IMP)windowWillClose,  "v@:@");
    
    objc_registerClassPair(AppDelClass);
}

void RunApplication() {
    if (app == NULL) {
        fprintf(stderr,"Failed to initialized NSApplication\n");
        return;
    }
    
    objc_msgSend(app, sel_getUid("run"));
}

void ui_app_iterate() {
    app_run_iterate_once(app);
}

int ui_app_init(ui_trigger_event event_handler) {
    id pool = (id)objc_getClass("NSAutoreleasePool");
    if (!pool)
        return 0;

    pool = objc_msgSend(pool, sel_registerName("alloc"));
    if (!pool)
        return 0;

    pool = objc_msgSend(pool, sel_registerName("init"));
    
    app = objc_msgSend((id)objc_getClass("NSApplication"), sel_registerName("sharedApplication"));

    CreateAppDelegate();
    
    id appDelObj = objc_msgSend((id)objc_getClass("AppDelegate"), sel_getUid("alloc"));
    appDelObj = objc_msgSend(appDelObj, sel_getUid("init"));
    
    objc_msgSend(app, sel_getUid("setDelegate:"), appDelObj);

    id menubar = objc_msgSend((id)objc_getClass("NSMenu"), sel_getUid("new"));
    menubar = objc_msgSend(menubar, sel_getUid("autorelease"));

    id appMenuItem = objc_msgSend((id)objc_getClass("NSMenuItem"), sel_getUid("new"));
    appMenuItem = objc_msgSend(appMenuItem, sel_getUid("autorelease"));

    objc_msgSend(menubar, sel_getUid("addItem:"), appMenuItem);

    objc_msgSend(app, sel_getUid("setMainMenu:"), menubar);

    id appMenu = objc_msgSend((id)objc_getClass("NSMenu"), sel_getUid("new"));
    appMenu = objc_msgSend(appMenu, sel_getUid("autorelease"));
    
    id quitMenuItem = objc_msgSend((id)objc_getClass("NSMenuItem"), sel_getUid("alloc"));
    objc_msgSend(quitMenuItem, sel_getUid("initWithTitle:action:keyEquivalent:"), CFSTR("Quit"), sel_getUid("terminate:"), CFSTR("q"));
    objc_msgSend(appMenu, sel_getUid("addItem:"), quitMenuItem);

    objc_msgSend(appMenuItem, sel_getUid("setSubmenu:"), appMenu);

    callback_event = event_handler;
    return 1;
}

int ui_app_done() {
    if (pool) {
        objc_msgSend(pool, sel_registerName("release"));
        return 1;
    }
    return 0;
}

void ui_message(const char *title, const char *body, int level) {
    CFStringRef title_str = CFStringCreateWithCString(NULL, title, kCFStringEncodingMacRoman);
    CFStringRef body_str = CFStringCreateWithCString(NULL, body, kCFStringEncodingMacRoman);

    id alert = objc_msgSend(objc_msgSend((id)objc_getClass("NSAlert"), sel_registerName("alloc")), sel_registerName("init"));
    switch (level) {
        case 1:
            objc_msgSend(alert, sel_getUid("setAlertStyle:"), (int)1);
            break;
        case 2:
            objc_msgSend(alert, sel_getUid("setAlertStyle:"), (int)2);
            break;
        case 3:
            objc_msgSend(alert, sel_getUid("setAlertStyle:"), (int)2);
            break;
    }
    objc_msgSend(alert, sel_getUid("setMessageText:"), title_str);
    objc_msgSend(alert, sel_getUid("setInformativeText:"), body_str);
    objc_msgSend(alert, sel_getUid("runModal"));
    if (body_str)
        CFRelease(body_str);
    if (title_str)    
        CFRelease(title_str);
}

int ui_question(const char *title, const char *body, int level) {
    CFStringRef title_str = CFStringCreateWithCString(NULL, title, kCFStringEncodingMacRoman);
    CFStringRef body_str = CFStringCreateWithCString(NULL, body, kCFStringEncodingMacRoman);

    id alert = objc_msgSend(objc_msgSend((id)objc_getClass("NSAlert"), sel_registerName("alloc")), sel_registerName("init"));

    switch (level) {
        case 1:
            objc_msgSend(alert, sel_getUid("setAlertStyle:"), (int)1);
            break;
        case 2:
            objc_msgSend(alert, sel_getUid("setAlertStyle:"), (int)2);
            break;
        case 3:
            objc_msgSend(alert, sel_getUid("setAlertStyle:"), (int)2);
            break;
    }
    objc_msgSend(alert, sel_getUid("setMessageText:"), title_str);
    objc_msgSend(alert, sel_getUid("setInformativeText:"), body_str);
    objc_msgSend(alert, sel_getUid("addButtonWithTitle:"), CFSTR("No"));
    objc_msgSend(alert, sel_getUid("addButtonWithTitle:"), CFSTR("Yes"));
    int yes_no = (int)objc_msgSend(alert, sel_getUid("runModal")) - 1000;
    if (body_str)
        CFRelease(body_str);
    if (title_str)    
        CFRelease(title_str);
        
    if (yes_no < 0)
        yes_no = 0;
    return yes_no;
}

int ui_input(const char *title, const char *body, char *val, int val_len, int masked) {
    CFStringRef title_str = CFStringCreateWithCString(NULL, title, kCFStringEncodingMacRoman);
    CFStringRef body_str = CFStringCreateWithCString(NULL, body, kCFStringEncodingMacRoman);
    if ((val_len > 0) && (val))
        val[0] = 0;
    id alert = objc_msgSend(objc_msgSend((id)objc_getClass("NSAlert"), sel_registerName("alloc")), sel_registerName("init"));
    id textfield;
    if (masked)
        textfield = objc_msgSend(objc_msgSend((id)objc_getClass("NSSecureTextField"), sel_registerName("alloc")), sel_registerName("initWithFrame:"), (edworkRect){0, 0, 295, 22});
    else
        textfield = objc_msgSend(objc_msgSend((id)objc_getClass("NSTextField"), sel_registerName("alloc")), sel_registerName("initWithFrame:"), (edworkRect){0, 0, 295, 22});
    objc_msgSend(alert, sel_getUid("setAlertStyle:"), (int)1);
    objc_msgSend(alert, sel_getUid("setMessageText:"), title_str);
    objc_msgSend(alert, sel_getUid("setInformativeText:"), body_str);
    objc_msgSend(alert, sel_getUid("setAccessoryView:"), textfield);
    objc_msgSend(alert, sel_getUid("addButtonWithTitle:"), CFSTR("Yes"));
    objc_msgSend(alert, sel_getUid("addButtonWithTitle:"), CFSTR("No"));
    int yes_no = (int)objc_msgSend(alert, sel_getUid("runModal")) - 1000;
    if (body_str)
        CFRelease(body_str);
    if (title_str)    
        CFRelease(title_str);
        
    if (yes_no < 0)
        yes_no = 0;
    else
        yes_no = !yes_no;
    if (yes_no) {
        CFStringRef str = (CFStringRef)objc_msgSend(textfield, sel_getUid("stringValue"));
        if (str)
            CFStringGetCString(str, val, val_len, kCFStringEncodingUTF8);
    }
    return yes_no;
}

void ui_app_quit() {
    if (app) {
        objc_msgSend(app, sel_getUid("terminate:"), app);
        app = NULL;
    }
}

void ui_set_event(int eid, ui_event callback, void *event_userdata) {
    if ((eid < 0) || (eid >= UI_EVENTS))
        return;

    ui_callbacks[eid] = callback;
    ui_data[eid] = event_userdata;
}

 void ui_app_run_with_notify(ui_idle_event event_idle, void *userdata) {
    idle_event = event_idle;
    idle_userdata = userdata;

    RunApplication();
}

void ui_app_run_schedule_once(ui_idle_event scheduled, void *userdata) {
    if (schedule_count >= UI_SCHEDULER_SIZE)
        schedule_count = 0;
    scheduled_event[schedule_count] = scheduled;
    scheduled_data[schedule_count] = userdata;
    schedule_count ++;
}

void ui_app_run() {
    RunApplication();
}

void ui_js(void *window, const char *js) {
    if ((!window) || (!js))
        return;
    CFStringRef js_str = CFStringCreateWithCString(NULL, js, kCFStringEncodingMacRoman);
    
    objc_msgSend(objc_msgSend(window, sel_getUid("contentView")), sel_getUid("evaluateJavaScript:completionHandler:"), js_str, NULL);

    if (js_str)
        CFRelease(js_str);
}

char *ui_call(void *window, const char *js, const char *arguments[]) {
    if ((!window) || (!js))
        return NULL;

    char *data = NULL;
    char buffer[8192];
    char arg_str[8192];
    arg_str[0] = 0;
    char *arg = arg_str;
    int arg_size = sizeof(arg_str);
    while ((arguments) && (*arguments)) {
        if (arg != arg_str) {
            arg[0] = ',';
            arg ++;
            arg_size --;
        }
        int len = strlen(*arguments);
        int i;
        int bi = 0;
        for (i = 0; i < len; i++) {
            char e = (*arguments)[i];
            switch (e) {
                case '\r':
                    buffer[bi ++] = '\\';
                    buffer[bi ++] = 'r';
                    break;
                case '\n':
                    buffer[bi ++] = '\\';
                    buffer[bi ++] = 'n';
                    break;
                case '\'':
                    buffer[bi ++] = '\\';
                    buffer[bi ++] = '\'';
                    break;
                case '\\':
                    buffer[bi ++] = '\\';
                    buffer[bi ++] = '\\';
                    break;
                default:
                    buffer[bi ++] = e;
            }
        }
        buffer[bi] = 0;
        int written = snprintf(arg, arg_size, "'%s'", buffer);
        arguments ++;
        if (written > 0) {
            arg += written;
            arg_size -= written;
            if (arg_size <= 1)
                break;
        } else
            break;
    }
    snprintf(buffer, sizeof(buffer), "%s(%s)", js, arg_str);
    CFStringRef js_str = CFStringCreateWithCString(NULL, buffer, kCFStringEncodingMacRoman);                                                                                                                                                                                                       
    __block int finished = 0;
    __block char *str_data = NULL;
    objc_msgSend(objc_msgSend(window, sel_getUid("contentView")), sel_getUid("evaluateJavaScript:completionHandler:"), js_str, ^(id value, void *error) {
        // WARNING: non-standard C extension
        if ((value) && (!finished)) {
            const char *str = (const char *)objc_msgSend(value, sel_getUid("UTF8String"));
            if (str) {
                int len = strlen(str);
                str_data = malloc(len + 1);
                if (str_data) {
                    memcpy(str_data, str, len);
                    str_data[len] = 0;
                }
            }
        }
        finished = 1;
    });
    if (js_str)
        CFRelease(js_str);

    while (!finished)
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.25, 0);
    finished = 1;
    data = str_data;
    
    return data;
}

void ui_app_tray_icon(const char *tooltip, const char *notification_title, const char *notification_text, ui_tray_event event_tray) {
    event_tray_event = event_tray;
    if (event_tray)
        window_count ++;
}

void ui_app_tray_remove() {
    if ((event_tray_event) && (window_count > 0))
        window_count --;
    event_tray_event = NULL;
}

void ui_free_string(void *ptr) {
    if (ptr)
        free(ptr);
}

int ui_window_count() {
    return window_count;
}

void ui_lock() { 
    gui_lock ++;
}

void ui_unlock() {
    if (gui_lock) {
        gui_lock --;
        if ((window_count <= 0) && (!gui_lock))
            ui_app_quit();
    }
}
