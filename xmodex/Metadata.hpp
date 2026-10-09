#ifndef XMODEX_METADATA_HPP
#define XMODEX_METADATA_HPP

#include "fmod.hpp"
#include "xmodex/LinkedListNode.hpp"

namespace FMOD {

class TagNode : public LinkedListNode {
    public:
        FMOD_TAGTYPE mType;
        FMOD_TAGDATATYPE mDataType;
        char* mName;
        void* mData[2];
        uint32_t mDataLen;
        bool mUpdated;
        bool mUnique;
        int32_t mCurrentBuffer;

    public:
        TagNode() {
            this->mType = FMOD_TAGTYPE_UNKNOWN;
            this->mDataType = FMOD_TAGDATATYPE_BINARY;
            this->mName = nullptr;
            this->mData[0] = nullptr;
            this->mData[1] = nullptr;
            this->mDataLen = 0;
            this->mUpdated = false;
            this->mUnique = false;
            this->mCurrentBuffer = 0;
        }

        FMOD_RESULT init(FMOD_TAGTYPE type, const char* name, void* data, uint32_t datalen, FMOD_TAGDATATYPE datatype);
        FMOD_RESULT release();
        FMOD_RESULT update(void* data, uint32_t datalen);
};

class Metadata {
    public:
        TagNode mList;

    public:
        FMOD_RESULT release();
        FMOD_RESULT getNumTags(int32_t* numtags, int32_t* numtagsupdated);
        FMOD_RESULT getTag(const char* name, int32_t index, FMOD_TAG* tag);
        FMOD_RESULT add(Metadata* metadata);
        FMOD_RESULT addTag(TagNode* node);
        FMOD_RESULT addTag(FMOD_TAGTYPE type, const char* name, void* data, uint32_t datalen, FMOD_TAGDATATYPE datatype, bool unique);
};

}; // namespace FMOD

#endif
