
class RuleEngineInterface {
    virtual ~RuleEngineInterface() = 0;

    virtual void evaluateRules() = 0;

    virtual void resetCache() = 0;
};