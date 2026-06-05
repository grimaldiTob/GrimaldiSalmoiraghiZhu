# AstraLog-HPC: Full Track Implementation

### **Software Engineering for HPC - A.Y. 2025-2026**

This repository contains the **Full Track** solution for the **AstraLog-HPC** project, developed to respond to a simulated "Call for Tenders" issued by the European Space Agency (ESA).

**Selected track**: Full

---

## AstraLog Control

Here you can access the official documentation hub and web interface for the **AstraLog-HPC** project.

**[Click here to access AstraLog Control](https://simonereale.github.io/astralog-control/)**

---

## Team Members & Effort

**To be changed**

| Name Surname         | Person Code | Role / Main Focus                        | Effort (Hours) |
| :------------------- | :---------- | :--------------------------------------- | :------------- |
| **Tobia Grimaldi**   | 11127377    | Logic, Parallelization & Singularity     | XXh            |
| **Luca Salmoiraghi** | 12345678    | e.g., DevOps, CI/CD Pipeline & SLURM     | XXh            |
| **Dong Hua Zhu**     | 12345678    | e.g., QA, Pytest & Singularity Container | XXh            |

Group of 3. **Parallelization**:
Parallelization logic and testing was mostly handled by Tobia Grimaldi with the aid of Dong Hua Zhu with class and functions refactoring.

---

## Repository Structure

Quick map of the main folders and files:

- `src/`: C++ and Python sources: here we defined the core AstraLog logic with its components, interfaces and types.
- `tests/`: C++ Catch2 tests, integration fixtures and parallelization testing to assess the model performance.
- `docs/`: Latest version of our requirement analysis and specification document.
- `input/`: Sample CSV datasets and `rules.json`.
- `collector_output/`: Runtime directory used to collect AstraLog control data.
- `external/`: Reference to external libraries used in the project (just `simdjson` in this case).
- `singularity/`: Singularity container definition.
- `.github/`: CI workflows;

In addition to these folders we also have main directory files, scripts (`build.sh`, `submit_job.sh`, `listener.sh`) and configs (`CMakeLists.txt`, `requirements.txt`).

---

## Software Organization & Architecture

**To be completed**

### Language and Libraries

- **Language:** C++17 (for core pipeline) and Python 3 (using legacy code provided in the start).
- **Libraries:**
  - `simdjson`: for rules and measurements efficient parsing.
  - `Catch2`: C++ unit testing framework used to define and run the test suite.
  - `OpenMP`: used to define the main parallelism workflow.
  - `OpenMPI`: used to increase the level of parallelism (ended up reducing the speedup).
  - `paho-mqtt`: used to connect our application to the Broker of the 'fake' spaceship simulation.
  - `pytest` TO REMOVE I GUESS ???

### Architecture & Relation to Phase 1

**To be completed**

### Simplifications and variations (if any)

**To be completed**

### Distribution and parallelization approach

**Description of the parallelism strategy adopted:**

In the first place our idea was to exploit the evaluation of rules in parallel, still respecting the constraint of the `priority` limitation which imposed us to evaluate rules in a certain sequential order.

We used OpenMP for the thread parallelism. Rules are evaluated in parallel within each priority group, and each thread accumulates results locally; the shared `rules_cache` is updated only after the parallel section to avoid race conditions.

To scale across processes, we added an MPI-based rule engine (enabled when compiling with `ASTRALOG_MPI`). Rank 0 broadcasts the telemetry batch, each rank evaluates a subset of rules (still by priority, with OpenMP inside each rank), and results are synchronized with a `Gather` so every rank updates its cache consistently.

In practice, the MPI communication overhead (broadcast + all-gather) dominated at the scale of the provided dataset. In our setup (with roughly ~20 rules), the OpenMP version was consistently faster, and performance degraded as the number of MPI ranks increased.

**Commands to run in order to test parallelization performance**

```bash
cmake --build build --target benchmark_parallelization
./build/benchmark_parallelization --engine omp --measurements 100000 --batch-size 1000 --rules 1000 --iterations 3
```

for OMP

```bash
mpirun -n 4 ./build/benchmark_parallelization --engine mpi --measurements 100000 --batch-size 1000 --rules 1000 --iterations 3
```

for MPI

The results obtained are coherent with what we expected. The simpler and lighter implementation of OpenMP is **relatively faster** (~20%/~30% speedup) against the OpenMPI+OpenMP implementation.

In addition to that we can notice how the performance of OpenMPI implementation gets worse if we increase the number of parallel processes, demonstrating how **the bottleneck of the application is indeed the telemetry broadcasting** and **gather** operations performed.

### Usage of AI (if any)

We used AI to accelerate testing and performance assessment. Some scripts where used to generate deterministic rules and test scenarios.

In particular, we used AI models (`Gemini`, `Claude`) to generate scripts that tested our model parallelization performance. This allowed rapid andrepeatable comparisons between the OpenMP and OpenMP+MPI variants and helped identify that MPI communication overhead dominated the execution time. AI-driven test generation and result aggregation significantly sped up our benchmarking iterations.

---

## Testing & Rationale

**To be completed**

---

## Pipeline & DevOps Workflow

**To be completed**

---

## License

**Change this if you prefer to adopt a different license**

This project is licensed under the **MIT License**. See the `LICENSE` file for more details.

---

## Instructors' notes on the current content of this template

### Current repository structure

```text
.
├── csv_input/                 # Input datasets for groups of two and three students
│   ├── export_sat_alpha_large.csv
│   ├── export_sat_alpha_medium.csv
│   └── export_sat_alpha_small.csv
├── requirements.txt           # Python dependencies (paho-mqtt, pytest)
├── src/
│   ├── astralog_collector.py  # Core ingestion & ESA validation logic
│   └── __init__.py
└── tests/
    ├── test_collector.py      # Automated test suite
    └── __init__.py
```

### ESA Compliance & Implementation Details

The following website acts as documentation hub for the project and contains AstraLog Control, a wed dashboard that allows you to visualize the messages incoming from the spacecrafts' digital twins through the MQTT broker: https://github.com/SimoneReale/astralog-control. Please analyze its content carefully.

Groups of two and three students will find in 'csv_input' three csv files containing sets of events generated by spacecrafts' sensors. The three sets have all the same structure and contain different amounts of data to allow you to test your system in different situations.

Groups of four students can take inspiration or can reuse the implementation in `astralog_collector.py`. It acquires data from the MQTT broker.
`astralog_collector.py` filters incoming data to handle real-world space communication noise:

1. **Malformed JSON:** Drops packets with invalid syntax.
2. **Schema Errors:** Ensures all mandatory fields (`timestamp`, `sensor_id`, `value`, `priority`) are present.
3. **Type Errors:** Verifies that sensor `value` is numerical.

The system supports two **Batch Accumulation Strategies**:

- **Count-based:** Flushes to a timestamped `.txt` file every _N_ valid messages.
- **Time-based:** Flushes to a timestamped `.txt` file every _N_ milliseconds.

### Local Setup & Usage

#### 1. Installation

Install the required dependencies:

```bash
pip install -r requirements.txt
```

#### 2. Running the Collector

To avoid module resolution errors, always run the module from the root directory using the `-m` flag.

Before run the command, make sure to not runn with a eduroam connnection

**Example 1: Batch every 100 valid messages**

```bash
python3 -m src.astralog_collector --mode count --limit 100
```

**Example 2: Batch every 5000 milliseconds (5 seconds)**

```bash
python3 -m src.astralog_collector --mode time --limit 5000
```

### Test Suite Architecture & Rationale

The testing framework is designed to validate the entire lifecycle of the AstraLog-HPC pipeline, ensuring strict operational correctness from initial JSON ingestion up to rule evaluation.

#### Test Suite Architecture & Rationale

The testing strategy directly maps to the modular components of our C++ architecture, ensuring decoupled unit testing, zero-side-effect profiling, and deterministic verification:

* **Ingestion & Validation (`test_data_ingestor.cpp`):**
    * *Rationale:* Telemetry streams from spacecraft are highly susceptible to communication noise and file system structural anomalies.
    * *Test Coverage:* Verifies file I/O robustness, data mapping correctness, and real-world integrity integration.

* **Batch Accumulation & Memory Buffering (`test_batch_accumulator.cpp`):**
    * *Rationale:* Telemetry packets must be buffered up to specified constraints before dispatching to the processing object to minimize processing invocation overhead.
    * *Test Coverage:* Verifies threshold dynamics, overflow and carry-over semantics, and FIFO queue ordering. 

* **Thread-Safe Buffering & Back-Pressure Synchronization (`test_ThreadSafeBuffer.cpp`):**
    * *Rationale:* This template class forms the primary inter-thread communication line between ingestion threads and evaluation worker pools. A race condition or stalling issue here could dead-lock an entire HPC cluster node.
    * *Test Coverage:* Verifies zero-copy move semantics, capacity back-pressure handling, clean pipeline teardown, and concurrency stress testing.

* **Polymorphic Rule Evaluation Engine (`test_*_rule.cpp`):**
    * *Rationale:* The system relies on an extensible collection of logic filters—ranging from instant scalar thresholds to stateful sliding windows and recursive dependency trees. Each specialized rule must process telemetry deterministically under strict behavioral constraints.
    * *Test Coverage:* Verifies behavioral specifications across individual rule classes and validates comprehensive fault tolerance. 

* **Rule Parsing Configuration (`test_RuleLoader.cpp`):**
    * *Rationale:* Space operators configure system behaviors dynamically via JSON specifications. The component must check first the JSON format correctness before injecting into the system.
    * *Test Coverage:* Verifies polymorphic object creation, strict priority ordering mechanics, and error fallback adaptability. 

* **Rule Engine Execution Lifecycle (`test_rule_engine.cpp`):**
    * *Rationale:* Validates that the engine cleanly coordinates chronological sliding-window events and tracks evaluation cache states across alternating batches.
    * *Test Coverage:* Verifies execution flows under isolated framework conditions, handles temporal evaluation limits, and asserts ternary state classifications.

* **Thread-Safe Structured Logging (`test_OutputDispatcher.cpp`):**
    * *Rationale:* In a multi-threaded consumer architecture, multiple threads may trigger ESA compliance violations simultaneously. Unsynchronized file access would lead to race conditions, overlapping text fragments, or file corruption.
    * *Test Coverage:* Verifies output format compliance, missing state fallbacks, and complex rule data relationships.

* **End-to-End System Orchestration (`test_astralog.cpp`):**
    * *Rationale:* A full integration test guarantees that the full multi-threaded ingestion-to-evaluation application pipeline shuts down safely, prevents side effects across tests, and handles disk cleanup identically to a production run on an ESA cluster.
    * *Test Coverage:* Verifies sandbox environment isolation, raw ingestion input cleanup, and deterministic application lifecycles.

#### How to run tests
We implemented our test suite using `pytest`. The tests are located in `tests/test_collector.py`.

- **Rationale behind test cases:** The tests were designed to cover the core business logic without requiring an active MQTT connection.

To run the tests locally:

```bash
python3 -m pytest tests/
```
---

### CI/CD PIPELINE

Just a sketch, but I have to write something otherwise I end up forgetting what I have already configured.
Multistep pipeline, both on the dev local machine and on GitHub servers:

1. At every commit, `pre-commit` automatically checks code against the LLVM style guidelines; if it doesn't comply, the commit is blocked and `clang-format` is applied automatically.
2. GitHub Actions workflows, which:
   - Check compliance with the LLVM style guidelines
   - Perform static analysis with CodeQL
   - Compile and run unit tests
   - Check for memory leaks with Valgrind
   - run complete integrate testing

---

### Singularity so far...

Run the `build.sh` script to build the container from the custom image file in `./singularity/Singularity.def`.

Wait for the image to stop the building process and run the `./script.sh`.

At the moment I specified some placeholder commands just to check if everything is working.
At some point we will change the script file in order to run the executable of the project.

In addition to that, in the `Singularity.def` file I specified the SLURM scheduler command in order to run the container on the Galileo100 cluster.

### Parallelization testing and benchmarking

Wrote a little script (AI slopped, this can be useful to write in the documentation required by prof Di Nitto). It generates on the fly a set of rules which is deterministic (much more than the rules provided by Reale) and compares the performance between OMP and MPI.

In my opinion we should remove the compilation of the executable from the CMakeLists once benchmarking is over.
In order to run the tests use the following commands:
