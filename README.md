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
| **Tobia Grimaldi**   | 11127377    | Logic, Parallelisation & Singularity     | ~50h           |
| **Luca Salmoiraghi** | 10849129    | Logic, DevOps, CI/CD Pipeline & SLURM    | XXh            |
| **Dong Hua Zhu**     | 10827613    | e.g., QA, Pytest & Singularity Container | XXh            |

Group of 3. **Parallelisation**:
Parallelisation logic and testing was mostly handled by Tobia Grimaldi with the aid of Dong Hua Zhu with class and functions refactoring.

---

## Running the software

### Compiling and running the software

To build and run the software, the following steps should be performed:

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

To actually receive data to be processed, first the collector has to be activated. Note that the collector has to remain active for the whole duration of the data anlysis.

### Running the collector

#### 1. Installation

Install the required dependencies:

```bash
pip install -r requirements.txt
```

#### 2. Running the Collector

To avoid module resolution errors, always run the module from the root directory using the `-m` flag.

Before run the command, make sure to not runn with a eduroam connnection

**Example 1: Batch every 12 valid messages**

```bash
python3 -m src.astralog_collector --mode count --limit 12
```

**Example 2: Batch every 1000 milliseconds (1 second)**

```bash
python3 -m src.astralog_collector --mode time --limit 1000
```

#### 3. Run the whole System

Execute both the data collector and the astralog System with the following command in the project's home directory.

```bash
./script.sh
```

**N.B.**: in order to ingest data you need to make sure that the collector is working (which is not true with a consistent and non negligible probability).

---

## Repository Structure

The repository is organised into the following main directories:

```bash
.
├── .github
│
├── docs
│   ├── Configuration_files
│   └── images
│       └── SequenceDiagImages
├── external
├── galileo_scripts
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

- `.github/`: CI workflows;
- `docs/`: Latest version of the RAAD (Requirement Analysis and Design Document), as well as the `latex` source code to compile it.
- `external/`: Reference to external libraries used in the project (just `simdjson` in this case).
- `galileo_scripts`: Galielo listener scripts and instructions on its configuration.
- `input/`: Sample CSV datasets and `rules.json`.
- `singularity/`: Singularity container definition.
- `src/`: C++ and Python sources: here the core AstraLog logic is defined, with its components, interfaces and types.
- `tests/`: C++ Catch2 tests, integration fixtures and parallelisation testing to assess the model performance.

In addition to these folders the main directory also includes scripts (`.gitlab-ci.yml`) and configuration files (`CMakeLists.txt`, `requirements.txt`, `.clang-format`, `.pre-commit-config.yaml`).

---

## Software Organisation & Architecture

### Language and Libraries

- **Languages:** C++17 (for core pipeline) and Python3 (using legacy code provided from the start).
- **Libraries:**
  - `simdjson`: for rules and measurements efficient parsing.
  - `Catch2`: C++ unit testing framework used to define and run the test suite.
  - `OpenMP`: used to define the main parallelism workflow.
  - `OpenMPI`: used to increase the level of parallelism (ended up reducing the speedup).
  - `paho-mqtt`: used to connect our application to the Broker of the 'fake' spaceship simulation.
  - `pytest`: used to test legacy Python3 code.

### Architecture & Relation to Phase 1

The AtraLog software base relies on a **Component-Based Architecture**. Each component of the system performs a specific, well-defined job and interacts with the others exclusively through interfaces, ensuring reliability, scalability and maintainability.

The system is composed of 7 components: DataIngestor, BatchAccumulator, ThreadSafeBuffer, RuleEngine, RuleLoader, OutputDispatcher and MeasDatabase.

The data flow follows a clear pipeline: DataIngestor receives raw stream data from external sources, filters out malformed JSON entries, and parses the valid ones. The parsed data is forwarded to BatchAccumulator, which buffers incoming records until a configured batch size is reached, then pushes the batch into ThreadSafeBuffer. The RuleEngine consumes batches from the buffer, applies the configured rules, and sends a result summary to OutputDispatcher.

To structure the pipeline, three design patterns are employed:

- **Producer-Consumer**: BatchAccumulator acts as the producer, pushing batches into the ThreadSafeBuffer queue, while RuleEngine acts as the consumer, extracting and processing them. This decouples data accumulation from rule processing and ensures thread-safe communication between the two;
- **Strategy**: RuleEngine acts as the context and operates on a BaseRule abstract class. Each concrete rule is a subclass of BaseRule, allowing different rule implementations to be selected and applied interchangeably at runtime without modifying the engine logic;
- **Centralised Rule Creation**: RuleLoader is responsible for instantiating the correct BaseRule subclass based on the rule configuration, centralising and decoupling object creation from the rest of the system.

### Simplifications and variations (if any)

The RuleLoader component was originally conceived following the Factory Method pattern, where separate creator subclasses would each be responsible for instantiating a specific BaseRule subtype.

In the actual implementation, this was simplified into a Simple Factory approach: a single loadRules method dispatches object creation through an if/elseif chain based on the rule "type" field, with dedicated private parsing methods (parseSimpleRule, parseStepDifferenceRule, parseStatefulRule, parseLogicalCorrelationRule) for each concrete type. While this does not constitute a formal GoF pattern, it preserves the core intent of centralising and decoupling rule instantiation from the rest of the system.

### Distribution and parallelisation approach

**Description of the parallelism strategy adopted:**

The original approach to parallelisation leaveraged on the evaluation of rules in parallel, still respecting the constraint of the `priority` limitation which constraints a sequential order in rule evaluation.

OpenMP has been used for the thread parallelism. Rules are evaluated in parallel within each priority group, and each thread accumulates results locally; the shared `rules_cache` is updated only after the parallel section to avoid race conditions.

To scale across processes, an MPI-based rule engine was introduced, which can be enabled when compiling with `ASTRALOG_MPI`. Rank 0 broadcasts the telemetry batch, each rank evaluates a subset of rules (still by priority, with OpenMP inside each rank), and results are synchronised with a `Gather`, wherefore every rank updates its cache consistently.

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

The results obtained are coherent with the expected outcome. The simpler and lighter implementation of OpenMP is **relatively faster** (~20%/~30% speedup) against the OpenMPI+OpenMP implementation.

In addition to that we can notice how the performance of OpenMPI implementation gets worse the as number of parallel processes increase, demonstrating how **the bottleneck of the application is indeed the telemetry broadcasting** and **gather** operations performed.

### Usage of AI (if any)

LLMs were largely used to review code quality and assist with architectural choices. These are some examples of the prompts used:

```
- Is the function <function-name> consistent? Please review its code quality and suggest improvements.

- What is the most efficient way to use MPI communicators for this specific function?

- Can you review this c++ code and tell if its structure is correct?
```

We used AI to accelerate testing and performance assessment. Some scripts where used to generate deterministic rules and test scenarios.

In particular, we used AI models (`Gemini`, `Claude`) to generate scripts that tested our model parallelisation performance. This allowed rapid and repeatable comparisons between the OpenMP and OpenMP+MPI variants and helped identify that MPI communication overhead dominated the execution time. AI-driven test generation and result aggregation significantly sped up our benchmarking iterations.

```
- Generate a script in c++ that tests the parallelization workflow. I want a deterministic set of rules and measurements that the system has to evaluate with both OpenMP and OpenMPI+OpenMP application.

Provide also a bash script I can use to test different OpenMPI parallel distributions. Append the results in a .csv file.
```

In addition to that, AI was largely used in order to understand the constraints of Galileo100 cluster architecture. With the joint usage of Cineca's documentations and LLM we were able to figure out a CD pipeline able to deploy the application on an HPC environment with many limitations. Some examples:

```
- How can I configure a CI/CD pull-based workflow on the Galileo100 cluster?

- Is Galileo100 cluster allowing api calls to the Github API?

- Is it possible to run consecutive slurm jobs after a certain period of time given the cluster's crontab limitations?
```

---

### Test Suite Architecture & Rationale

The testing framework is designed to validate the entire lifecycle of the AstraLog-HPC pipeline, ensuring strict operational correctness from initial JSON ingestion up to rule evaluation.
The testing strategy directly maps to the modular components of our C++ architecture, ensuring decoupled unit testing, zero-side-effect profiling, and deterministic verification:

- **Ingestion & Validation ([`test_data_ingestor.cpp`](tests/components/test_data_ingestor.cpp)):**
  - _Rationale:_ Telemetry streams from spacecraft are highly susceptible to communication noise and file system structural anomalies.
  - _Test Coverage:_ Verifies file I/O robustness, data mapping correctness, and real-world integrity integration.

- **Batch Accumulation & Memory Buffering ([`test_batch_accumulator.cpp`](tests/components/test_batch_accumulator.cpp)):**
  - _Rationale:_ Telemetry packets must be buffered up to specified constraints before dispatching to the processing object to minimise processing invocation overhead.
  - _Test Coverage:_ Verifies threshold dynamics, overflow and carry-over semantics, and FIFO queue ordering.

- **Thread-Safe Buffering & Back-Pressure Synchronisation ([`test_ThreadSafeBuffer.cpp`](tests/components/test_ThreadSafeBuffer.cpp)):**
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
  - _Rationale:_ In a multi-threaded consumer architecture, multiple threads may trigger ESA compliance violations simultaneously. Unsynchronised file access would lead to race conditions, overlapping text fragments, or file corruption.
  - _Test Coverage:_ Verifies output format compliance, missing state fallbacks, and complex rule data relationships.

- **End-to-End System Orchestration ([`test_astralog.cpp`](tests/test_astralog.cpp)):**
  - _Rationale:_ A full integration test guarantees that the full multi-threaded ingestion-to-evaluation application pipeline shuts down safely, prevents side effects across tests, and handles disk cleanup identically to a production run on an ESA cluster.
  - _Test Coverage:_ Verifies sandbox environment isolation, raw ingestion input cleanup, and deterministic application lifecycles.

#### How to run tests

The Catch2-based tests can be run locally, leveraging the `ctest` framework.

To run them locally, first build the project. While in the build directory, the entire test suite can be run by invoking:

```bash
ctest
```

The legacy code providded can be tested using `pytest`. The tests are located in `tests/test_collector.py`. The tests were designed to cover the core business logic of the collector without requiring an active MQTT connection.

To run the tests locally:

```bash
python3 -m pytest tests/
```

---

---

## Pipeline & DevOps Workflow

This project implements a multi-step automation pipeline distributed across the **developer's local machine**, **GitHub servers**, and the **CINECA Galileo100 cluster**.

### CI workflow (All branches)

The following steps are triggered automatically at every commit across all branches:

1. **Developer Local Machine**
   - **Linting:** `pre-commit` automatically checks code against LLVM style guidelines.
   - **Formatting:** If non-compliant, the commit is blocked and `clang-format` is applied. _The developer must then re-commit and push._
2. **GitHub Servers (GitHub Actions)**
   - **Gate 1: Style Guard:** Verifies LLVM compliance (in case local pre-commits were bypassed).
   - **Gate 2: Static Analysis:** Executes security and quality scans via `CodeQL`.
   - **Gate 3: Unit Testing:** Compiles the codebase and runs the unit test suite.
   - **Gate 4: Memory Profiling:** Checks for memory leaks and errors using `Valgrind`.
   - **Gate 5: Integration Testing:** Executes full end-to-end integration tests.

Configuration files can be found:

- [pre-commit config file](.pre-commit-config.yaml) for the `pre-commit` software. Note that this configuration is applied automatically every time the project is builded. A developer should nontheless install `pre-commit` before contributing.
- [GitHub pipeline config file](.github/workflows/automated_CI.yml) for the GitHub actions workflow.

### CD workflow (Main branch only)

When changes are merged into the `main` branch, the pipeline executes these additional steps:

3. **GitHub Servers**
   - **Gate 6: Container Build** – Builds the `Apptainer` container according to the [definition file](singularity/Singularity.def).
4. **Galileo100 Cluster**
   - **Monitoring:** Polls the GitHub API to verify when a new build artifact from Gate 6 becomes available.
   - **SLURM Submission:** Submits the SLURM job to the compute nodes for execution.

Configuration files can be found:

- [container build](.github/workflows/automated_CI.yml#L178-L208) for the GitHub to GitLab mirroring action
- [instructions](galileo_scripts/ISTRUCTIONS.md) for the instructions to set up the poller job on CINECA Galileo100 for automatic deployment.

> [!WARNING]
> **Important Disclaimer**
>
> Automated Continuous Deployment (CD) is not officially supported on the CINECA Galileo100 cluster.
>
> The current setup utilises a self-scheduling SLURM loop as a workaround: an active job polls for new build artifacts, triggers the software execution if a new artifact is detected, and automatically resubmits a new monitoring job to the queue 10 minutes later.
>
> While functional, this cascading job approach is strictly a proof of concept designed to demonstrate automation workflows under highly restricted environment policies. **It should NOT be deployed in production environments.**

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

#### Integration through Cineca's internal GitLab

Another attempted solutions is the following: GitHub mirrors the content of the repository on Cineca's internal GitLab. This gives access to automatic execution on Galielo100 cluster. Unfortunately, the execution enviroment is highly limited, since the GitLab CI runners provided by CINECA run in unprivileged Docker containers based on Alpine Linux. This prevents the use of apptainer build from a definition file, which requires either `root` privileges or `fakeroot` support (`newuidmap`), neither of which is available. Moreover, the `sbatch` command is not available inside the CI containers, and `slurmrestd` is not exposed to the pipeline environment.

These constraints were confirmed both through direct testing and by CINECA's user support team, who explicitly stated that automated job submission from a CI/CD pipeline is not supported on Galileo100.
As a result, the [GitLab CI pipeline ](.gitlab-ci.yml) demonstrates the intended HPC workflow in a simulated environment: it just provieds how a container should be build and then submitted through SLURM.

#### Listener script on Cineca's login node

A [listener script](singularity/listener.sh) designed for execution within `tmux` has been provided. When active on the cluster's login node, this script periodically (every minute) monitors the repository status, detects new commits, polls the GitHub pipeline until completion, and, upon success, submits a SLURM job to the compute nodes.
However, due to strict process limitations enforced on the login node, script execution is automatically terminated after 10 hours, severely restricting long-term automation capabilities.

> [!WARNING]
> **Important Disclaimer**
>
> Attempting this approach on any shared cluster is **highly discouraged**. Even in environments where processes are not automatically killed, running unattended, long-polling scripts on login nodes violates standard HPC etiquette. Login nodes are strictly reserved for light, sporadic tasks (such as code compilation or job submission). Please avoid using this approach in production environments.

#### Automatic Resubmitting SLURM Listener Job

This final approach, currently implemented in the repository, is a robust variation of the local background script.

Instead of running on a restricted login node, a persistent [SLURM poller job](galileo_scripts/poller.slurm) is submitted directly to Galileo100's compute queue. This job checks the repository for updated build artifacts at regular intervals. When a new artifact is detected, it automatically downloads the asset and launches the [primary executor job](singularity/job.slurm) using the cluster's native `Apptainer` module.

---

---

## License

This project is licensed under the **MIT License**. See the [`LICENSE`](LICENSE) file for more details.

---
