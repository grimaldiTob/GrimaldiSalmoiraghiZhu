#pragma once

#include <mpi.h>

#include "RuleEngine.h"

/**
 * ==================== MOTIVATION OF THE CLASS
 * ========================================= [Mike] Instead of having both
 * sequential and MPI logics inside the only RuleEngine class we can directly
 * create MpiRuleEngine derived by RuleEngine so it inherits all public and
 * protected methods and attributes. We just need to ovveride the logic of
 * evaluateRules() and run().
 *
 * The Astrolog's implementation will not be affected by those changes beause it
 * cares only to instanciate either RuleEngine or MpiRuleEngine (or even other
 * prototypes), in particular it is sufficient to know that it can call the
 * run() method.
 *
 * Even though is just a prototype to demonstrate that it does not bring speedup
 * over the sequential algorithm, it is important to keep decoupled the
 * sequential and parallelized logics
 * ===============================================================================
 */

/** Broadcast of Telemetry Batch Data across processors using MPI.
 * It is important to notice that since MPI does not support the string object,
 * but just the char we had to create a custom string with all the sensors name
 * concatenated.
 */
static void broadcastTelemetryBatch(TelemetryBatch &batch, int root,
                                    MPI_Comm comm);

/**
 * Couple of helper functions defined here in order to handle
 * MPI massage passing correctly.
 *
 * Since MPI does not support passing std::optional<bool>
 * we encode it in an integer.
 */
static inline int8_t encode(std::optional<bool> v);
static inline std::optional<bool> decode(int8_t v);

struct RuleEvalMsg {
    int32_t idx;
    int8_t value;
};

class MpiRuleEngine : public RuleEngine {

  public:
    MpiRuleEngine(ConsumerBuffer<TelemetryBatch> &broker,
                  MeasDatabaseInterface &db,
                  std::optional<int64_t> initialTimestamp, MPI_Comm &comm)
        : RuleEngine(broker, db, initialTimestamp), m_comm(comm) {}

    void evaluateRules(const TelemetryBatch &batch) override;
    void run() override;

  private:
    MPI_Comm &m_comm;
};
