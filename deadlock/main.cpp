/*
 * main.cpp — Park-a-Lot Control System (PLCS)
 *
 * Activity 2:
 *   Step 1: Single car        (3 round trips)
 *   Step 2: Multiple cars,    unlimited capacity
 *
 * Activity 3:
 *   Step 3: Multiple cars,    limited capacity (MAX_CAPACITY spots)
 *
 * Rules:
 *   - Only std::mutex and std::condition_variable (no semaphores, no atomics).
 *   - Each car is a thread. Entry gate and exit gate are each a thread.
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>

static const int MAX_CAPACITY = 2;  // Activity 3: only 2 parking spots

int main()
{
    // ── Entry gate shared state ───────────────────────────────────────────────
    int  cars_at_entry = 0;    // how many cars are currently waiting at entry
    bool entry_open    = false; // true while one car is passing through
    bool entry_running = true;  // set to false to shut the gate thread down
    std::mutex              entry_mtx;
    std::condition_variable entry_cv;

    // ── Exit gate shared state ────────────────────────────────────────────────
    int  cars_at_exit = 0;
    bool exit_open    = false;
    bool exit_running = true;
    std::mutex              exit_mtx;
    std::condition_variable exit_cv;

    // ── Parking capacity shared state (Activity 3) ───────────────────────────
    int capacity = MAX_CAPACITY;  // available spots; protected by capacity_mtx
    std::mutex              capacity_mtx;
    std::condition_variable capacity_cv;  // cars wait here when lot is full

    // ── Entry gate thread ─────────────────────────────────────────────────────
    auto entry_gate = [&]()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lk(entry_mtx);

            // Sleep until a car arrives, or we are told to shut down
            entry_cv.wait(lk, [&]{ return cars_at_entry > 0 || !entry_running; });
            if (!entry_running) break;

            std::cout << "[Entry Gate] Car detected — opening gate.\n";

            // Open gate and wake all cars waiting at entry
            entry_open = true;
            entry_cv.notify_all();

            // Wait until the car that passed through closes the gate
            // (car sets entry_open = false and calls notify_all)
            entry_cv.wait(lk, [&]{ return !entry_open; });

            std::cout << "[Entry Gate] Car passed  — closing gate.\n";
        }
        std::cout << "[Entry Gate] Shutting down.\n";
    };

    // ── Exit gate thread ──────────────────────────────────────────────────────
    auto exit_gate = [&]()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lk(exit_mtx);

            exit_cv.wait(lk, [&]{ return cars_at_exit > 0 || !exit_running; });
            if (!exit_running) break;

            std::cout << "[Exit Gate]  Car detected — opening gate.\n";

            exit_open = true;
            exit_cv.notify_all();

            exit_cv.wait(lk, [&]{ return !exit_open; });

            std::cout << "[Exit Gate]  Car passed  — closing gate.\n";
        }
        std::cout << "[Exit Gate]  Shutting down.\n";
    };

    // ── Car thread (Activity 2 — no capacity limit) ───────────────────────────
    auto car = [&](int id, int park_ms)
    {
        for (int i = 0; i < 3; i++)  // each car makes 3 round trips
        {
            // ── Arrive at entry ───────────────────────────────────────────────
            {
                std::unique_lock<std::mutex> lk(entry_mtx);

                cars_at_entry++;
                std::cout << "[Car " << id << "]       Waiting at entry.\n";

                entry_cv.notify_one();
                entry_cv.wait(lk, [&]{ return entry_open; });

                entry_open = false;
                cars_at_entry--;

                // notify_all because the gate thread is ALSO waiting on this CV.
                // notify_one() could accidentally wake another car instead of the gate,
                // leaving the gate stuck waiting.
                entry_cv.notify_all();

                std::cout << "[Car " << id << "]       Entered lot.\n";
            }

            // ── Park ──────────────────────────────────────────────────────────
            std::this_thread::sleep_for(std::chrono::milliseconds(park_ms));
            std::cout << "[Car " << id << "]       Done parking.\n";

            // ── Arrive at exit ────────────────────────────────────────────────
            {
                std::unique_lock<std::mutex> lk(exit_mtx);

                cars_at_exit++;
                std::cout << "[Car " << id << "]       Waiting at exit.\n";

                exit_cv.notify_one();
                exit_cv.wait(lk, [&]{ return exit_open; });

                exit_open = false;
                cars_at_exit--;
                exit_cv.notify_all();

                std::cout << "[Car " << id << "]       Exited lot.\n";
            }
        }
        std::cout << "[Car " << id << "]       All trips done.\n";
    };

    // ── Car thread with capacity limit (Activity 3) ───────────────────────────
    //
    // Lock acquisition order:
    //   Entering:  entry_mtx  →  capacity_mtx   (entry gate, then grab a spot)
    //   Exiting:   capacity_mtx  →  exit_mtx    (release spot, then exit gate)
    //
    // This consistent one-way ordering (entry → capacity → exit) prevents
    // circular wait: no thread ever holds a "later" lock while waiting for an
    // "earlier" one, so the circular-wait condition cannot be satisfied.
    //
    auto car_limited = [&](int id, int park_ms)
    {
        for (int i = 0; i < 3; i++)
        {
            // ── Step 1: Pass through entry gate  [acquires/releases entry_mtx] ─
            {
                std::unique_lock<std::mutex> lk(entry_mtx);

                cars_at_entry++;
                std::cout << "[Car " << id << "]       Waiting at entry gate.\n";

                entry_cv.notify_one();
                entry_cv.wait(lk, [&]{ return entry_open; });

                entry_open = false;
                cars_at_entry--;
                entry_cv.notify_all();

                std::cout << "[Car " << id << "]       Passed entry gate.\n";
            }
            // entry_mtx is now released before we touch capacity_mtx
            // → "entry gate → capacity mutex" ordering is respected

            // ── Step 2: Acquire a parking spot  [acquires/releases capacity_mtx] ─
            {
                std::unique_lock<std::mutex> lk(capacity_mtx);

                // Wait if the lot is full — predicate guards against spurious wake-ups
                capacity_cv.wait(lk, [&]{ return capacity > 0; });

                capacity--;
                std::cout << "[Car " << id << "]       Took a spot."
                          << " Spots left: " << capacity << "/" << MAX_CAPACITY << "\n";
            }

            // ── Step 3: Park ──────────────────────────────────────────────────
            std::this_thread::sleep_for(std::chrono::milliseconds(park_ms));
            std::cout << "[Car " << id << "]       Done parking.\n";

            // ── Step 4: Release parking spot  [acquires/releases capacity_mtx] ─
            {
                std::lock_guard<std::mutex> lk(capacity_mtx);

                capacity++;
                std::cout << "[Car " << id << "]       Released spot."
                          << " Spots left: " << capacity << "/" << MAX_CAPACITY << "\n";

                // notify_one: only one waiting car needs to wake up to take the spot
                capacity_cv.notify_one();
            }
            // capacity_mtx is now released before we touch exit_mtx
            // → "capacity mutex → exit gate" ordering is respected

            // ── Step 5: Pass through exit gate  [acquires/releases exit_mtx] ──
            {
                std::unique_lock<std::mutex> lk(exit_mtx);

                cars_at_exit++;
                std::cout << "[Car " << id << "]       Waiting at exit gate.\n";

                exit_cv.notify_one();
                exit_cv.wait(lk, [&]{ return exit_open; });

                exit_open = false;
                cars_at_exit--;
                exit_cv.notify_all();

                std::cout << "[Car " << id << "]       Exited lot.\n";
            }
        }
        std::cout << "[Car " << id << "]       All trips done.\n";
    };

    // ─────────────────────────────────────────────────────────────────────────
    // Step 1: Single car
    // ─────────────────────────────────────────────────────────────────────────
    std::cout << "=== Step 1: Single Car ===\n\n";
    {
        std::thread t_entry(entry_gate);
        std::thread t_exit(exit_gate);
        std::thread t_car(car, 0, 300);

        t_car.join();

        { std::lock_guard<std::mutex> lk(entry_mtx); entry_running = false; }
        entry_cv.notify_all();
        t_entry.join();

        { std::lock_guard<std::mutex> lk(exit_mtx); exit_running = false; }
        exit_cv.notify_all();
        t_exit.join();
    }

    // Reset — safe without locks, all Step 1 threads are joined
    entry_running = true;  exit_running = true;
    entry_open    = false; exit_open    = false;
    cars_at_entry = 0;     cars_at_exit = 0;

    // ─────────────────────────────────────────────────────────────────────────
    // Step 2: Multiple cars — unlimited capacity
    // ─────────────────────────────────────────────────────────────────────────
    std::cout << "\n=== Step 2: Multiple Cars (unlimited capacity) ===\n\n";
    {
        std::thread t_entry(entry_gate);
        std::thread t_exit(exit_gate);

        std::vector<std::thread> cars;
        cars.emplace_back(car, 1, 500);
        cars.emplace_back(car, 2, 200);
        cars.emplace_back(car, 3, 800);

        for (auto& t : cars) t.join();

        { std::lock_guard<std::mutex> lk(entry_mtx); entry_running = false; }
        entry_cv.notify_all();
        t_entry.join();

        { std::lock_guard<std::mutex> lk(exit_mtx); exit_running = false; }
        exit_cv.notify_all();
        t_exit.join();
    }

    // Reset — safe without locks, all Step 2 threads are joined
    entry_running = true;  exit_running = true;
    entry_open    = false; exit_open    = false;
    cars_at_entry = 0;     cars_at_exit = 0;
    capacity      = MAX_CAPACITY;

    // ─────────────────────────────────────────────────────────────────────────
    // Step 3: Multiple cars — limited capacity (MAX_CAPACITY = 2, 4 cars)
    // ─────────────────────────────────────────────────────────────────────────
    std::cout << "\n=== Step 3: Limited Capacity (max " << MAX_CAPACITY << " spots, 4 cars) ===\n\n";
    {
        std::thread t_entry(entry_gate);
        std::thread t_exit(exit_gate);

        std::vector<std::thread> cars;
        cars.emplace_back(car_limited, 1, 600);
        cars.emplace_back(car_limited, 2, 300);
        cars.emplace_back(car_limited, 3, 900);
        cars.emplace_back(car_limited, 4, 400);

        for (auto& t : cars) t.join();

        { std::lock_guard<std::mutex> lk(entry_mtx); entry_running = false; }
        entry_cv.notify_all();
        t_entry.join();

        { std::lock_guard<std::mutex> lk(exit_mtx); exit_running = false; }
        exit_cv.notify_all();
        t_exit.join();
    }

    return 0;
}
