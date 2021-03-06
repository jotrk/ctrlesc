#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

// for restoring auto repeat
Display * g_dpy;
XKeyboardState g_keyboard_state;

static void
handler(int signum)
{
  XKeyboardControl kc;

  if (g_keyboard_state.global_auto_repeat) {
    kc.key = XK_Escape;
    kc.auto_repeat_mode = AutoRepeatModeOff;
    XChangeKeyboardControl(g_dpy, KBKey | KBAutoRepeatMode, &kc);
  } else {
    // stub, until i find out how to use XKeyboardState to enable auto repeat
    // for single keys again
  }

  exit(EXIT_SUCCESS);
}

int
main(int argc, char ** argv)
{
  Display * dpy = XOpenDisplay("");
  Window root = DefaultRootWindow(dpy);
  KeyCode kc_escape = XKeysymToKeycode(dpy, XK_Escape);

  Window focus = None;
  Bool want_ctrl = False;

  g_dpy = dpy;
  XGetKeyboardControl(dpy, &g_keyboard_state);

  struct sigaction sa;
  sa.sa_handler = handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if (sigaction(SIGINT, &sa, NULL) == -1
      || sigaction(SIGTERM, &sa, NULL) == -1
     ) {
    fprintf(stderr, "Couldn't install signal handler\n");
    exit(EXIT_FAILURE);
  }

  XKeyboardControl kc;
  kc.key = kc_escape;
  kc.auto_repeat_mode = AutoRepeatModeOff;
  XChangeKeyboardControl(dpy, KBKey | KBAutoRepeatMode, &kc);

  XGrabKey(dpy, kc_escape, 0, root, False, GrabModeAsync, GrabModeAsync);

  XEvent event;
  while (1) {
    XNextEvent(dpy, &event);

    if (event.type == KeyPress) {
      XKeyEvent * e = &event.xkey;

      if (e->keycode == kc_escape) {
        int revert;
        XGetInputFocus(dpy, &focus, &revert);

        XGrabKeyboard(dpy, focus, False, GrabModeAsync, GrabModeAsync, CurrentTime);

      } else if (focus != None) {
        want_ctrl = True;

        e->window = focus;
        e->state |= ControlMask;
        e->time = CurrentTime;
        e->send_event = True;

        XSendEvent(dpy, focus, True, KeyPressMask, (XEvent *)&event);
      }

    } else if (focus != None && event.type == KeyRelease) {
      XKeyEvent * e = &event.xkey;

      if (e->keycode == kc_escape) {
        if (! want_ctrl) {

          e->type = KeyPress;
          e->window = focus;
          e->keycode = kc_escape;
          e->time = CurrentTime;
          e->send_event = True;

          XSendEvent(dpy, focus, True, KeyPressMask, (XEvent *)&event);

          e->type = KeyRelease;
          XSendEvent(dpy, focus, True, KeyReleaseMask, (XEvent *)&event);
        }

        focus = None;
        want_ctrl = False;
        XUngrabKeyboard(dpy, CurrentTime);
      }
    }
  }

  return 0;
}
