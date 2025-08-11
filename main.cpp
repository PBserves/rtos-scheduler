// RTOS Scheduler Simulator - v0.1
// Compile: g++ -std=c++17 main.cpp -O2 -o scheduler
// Run: ./scheduler

#include <bits/stdc++.h>
using namespace std;

struct Task {
    int id;
    string name;
    int base_priority;      // original priority (larger => higher priority)
    int cur_priority;       // current effective priority (for inheritance)
    int period;             // ms (not used heavily here, for extension)
    int wcet;               // execution time of job in ms
    int remaining;          // remaining exec time in current job
    int exec_progress;      // executed ms in current job
    int release_time;       // next release time
    bool active;            // job released and not finished
    // mutex usage: start offset and duration (ms) relative to job start
    int mutex_start = -1;
    int mutex_dur = 0;
    Task(int _id=0,string _name="T",int bp=1,int p=100,int w=10,int rel=0)
        : id(_id), name(_name), base_priority(bp), cur_priority(bp),
          period(p), wcet(w), remaining(0), exec_progress(0),
          release_time(rel), active(false) {}
};

struct SimpleMutex {
    int holder_id = -1; // -1 none
};

void log_event(vector<string>& logs, int t, const string &s) {
    char buf[64];
    sprintf(buf, "%04d ms: %s", t, s.c_str());
    logs.push_back(string(buf));
}

int pick_task(const vector<Task>& tasks) {
    int best = -1;
    int best_prio = -1;
    for (int i = 0; i < (int)tasks.size(); ++i) {
        if (!tasks[i].active || tasks[i].remaining<=0) continue;
        if (tasks[i].cur_priority > best_prio) {
            best_prio = tasks[i].cur_priority;
            best = i;
        }
    }
    return best;
}

int main(){
    // SIMULATION PARAMETERS
    const int SIM_TIME = 1000; // ms
    vector<string> logs;

    // Create tasks
    // For demo we create 3 tasks that illustrate priority inversion
    // Task(id, name, base_prio, period(ms), wcet(ms), release_time(ms))
    vector<Task> tasks;
    // Low priority task (holder of mutex)
    Task t0(0, "Low", 1, 1000, 120, 0);
    t0.mutex_start = 10; t0.mutex_dur = 200; // it locks early for long time
    // Medium priority task (can preempt low normally)
    Task t1(1, "Med", 5, 1000, 50, 20);
    // High priority task that will contend for mutex
    Task t2(2, "High", 10, 1000, 30, 30);
    tasks.push_back(t0);
    tasks.push_back(t1);
    tasks.push_back(t2);

    SimpleMutex mu;

    // release initial jobs at their release_time
    for (int i=0;i<(int)tasks.size();++i){
        if (tasks[i].release_time==0) {
            tasks[i].remaining = tasks[i].wcet;
            tasks[i].exec_progress = 0;
            tasks[i].active = true;
            log_event(logs, 0, tasks[i].name + " released");
        }
    }

    // For simplicity, manually release jobs at given times
    for (int time=0; time<=SIM_TIME; ++time) {
        // releases
        for (int i=0;i<(int)tasks.size();++i){
            if (time == tasks[i].release_time && time!=0) {
                tasks[i].remaining = tasks[i].wcet;
                tasks[i].exec_progress = 0;
                tasks[i].active = true;
                tasks[i].cur_priority = tasks[i].base_priority;
                log_event(logs, time, tasks[i].name + " released");
            }
        }

        // pick a task to run
        int idx = pick_task(tasks);
        if (idx==-1) {
            // idle
            continue;
        }

        Task &tk = tasks[idx];

        // Check if this task needs to acquire mutex now
        if (tk.exec_progress == tk.mutex_start && tk.mutex_start>=0) {
            if (mu.holder_id == -1) {
                // acquire
                mu.holder_id = tk.id;
                log_event(logs, time, tk.name + " acquired mutex");
            } else if (mu.holder_id != tk.id) {
                // need to wait: block this task
                // mark waiting by setting remaining unchanged and active=false for run
                log_event(logs, time, tk.name + " blocked waiting for mutex (holder=" + tasks[mu.holder_id].name + ")");
                // apply priority inheritance: raise holder priority to waiting priority if needed
                if (tasks[mu.holder_id].cur_priority < tk.cur_priority) {
                    log_event(logs, time, "Priority inheritance: raising " + tasks[mu.holder_id].name + " prio from " +
                              to_string(tasks[mu.holder_id].cur_priority) + " to " + to_string(tk.cur_priority));
                    tasks[mu.holder_id].cur_priority = tk.cur_priority;
                }
                // Make this task inactive for scheduling (it will remain active but not runnable until mutex free)
                tk.active = false; // simulate blocked state until unblocked
                continue; // go to next time tick
            }
        }

        // run one ms of execution
        tk.remaining -= 1;
        tk.exec_progress += 1;
        char evbuf[128];
        sprintf(evbuf, "%s running (remaining=%d)", tk.name.c_str(), tk.remaining);
        log_event(logs, time, string(evbuf));

        // if this run causes mutex release
        if (tk.mutex_start>=0 && tk.exec_progress == tk.mutex_start + tk.mutex_dur && mu.holder_id == tk.id) {
            mu.holder_id = -1;
            log_event(logs, time, tk.name + " released mutex");
            // restore holder priority to base (simple model)
            tk.cur_priority = tk.base_priority;
            // now wake up any tasks that were blocked waiting for mutex
            for (auto &other : tasks) {
                if (!other.active && other.remaining>0 && other.exec_progress>=0 && other.id!=tk.id && other.mutex_start==other.exec_progress) {
                    // we blocked them earlier by setting active=false
                    other.active = true;
                    log_event(logs, time, other.name + " unblocked (mutex freed)");
                }
            }
        }

        if (tk.remaining <= 0) {
            log_event(logs, time, tk.name + " completed job");
            tk.active = false;
            // For simplicity no periodic re-release in this demo
        }
    }

    // print logs
    for (auto &s: logs) {
        cout << s << endl;
    }
    return 0;
}
