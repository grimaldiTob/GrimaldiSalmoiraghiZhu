/** 
 * @brief Interface used to send an item to an object  
 */
template<typename T>
class ProducerBuffer {
public:
    virtual void push(T item) = 0;
    virtual void finish_production() = 0;
};