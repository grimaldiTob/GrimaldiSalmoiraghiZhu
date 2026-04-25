#include  <memory>
#include "../interfaces/RuleEngineInterface.h"
#include "../interfaces/BatchProviderInterface.h"

class RuleEngine : public RuleEngineInterface {

public:
    RuleEngine(std::shared_ptr<BatchProviderInterface> provider);

private:

};