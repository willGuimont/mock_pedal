#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>
#include <string.h>

double gettime() {
    struct timeval tim;
    gettimeofday(&tim, NULL);
    double t1 = tim.tv_sec + (tim.tv_usec / 1000000.0);
    return t1;
}

int main() {
    Display *display;
    display = XOpenDisplay(NULL);
    char keys_return[32];
    unsigned int key = XK_Shift_L;
    unsigned int keycode = XKeysymToKeycode(display, key);
    printf("Simulating %d\n", keycode);

    int toggle = 0;
    int loop = 1;
    int pressed_key = 0;
    while (loop) {
        XQueryKeymap(display, keys_return);
        for (int i = 0; i < 32; i++) {
            if (keys_return[i] != 0) {
                int pos = 0;
                int num = keys_return[i];
                while (pos < 8) {
                    if ((num & 0x01) == 1) {
                        pressed_key = i * 8 + pos;
                        if (pressed_key != 50) { // if not shift
                            usleep(30000);
                            XTestFakeKeyEvent(display, keycode, toggle % 2, 0);
                            XFlush(display);

                            ++toggle;
                            if (toggle > 255)
                                loop = 0;
                        }
                    }
                    pos++;
                    num /= 2;
                }
            }
        }
        usleep(30000);
    }
    XCloseDisplay(display);
}
