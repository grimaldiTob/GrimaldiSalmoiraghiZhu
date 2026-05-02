#include <memory>
#include <string>
#include "../interfaces/RuleEngineInterface.h"
#include "../interfaces/BatchProviderInterface.h"
#include "../../external/simdjson.h"
#include "../rules/BaseRule.h"
#include "../rules/SimpleRule.h"
#include "../rules/StepDifferenceRule.h"

class RuleEngine : public RuleEngineInterface {

public:

    RuleEngine() = default;

    RuleEngine(std::shared_ptr<BatchProviderInterface> provider);

    void ruleParsing(simdjson::ondemand::parser& parser, const std::string &filename);

    void setProviderInterface(std::shared_ptr<BatchProviderInterface>);

    RulePriority parsePriority(std::string_view);

private:

};