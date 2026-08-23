# The Odyssey Monitoring System

Two independent tasks: a Python pipeline for processing depth-sensor readings, and a Tinkercad/Arduino state machine for tracking the ship's status.

---

## Task 1 — Finding the Sea Floor

**Files:** `depth_monitor.py`, `depth_data.csv`

**Approach**

1. **Extract the data.** `depth_data.csv` is read with `pandas`. The depth column is coerced to numeric with `pd.to_numeric(errors="coerce")`, which turns any corrupted cell (the sample data contains a literal `#VALUE!` string) into `NaN` instead of crashing the parse.

2. **Detect erratic/corrupted points.** Real depth sensors don't just fail with obvious garbage — they also produce plausible-looking but wrong spikes (the sample data has one point that jumps to `-1271 m` in the middle of a `-150 m` to `-400 m` range). To catch these without also catching legitimate depth changes:
   - A rolling **median** (window of 5, centered) is computed as a robust "expected" value at each point.
   - Each point's deviation from that local median is compared against a rolling **Median Absolute Deviation (MAD)**.
   - Any point deviating more than `6×MAD` from its local median is flagged as bad and masked out.

   Median/MAD is used instead of mean/standard deviation specifically because a single wild spike disproportionately drags a mean and inflates a standard deviation, which would make the outlier detector blind to the very outlier it's trying to catch.

3. **Fill the gaps.** Masked (bad) points, plus any genuinely missing values, are filled by linear interpolation between their nearest valid neighbors.

4. **Reduce noise.** On top of the cleaned series, a rolling mean (same 5-sample window) smooths out ordinary sensor jitter, producing a stable line for the readout without lagging too far behind real depth changes.

5. **Animate and label.** `matplotlib.animation.FuncAnimation` reveals one new point per frame at `interval=1000` ms, matching the stated 1-sample-per-second recording rate. The plot has a title, axis labels, a legend, and a dashed reference line marking the shallow-water threshold. A warning message appears on-screen whenever the smoothed depth is within `SAFE_DEPTH` (10 m) of the surface.

**Bug found and fixed:** the depth values in the sample data are negative-down (`0` = surface, more negative = deeper). The original threshold check (`current_depth < SAFE_DEPTH`) compared a negative number to a positive one, so it was true on every single frame — the "shallow water" warning fired constantly regardless of actual depth. The fix compares distance from the surface instead: `abs(current_depth) < SAFE_DEPTH`. With this data, the ship's smoothed depth never actually comes within 10 m of the surface, so the corrected script (correctly) never raises a false alarm — it only would if the ship genuinely got that shallow.

**Run it:**
```bash
pip install pandas numpy matplotlib
python3 depth_monitor.py
```

---

## Task 2 — Keeping Watch Over Odysseus

**Files:** `odysseus_watch.ino`, Tinkercad circuit (see screenshot)

**Circuit**

| Component | Arduino Pin |
|---|---|
| Push button | D2 (INPUT_PULLUP, other leg to GND) |
| LED (+ resistor) | D3 |
| Buzzer | D4 |
| LCD (RS, E, D4–D7) | D7, D8, D5, D6, D11, D12 |
| HC-SR04 TRIG / ECHO | D9 / D10 |
| LDR (voltage divider) | A0 |

The LDR forms a voltage divider with a fixed resistor so light level shows up as an analog voltage; the LED and buzzer each sit behind a current-limiting resistor; the button uses the Arduino's internal pull-up so no extra resistor is needed for it.

**Approach — a five-state machine**

- `OPEN_SEA` (default) checks the sensors each loop: if light drops below half-scale (`analogRead < 512`) it moves to `STORM`; if the ultrasonic sensor reads under 100 cm it moves to `CHARYBDIS`. Storm is checked first, so a simultaneous trigger resolves to `STORM`.
- `STORM` blinks the LED (toggled every 300 ms via `millis()`, non-blocking) and starts a danger timer. If the light recovers before 5 s, it returns to `OPEN_SEA`; otherwise it becomes `WRECKED`.
- `CHARYBDIS` sounds the buzzer continuously and runs the same 5-second timer/escape logic, using the distance reading instead of light.
- `ANCHOR_DROPPED` is reached from any of `OPEN_SEA`, `STORM`, or `CHARYBDIS` via a button press (detected on the HIGH→LOW edge, debounced with a short delay). It forces the LED/buzzer off and returns early from the loop before sensors are even read, so the ship is fully protected. Pressing the button again returns to `OPEN_SEA`; if a storm or Charybdis is still physically present, it's re-detected on the very next loop with a fresh timer — matching the "resets the timer" requirement.
- `WRECKED` is a dead end: LED and buzzer lock on, and no button press or sensor state is handled for it, so nothing in the code can move the ship out of it short of restarting the simulation.

All timing (LED blink, 5-second wreck countdown) is done by comparing `millis()` timestamps rather than using blocking `delay()` calls, so the button and sensors stay responsive at all times. The current state and a short message are written to the LCD via a shared `displayState()` helper every time the state changes.

**Bug found and fixed:** the original code only handled anchor-drop presses while in `OPEN_SEA` or `ANCHOR_DROPPED`. That meant pressing the button during a `STORM` or near `CHARYBDIS` did nothing — there was no way to actually save the ship before the 5-second wreck timer ran out, which contradicts the spec directly. The fix widens the drop-anchor condition to include `STORM` and `CHARYBDIS`.

**Simulate it:** open the Tinkercad circuit, paste `odysseus_watch.ino` into the code editor, and start the simulation.
