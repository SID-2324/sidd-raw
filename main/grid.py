"""
Reference Grid — Zone geometry and position classification
============================================================
Builds a reference grid over the camera frame and classifies a point
as LEFT ('L'), RIGHT ('R'), or CENTER ('S').

Grid Layout
-----------
The frame is divided into three horizontal zones:

   ┌────────────┬─────────────────────┬────────────┐
   │            │    ╔═══════════╗    │            │
   │   LEFT     │    ║ DEAD ZONE ║    │   RIGHT    │
   │   (L)      │    ║   (lock)  ║    │   (R)      │
   │            │    ╚═══════════╝    │            │
   │            │     CENTER (S)      │            │
   └────────────┴─────────────────────┴────────────┘

The CENTER zone is the target. Inside it, there's a smaller DEAD ZONE — once
the spearhead center enters the dead zone, we consider it "locked" and won't
send any new commands until it drifts out again.  This prevents jitter.
"""

import cv2


class ReferenceGrid:
    """Computes and draws the reference grid for a given frame size."""

    def __init__(self, frame_width: int, frame_height: int,
                 center_w_ratio: float = 0.15,
                 center_h_ratio: float = 0.25,
                 dead_zone_ratio: float = 0.05):
        self.fw = frame_width
        self.fh = frame_height

        # ── Center zone (the "S" target box) ──
        cw = int(frame_width  * center_w_ratio)
        ch = int(frame_height * center_h_ratio)
        self.center_x1 = (frame_width  - cw) // 2
        self.center_y1 = (frame_height - ch) // 2
        self.center_x2 = self.center_x1 + cw
        self.center_y2 = self.center_y1 + ch

        # ── Dead zone (inner hysteresis band) ──
        dw = int(frame_width  * dead_zone_ratio)
        dh = int(frame_height * dead_zone_ratio)
        self.dead_x1 = (frame_width  - dw) // 2
        self.dead_y1 = (frame_height - dh) // 2
        self.dead_x2 = self.dead_x1 + dw
        self.dead_y2 = self.dead_y1 + dh

        # ── Vertical center line (visual only) ──
        self.center_line_x = frame_width // 2

    def classify(self, cx: int, cy: int) -> str:
        """
        Classify a point (cx, cy) relative to the reference grid.

        Returns
        -------
        'L'  — object center is LEFT of the center zone
        'R'  — object center is RIGHT of the center zone
        'S'  — object center is inside the center zone (aligned)
        """
        if cx < self.center_x1:
            return 'L'
        elif cx > self.center_x2:
            return 'R'
        else:
            return 'S'

    def is_in_dead_zone(self, cx: int, cy: int) -> bool:
        """Check if the point is inside the inner dead zone (very precisely centered)."""
        return (self.dead_x1 <= cx <= self.dead_x2 and
                self.dead_y1 <= cy <= self.dead_y2)

    def draw(self, frame, current_command: str = None, locked: bool = False):
        """
        Draw the reference grid overlay on the frame.

        Parameters
        ----------
        frame : numpy array (BGR)
        current_command : str or None — the current alignment command
        locked : bool — whether alignment is locked (complete)
        """
        overlay = frame.copy()

        # ── Left zone shading (semi-transparent red tint) ──
        cv2.rectangle(overlay,
                      (0, self.center_y1),
                      (self.center_x1, self.center_y2),
                      (0, 0, 180), cv2.FILLED)

        # ── Right zone shading (semi-transparent blue tint) ──
        cv2.rectangle(overlay,
                      (self.center_x2, self.center_y1),
                      (self.fw, self.center_y2),
                      (180, 0, 0), cv2.FILLED)

        # ── Blend the overlay ──
        alpha = 0.15
        cv2.addWeighted(overlay, alpha, frame, 1 - alpha, 0, frame)

        # ── Center zone border ──
        center_color = (0, 255, 0) if locked else (0, 200, 200)
        thickness = 3 if locked else 2
        cv2.rectangle(frame,
                      (self.center_x1, self.center_y1),
                      (self.center_x2, self.center_y2),
                      center_color, thickness)

        # ── Dead zone (dashed-style, thin) ──
        cv2.rectangle(frame,
                      (self.dead_x1, self.dead_y1),
                      (self.dead_x2, self.dead_y2),
                      (200, 200, 200), 1)

        # ── Vertical center line ──
        cv2.line(frame,
                 (self.center_line_x, 0),
                 (self.center_line_x, self.fh),
                 (100, 100, 100), 1, cv2.LINE_AA)

        # ── Horizontal center line ──
        cy = self.fh // 2
        cv2.line(frame, (0, cy), (self.fw, cy),
                 (100, 100, 100), 1, cv2.LINE_AA)

        # ── Zone labels ──
        label_y = self.center_y1 - 12
        cv2.putText(frame, "L", (self.center_x1 // 2 - 10, label_y),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 200), 2)
        cv2.putText(frame, "S", (self.center_line_x - 8, label_y),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.8, center_color, 2)
        cv2.putText(frame, "R", ((self.center_x2 + self.fw) // 2 - 10, label_y),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.8, (200, 0, 0), 2)

        # ── Status banner ──
        if locked:
            banner = "ALIGNED — LOCKED"
            banner_color = (0, 255, 0)
        elif current_command == 'L':
            banner = ">>> MOVE LEFT <<<"
            banner_color = (0, 0, 255)
        elif current_command == 'R':
            banner = ">>> MOVE RIGHT <<<"
            banner_color = (255, 0, 0)
        elif current_command == 'S':
            banner = "CENTERING..."
            banner_color = (0, 255, 255)
        else:
            banner = "SEARCHING..."
            banner_color = (128, 128, 128)

        # Draw banner background
        (tw, th), _ = cv2.getTextSize(banner, cv2.FONT_HERSHEY_SIMPLEX, 0.9, 2)
        bx = (self.fw - tw) // 2
        by = 40
        cv2.rectangle(frame, (bx - 10, by - th - 10), (bx + tw + 10, by + 10),
                      (0, 0, 0), cv2.FILLED)
        cv2.putText(frame, banner, (bx, by),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.9, banner_color, 2, cv2.LINE_AA)

        return frame