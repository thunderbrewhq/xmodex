#include "xmodex/Metadata.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/String.hpp"

namespace FMOD {

FMOD_RESULT TagNode::init(FMOD_TAGTYPE type, const char* name, void* data, uint32_t datalen, FMOD_TAGDATATYPE datatype) {
    int32_t memsize;
    this->mName = FMOD_strdup(name);
    if (!mName) {
        return FMOD_ERR_MEMORY;
    }

    memsize = datalen;
    if (datatype == FMOD_TAGDATATYPE_STRING) {
        memsize += 1;
    }
    this->mData[0] = FMOD_Memory_Calloc(memsize);
    if (!this->mData[0]) {
        return FMOD_ERR_MEMORY;
    }

    FMOD_memcpy(this->mData[0], data, datalen);
    this->mDataLen = datalen;
    this->mDataType = datatype;
    this->mType = type;
    this->mUpdated = true;
    this->mUnique = false;
    this->mCurrentBuffer = 0;
    return FMOD_OK;
}

FMOD_RESULT TagNode::release() {
    if (this->mName) {
        FMOD_Memory_Free(this->mName);
        this->mName = nullptr;
    }

    if (this->mData[0]) {
        FMOD_Memory_Free(this->mData[0]);
        this->mData[0] = nullptr;
    }

    FMOD_Memory_Free(this);
    return FMOD_OK;
}

FMOD_RESULT TagNode::update(void* data, uint32_t datalen) {
    if (this->mDataLen == datalen) {
        if (!memcmp(this->mData[0], data, datalen)) {
            this->mUpdated = true;
            return FMOD_OK;
        }
    }

    if (this->mData[0]) {
        FMOD_Memory_Free(this->mData[0]);
        this->mData[0] = nullptr;
    }

    this->mData[0] = FMOD_Memory_Alloc(datalen);
    if (!this->mData[0]) {
        return FMOD_ERR_MEMORY;
    }

    FMOD_memcpy(this->mData[0], data, datalen);
    this->mDataLen = datalen;
    this->mUpdated = true;
    return FMOD_OK;
}

FMOD_RESULT Metadata::release() {
    auto node = reinterpret_cast<TagNode*>(this->mList.getNext());

    while (node != &this->mList) {
        auto next = reinterpret_cast<TagNode*>(node->getNext());
        node->removeNode();
        node->release();
        node = next;
    }

    FMOD_Memory_Free(this);
    return FMOD_OK;
}

FMOD_RESULT Metadata::getNumTags(int32_t* numtags, int32_t* numtagsupdated) {
    int32_t num = 0;
    int32_t numupdated = 0;

    auto node = reinterpret_cast<TagNode*>(this->mList.getNext());
    while (node != &this->mList) {
        num++;
        if (node->mUpdated) {
            numupdated++;
        }
        node = reinterpret_cast<TagNode*>(node->getNext());
    }

    if (numtags) {
        *numtags = num;
    }
    if (numtagsupdated) {
        *numtagsupdated = numupdated;
    }

    return FMOD_OK;
}

FMOD_RESULT Metadata::getTag(const char* name, int32_t index, FMOD_TAG* tag) {
    TagNode* node;
    int32_t i;
    if (index >= 0) {
        if (!name) {
            node = static_cast<TagNode*>(this->mList.getNodeByIndex(index));
            if (!node) {
                return FMOD_ERR_TAGNOTFOUND;
            }
        } else {
            node = static_cast<TagNode*>(this->mList.getNext());
            i = 0;
            for (;;) {
                if (node == &this->mList) {
                    return FMOD_ERR_TAGNOTFOUND;
                }
                if (!FMOD_strcmp(node->mName, name)) {
                    if (i == index) {
                        break;
                    }
                    i++;
                }
                node = static_cast<TagNode*>(node->getNext());
            }
        }
    } else {
        node = static_cast<TagNode*>(this->mList.getNext());
        if (!name) {
            for (;;) {
                if (node == &this->mList) {
                    return FMOD_ERR_TAGNOTFOUND;
                }
                if (node->mUpdated) {
                    break;
                }
                node = static_cast<TagNode*>(node->getNext());
            }
        } else {
            for (;;) {
                if (node == &this->mList) {
                    return FMOD_ERR_TAGNOTFOUND;
                }
                if (node->mUpdated && !FMOD_strcmp(node->mName, name)) {
                    break;
                }
                node = static_cast<TagNode*>(node->getNext());
            }
        }
    }

    tag->type = node->mType;
    tag->datatype = node->mDataType;
    tag->name = node->mName;
    tag->data = node->mData[0];
    tag->datalen = node->mDataLen;
    tag->updated = node->mUpdated;
    if (node->mUpdated) {
        node->mUpdated = false;
    }

    return FMOD_OK;
}

FMOD_RESULT Metadata::add(Metadata* metadata) {
    auto node = reinterpret_cast<TagNode*>(metadata->mList.getNext());
    while (node != &metadata->mList) {
        auto next = reinterpret_cast<TagNode*>(node->getNext());
        node->removeNode();
        if (node->mUnique) {
            auto existingnode = reinterpret_cast<TagNode*>(this->mList.getNext());
            for (;;) {
                if (existingnode == &this->mList) {
                    existingnode = nullptr;
                    break;
                }
                if (!FMOD_strcmp(existingnode->mName, node->mName)) {
                    break;
                }
                existingnode = reinterpret_cast<TagNode*>(existingnode->getNext());
            }

            if (existingnode) {
                existingnode->update(node->mData[0], node->mDataLen);
                node->release();
            } else {
                this->addTag(node);
            }
        } else {
            this->addTag(node);
        }
    }
    return FMOD_OK;
}

FMOD_RESULT Metadata::addTag(TagNode* node) {
    node->addBefore(&this->mList);
    return FMOD_OK;
}

FMOD_RESULT Metadata::addTag(FMOD_TAGTYPE type, const char* name, void* data, uint32_t datalen, FMOD_TAGDATATYPE datatype, bool unique) {
    FMOD_RESULT result;
    TagNode* node = nullptr;

    if (unique) {
        node = reinterpret_cast<TagNode*>(this->mList.getNext());
        for (;;) {
            if (node == &this->mList) {
                node = nullptr;
                break;
            }
            if (!FMOD_strcmp(node->mName, name) && (node->mType == type)) {
                break;
            }
            node = reinterpret_cast<TagNode*>(node->getNext());
        }
    }

    if (node && unique) {
        result = node->update(data, datalen);
    } else {
        node = FMOD_Object_Alloc(TagNode);
        if (!node) {
            return FMOD_ERR_MEMORY;
        }
        node->init(type, name, data, datalen, datatype);
        result = this->addTag(node);
    }

    if (unique) {
        node->mUnique = true;
    }

    return result;
}

} // namespace FMOD
