#include "xmodex/Global.hpp"
#include "xmodex/SystemI.hpp"

namespace FMOD {

static Global gGlobalMem;

static SystemI gSystemHeadMem;

static MemPool gSystemPoolMem;

Global* gGlobal = &gGlobalMem;

Global::Global() {
    this->init();
}

void Global::init() {
    this->gSystemHead = &gSystemHeadMem;
    this->gSystemPool = &gSystemPoolMem;
}

} // namespace FMOD
