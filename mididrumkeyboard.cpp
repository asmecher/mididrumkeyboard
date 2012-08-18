/**
 * MIDI Drum Set To Keyboard Converter
 * Copyright (c) 2012 by Alec Smecher
 *
 * Distributed under the GNU GPL v2. For full terms see the file COPYING.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

// MIDI device filename
const char *midiDevice = "/dev/midi1";

// MIDI constants
#define MIDI_NOTE_ON 0x90
#define MIDI_TICK 0xf8

// Pad tuning
#define STROKE_THRESHOLD 0x05
#define HIT_THRESHOLD 0x0f
#define TICK_THRESHOLD 5

// Pad IDs
#define BD 0x21
#define HHO 0x2e // Open
#define HHC 0x2a // Closed
#define CRASH 0x31
#define SNARE 0x28
#define SIDETOM 0x30
#define RIDE 0x3b
#define FLOORTOM 0x2f
#define HATFOOT 0x2c

struct mapEntry {
	const char *content;
	int keycode;
	int modifiers;
};

/**
 * A map of drum sequences to X keyboard codes
 */
const struct mapEntry map[] = {
	// Letters
	{"RIDE.", XK_a, 0},
	{"RIDE.RIDE.", XK_A, ShiftMask},
	{"BD,SNARE.", XK_b, 0},
	{"BD,SNARE.BD,SNARE.", XK_B, ShiftMask},
	{"CRASH,SNARE.", XK_c, 0},
	{"CRASH,SNARE.CRASH,SNARE.", XK_C, ShiftMask},
	{"CRASH,SIDETOM.", XK_d, 0},
	{"CRASH,SIDETOM.CRASH,SIDETOM.", XK_D, ShiftMask},
	{"SNARE.", XK_e, 0},
	{"SNARE.SNARE.", XK_E, ShiftMask},
	{"BD,HATFOOT.", XK_f, 0},
	{"BD,HATFOOT.BD,HATFOOT.", XK_F, ShiftMask},
	{"BD,FLOORTOM.", XK_g, 0},
	{"BD,FLOORTOM.BD,FLOORTOM.", XK_G, ShiftMask},
	{"SNARE,SIDETOM.", XK_h, 0},
	{"SNARE,SIDETOM.SNARE,SIDETOM.", XK_H, ShiftMask},
	{"SIDETOM.", XK_i, 0},
	{"SIDETOM.SIDETOM.", XK_I, ShiftMask},
	{"FLOORTOM,HATFOOT.", XK_j, 0},
	{"FLOORTOM,HATFOOT.FLOORTOM,HATFOOT.", XK_J, ShiftMask},
	{"HH,FLOORTOM.", XK_k, 0},
	{"HH,FLOORTOM.HH,FLOORTOM.", XK_K, ShiftMask},
	{"BD,SIDETOM.", XK_l, 0},
	{"BD,SIDETOM.BD,SIDETOM.", XK_L, ShiftMask},
	{"HH,SNARE.", XK_m, 0},
	{"HH,SNARE.HH,SNARE.", XK_M, ShiftMask},
	{"FLOORTOM.", XK_n, 0},
	{"FLOORTOM.FLOORTOM.", XK_N, ShiftMask},
	{"CRASH.", XK_o, 0},
	{"CRASH.CRASH.", XK_O, ShiftMask},
	{"SNARE,FLOORTOM.", XK_p, 0},
	{"SNARE,FLOORTOM.SNARE,FLOORTOM.", XK_P, ShiftMask},
	{"BD,SNARE,HATFOOT.", XK_q, 0},
	{"BD,SNARE,HATFOOT.BD,SNARE,HATFOOT.", XK_Q, ShiftMask},
	{"HATFOOT.", XK_r, 0},
	{"HATFOOT.HATFOOT.", XK_R, ShiftMask},
	{"CRASH,RIDE.", XK_s, 0},
	{"CRASH,RIDE.CRASH,RIDE.", XK_S, ShiftMask},
	{"HH.", XK_t, 0},
	{"HH.HH.", XK_T, ShiftMask},
	{"HH,CRASH.", XK_u, 0},
	{"HH,CRASH.HH,CRASH.", XK_U, ShiftMask},
	{"BD,CRASH,RIDE.", XK_v, 0},
	{"BD,CRASH,RIDE.BD,CRASH,RIDE.", XK_V, ShiftMask},
	{"BD,HH,RIDE.", XK_w, 0},
	{"BD,HH,RIDE.BD,HH,RIDE.", XK_W, ShiftMask},
	{"RIDE,HATFOOT.", XK_x, 0},
	{"RIDE,HATFOOT.RIDE,HATFOOT.", XK_X, ShiftMask},
	{"BD,SNARE,FLOORTOM.", XK_y, 0},
	{"BD,SNARE,FLOORTOM.BD,SNARE,FLOORTOM.", XK_Y, ShiftMask},
	{"BD,SNARE,SIDETOM.", XK_z, 0},
	{"BD,SNARE,SIDETOM.BD,SNARE,SIDETOM.", XK_Z, ShiftMask},
	{"BD.", XK_space, 0},
	{"BD,CRASH.", XK_BackSpace, 0},
	{"SNARE,RIDE.", XK_period, 0},
	{"HH,RIDE.", XK_comma, 0},
	{"BD,RIDE.", XK_exclam, ShiftMask},
	{"SIDETOM,RIDE.", XK_semicolon},
	{"BD,SNARE,RIDE.", XK_Return},
	{"", 0, 0} // Terminator
};

/**
 * An enumeration of drums by pad ID
 */
const unsigned char drums[] = {BD, HHO, HHC, CRASH, SNARE, SIDETOM, RIDE, FLOORTOM, HATFOOT};

/**
 * A list of drum names
 */
const char *drumNames[] = {"BD", "HH", "HH", "CRASH", "SNARE", "SIDETOM", "RIDE", "FLOORTOM", "HATFOOT"};

// Quit flag
int stop=0;

/**
 * Create a key event using the given parameters
 * @param display Display
 * @param win Window
 * @param root Window
 * @param press true for press; false for release
 * @param keycode int
 * @param state int
 * @return XKeyEvent
 */
XKeyEvent createKeyEvent(Display *display, Window &win, Window &root, unsigned char press, int keycode, int state) {
	XKeyEvent e;
	e.display = display;
	e.window = win;
	e.root = root;
	e.subwindow = None;
	e.time = CurrentTime;
	e.x = e.y = 1;
	e.x_root = e.y_root = 1;
	e.same_screen = True;
	e.keycode = XKeysymToKeycode(display, keycode);
	e.state = state;
	e.type = press?KeyPress:KeyRelease;

	return e;
}

/**
 * Handle a signal
 */
void sighandler(int dum) {
	// Flag the main loop to quit.
	stop=1;
}

/**
 * Handle a "sentence" (sequence) from the drums.
 * @var s sentence
 * @var display X display
 * @var root X root window
 * @return boolean
 */
bool handleSentence(char *s, Display *display, Window &root) {
	for (int i=0; strlen(map[i].content); i++) {
		if (strcmp(s, map[i].content)==0) {
			// We found a match. Send the keystrokes.
			int revert;
			Window focus;
			XGetInputFocus(display, &focus, &revert);
			XKeyEvent e;

			// Send key down
			e = createKeyEvent(display, focus, root, 1, map[i].keycode, map[i].modifiers);
			XSendEvent(e.display, e.window, True, KeyPressMask, (XEvent *) &e);

			// Send key up
			e = createKeyEvent(display, focus, root, 0, map[i].keycode, map[i].modifiers);
			XSendEvent(e.display, e.window, True, KeyPressMask, (XEvent *) &e);

			// Flush for message handling.
			XFlush(display);
			return true;
		}
	}

	// Sequence not found.
	return false;
}

/**
 * Handle a stroke on the drums.
 * @var velocities
 * @var $s
 */
char *handleStroke (unsigned char *velocities, int s) {
	unsigned char i;
	char *strokeBuf = (char *) malloc(1024);
	strokeBuf[0] = 0;
	unsigned char empty = 1;
	for (i=0; i < s; i++) {
		if (velocities[i] >= STROKE_THRESHOLD) {
			if (!empty) strcat(strokeBuf, ",");
			empty = 0;
			strcat(strokeBuf, drumNames[i]);
		}
	}
	strcat(strokeBuf, ".");
	return strokeBuf;
}

/**
 * Main function
 */
int main(int argc,char** argv) {
	int fd = -1;
	unsigned char symbol;

	Display *display = XOpenDisplay(0);
	if (display == NULL) {
		fprintf(stderr, "Could not open display!\n");
		return -1;
	}
	Window root = XDefaultRootWindow(display);

	fd = open(midiDevice, O_RDONLY);
	if (fd < 0) {
		XCloseDisplay(display);
		fprintf(stderr, "Could not open %s for input.\n", midiDevice);
		return -1;
	}       

	// Allow us to quit if we're doing a blocking read
	signal(SIGINT,sighandler);

	unsigned char ch, key, velocity;
	unsigned char *velocities;
	unsigned char *c;
	char buf[1024];
	unsigned char ticksSinceStroke = 0;

	buf[0] = 0;
	velocities = (unsigned char *) malloc(sizeof(drums));

	while (1) {
		do {
			read(fd, &ch, 1);
			if (ch == MIDI_TICK) {
				unsigned char i;
				unsigned char needsHandling = 0;
				for (i=0; i<sizeof(drums); i++) {
					if (velocities[i] / 2 < STROKE_THRESHOLD && velocities[i] >= STROKE_THRESHOLD) needsHandling = 1;
				}
				if (needsHandling) {
					if (buf[0] == 0) ticksSinceStroke = 0;
					char *t = handleStroke(velocities, sizeof(drums));
					strcat(buf, t);
					free(t);
					memset(velocities, 0, sizeof(drums));
				}
				for (i=0; i<sizeof(drums); i++) {
					velocities[i] /= 2;
				}

				if (ticksSinceStroke++ > TICK_THRESHOLD && buf[0] != 0) {
					if (!handleSentence(buf, display, root)) {
						fprintf(stderr, "UNKNOWN SEQUENCE: %s\n", buf);
					}
					buf[0] = 0;
					ticksSinceStroke = 0;
					memset(velocities, 0, sizeof(drums));
				}
			}
		} while (!stop && (ch & 0xf0) != MIDI_NOTE_ON);

		if (stop) break;

		// If we've fallen through to this point, we must have encountered
		// a MIDI_NOTE_ON message. Read the key and velocity and store.
		read(fd, &key, 1);
		read(fd, &velocity, 1);
		if (velocity >= HIT_THRESHOLD && (c = (unsigned char *) strchr((char *)drums, key))) {
			velocities[c - drums] = velocity;
		}
	}

	// If we've fallen through it must be time to clean up and quit.
	free(velocities);
	XCloseDisplay(display);

	return 0;
}

