# Park-a-Lot Control System (PLCS) — Questions & Answers

---

## Activity 2 — Questions

### Did you need to use `notify_all()`? Why or why not?

Yes. In our implementation, both the gate thread and car threads wait on the same condition variable (e.g. `entry_cv`). When a car passes through and sets `entry_open = false`, it must wake the gate thread so it can close and serve the next car. If `notify_one()` were used, the scheduler might wake another car thread instead of the gate thread — that car would check its predicate (`entry_open == true`), find it false, and go back to sleep, leaving the gate thread stuck waiting indefinitely. Using `notify_all()` guarantees the gate thread wakes up, while any other cars that wake up simply re-evaluate their predicate, find it false, and return to waiting.

### Cars may not hold the line (they can overtake each other). Why does this happen? Can you think of a way to enforce ordering?

When `notify_all()` is called, all waiting car threads are woken up simultaneously and compete for the mutex. The OS scheduler decides which thread acquires the lock first, with no regard for the order in which they began waiting. This means a car that arrived later can acquire the mutex before one that has been waiting longer.

To enforce FIFO ordering, a **ticket system** could be used: each car atomically increments a shared counter when it arrives and stores its ticket number. A second counter tracks who is currently being served. A car may only proceed when `my_ticket == now_serving`. After passing through, it increments `now_serving` and calls `notify_all()` so the next car in line can check its predicate.

---

## Activity 3 — Discussion / Reflection

### Did your design risk circular wait? Why or why not?

No. Circular wait requires a cycle in the resource-allocation graph — thread A holds resource X and waits for Y, while thread B holds resource Y and waits for X. In our design the locks are always acquired in the same direction: `entry_mtx → capacity_mtx → exit_mtx`. No thread ever holds a later lock while waiting for an earlier one, so no cycle can form and circular wait is impossible.

### How did lock acquisition order influence deadlock possibility?

By enforcing a strict one-way ordering — entry gate first, then capacity, then exit gate — we eliminate the circular wait condition, which is one of the four necessary Coffman conditions for deadlock. Even in the scenario where an entering car holds `entry_mtx` and waits for `capacity_mtx` at the same time as an exiting car holds `capacity_mtx` and waits for `exit_mtx`, there is no cycle: nothing ever holds `exit_mtx` and waits for `entry_mtx` or `capacity_mtx`.

### Could a different ordering remove the circular wait risk?

The current ordering already removes it. A *different* ordering could however *introduce* the risk. For example, if exiting cars held `capacity_mtx` while blocking on `exit_mtx`, and entering cars held `entry_mtx` while blocking on `capacity_mtx`, a circular wait would become possible if a third thread held `exit_mtx` while waiting for `entry_mtx`. The key insight is that any ordering consistent with a total resource hierarchy is safe — orderings that allow cycles are not.

### How would you modify the locking strategy to avoid circular wait?

The current strategy already avoids it by following the principle of **lock ordering / lock hierarchy**: assign a global rank to every mutex (`entry_mtx = 1`, `capacity_mtx = 2`, `exit_mtx = 3`) and require that every thread always acquires mutexes in strictly increasing rank order. As long as this rule is respected across all threads, the circular wait condition can never be satisfied, and deadlock due to circular wait is structurally impossible.
