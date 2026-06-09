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
| **Luca Salmoiraghi** | 10849129    | Logic, DevOps, CI/CD Pipeline & SLURM    | XXh            |
| **Dong Hua Zhu**     | 10827613    | e.g., QA, Pytest & Singularity Container | XXh            |

Group of 3. **Parallelisation**:
Parallelisation logic and testing was mostly handled by Tobia Grimaldi with the aid of Dong Hua Zhu with class and functions refactoring.

---

## Running the software

### Compiling and running the software

To build and run the software, the following stepst should be performed:

1. create a `build` directory and move inside it:

```bash
mkdir build
cd build
```

2. generate the build dependencies by invoking:

```bash
cmake ..
```

3. build the software (as well as its dependencies and the test suite): 

```bash
make
```

4. return to the project's main folder. The software can be executed by invoking:

```bash
cd ..
./build/astralog
```
To actually receive data to be processed, first the collector has to be activated.

### Running the collector

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
---

## Repository Structure
The repository is organized into the following main directories:

```
.
├── docs
│   ├── Configuration_files
│   └── images
│       └── SequenceDiagImages
├── external
├── input
├── singularity
├── src
│   ├── components
│   ├── interfaces
│   └── types
│       └── rules
└── tests
    ├── components
    ├── integrations
    └── types
```


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
The AtroLog software base relies on a **Component-Based Architecture**. Each component of the system performs a specific, well-defined job and interacts with the others exclusively through interfaces, ensuring reliability, scalability and maintainability.

The system is composed of 7 components: DataIngestor, BatchAccumulator, ThreadSafeBuffer, RuleEngine, RuleLoader, OutputDispatcher and MeasDatabase.

The data flow follows a clear pipeline: DataIngestor receives raw stream data from external sources, filters out malformed JSON entries, and parses the valid ones. The parsed data is forwarded to BatchAccumulator, which buffers incoming records until a configured batch size is reached, then pushes the batch into ThreadSafeBuffer. The RuleEngine consumes batches from the buffer, applies the configured rules, and sends a result summary to OutputDispatcher.

To structure the pipeline, three design patterns are employed:

- **Producer-Consumer**: BatchAccumulator acts as the producer, pushing batches into the ThreadSafeBuffer queue, while RuleEngine acts as the consumer, extracting and processing them. This decouples data accumulation from rule processing and ensures thread-safe communication between the two
- **Strategy**: RuleEngine acts as the context and operates on a BaseRule abstract class. Each concrete rule is a subclass of BaseRule, allowing different rule implementations to be selected and applied interchangeably at runtime without modifying the engine logic
- **Centralized Rule Creation**: RuleLoader is responsible for instantiating the correct BaseRule subclass based on the rule configuration, centralizing and decoupling object creation from the rest of the system.

### Simplifications and variations (if any)

**To be completed**
The RuleLoader component was originally conceived following the Factory Method pattern, where separate creator subclasses would each be responsible for instantiating a specific BaseRule subtype.

In the actual implementation, this was simplified into a Simple Factory approach: a single loadRules method dispatches object creation through an if/else if chain based on the rule "type" field, with dedicated private parsing methods (parseSimpleRule, parseStepDifferenceRule, parseStatefulRule, parseLogicalCorrelationRule) for each concrete type. While this does not constitute a formal GoF pattern, it preserves the core intent of centralising and decoupling rule instantiation from the rest of the system.

### Distribution and parallelization approach

**Description of the parallelism strategy adopted:**

In the first place our idea was to exploit the evaluation of rules in parallel, still respecting the constraint of the `priority` limitation which imposed us to evaluate rules in a certain sequential order.

We used OpenMP for the thread parallelism. Rules are evaluated in parallel within each priority group, and each thread accumulates results locally; the shared `rules_cache` is updated only after the parallel section to avoid race conditions.

To scale across processes, we added an MPI-based rule engine (enabled when compiling with `ASTRALOG_MPI`). Rank 0 broadcasts the telemetry batch, each rank evaluates a subset of rules (still by priority, with OpenMP inside each rank), and results are synchronized with a `Gather` so every rank updates its cache consistently.

In practice, the MPI communication overhead (broadcast + all-gather) dominated at the scale of the provided dataset. In our setup (with roughly ~20 rules), the OpenMP version was consistently faster, and performance degraded as the number of MPI ranks increased.

#### Commands to run in order to test parallelisation performance

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

### Test Suite Architecture & Rationale

The testing framework is designed to validate the entire lifecycle of the AstraLog-HPC pipeline, ensuring strict operational correctness from initial JSON ingestion up to rule evaluation.
The testing strategy directly maps to the modular components of our C++ architecture, ensuring decoupled unit testing, zero-side-effect profiling, and deterministic verification:

- **Ingestion & Validation ([`test_data_ingestor.cpp`](tests/components/test_data_ingestor.cpp)):**
  - _Rationale:_ Telemetry streams from spacecraft are highly susceptible to communication noise and file system structural anomalies.
  - _Test Coverage:_ Verifies file I/O robustness, data mapping correctness, and real-world integrity integration.

- **Batch Accumulation & Memory Buffering ([`test_batch_accumulator.cpp`](tests/components/test_batch_accumulator.cpp)):**
  - _Rationale:_ Telemetry packets must be buffered up to specified constraints before dispatching to the processing object to minimize processing invocation overhead.
  - _Test Coverage:_ Verifies threshold dynamics, overflow and carry-over semantics, and FIFO queue ordering.

- **Thread-Safe Buffering & Back-Pressure Synchronization ([`test_ThreadSafeBuffer.cpp`](tests/components/test_ThreadSafeBuffer.cpp)):**
  - _Rationale:_ This template class forms the primary inter-thread communication line between ingestion threads and evaluation worker pools. A race condition or stalling issue here could dead-lock an entire HPC cluster node.
  - _Test Coverage:_ Verifies zero-copy move semantics, capacity back-pressure handling, clean pipeline teardown, and concurrency stress testing.

- **Polymorphic Rule Evaluation Engine (`test_*_rule.cpp`):**
  - _Rationale:_ The system relies on an extensible collection of logic filters—ranging from instant scalar thresholds to stateful sliding windows and recursive dependency trees. Each specialized rule must process telemetry deterministically under strict behavioral constraints.
  - _Test Coverage:_ Verifies behavioral specifications across individual rule classes and validates comprehensive fault tolerance.

- **Rule Parsing Configuration ([`test_RuleLoader.cpp`](tests/components/test_RuleLoader.cpp)):**
  - _Rationale:_ Space operators configure system behaviors dynamically via JSON specifications. The component must check first the JSON format correctness before injecting into the system.
  - _Test Coverage:_ Verifies polymorphic object creation, strict priority ordering mechanics, and error fallback adaptability.

- **Rule Engine Execution Lifecycle ([`test_rule_engine.cpp`](tests/components/test_rule_engine.cpp)):**
  - _Rationale:_ Validates that the engine cleanly coordinates chronological sliding-window events and tracks evaluation cache states across alternating batches.
  - _Test Coverage:_ Verifies execution flows under isolated framework conditions, handles temporal evaluation limits, and asserts ternary state classifications.

- **Thread-Safe Structured Logging ([`test_OutputDispatcher.cpp`](tests/components/test_OutputDispatcher.cpp)):**
  - _Rationale:_ In a multi-threaded consumer architecture, multiple threads may trigger ESA compliance violations simultaneously. Unsynchronized file access would lead to race conditions, overlapping text fragments, or file corruption.
  - _Test Coverage:_ Verifies output format compliance, missing state fallbacks, and complex rule data relationships.

- **End-to-End System Orchestration ([`test_astralog.cpp`](tests/test_astralog.cpp)):**
  - _Rationale:_ A full integration test guarantees that the full multi-threaded ingestion-to-evaluation application pipeline shuts down safely, prevents side effects across tests, and handles disk cleanup identically to a production run on an ESA cluster.
  - _Test Coverage:_ Verifies sandbox environment isolation, raw ingestion input cleanup, and deterministic application lifecycles.

#### How to run tests
The Catch2-based tests can be run locally, leveraging the ctest framework.

To run them locally, first build the project. While in the build directory, the entire test suite can be run by invoking:

```bash
ctest
```

**THIS IS STILL STUFF FROM REALE, NEED TO MODIFY**

We implemented our test suite using `pytest`. The tests are located in `tests/test_collector.py`.

- **Rationale behind test cases:** The tests were designed to cover the core business logic without requiring an active MQTT connection.

To run the tests locally:

```bash
python3 -m pytest tests/
```

---

---

## Pipeline & DevOps Workflow

This project implements a multi-step automation pipeline distributed across the **developer's local machine**, **GitHub servers**, and the **Cineca Galileo100 cluster**.

### CI workflow (All branches)

The following steps are triggered automatically at every commit across all branches:

1. **Developer Local Machine**
   - **Linting:** `pre-commit` automatically checks code against LLVM style guidelines.
   - **Formatting:** If non-compliant, the commit is blocked and `clang-format` is applied. _The developer must then re-commit and push._
2. **GitHub Servers (GitHub Actions)**
   - **Style Guard:** Verifies LLVM compliance (in case local pre-commits were bypassed).
   - **Static Analysis:** Executes security and quality scans via `CodeQL`.
   - **Unit Testing:** Compiles the codebase and runs the unit test suite.
   - **Memory Profiling:** Checks for memory leaks and errors using `Valgrind`.
   - **Integration Testing:** Executes full end-to-end integration tests.

Configuration files can be found:

- [pre-commit config file](.pre-commit-config.yaml) for the `pre-commit` software. Note that this configuration is applied automatically every time the project is builded.
- [GitHub pipeline config file](.github/workflows/automated_CI.yml) for the GitHub actions workflow.

### CD workflow (Main branch only)

When changes are merged into the `main` branch, the pipeline executes these additional steps:

3. **GitHub Servers**
   - **Repository Mirroring:** A GitHub Actions workflow mirrors the repository content directly to Cineca's internal GitLab instance (this happens thanks to a secure connection established using token provided through GitHub Secrets)
4. **Galileo100 Cluster (Cineca GitLab CI)**
   - **Containerisation:** Builds the execution container using `Apptainer`/`Singularity`.
   - **HPC Orchestration:** Submits the compiled job to the cluster for execution via the `SLURM` scheduler.

Configuration files can be found:

- [mirroring action](.github/workflows/automated_CI.yml#L175-L191) for the GitHub to GitLab mirroring action
- [GitLab deployment on Galileo100](.gitlab-ci.yml) for the actual deployment on Galileo100 cluster.

> [!WARNING]
> **Disclaimer / Proof of Concept**
>
> The final two deployment steps are strictly a **proof of concept**. Cineca does not currently provide an official, fully automated method to submit SLURM jobs through the login node.
>
> Access to Galileo100 is achieved via a workaround utilizing the _experimental_ CI/CD integration between Cineca's internal GitLab and the cluster. Due to this:
>
> - Execution on Galileo100 is extremely limited.
> - The pipeline configuration in [.gitlab-ci.yml](.gitlab-ci.yml) serves purely as a demonstration, as container building and full SLURM orchestration require administrative privileges not granted to users.
>   To know more, check [the detailed section](#galileo100-job-submission-ad-cicd-integration).

---

---

## Known problems and limitations

### Galileo100 job submission ad CI/CD integration

The project guidelines originally mandated a fully automated, unattended CI/CD pipeline capable of testing the software, containerising it, and automatically submitting it for execution via SLURM on Cineca's Galileo100 cluster.
However, fully automatic deployment on Cineca's clusters is restricted by system administrators due to security and policy constraints. This limitation required exploring alternative workarounds and architectural approaches to achieve the closest possible automation.
What follows is a detailed description of the tested approaches and the technical constraints that rendered them unfeasible.

#### SSH key saved through GitHub Secrets

As outlined in [Cineca's official documentation](https://docs.hpc.cineca.it/general/access.html), cluster access is strictly governed by the SSH protocol, requiring a temporary SSH key generated on-the-fly via `smallstep` alongside Two-Factor Authentication (2FA).
Because these generated keys expire after 12 hours and require mandatory human interaction for token generation, they are fundamentally incompatible with an automated GitHub Actions pipeline. Even if a user manually generated a key to seed the pipeline, the 12-hour lifespan would severely restrict long-term automation capabilities.

#### Listener script on Cineca's login node

A [listener script](singularity/listener.sh) designed for execution within `tmux` has been provided. When active on the cluster's login node, this script periodically (every minute) monitors the repository status, detects new commits, polls the GitHub pipeline until completion, and, upon success, submits a SLURM job to the compute nodes.
However, due to strict process limitations enforced on the login node, script execution is automatically terminated after 10 hours, severely restricting long-term automation capabilities.

> [!WARNING]
> **Important Disclaimer**
>
> Attempting this approach on any shared cluster is **highly discouraged**. Even in environments where processes are not automatically killed, running unattended, long-polling scripts on login nodes violates standard HPC etiquette. Login nodes are strictly reserved for light, sporadic tasks (such as code compilation or job submission). Please avoid using this approach in production environments.

#### Integration through Cineca's internal GitLab

This last option is the one currently employed. GitHub mirrors the content of the repository on Cineca's internal GitLab. This gives access to automatic execution on Galielo100 cluster. Unfortunately, the execution enviroment is highly limited, since the GitLab CI runners provided by CINECA run in unprivileged Docker containers based on Alpine Linux. This prevents the use of apptainer build from a definition file, which requires either `root` privileges or `fakeroot` support (`newuidmap`), neither of which is available. Moreover, the `sbatch` command is not available inside the CI containers, and `slurmrestd` is not exposed to the pipeline environment.

These constraints were confirmed both through direct testing and by CINECA's user support team, who explicitly stated that automated job submission from a CI/CD pipeline is not supported on Galileo100.
As a result, the GitLab CI pipeline demonstrates the intended HPC workflow in a simulated environment: it just provieds how a container should be build and then submitted through SLURM.

---

---

## License

This project is licensed under the **MIT License**. See the [`LICENSE`](LICENSE) file for more details.

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

```bash
# SBATCH POLLER ON THE CLUSTER

#!/bin/bash
# ~/astralog/singularity/poller.slurm
#SBATCH --job-name=astralog-poller
#SBATCH --time=00:10:00
#SBATCH --partition=whatever
#SBATCH --output=%HOME/logs/poller_%j.out
#SBATCH --error=%HOME/logs/poller_%j.err

# Run the deploy logic
~/bin/poll_and_deploy.sh

# this is the only way I found to run a script multiple times on cineca cluster
# also I check if there is already an active request to not populate the queue
ALREADY_QUEUED=$(squeue --me --name=astralog-poller --noheader | wc -l)
if [[ "$ALREADY_QUEUED" -eq 0 ]]; then
  sbatch --begin=now+600 ~/astralog/singularity/poller.slurm
  echo "Poller called"
else
  echo "Poller already in queue, not resubmitting."
fi
```
