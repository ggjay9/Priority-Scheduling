# Priority Scheduling with General Services

**Implement** a n-class priority scheduling for a M/D/1 queue with preemption-resume
- Each class is characterized by different arrival rate and service time
- Preemption can be disabled via NED parameters
  
All settings must be available as NED parameters in omnetpp.ini

**Collect** statistics on
- Per-class and generic average queueing and response times
- Per-class extended service time
- Per-class server utilization factor

**Compare** experimental and theoretical values of
- Per-class average response time
- Per-class server utilization factor
- Per-class extended service time
