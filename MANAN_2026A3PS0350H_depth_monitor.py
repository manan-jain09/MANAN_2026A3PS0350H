"""
Task 1: Finding the Sea Floor
Reads depth sensor data, cleans erratic/corrupted readings, smooths noise,
and animates a depth-vs-time plot with a shallow-water warning.
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# ---- Config ----
FILE_NAME = "depth_data.csv"
DEPTH_COLUMN = "Depth (m)"
SAFE_DEPTH = 10                  # meters from the surface considered "too shallow"
ROLLING_WINDOW = 5
MAD_THRESHOLD_MULTIPLIER = 6

# ---- 1. Grab the data ----
data = pd.read_csv(FILE_NAME)
depth = pd.to_numeric(data[DEPTH_COLUMN], errors="coerce")  # corrupted cells (e.g. "#VALUE!") -> NaN
time = np.arange(len(depth))                                # 1 sample/sec, per the task

# ---- 2. Detect and remove erratic/corrupted points ----
# A point is "bad" if it deviates too far from its local (rolling) median,
# using a rolling Median Absolute Deviation (MAD) as the yardstick.
# MAD-based detection is robust to outliers themselves (unlike mean/std),
# so a single spike doesn't distort the threshold used to catch it.
rolling_median = depth.rolling(window=ROLLING_WINDOW, center=True, min_periods=1).median()
deviation = (depth - rolling_median).abs()
mad = deviation.rolling(window=ROLLING_WINDOW, center=True, min_periods=1).median()
threshold = MAD_THRESHOLD_MULTIPLIER * mad
bad_data = deviation > threshold

depth_clean = depth.mask(bad_data)                          # drop flagged points
depth_clean = depth_clean.interpolate(method="linear", limit_direction="both")  # fill gaps

# ---- 3. Reduce random noise (brownie points) ----
smooth_depth = depth_clean.rolling(window=ROLLING_WINDOW, center=True, min_periods=1).mean()

# ---- 4. Animated, labeled depth-time graph ----
fig, ax = plt.subplots(figsize=(10, 6))
ax.set_title("Ship Depth Monitoring System")
ax.set_xlabel("Time (seconds)")
ax.set_ylabel("Depth (m, negative = below surface)")
ax.set_xlim(time.min(), time.max())
ax.set_ylim(smooth_depth.min() * 1.1, max(smooth_depth.max() * 1.1, 1))
ax.axhline(-SAFE_DEPTH, color="red", linestyle="--", linewidth=1,
           label=f"Safe depth line (-{SAFE_DEPTH} m)")
ax.legend(loc="upper right")
ax.grid(alpha=0.3)

line, = ax.plot([], [], color="steelblue", linewidth=2, label="Smoothed depth")
warning_text = ax.text(
    0.02, 0.95, "", transform=ax.transAxes,
    color="red", fontsize=11, fontweight="bold", va="top"
)


def init():
    line.set_data([], [])
    warning_text.set_text("")
    return line, warning_text


def update(frame):
    line.set_data(time[:frame + 1], smooth_depth.iloc[:frame + 1])

    current_depth = smooth_depth.iloc[frame]
    # Shallow-water check: how close to the surface (0), not "less than a positive number".
    if pd.notna(current_depth) and abs(current_depth) < SAFE_DEPTH:
        warning_text.set_text(f"WARNING: Shallow water! Depth = {current_depth:.1f} m")
    else:
        warning_text.set_text("")

    return line, warning_text


anim = FuncAnimation(
    fig,
    update,
    frames=len(smooth_depth),
    init_func=init,
    interval=1000,   # 1 new point per second, matching the sensor's sample rate
    blit=True,
    repeat=False,
)

plt.tight_layout()
plt.show()
