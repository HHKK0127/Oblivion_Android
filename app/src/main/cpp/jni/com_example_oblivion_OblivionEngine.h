#ifndef COM_EXAMPLE_OBLIVION_OBLIVIONENGINE_H_
#define COM_EXAMPLE_OBLIVION_OBLIVIONENGINE_H_

#include <jni.h>
#include <memory>

namespace oblivion {
class Engine;
}

class OblivionEngineJNI {
public:
    static std::unique_ptr<oblivion::Engine> sEngine;

private:
};

#endif // COM_EXAMPLE_OBLIVION_OBLIVIONENGINE_H_
