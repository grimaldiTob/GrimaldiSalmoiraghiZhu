#pragma once

#include <mpi.h>

#include "RuleEngine.h"

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
    // contructor calls the basis class contructor
    MpiRuleEngine(ConsumerBuffer<TelemetryBatch> &broker,
                  MeasDatabaseInterface &db,
                  OutputDispatcherInterface &outputDispatcher,
                  RuleLoaderInterface &loader,
                  std::optional<int64_t> initialTimestamp, MPI_Comm &comm)
        : RuleEngine(broker, db, outputDispatcher, loader, initialTimestamp),
          m_comm(comm) {}

    void evaluateRules(const TelemetryBatch &batch) override;
    void run() override;

  private:
    MPI_Comm &m_comm;
};
