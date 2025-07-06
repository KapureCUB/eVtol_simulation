# eVtol Simulation Problem

## Description

This project simulates a fleet of electric Vertical Takeoff and Landing (eVtol) aircraft operating over a set time period. The simulation models flight dynamics, fault injection, charging behavior, and data logging. It is designed to analyze aircraft performance under probabilistic fault conditions and constrained charger availability.

### Key Assumptions

- **Aircraft Categories**: Each aircraft is of a specific type (ALPHA, BRAVO, etc.), with unique flight and charge parameters.
- **Simulation Time**: Default is 3 hours with 1ms resolution, where 1 simulated minute = 1 real-world hour.
- **State Machine**: Each aircraft runs independently in its own thread, managing states like `IN_FLIGHT`, `CHARGING`, or `FAULTED`.
- **Fault Injection**: Faults are randomly injected using an exponential distribution to simulate real-world failures. References are included in the code sections for selection of this model.
- **Charging Queue**: Aircraft are queued and assigned to 1 of 3 chargers, with real-time update on charging sessions.
- **Data Recording**: A Flight Data Recorder logs each aircraft’s parameters periodically for post-simulation analysis. 
- **Charge SOC limit**: The aircrafts only use upto 90% of the battery capacity and returns to charger/charge queue to simulate a more realistic scenario.
- **Fault handling**: Fault handling is not mentioned explicitly mentioned in the requirement doc. So and assumption that if a fualt arises, there is a 30 min service downtime in the flight, at any point. That include if it is in flight, in charge queue or charging.

### Services 
- **aircraft_simul**: Servicharging_servicece responsible for executing state machine for the different aircrafts. Interval is 50 ms and is scalable as per user. 
- **charging_service**: This service keeps track of the chargers and charge queue. It check active charging status on a specific charger and assigns aircraft to a charger when done. Serive interval is kept faster than aircraft simulation so as to minimize errors in flight time due to slower scheduling for charging service. Interval is 25 ms and is scalable as per user.
- **fault_service**: This service introduces faults in the aircrafts (simul services) according to the precalculated fault times. Interval is 20 msec.
- **data_recorder_service**: This service is responsible for logging simulation data on approximately 2sec interval. This is best effort as its a write back service but only a minimum logging interval is selected.

---

## File Overview

| File                  | Purpose                                                                 |
|-----------------------|-------------------------------------------------------------------------|
| `main.cpp`            | Main entry point for simulation setup, initialization, and teardown     |
| `ac_simul.cpp`        | Aircraft simulation loop, thread spawning, charging logic               |
| `fdr.cpp`             | Flight data recording, fault injection algorithm, and output formatting |
| `definitions.hpp`     | Constants, enums, macros, and shared type definitions                   |
| `ac_simul.hpp`        | Declarations for aircraft simulation and charger control functions      |
| `timer.cpp/hpp`       | Simulation timer functions and helpers                                  |
| `Makefile`            | Build script (if applicable)                                            |
| `evtol_sim_log.txt`   | Output log file with recorded data for analysis                         |
| `evtol_sim_input.txt` | Summary of initial inputs for aircraft simulation                       |

---

## Core Classes and Functions

### `aircraft` (class)

Represents an individual aircraft. Key methods:

- `state_machine(...)` — Handles state transitions (in-flight, fault, charging).
- `get_*()` — Accessors for aircraft stats (battery, miles, faults, etc.)

### `charger` (class)

Manages 3 charger units:

- `update_charger_stat()` — Update status (busy/ready).
- `update_usetime()` — Track charger usage time.

### Key Functions

- `spawn_threads()` — Launches aircraft threads.
- `aircraft_simul()` — Main aircraft behavior loop.
- `charging_service()` — Manages charger assignments and charge completion.
- `fault_injection()` — Populates a fault queue using exponential failure model.
- `fault_service()` — Injects faults during runtime based on schedule.
- `data_recorder_service()` — Logs flight and charge data at regular intervals.
- `sim_analysis()` — Summarizes performance and writes final results.

---

## Instructions to Build and Run

### Prerequisites

- C++17 or higher
- g++ / clang++
- `make` or manual build script

### Build

- Calling `make` via a terminal in the repo home directory will build the `evtol_sim` executible in the home directory itself. 

### Run

- Run the executible `evtol_sim` using the following command `./evtol_sim`
- Simulation time and input can be changed by chaging the below parameters in definition.hpp
    <pre><code>``` 
    #define SIMULATION_TIME_HRS 3 
    #define TOTAL_AIRCRAFTS 20 
    #define SIMULATION_FACTOR (60*1000) // 1 min = 1 hr real time 
    ``` </code></pre>

### Results

- Both input and output logs are saved in the `evtol_sim_input.txt` and `evtol_sim_log.txt` and are formated to to opened in excel convinently.
- Parameters saved in input log: Distribution of aircrafts and fault times per aircraft numbers based on the probablity

![Input log on console](https://github.com/KapureCUB/eVtol_simulation/blob/main/console_log.png)

- Parameters saved in output log: Aircraft state parameters and final simulation analysis
- Sample log included in `Sample_evtol_sim_log.txt`. This has data for 3 hours and 20 aircrafts. 
[Sample Simulation Output Spreadsheet](https://github.com/KapureCUB/eVtol_simulation/blob/main/Sample_evtol_sim_log.xlsx)

