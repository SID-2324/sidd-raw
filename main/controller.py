"""
Alignment Controller — Continuous debounced command logic with hysteresis
==========================================================================
Alignment Controller — Debounced command logic with hysteresis
================================================================
This is the brain of the alignment system.  It takes raw position readings
from the detector and produces stable, debounced commands for the STM32.
Algorithm
---------
1. Each frame, the detector reports the spearhead's center (cx, cy).
2. The ReferenceGrid classifies it as L / R / S.
3. The AlignmentController applies debouncing:
   - A new command must appear DEBOUNCE_COUNT consecutive times before it
     replaces the current active command.  This filters transient noise.
4. Dead-zone hysteresis:
   - If the spearhead enters the dead zone (very center), we set a "stick"
     flag.  While sticky, we keep sending 'S' even if the point drifts
   - If the spearhead enters the dead zone (very center), we set a "lock"
     flag.  While locked, we keep sending 'S' even if the point drifts
     slightly outside the dead zone (but still inside the center zone).
   - The stick only breaks if the point exits the center zone entirely.
5. Continuous polling:
   - The system NEVER locks. It keeps sending commands every frame.
   - If the spearhead overshoots past center, it immediately reacts
     with the correct L or R command.
   - The lock only breaks if the point exits the center zone entirely.
5. Alignment lock:
   - If we receive ALIGNMENT_LOCK_COUNT consecutive 'S' readings, we
     declare alignment complete.  The MCU receives a final 'S' and the
     system stops sending further commands until the user resets.
6. Rate limiting:
   - Commands are sent no faster than COMMAND_INTERVAL_SEC apart to avoid
     flooding the UART buffer.
"""
import time
class AlignmentController:
    """
    Stateful controller that converts raw grid classifications into
    stable, debounced motor commands.  Runs continuously — never locks.
    stable, debounced motor commands.
    """
    def __init__(self, debounce_count: int = 3,
                 alignment_lock_count: int = 10,
                 command_interval: float = 0.05):
        # ── Debounce state ──
        self._debounce_count = debounce_count
        self._pending_cmd = None      # the command we're accumulating
        self._pending_streak = 0      # how many consecutive times we've seen it
        # ── Active command (what we're actually sending) ──
        self._active_cmd = None
        # ── Dead-zone hysteresis (sticky center) ──
        # ── Alignment lock ──
        self._lock_count = alignment_lock_count
        self._s_streak = 0            # consecutive 'S' frames
        self._locked = False          # True = alignment complete
        # ── Dead-zone hysteresis ──
        self._in_dead_zone = False
        # ── Rate limiting ──
        self._interval = command_interval
        self._last_send_time = 0.0
    @property
    def is_locked(self) -> bool:
        """True if alignment has been declared complete."""
        return self._locked
    @property
    def active_command(self) -> str | None:
        """The current debounced command ('L', 'R', 'S', or None)."""
        return self._active_cmd
    @property
    def s_streak(self) -> int:
        """How many consecutive 'S' readings we've accumulated."""
        return self._s_streak
    def reset(self):
        """Reset all internal state (e.g. to restart alignment)."""
        self._pending_cmd = None
        self._pending_streak = 0
        self._active_cmd = None
        self._s_streak = 0
        self._locked = False
        self._in_dead_zone = False
        self._last_send_time = 0.0
    def update(self, raw_cmd: str | None, in_dead_zone: bool = False) -> str | None:
        """
        Feed a new raw classification and get back the command to send.
        Parameters
        ----------
        raw_cmd : 'L', 'R', 'S', or None (no detection this frame)
        in_dead_zone : True if the detection is inside the dead zone
        Returns
        -------
        str or None — the command to send to the STM32 this frame,
                       or None if no command should be sent (debouncing
                       or rate-limiting).
                       or None if no command should be sent (debouncing,
                       rate-limiting, or already locked).
        """
        # ── If already locked, don't send anything ──
        if self._locked:
            return None
        # ── No detection: reset streaks ──
        if raw_cmd is None:
            self._pending_cmd = None
            self._pending_streak = 0
            self._s_streak = 0
            self._in_dead_zone = False
            return None
        # ── Dead-zone hysteresis ──
        if in_dead_zone:
            self._in_dead_zone = True
        elif raw_cmd != 'S':
            # Exited the center zone entirely → break sticky center
            # Exited the center zone entirely → break lock
            self._in_dead_zone = False
        # If we're sticky in the dead zone and still within center, force 'S'
        # If we're in the dead zone, force 'S' regardless
        if self._in_dead_zone and raw_cmd == 'S':
            raw_cmd = 'S'
        # ── Debounce logic ──
        if raw_cmd == self._pending_cmd:
            self._pending_streak += 1
        else:
            self._pending_cmd = raw_cmd
            self._pending_streak = 1
        # Only promote to active after enough consecutive readings
        if self._pending_streak >= self._debounce_count:
            self._active_cmd = self._pending_cmd
        # ── Alignment lock tracking ──
        if self._active_cmd == 'S':
            self._s_streak += 1
            if self._s_streak >= self._lock_count:
                self._locked = True
                return 'S'  # send one final 'S' to confirm
        else:
            self._s_streak = 0
        # ── Rate limiting ──
        now = time.time()
        if now - self._last_send_time < self._interval:
            return None  # too soon, skip this frame
        if self._active_cmd is not None:
            self._last_send_time = now
            return self._active_cmd
        return None