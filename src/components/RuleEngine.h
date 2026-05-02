#include  <memory>
#include <string>
#include "../interfaces/RuleEngineInterface.h"
#include "../interfaces/BatchProviderInterface.h"
#include "../../external/simdjson.h"

class RuleEngine : public RuleEngineInterface {

public:

    RuleEngine() = default;

    RuleEngine(std::shared_ptr<BatchProviderInterface> provider);

    void ruleParsing(simdjson::ondemand::parser& parser, const std::string &filename);

    void setProviderInterface(BatchProviderInterface&);

private:

};