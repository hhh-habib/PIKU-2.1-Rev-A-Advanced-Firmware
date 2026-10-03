# V3.3 owner physical checklist

**Stage 5D / V3.3 / Dashboard v1.0 physical validation NOT RUN.** All items pending.

- [ ] Ordinary AUTO keeps its 35-degree policy and local autonomy.
- [ ] Boxed recovery commits a side; count at most six turn commands in one episode.
- [ ] No repeated near-full-circle / 300+ degree pattern; 150 degrees is a target sum, physical angle may differ.
- [ ] Interrupted turns consume attempts; cooldown and brief clear handoff retain budgets.
- [ ] IR clearance remains at most five approximately 200 ms pulses per episode.
- [ ] Sonar disconnect/stale immediately stops/HOLDs without spin or reverse.
- [ ] Sonar restore uses two fresh trusted front samples and resumes the same episode through fresh sensing.
- [ ] Exhaustion/hazard/other holds remain stopped and report their real state.
- [ ] Stationary fresh clear evidence can release assistance HOLD without more turns.
- [ ] Stable trusted forward progress resets an episode; mode restart/reboot reset explicitly.
- [ ] Enable eligible Override; release and STOP preserve it; a fresh direction press is required.
- [ ] Speed, turn and Sensor Head interactions preserve permission and applicable safety stops.
- [ ] Session loss stops hazardous movement without disabling permission; fresh session does not auto-resume.
- [ ] Explicit Disable, leaving MANUAL, reboot, stable hazard-clear and critical safety revoke Override.

RF PA/range engineering NOT RUN and remains deferred. No item is marked passed by software tests.
