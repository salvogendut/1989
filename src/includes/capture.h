/* capture.h — screenshot and GIF recording helpers for the 1989 UI.
 *
 * Screenshots are written as PPM to the current working directory with a
 * "1989-<timestamp>" name (matching the sibling emulators' F4 behaviour).
 * GIF recording uses the in-tree gifcap encoder, fed from the NeXT
 * framebuffer at a user-selectable frame rate. */

#ifndef PREV_CAPTURE_H
#define PREV_CAPTURE_H

#include <stdbool.h>

/* Save a PPM screenshot of the current framebuffer. Returns true on
 * success. */
bool Capture_Screenshot(void);

/* Start GIF recording. out_w is the output width (height keeps 1120x832
 * aspect); fps selects the recorded frame rate (10/20/25). Returns true if
 * recording started. */
bool Capture_GifStart(int out_w, int fps);
/* Stop GIF recording, finalising the file. Returns true if a recording was
 * open. */
bool Capture_GifStop(void);
/* True while a GIF recording is in progress. */
bool Capture_GifActive(void);
/* Number of frames captured so far (for the status/toast). */
int  Capture_GifFrames(void);
/* Called every repaint; captures a frame if the target frame rate window
 * has elapsed. */
void Capture_Tick(void);

#endif /* PREV_CAPTURE_H */