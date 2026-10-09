#include "fmod.hpp"
#include "xmodex/dsp/DSPI.hpp"

namespace FMOD {

DSP::DSP() {
}

FMOD_RESULT F_API DSP::release() {
    FMOD_RESULT result;
    DSPI* dspi;

    result = DSPI::validate(this, &dspi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return dspi->release(true);
    }
}

FMOD_RESULT F_API DSP::addInput(DSP* target) {
    FMOD_RESULT result;
    DSPI* dspi;

    result = DSPI::validate(this, &dspi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return dspi->addInput(reinterpret_cast<DSPI*>(target));
    }
}

FMOD_RESULT F_API DSP::disconnectAll(bool inputs, bool outputs) {
    FMOD_RESULT result;
    DSPI* dspi;

    result = DSPI::validate(this, &dspi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return dspi->disconnectAll(inputs, outputs);
    }
}

FMOD_RESULT F_API DSP::remove() {
    FMOD_RESULT result;
    DSPI* dspi;

    result = DSPI::validate(this, &dspi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return dspi->remove();
    }
}

FMOD_RESULT F_API DSP::getOutput(int32_t index, DSP** output) {
    FMOD_RESULT result;
    DSPI* dspi;

    result = DSPI::validate(this, &dspi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return dspi->getOutput(index, reinterpret_cast<DSPI**>(output));
    }
}

FMOD_RESULT F_API DSP::setActive(bool active) {
    FMOD_RESULT result;
    DSPI* dspi;

    result = DSPI::validate(this, &dspi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return dspi->setActive(active);
    }
}

FMOD_RESULT F_API DSP::setBypass(bool bypass) {
    FMOD_RESULT result;
    DSPI* dspi;

    result = DSPI::validate(this, &dspi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return dspi->setBypass(bypass);
    }
}

FMOD_RESULT F_API DSP::setParameter(int32_t index, float value) {
    FMOD_RESULT result;
    DSPI* dspi;

    result = DSPI::validate(this, &dspi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return dspi->setParameter(index, value);
    }
}

FMOD_RESULT F_API DSP::getUserData(void** _userdata) {
    FMOD_RESULT result;
    DSPI* dspi;

    result = DSPI::validate(this, &dspi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return dspi->getUserData(_userdata);
    }
}

} // namespace FMOD
