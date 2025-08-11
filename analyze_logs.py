#!/usr/bin/env python3
"""
analyze_logs.py
- Reads logs/sample.log
- Produces logs/sample.csv
- Produces metrics.txt with CPU utilization, context switches, per-task times
- Saves a Gantt chart to plots/gantt.png
Run:
    python analyze_logs.py
"""

import os, re, csv
from collections import defaultdict
import matplotlib.pyplot as plt

LOGFILE = "logs/sample.log"
CSVFILE = "logs/sample.csv"
METRICS = "metrics.txt"
PLOTDIR = "plots"
os.makedirs(PLOTDIR, exist_ok=True)

line_re = re.compile(r'^\s*(\d+)\s*ms:\s*([^\s]+)\s*(.*)$')

events = []
if not os.path.exists(LOGFILE):
    print(f"Log file not found: {LOGFILE}")
    raise SystemExit(1)

with open(LOGFILE, 'r') as f:
    for line in f:
        m = line_re.match(line.strip())
        if not m:
            continue
        t = int(m.group(1))
        task = m.group(2)
        rest = m.group(3).strip()
        events.append((t, task, rest))

# Write CSV
with open(CSVFILE, 'w', newline='') as cf:
    w = csv.writer(cf)
    w.writerow(['time_ms', 'task', 'event'])
    for t, task, rest in events:
        w.writerow([t, task, rest])
print("Wrote", CSVFILE)

# Build timeline for running events
timeline = {}
for t, task, rest in events:
    if rest.startswith('running'):
        timeline[t] = task

if not timeline:
    print("No running events found in log. Exiting.")
    raise SystemExit(1)

# Compress into intervals
times = sorted(timeline.keys())
intervals = defaultdict(list)
s = times[0]
prev = times[0]
prev_task = timeline[s]
for t in times[1:]:
    cur_task = timeline[t]
    if cur_task == prev_task and t == prev + 1:
        prev = t
        continue
    intervals[prev_task].append((s, prev))
    s = t
    prev = t
    prev_task = cur_task
intervals[prev_task].append((s, prev))

# Metrics
total_time = max(times) + 1
running_count = len(times)
cpu_util = running_count / total_time * 100.0

context_switches = 0
last = None
for t in times:
    cur = timeline[t]
    if last is None:
        last = cur
    else:
        if cur != last:
            context_switches += 1
            last = cur

runtimes = {task: sum((end - start + 1) for start, end in ivals) for task, ivals in intervals.items()}

with open(METRICS, 'w') as mf:
    mf.write(f"Total simulated time (ms): {total_time}\n")
    mf.write(f"CPU busy ms: {running_count}\n")
    mf.write(f"CPU utilization (%): {cpu_util:.2f}\n")
    mf.write(f"Context switches: {context_switches}\n")
    mf.write("Per-task runtimes (ms and %):\n")
    for task, rt in sorted(runtimes.items()):
        mf.write(f"  {task}: {rt} ms ({rt/total_time*100:.2f}%)\n")

print("Wrote", METRICS)

# Plot Gantt chart
tasks_list = sorted(intervals.keys())
fig_height = max(2, 0.6 * len(tasks_list) + 1)
fig, ax = plt.subplots(figsize=(10, fig_height))
ypos = list(range(len(tasks_list)))

for i, task in enumerate(tasks_list):
    segs = [(start, end - start + 1) for start, end in intervals[task]]
    ax.broken_barh(segs, (i - 0.3, 0.6))

ax.set_yticks(ypos)
ax.set_yticklabels(tasks_list)
ax.set_xlabel('Time (ms)')
ax.set_ylabel('Tasks')
ax.grid(axis='x', linestyle='--', linewidth=0.5)
plt.tight_layout()
plot_path = os.path.join(PLOTDIR, "gantt.png")
plt.savefig(plot_path)
print("Saved Gantt chart:", plot_path)
