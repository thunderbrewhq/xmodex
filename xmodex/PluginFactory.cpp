#include "xmodex/PluginFactory.hpp"
#include "fmod.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/dsp/DSPCodec.hpp"
#include "xmodex/dsp/DSPCodecADPCM.hpp"
#include "xmodex/dsp/DSPCodecMPEG.hpp"
#include "xmodex/dsp/DSPCodecRaw.hpp"
#include "xmodex/dsp/DSPI.hpp"
#include "xmodex/dsp/DSPResamplerMultiInput.hpp"
#include "xmodex/dsp/DSPSoundCard.hpp"
#include "xmodex/dsp/DSPWaveTable.hpp"
#include "xmodex/output/OutputPolled.hpp"

namespace FMOD {

PluginFactory::PluginFactory() {
    this->mSystem = nullptr;
    FMOD_memset(this->mPluginPath, 0, FMOD_STRING_MAXPATHLEN);

    this->mDSPHead.initNode();
    this->mCodecHead.initNode();
    this->mOutputHead.initNode();
}

FMOD_RESULT PluginFactory::createCodec(FMOD_CODEC_DESCRIPTION_EX* codecdesc, Codec** codec) {
    if (!codecdesc || !codec) {
        return FMOD_ERR_INVALID_PARAM;
    }

    auto newcodec = FMOD_Object_CallocSize(Codec, codecdesc->mSize);
    if (!newcodec) {
        return FMOD_ERR_MEMORY;
    }

    FMOD_memcpy(&newcodec->mDescription, codecdesc, sizeof(FMOD_CODEC_DESCRIPTION_EX));

    *codec = newcodec;

    if (!newcodec->mDescription.getwaveformat) {
        newcodec->mDescription.getwaveformat = Codec::defaultGetWaveFormat;
    }

    return FMOD_OK;
}

FMOD_RESULT PluginFactory::createDSP(FMOD_DSP_DESCRIPTION_EX* dspdesc, DSPI** dsp) {
    FMOD_RESULT result;
    DSPI* newdsp;
    uint32_t size;

    if (!dspdesc || !dsp) {
        return FMOD_ERR_INVALID_PARAM;
    }

    size = dspdesc->mSize;

    if (*dsp) {
        newdsp = *dsp;
    } else {
        switch (dspdesc->mCategory) {
        case FMOD_DSP_CATEGORY_SOUNDCARD: {
            newdsp = FMOD_Object_CallocSize(DSPSoundCard, size);
            break;
        }
        case FMOD_DSP_CATEGORY_FILTER: {
            newdsp = FMOD_Object_CallocSize(DSPFilter, size);
            break;
        }
        case FMOD_DSP_CATEGORY_DSPCODECMPEG:
        case FMOD_DSP_CATEGORY_DSPCODECADPCM:
        case FMOD_DSP_CATEGORY_DSPCODECXMA:
        case FMOD_DSP_CATEGORY_DSPCODECRAW: {
            size = size < sizeof(DSPCodec) ? sizeof(DSPCodec) : size;

            newdsp = reinterpret_cast<DSPI*>(FMOD_Memory_Calloc(size));
            if (!newdsp) {
                return FMOD_ERR_MEMORY;
            }

            if (dspdesc->mCategory == FMOD_DSP_CATEGORY_DSPCODECMPEG) {
                new (dspdesc) DSPCodecMPEG();
            } else if (dspdesc->mCategory == FMOD_DSP_CATEGORY_DSPCODECADPCM) {
                new (dspdesc) DSPCodecADPCM();
            } else if (dspdesc->mCategory == FMOD_DSP_CATEGORY_DSPCODECRAW) {
                new (dspdesc) DSPCodecRaw();
            } else {
                return FMOD_ERR_FORMAT;
            }

            break;
        }
        case FMOD_DSP_CATEGORY_RESAMPLER: {
            newdsp = FMOD_Object_CallocSize(DSPResamplerMultiInput, size);
            break;
        }
        case FMOD_DSP_CATEGORY_WAVETABLE: {
            newdsp = FMOD_Object_CallocSize(DSPWaveTable, size);
            break;
        }
        default: {
            return FMOD_ERR_INVALID_PARAM;
        }
        }
        if (!newdsp) {
            *dsp = nullptr;
            return FMOD_ERR_MEMORY;
        }
    }

    newdsp->mSystem = this->mSystem;

    result = newdsp->alloc(dspdesc);
    if (result != FMOD_OK) {
        FMOD_Memory_Free(newdsp);
        return result;
    }

    if (dspdesc->create) {
        // if (dspdesc->mAEffect) {
        //     auto effect = reintepret_cast<AEffect*>(dspdesc->mAEffect);
        //     if (effect->magic == kEffectMagic) {
        //         reinterpret_cast<DSPVSTPlugin*>(newdsp)->mEffect = effect;
        //     }
        //
        //     if (!FMOD_strncmp(dspdesc->name, "WINAMP", 6)) {
        //         reinterpret_cast<DSPWinampPlugin*>(newdsp)->mEffect = effect;
        //     }
        // }

        newdsp->instance = reinterpret_cast<FMOD_DSP*>(newdsp);

        result = dspdesc->create(newdsp);
        if (result != FMOD_OK) {
            FMOD_Memory_Free(newdsp);
            return result;
        }
    }

    *dsp = newdsp;

    return FMOD_OK;
}

FMOD_RESULT PluginFactory::createOutput(FMOD_OUTPUT_DESCRIPTION_EX* outputdesc, Output** output) {
    Output* newoutput;

    if (!outputdesc || !output) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (outputdesc->mType == FMOD_OUTPUTTYPE_OPENAL) {
        // TODO: pending OpenAL support
        // newoutput = FMOD_Object_Calloc(OutputOpenAL);
        newoutput = nullptr;
    } else if (outputdesc->polling) {
        newoutput = FMOD_Object_CallocSize(OutputPolled, outputdesc->mSize);
    } else {
        newoutput = FMOD_Object_CallocSize(Output, outputdesc->mSize);
    }

    if (!newoutput) {
        *output = nullptr;
        return FMOD_ERR_MEMORY;
    }

    FMOD_memcpy(&newoutput->mDescription, outputdesc, sizeof(FMOD_OUTPUT_DESCRIPTION_EX));

    newoutput->mSystem = this->mSystem;
    newoutput->readfrommixer = Output::mixCallback;

    *output = newoutput;

    return FMOD_OK;
}

FMOD_RESULT PluginFactory::getCodec(int32_t handle, FMOD_CODEC_DESCRIPTION_EX** codecdesc) {
    if (!codecdesc) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *codecdesc = nullptr;

    int32_t i = 0;
    auto current = static_cast<FMOD_CODEC_DESCRIPTION_EX*>(this->mCodecHead.getNext());
    while (current != &this->mCodecHead) {
        if (i == handle) {
            *codecdesc = current;
            return FMOD_OK;
        }
        i++;
        current = static_cast<FMOD_CODEC_DESCRIPTION_EX*>(current->getNext());
    }

    return FMOD_ERR_PLUGIN_MISSING;
}

FMOD_RESULT PluginFactory::getDSP(int32_t handle, FMOD_DSP_DESCRIPTION_EX** dspdesc) {
    if (!dspdesc) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *dspdesc = nullptr;

    int32_t i = 0;
    auto current = static_cast<FMOD_DSP_DESCRIPTION_EX*>(this->mDSPHead.getNext());
    while (current != &this->mDSPHead) {
        if (i == handle) {
            *dspdesc = current;
            return FMOD_OK;
        }

        i++;
        current = static_cast<FMOD_DSP_DESCRIPTION_EX*>(current->getNext());
    }

    return FMOD_ERR_PLUGIN_MISSING;
}

FMOD_RESULT PluginFactory::getNumCodecs(int32_t* numcodecs) {
    if (!numcodecs) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *numcodecs = this->mCodecHead.count();
    return FMOD_OK;
}

FMOD_RESULT PluginFactory::getNumDSPs(int32_t* numdsps) {
    if (!numdsps) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *numdsps = this->mDSPHead.count();
    return FMOD_OK;
}

FMOD_RESULT PluginFactory::getNumOutputs(int32_t* numoutputs) {
    if (!numoutputs) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *numoutputs = this->mOutputHead.count();
    return FMOD_OK;
}

FMOD_RESULT PluginFactory::getOutput(int32_t handle, FMOD_OUTPUT_DESCRIPTION_EX** outputdesc) {
    if (!outputdesc) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *outputdesc = nullptr;

    int32_t i = 0;
    auto current = static_cast<FMOD_OUTPUT_DESCRIPTION_EX*>(this->mOutputHead.getNext());
    while (current != &this->mOutputHead) {
        if (i == handle) {
            *outputdesc = current;
            return FMOD_OK;
        }

        i++;
        current = static_cast<FMOD_OUTPUT_DESCRIPTION_EX*>(current->getNext());
    }

    return FMOD_ERR_PLUGIN_MISSING;
}

FMOD_RESULT PluginFactory::registerCodec(FMOD_CODEC_DESCRIPTION_EX* description, FMOD_PLUGINTYPE* plugintype, int32_t* handle) {
    if (!description) {
        return FMOD_ERR_INVALID_PARAM;
    }

    auto newdesc = FMOD_Object_Calloc(FMOD_CODEC_DESCRIPTION_EX);
    if (!newdesc) {
        return FMOD_ERR_MEMORY;
    }

    newdesc->name = description->name;
    newdesc->version = description->version;
    newdesc->timeunits = description->timeunits;
    newdesc->defaultasstream = description->defaultasstream;
    newdesc->open = description->open;
    newdesc->close = description->close;
    newdesc->read = description->read;
    newdesc->getlength = description->getlength;
    newdesc->setposition = description->setposition;
    newdesc->getposition = description->getposition;
    newdesc->soundcreate = description->soundcreate;
    newdesc->getwaveformat = description->getwaveformat;

    newdesc->mType = description->mType;
    newdesc->mSize = description->mSize;
    newdesc->mModule = description->mModule;
    newdesc->reset = description->reset;
    newdesc->canpoint = description->canpoint;

    newdesc->addBefore(&this->mCodecHead);

    if (plugintype) {
        *plugintype = FMOD_PLUGINTYPE_CODEC;
    }

    if (handle) {
        *handle = this->mCodecHead.getNodeIndex(newdesc);
    }

    return FMOD_OK;
}

FMOD_RESULT PluginFactory::registerDSP(FMOD_DSP_DESCRIPTION_EX* description, FMOD_PLUGINTYPE* plugintype, int32_t* handle) {
    if (!description) {
        return FMOD_ERR_INVALID_PARAM;
    }

    auto newdesc = FMOD_Object_Calloc(FMOD_DSP_DESCRIPTION_EX);
    if (!newdesc) {
        return FMOD_ERR_MEMORY;
    }

    newdesc->version = description->version;
    newdesc->channels = description->channels;
    newdesc->create = description->create;
    newdesc->release = description->release;
    newdesc->reset = description->reset;
    newdesc->read = description->read;
    newdesc->setposition = description->setposition;
    newdesc->numparameters = description->numparameters;
    newdesc->paramdesc = description->paramdesc;
    newdesc->setparameter = description->setparameter;
    newdesc->getparameter = description->getparameter;
    newdesc->config = description->config;
    newdesc->configwidth = description->configwidth;
    newdesc->configheight = description->configheight;
    newdesc->userdata = description->userdata;

    newdesc->mType = description->mType;
    newdesc->mCategory = description->mCategory;
    newdesc->mSize = description->mSize;
    newdesc->mModule = description->mModule;
    newdesc->mAEffect = description->mAEffect;
    newdesc->mFormat = description->mFormat;
    newdesc->mResamplerBlockLength = description->mResamplerBlockLength;

    newdesc->addBefore(&this->mDSPHead);

    if (plugintype) {
        *plugintype = FMOD_PLUGINTYPE_DSP;
    }

    if (handle) {
        *handle = this->mDSPHead.getNodeIndex(newdesc);
    }

    return FMOD_OK;
}

FMOD_RESULT PluginFactory::registerOutput(FMOD_OUTPUT_DESCRIPTION_EX* description, FMOD_PLUGINTYPE* plugintype, int32_t* handle) {
    if (!description) {
        return FMOD_ERR_INVALID_PARAM;
    }

    auto newdesc = FMOD_Object_Calloc(FMOD_OUTPUT_DESCRIPTION_EX);
    if (!newdesc) {
        return FMOD_ERR_MEMORY;
    }

    newdesc->name = description->name;
    newdesc->version = description->version;
    newdesc->polling = description->polling;
    newdesc->getnumdrivers = description->getnumdrivers;
    newdesc->getdrivername = description->getdrivername;
    newdesc->getdrivercaps = description->getdrivercaps;
    newdesc->init = description->init;
    newdesc->close = description->close;

    newdesc->start = description->start;
    newdesc->stop = description->stop;

    newdesc->update = description->update;
    newdesc->gethandle = description->gethandle;
    newdesc->getposition = description->getposition;
    newdesc->lock = description->lock;
    newdesc->unlock = description->unlock;
    newdesc->getsamplemaxchannels = description->getsamplemaxchannels;
    newdesc->getdrivercapsex = description->getdrivercapsex;
    newdesc->getdrivercapsex2 = description->getdrivercapsex2;
    newdesc->initex = description->initex;

    newdesc->start = description->start;
    newdesc->stop = description->stop;

    newdesc->updatedfinished = description->updatedfinished;
    newdesc->createsample = description->createsample;
    newdesc->getsoundram = description->getsoundram;
    newdesc->record_getnumdrivers = description->record_getnumdrivers;
    newdesc->record_getdriverinfo = description->record_getdriverinfo;
    newdesc->record_getdrivercaps = description->record_getdrivercaps;
    newdesc->record_start = description->record_start;
    newdesc->record_stop = description->record_stop;
    newdesc->record_getposition = description->record_getposition;
    newdesc->record_lock = description->record_lock;
    newdesc->record_unlock = description->record_unlock;
    newdesc->reverb_setproperties = description->reverb_setproperties;
    newdesc->postmixcallback = description->postmixcallback;
    newdesc->mType = description->mType;
    newdesc->mSize = description->mSize;
    newdesc->mModule = description->mModule;

    newdesc->addBefore(&this->mOutputHead);

    if (plugintype) {
        *plugintype = FMOD_PLUGINTYPE_OUTPUT;
    }

    if (handle) {
        *handle = this->mOutputHead.getNodeIndex(newdesc);
    }

    return FMOD_OK;
}

FMOD_RESULT PluginFactory::unloadPlugin(FMOD_PLUGINTYPE plugintype, int32_t handle) {
    FMOD_RESULT result;
    FMOD_OUTPUT_DESCRIPTION_EX* output;
    FMOD_CODEC_DESCRIPTION_EX* codec;
    FMOD_DSP_DESCRIPTION_EX* dsp;

    if (plugintype == FMOD_PLUGINTYPE_CODEC) {
        result = this->getCodec(handle, &codec);
        if (result != FMOD_OK) {
            return result;
        }

        if (codec->mModule) {
            FMOD_OS_Library_Free(codec->mModule);
        }

        codec->removeNode();

        FMOD_Memory_Free(codec);
    } else if (plugintype == FMOD_PLUGINTYPE_DSP) {
        result = this->getDSP(handle, &dsp);
        if (result != FMOD_OK) {
            return result;
        }

        if (dsp->mAEffect) {
            if (dsp->paramdesc) {
                FMOD_Memory_Free(dsp->paramdesc);
            }
        }

        if (dsp->mModule) {
            FMOD_OS_Library_Free(dsp->mModule);
        }

        dsp->removeNode();

        FMOD_Memory_Free(dsp);
    } else if (plugintype == FMOD_PLUGINTYPE_OUTPUT) {
        result = this->getOutput(handle, &output);
        if (result != FMOD_OK) {
            return result;
        }

        if (output->mModule) {
            FMOD_OS_Library_Free(output->mModule);
        }

        output->removeNode();

        FMOD_Memory_Free(output);
    } else {
        return FMOD_ERR_INVALID_PARAM;
    }

    return FMOD_OK;
}

FMOD_RESULT PluginFactory::release() {
    FMOD_RESULT result;
    int32_t num;

    result = this->getNumCodecs(&num);
    if (result != FMOD_OK) {
        return result;
    }

    for (int32_t i = 0; i < num; i++) {
        result = this->unloadPlugin(FMOD_PLUGINTYPE_CODEC, 0);
        if (result != FMOD_OK) {
            return result;
        }
    }

    result = this->getNumDSPs(&num);
    if (result != FMOD_OK) {
        return result;
    }

    for (int32_t i = 0; i < num; i++) {
        result = this->unloadPlugin(FMOD_PLUGINTYPE_DSP, 0);
        if (result != FMOD_OK) {
            return result;
        }
    }

    result = this->getNumOutputs(&num);
    if (result != FMOD_OK) {
        return result;
    }

    for (int32_t i = 0; i < num; i++) {
        result = this->unloadPlugin(FMOD_PLUGINTYPE_OUTPUT, 0);
        if (result != FMOD_OK) {
            return result;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT PluginFactory::setPluginPath(const char* path) {
    if (FMOD_strlen(path) >= FMOD_STRING_MAXPATHLEN) {
        return FMOD_ERR_INVALID_PARAM;
    }

    FMOD_strncpy(this->mPluginPath, path, FMOD_STRING_MAXPATHLEN);
    return FMOD_OK;
}

FMOD_RESULT PluginFactory::setSystem(SystemI* system) {
    this->mSystem = system;
    return FMOD_OK;
}

} // namespace FMOD
