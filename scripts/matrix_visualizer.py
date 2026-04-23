import os
import sys
import json
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# Get the absolute path to the trace log
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
LOG_FILE = os.path.join(SCRIPT_DIR, "..", "tsacore_trace.jsonl")

# Flag to optionally read historical data from the start of the file
read_from_start = "--from-start" in sys.argv

# Set up the Matplotlib figure and subplots
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 5))
fig.canvas.manager.set_window_title("Real-time Stylus Matrix Visualizer")

# Initialize empty 9x9 heatmaps (using 'coolwarm' where 0 is neutral/white, + is red, - is blue)
f0_im = ax1.imshow(np.zeros((9, 9)), cmap='coolwarm', vmin=-100, vmax=100)
f1_im = ax2.imshow(np.zeros((9, 9)), cmap='coolwarm', vmin=-100, vmax=100)

ax1.set_title("F0 Matrix")
ax2.set_title("F1 Matrix")
fig.colorbar(f0_im, ax=ax1)
fig.colorbar(f1_im, ax=ax2)

# Global variables for state tracking
f_log = None
last_f0 = np.zeros((9, 9))
last_f1 = np.zeros((9, 9))
last_seq = 0
last_size = 0

def update(frame):
    global f_log, last_f0, last_f1, last_seq, last_size
    
    if not os.path.exists(LOG_FILE):
        return f0_im, f1_im
        
    current_size = os.path.getsize(LOG_FILE)
    
    # If file was truncated/deleted (e.g., script restarted), reset the file handle
    if current_size < last_size:
        if f_log:
            f_log.close()
        f_log = None
        
    # Open file if not open
    if f_log is None:
        f_log = open(LOG_FILE, 'r', encoding='utf-8')
        # If we are starting fresh and don't want old data, seek to the end
        if not read_from_start and current_size > 0:
            f_log.seek(0, os.SEEK_END)
            
    last_size = current_size
    
    # Read all new lines appended since last check
    lines = f_log.readlines()
    if not lines:
        return f0_im, f1_im
        
    updated = False
    for line in lines:
        if not line.strip():
            continue
        try:
            data = json.loads(line)
            # Only process struct-validate events containing our matrix data
            if data.get("kind") == "struct-validate" and "stylusDeref" in data:
                deref = data["stylusDeref"]
                f0_data = deref.get("F0Data")
                f1_data = deref.get("F1Data")
                
                if f0_data and len(f0_data) == 81:
                    last_f0 = np.array(f0_data).reshape((9, 9))
                    updated = True
                if f1_data and len(f1_data) == 81:
                    last_f1 = np.array(f1_data).reshape((9, 9))
                    updated = True
                    
                # Update the sequence ID for the title
                fields = data.get("fields", {})
                last_seq = fields.get("SequenceId", last_seq)
        except json.JSONDecodeError:
            pass # Ignore malformed lines
            
    if updated:
        # Update heatmap data
        f0_im.set_array(last_f0)
        f1_im.set_array(last_f1)
        
        # Dynamically adjust the color scale based on current min/max values
        # We ensure the scale is symmetric around 0 (e.g., -500 to +500)
        abs_max = max(np.abs(last_f0).max(), np.abs(last_f1).max(), 10)
        
        f0_im.set_clim(vmin=-abs_max, vmax=abs_max)
        f1_im.set_clim(vmin=-abs_max, vmax=abs_max)
        
        fig.suptitle(f"Real-time Stylus Matrix - Frame Seq: {last_seq}")
        
    return f0_im, f1_im

print(f"Monitoring {LOG_FILE} for real-time updates...")
print("Please run 'python scripts/tsacore_frida_trace_launcher.py --process HuaweiThpService' in another terminal.")

# Run the animation loop updating every 30ms (~33 fps)
ani = animation.FuncAnimation(fig, update, interval=30, blit=False, cache_frame_data=False)
plt.tight_layout()
plt.show()
