#import <Cocoa/Cocoa.h>
#import <WebKit/WKNavigationDelegate.h>

Protocol *_reference_all() {
    Protocol *dummy = @protocol(WKNavigationDelegate);
    Protocol *dummy2 = @protocol(NSWindowDelegate);
    Protocol *dummy3 = @protocol(NSApplicationDelegate);
    return dummy;
}

void app_run_iterate_once(NSApplication *app) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    static int started = 0;
    if (!started) {
        [app finishLaunching];
        started = 1;
    }

    pool = [[NSAutoreleasePool alloc] init];

    NSEvent *event = NULL;
    do {
        event =
            [app
                nextEventMatchingMask:NSEventMaskAny
                untilDate:[NSDate distantPast]
                inMode:NSDefaultRunLoopMode
                dequeue:YES];

        if (event) {
            [app sendEvent:event];
            [app updateWindows];
        }
    } while (event);
    [pool release];
}
