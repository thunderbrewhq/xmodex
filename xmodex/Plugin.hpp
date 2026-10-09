#ifndef XMODEX_PLUGIN_HPP
#define XMODEX_PLUGIN_HPP

#include "xmodex/Global.hpp"
#include "xmodex/LinkedListNode.hpp"

namespace FMOD {

class SystemI;

class Plugin : public LinkedListNode {
    public:
        SystemI* mSystem;
        Global* mGlobal;

    public:
        Plugin() {
            this->mGlobal = gGlobal;
        }

        virtual FMOD_RESULT release();

        FMOD_RESULT init() {
            gGlobal = this->mGlobal;
            return FMOD_OK;
        }
};

} // namespace FMOD

#endif
