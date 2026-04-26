#include  <memory>
#include <string>
#include "../interfaces/RuleEngineInterface.h"
#include "../interfaces/BatchProviderInterface.h"
#include "../../external/simdjson.h"

class RuleEngine : public RuleEngineInterface {

public:
    RuleEngine(std::shared_ptr<BatchProviderInterface> provider);

    void ruleParsing(simdjson::ondemand::parser& parser, const std::string &filename);

private:

};