# RTOS Scheduler Simulator

Short description:
C++ simulator demonstrating preemptive priority scheduling and a simple priority-inheritance mutex to illustrate priority inversion.

How to build:
g++ -std=c++17 main.cpp -O2 -o scheduler

How to run:
./scheduler > logs/sample.log
(or) ./scheduler

What this demo shows:
- Task release and scheduling decisions (highest effective priority runs).
- A low-priority task acquiring a mutex early, being preempted by a medium task, and a high-priority task later blocking — demonstrating priority inversion and priority inheritance.
- Log output saved to logs/sample.log

Status:
Working prototype (v0.1). Code prints a timeline of events; more features (periodic releases, multiple jobs, detailed statistics) planned.

Contact:
Prakhar Bhagat — prakhar.pb01@gmail.com
