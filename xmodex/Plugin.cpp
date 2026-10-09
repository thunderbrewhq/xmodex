#include "xmodex/Plugin.hpp"
#include "xmodex/Memory.hpp"

namespace FMOD {

FMOD_RESULT Plugin::release() {
    FMOD_Memory_Free(this);
    return FMOD_OK;
}

} // namespace FMOD
