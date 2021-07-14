#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>

#include <xdo.h>

static int has_xi2(Display *display) {
    int major = 2, minor = 2;
    int rc;

    rc = XIQueryVersion(display, &major, &minor);
    if (rc == BadRequest) {
        fprintf(stderr, "Bad XI2 version. Server provides version %d.%d\n", major, minor);
        return 0;
    } else if (rc != Success) {
        fprintf(stderr, "Could not query XI2 version\n");
        return 0;
    }
    printf("XI2 supported. Server provides version %d.%d\n", major, minor);
    return 1;
}

static void select_events(Display *display, Window win) {
    XIEventMask event_mask[1];
    unsigned char mask[(XI_LASTEVENT + 7) / 8];
    memset(mask, 0, sizeof(mask));
    XISetMask(mask, XI_RawKeyPress);

    event_mask[0].deviceid = XIAllMasterDevices;
    event_mask[0].mask_len = sizeof(mask);
    event_mask[0].mask = mask;
    XISelectEvents(display, win, event_mask, 1);
    XFlush(display);
}

int main() {
    Display *display;
    int xi_opcode, event, error;
    XEvent xevent;
    int press = 1;
    xdo_t *x = xdo_new(":0.0");

    display = XOpenDisplay(NULL);

    if (!display) {
        fprintf(stderr, "Failed to open display\n");
        return 1;
    }

    if (!XQueryExtension(display, "XInputExtension", &xi_opcode, &event, &error)) {
        fprintf(stderr, "X Input extension not available\n");
        return 1;
    }

    if (!has_xi2(display)) {
        return 1;
    }

    select_events(display, DefaultRootWindow(display));

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (1) {
        XGenericEventCookie *cookie = &xevent.xcookie;

        XNextEvent(display, &xevent);

        if (cookie->type != GenericEvent || cookie->extension != xi_opcode || !XGetEventData(display, cookie)) {
            continue;
        }

        printf("%d\n", cookie->evtype);
        switch (cookie->evtype) {
            case XI_RawKeyPress:
                if (cookie->send_event)
                    break;
                printf("KEY PRESS: %d\n", press);

                select_events(display, DefaultRootWindow(display));
                if (press % 2 == 0) {
                    xdo_send_keysequence_window_down(x, CURRENTWINDOW, "Shift_L", 0);
                }
                else {
                    xdo_send_keysequence_window_up(x, CURRENTWINDOW, "Shift_L", 0);
                }
                press++;
                break;
        }
        XFreeEventData(display, cookie);
    }
#pragma clang diagnostic pop

    return 0;
}
