#pragma once

/** 
 * @brief Interface used to request an item from an object  
 */
template<typename T>
class ConsumerBuffer {
public:
    virtual bool pop(T& item) = 0;
};