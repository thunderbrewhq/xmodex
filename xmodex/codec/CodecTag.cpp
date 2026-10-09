#include "xmodex/codec/CodecTag.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/file/File.hpp"
#include <cstdio>

#define ID3V2_FLAGS_UNSYNCHRONISATION 0x80
#define ID3V2_FLAGS_EXTHEADER 0x40
#define ID3V2_FLAGS_EXPERIMENTAL 0x20
#define ID3V2_FLAGS_FOOTER 0x10

#define ISASCII(_x) (((_x) >= 32 && (_x) < 128) || ((_x) == 0))

namespace FMOD {

static const FMOD_GUID ASF_Header_Object = { 0x75B22630, 0x668E, 0x11CF, { 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C } };
static const FMOD_GUID ASF_Content_Description_Object = { 0x75B22633, 0x668E, 0x11CF, { 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C } };
static const FMOD_GUID ASF_Extended_Content_Description_Object = { 0xD2D0A440, 0xE307, 0x11D2, { 0x97, 0xF0, 0x00, 0xA0, 0xC9, 0x5E, 0xA8, 0x50 } };

#pragma pack(push, 1)

struct ASF_CHUNK {
        FMOD_GUID guid;
        FMOD_UINT64 size;
};

struct ASF_HEADEROBJECT {
        int32_t number;
        char Reserved1;
        char Reserved2;
};

#pragma pack(pop)

FMOD_CODEC_DESCRIPTION_EX tagcodec;

FMOD_RESULT CodecTag::readID3v1() {
    FMOD_RESULT result;
    char value[31];
    char tmp[8];
    uint32_t itemsread;

    FMOD_memset(value, 0, 31);
    result = this->mFile->read(value, 1, 30, &itemsread);
    if (result != FMOD_OK) {
        return result;
    }
    if (itemsread != 30) {
        return FMOD_ERR_FILE_BAD;
    }
    if (FMOD_strlen(value)) {
        this->metaData(FMOD_TAGTYPE_ID3V1, "TITLE", value, FMOD_strlen(value) + 1, FMOD_TAGDATATYPE_STRING, false);
    }

    FMOD_memset(value, 0, 31);
    result = this->mFile->read(value, 1, 30, &itemsread);
    if (result != FMOD_OK) {
        return result;
    }
    if (itemsread != 30) {
        return FMOD_ERR_FILE_BAD;
    }
    if (FMOD_strlen(value)) {
        this->metaData(FMOD_TAGTYPE_ID3V1, "ARTIST", value, FMOD_strlen(value) + 1, FMOD_TAGDATATYPE_STRING, false);
    }

    FMOD_memset(value, 0, 31);
    result = this->mFile->read(value, 1, 30, &itemsread);
    if (result != FMOD_OK) {
        return result;
    }
    if (itemsread != 30) {
        return FMOD_ERR_FILE_BAD;
    }
    if (FMOD_strlen(value)) {
        this->metaData(FMOD_TAGTYPE_ID3V1, "ALBUM", value, FMOD_strlen(value) + 1, FMOD_TAGDATATYPE_STRING, false);
    }

    FMOD_memset(value, 0, 31);
    result = this->mFile->read(value, 1, 4, &itemsread);
    if (result != FMOD_OK) {
        return result;
    }
    if (itemsread != 4) {
        return FMOD_ERR_FILE_BAD;
    }
    if (FMOD_strlen(value)) {
        this->metaData(FMOD_TAGTYPE_ID3V1, "YEAR", value, FMOD_strlen(value) + 1, FMOD_TAGDATATYPE_STRING, false);
    }

    FMOD_memset(value, 0, 31);
    result = this->mFile->read(value, 1, 30, &itemsread);
    if (result != FMOD_OK) {
        return result;
    }
    if (itemsread != 30) {
        return FMOD_ERR_FILE_BAD;
    }
    if (FMOD_strlen(value)) {
        this->metaData(FMOD_TAGTYPE_ID3V1, "COMMENT", value, FMOD_strlen(value) + 1, FMOD_TAGDATATYPE_STRING, false);
    }

    if ((value[28] == 0) && (value[29] != 0)) {
        sprintf(tmp, "%d", static_cast<int32_t>(value[29] & 0xff));
        this->metaData(FMOD_TAGTYPE_ID3V1, "TRACK", tmp, FMOD_strlen(tmp) + 1, FMOD_TAGDATATYPE_STRING, false);
    }

    FMOD_memset(value, 0, 31);
    result = this->mFile->read(value, 1, 1, &itemsread);
    if (result != FMOD_OK) {
        return result;
    }
    if (itemsread != 1) {
        return FMOD_ERR_FILE_BAD;
    }
    sprintf(tmp, "%d", static_cast<int32_t>(value[0] & 0xff));
    this->metaData(FMOD_TAGTYPE_ID3V1, "GENRE", tmp, FMOD_strlen(tmp) + 1, FMOD_TAGDATATYPE_STRING, false);

    return FMOD_OK;
}

FMOD_RESULT CodecTag::readID3v2() {
    FMOD_RESULT result;
    uint8_t size[4];
    uint8_t flags;
    uint16_t version;
    uint32_t taglen;
    uint32_t tagdatalen = 0;
    uint32_t tagoff;
    uint32_t filepos;
    uint32_t itemsread;
    int32_t wavdatapos;

    result = this->mFile->tell(&filepos);
    if (result != FMOD_OK) {
        return result;
    }
    wavdatapos = filepos;

    result = this->mFile->read(&version, 1, 2, &itemsread);
    if (result != FMOD_OK) {
        return result;
    }
    if (itemsread != 2) {
        return FMOD_ERR_FILE_BAD;
    }

    result = this->mFile->read(&flags, 1, 1, &itemsread);
    if (result != FMOD_OK) {
        return result;
    }
    if (itemsread != 1) {
        return FMOD_ERR_FILE_BAD;
    }

    result = this->mFile->read(size, 1, 4, &itemsread);
    if (result != FMOD_OK) {
        return result;
    }
    if (itemsread != 4) {
        return FMOD_ERR_FILE_BAD;
    }

    taglen = (static_cast<uint32_t>(size[0]) << 21) + (static_cast<uint32_t>(size[1]) << 14) + (static_cast<uint32_t>(size[2]) << 7) + static_cast<uint32_t>(size[3]);

    if (flags & ID3V2_FLAGS_FOOTER) {
        taglen += 10;
    }

    wavdatapos += ((taglen + 10) - 3);
    tagoff = 10;

    do {
        uint8_t tagdataid[5];
        uint16_t tagdataflags;
        int8_t ascii;

        tagdataid[0] = tagdataid[1] = tagdataid[2] = tagdataid[3] = tagdataid[4] = 0;

        if (version <= 2) {
            result = this->mFile->read(tagdataid, 3, 1, &itemsread);
            if (result != FMOD_OK) {
                return result;
            }
            if (itemsread != 1) {
                return FMOD_ERR_FILE_BAD;
            }

            result = this->mFile->read(size, 3, 1, &itemsread);
            if (result != FMOD_OK) {
                return result;
            }
            if (itemsread != 1) {
                return FMOD_ERR_FILE_BAD;
            }

            tagdatalen = (static_cast<uint32_t>(size[0]) << 16) | (static_cast<uint32_t>(size[1]) << 8) | static_cast<uint32_t>(size[2]);
        } else if (version >= 3) {
            result = this->mFile->read(tagdataid, 4, 1, &itemsread);
            if (result != FMOD_OK) {
                return result;
            }
            if (itemsread != 1) {
                return FMOD_ERR_FILE_BAD;
            }

            result = this->mFile->read(size, 4, 1, &itemsread);
            if (result != FMOD_OK) {
                return result;
            }
            if (itemsread != 1) {
                return FMOD_ERR_FILE_BAD;
            }

            result = this->mFile->read(&tagdataflags, 2, 1, &itemsread);
            if (result != FMOD_OK) {
                return result;
            }
            if (itemsread != 1) {
                return FMOD_ERR_FILE_BAD;
            }

            tagdatalen = (static_cast<uint32_t>(size[0]) << 24) + (static_cast<uint32_t>(size[1]) << 16) + (static_cast<uint32_t>(size[2]) << 8) + static_cast<uint32_t>(size[3]);
        }

        ascii = (ISASCII(tagdataid[0]) && ISASCII(tagdataid[1]) && ISASCII(tagdataid[2]) && ISASCII(tagdataid[3]));

        if (ascii && tagdatalen > 0 && tagdatalen < (1 * 1024 * 1024)) {
            FMOD_TAGDATATYPE datatype = FMOD_TAGDATATYPE_BINARY;
            char* tagdata;
            int32_t taglen = tagdatalen;

            tagdata = static_cast<char*>(FMOD_Memory_Alloc(tagdatalen));
            if (!tagdata) {
                result = this->mFile->seek(wavdatapos, SEEK_SET);
                return FMOD_ERR_MEMORY;
            }
            result = this->mFile->read(tagdata, 1, tagdatalen, &itemsread);
            if (result != FMOD_OK) {
                return result;
            }
            if (itemsread != tagdatalen) {
                FMOD_Memory_Free(tagdata);
                return result;
            }

            if (tagdataid[0] == 'T') {
                switch (tagdata[0]) {
                case 0x00:
                    datatype = FMOD_TAGDATATYPE_STRING;
                    break;
                case 0x01:
                    datatype = FMOD_TAGDATATYPE_STRING_UTF16;
                    break;
                case 0x02:
                    datatype = FMOD_TAGDATATYPE_STRING_UTF16BE;
                    break;
                case 0x03:
                    datatype = FMOD_TAGDATATYPE_STRING_UTF8;
                    break;
                default:
                    break;
                }

                FMOD_memcpy(tagdata, tagdata + 1, tagdatalen - 1);
                tagdata[tagdatalen - 1] = 0;
                taglen--;
            } else if (tagdataid[0] == 'W') {
            }

            this->metaData(FMOD_TAGTYPE_ID3V2, reinterpret_cast<char*>(tagdataid), tagdata, taglen, datatype, false);

            FMOD_Memory_Free(tagdata);
        }

        tagoff += (tagdatalen + 10);

    } while (tagoff < taglen);

    result = this->mFile->seek(wavdatapos, SEEK_SET);
    if (result != FMOD_OK) {
        return result;
    }

    return FMOD_OK;
}

FMOD_RESULT CodecTag::readID3v2FromFooter() {
    FMOD_RESULT result;
    char size[4];
    uint16_t version;
    uint8_t flags;
    int32_t taglen;
    int32_t pos;
    uint32_t itemsread;
    uint32_t filepos;

    result = this->mFile->read(&version, 1, 2, &itemsread);
    if (result != FMOD_OK) {
        return result;
    }
    if (itemsread != 2) {
        return FMOD_ERR_FILE_BAD;
    }

    result = this->mFile->read(&flags, 1, 1, &itemsread);
    if (result != FMOD_OK) {
        return result;
    }
    if (itemsread != 1) {
        return FMOD_ERR_FILE_BAD;
    }

    result = this->mFile->read(size, 1, 4, &itemsread);
    if (result != FMOD_OK) {
        return result;
    }
    if (itemsread != 4) {
        return FMOD_ERR_FILE_BAD;
    }

    taglen = (static_cast<int32_t>(size[0]) << 21) + (static_cast<int32_t>(size[1]) << 14) + (static_cast<int32_t>(size[2]) << 7) + size[3];

    if (flags & ID3V2_FLAGS_FOOTER) {
        taglen += 10;
    }

    result = this->mFile->seek(-taglen + 3, SEEK_CUR);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->mFile->tell(&filepos);
    if (result != FMOD_OK) {
        return result;
    }
    pos = filepos - 3;

    result = this->readID3v2();
    if (result != FMOD_OK) {
        return result;
    }

    result = this->mFile->seek(pos, SEEK_SET);
    if (result != FMOD_OK) {
        return result;
    }

    return FMOD_OK;
}

FMOD_RESULT CodecTag::readASF() {
    FMOD_RESULT result;
    FMOD_UINT64 offset = 0;
    FMOD_UINT64 size = -1;
    ASF_CHUNK chunk;
    uint32_t itemsread;
    uint32_t filepos;

    result = this->mFile->seek(0, SEEK_END);
    if (result != FMOD_OK) {
        return result;
    }
    result = this->mFile->tell(&filepos);
    if (result != FMOD_OK) {
        return result;
    }
    size = filepos;
    result = this->mFile->seek(0, SEEK_SET);
    if (result != FMOD_OK) {
        return result;
    }

    do {
        result = this->mFile->seek(static_cast<int32_t>(offset), SEEK_SET);
        if (result != FMOD_OK) {
            return result;
        }

        result = this->mFile->read(&chunk, 1, sizeof(ASF_CHUNK), &itemsread);
        if ((result != FMOD_OK) || (itemsread != sizeof(ASF_CHUNK))) {
            return result;
        }

#if defined(XMODEX_CPU_BIGENDIAN)
        chunk.size = FMOD_SWAPENDIAN_QWORD(chunk.size);
#endif

        if (!memcmp(&chunk.guid, &ASF_Header_Object, sizeof(FMOD_GUID))) {
            ASF_CHUNK subchunk;
            ASF_HEADEROBJECT headerobject;
            FMOD_UINT64 suboffset;

            result = this->mFile->read(&headerobject, 1, sizeof(headerobject), &itemsread);
            if ((result != FMOD_OK) || (itemsread != sizeof(headerobject))) {
                return result;
            }

            suboffset = sizeof(ASF_CHUNK) + sizeof(headerobject);

            for (int32_t i = 0; i < headerobject.number; i++) {
                result = this->mFile->seek(static_cast<int32_t>(offset + suboffset), SEEK_SET);
                if (result != FMOD_OK) {
                    return result;
                }

                result = this->mFile->read(&subchunk, 1, sizeof(ASF_CHUNK), &itemsread);
                if ((result != FMOD_OK) || (itemsread != sizeof(ASF_CHUNK))) {
                    return result;
                }

                if (!memcmp(&subchunk.guid, &ASF_Content_Description_Object, sizeof(FMOD_GUID))) {
                    static char tmp[4098];
                    static char tmp2[4098];
                    int32_t title_length = 0;
                    int32_t author_length = 0;
                    int32_t copyright_length = 0;
                    int32_t description_length = 0;
                    int32_t rating_length = 0;

                    result = this->mFile->read(&title_length, 2, 1, &itemsread);
                    if ((result != FMOD_OK) || (itemsread != 1)) {
                        return result;
                    }
                    result = this->mFile->read(&author_length, 2, 1, &itemsread);
                    if ((result != FMOD_OK) || (itemsread != 1)) {
                        return result;
                    }
                    result = this->mFile->read(&copyright_length, 2, 1, &itemsread);
                    if ((result != FMOD_OK) || (itemsread != 1)) {
                        return result;
                    }
                    result = this->mFile->read(&description_length, 2, 1, &itemsread);
                    if ((result != FMOD_OK) || (itemsread != 1)) {
                        return result;
                    }
                    result = this->mFile->read(&rating_length, 2, 1, &itemsread);
                    if ((result != FMOD_OK) || (itemsread != 1)) {
                        return result;
                    }

                    if (title_length) {
                        result = this->mFile->read(tmp, 1, title_length, &itemsread);
                        if ((result != FMOD_OK) || (itemsread != static_cast<uint32_t>(title_length))) {
                            return result;
                        }

                        wcstombs(tmp2, reinterpret_cast<const wchar_t*>(tmp), 4096);

                        result = this->metaData(FMOD_TAGTYPE_ASF, "TITLE", tmp2, FMOD_strlen(tmp2) + 1, FMOD_TAGDATATYPE_STRING, false);
                        if (result != FMOD_OK) {
                            return result;
                        }
                    }

                    if (author_length) {
                        result = this->mFile->read(tmp, 1, author_length, &itemsread);
                        if ((result != FMOD_OK) || (itemsread != static_cast<uint32_t>(author_length))) {
                            return result;
                        }

                        wcstombs(tmp2, reinterpret_cast<const wchar_t*>(tmp), 4096);

                        result = this->metaData(FMOD_TAGTYPE_ASF, "AUTHOR", tmp2, FMOD_strlen(tmp2) + 1, FMOD_TAGDATATYPE_STRING, false);
                        if (result != FMOD_OK) {
                            return result;
                        }
                    }

                    if (copyright_length) {
                        result = this->mFile->read(tmp, 1, copyright_length, &itemsread);
                        if ((result != FMOD_OK) || (itemsread != static_cast<uint32_t>(copyright_length))) {
                            return result;
                        }

                        wcstombs(tmp2, reinterpret_cast<const wchar_t*>(tmp), 4096);

                        result = this->metaData(FMOD_TAGTYPE_ASF, "COPYRIGHT", tmp2, FMOD_strlen(tmp2) + 1, FMOD_TAGDATATYPE_STRING, false);
                        if (result != FMOD_OK) {
                            return result;
                        }
                    }

                    if (description_length) {
                        result = this->mFile->read(tmp, 1, description_length, &itemsread);
                        if ((result != FMOD_OK) || (itemsread != static_cast<uint32_t>(description_length))) {
                            return result;
                        }

                        wcstombs(tmp2, reinterpret_cast<const wchar_t*>(tmp), 4096);

                        result = this->metaData(FMOD_TAGTYPE_ASF, "DESCRIPTION", tmp2, FMOD_strlen(tmp2) + 1, FMOD_TAGDATATYPE_STRING, false);
                        if (result != FMOD_OK) {
                            return result;
                        }
                    }

                    if (rating_length) {
                        result = this->mFile->read(tmp, 1, rating_length, &itemsread);
                        if ((result != FMOD_OK) || (itemsread != static_cast<uint32_t>(rating_length))) {
                            return result;
                        }

                        wcstombs(tmp2, reinterpret_cast<const wchar_t*>(tmp), 4096);

                        result = this->metaData(FMOD_TAGTYPE_ASF, "RATING", tmp2, FMOD_strlen(tmp2) + 1, FMOD_TAGDATATYPE_STRING, false);
                        if (result != FMOD_OK) {
                            return result;
                        }
                    }
                } else if (!memcmp(&subchunk.guid, &ASF_Extended_Content_Description_Object, sizeof(FMOD_GUID))) {
                    uint16_t descriptorcount;
                    static char tmp[4098];
                    static char tmp2[4098];

                    result = this->mFile->read(&descriptorcount, 2, 1, &itemsread);
                    if ((result != FMOD_OK) || (itemsread != 1)) {
                        return result;
                    }

                    for (int32_t i = 0; i < descriptorcount; i++) {
                        uint16_t Descriptor_Name_Length;
                        uint16_t Descriptor_Value_Data_Type;
                        uint16_t Descriptor_Value_Length;
                        char* Descriptor_Value;

                        result = this->mFile->read(&Descriptor_Name_Length, 2, 1, &itemsread);
                        if ((result != FMOD_OK) || (itemsread != 1)) {
                            return result;
                        }
                        result = this->mFile->read(&tmp2, 1, Descriptor_Name_Length, &itemsread);
                        if ((result != FMOD_OK) || (itemsread != Descriptor_Name_Length)) {
                            return result;
                        }
                        wcstombs(tmp, reinterpret_cast<const wchar_t*>(tmp2), 4096);

                        result = this->mFile->read(&Descriptor_Value_Data_Type, 2, 1, &itemsread);
                        if ((result != FMOD_OK) || (itemsread != 1)) {
                            return result;
                        }
                        result = this->mFile->read(&Descriptor_Value_Length, 2, 1, &itemsread);
                        if ((result != FMOD_OK) || (itemsread != 1)) {
                            return result;
                        }

                        Descriptor_Value = static_cast<char*>(FMOD_Memory_Calloc(Descriptor_Value_Length));
                        if (!Descriptor_Value) {
                            return FMOD_ERR_MEMORY;
                        }

                        result = this->mFile->read(Descriptor_Value, 1, Descriptor_Value_Length, &itemsread);
                        if ((result != FMOD_OK) || (itemsread != Descriptor_Value_Length)) {
                            return result;
                        }

                        switch (Descriptor_Value_Data_Type) {
                        case 0x0000: {
                            wcstombs(tmp2, reinterpret_cast<const wchar_t*>(Descriptor_Value), 4096);

                            result = this->metaData(FMOD_TAGTYPE_ASF, tmp, tmp2, FMOD_strlen(tmp2) + 1, FMOD_TAGDATATYPE_STRING, false);
                            if (result != FMOD_OK) {
                                return result;
                            }
                            break;
                        }
                        case 0x0002: {
                            result = this->metaData(FMOD_TAGTYPE_ASF, tmp, Descriptor_Value, Descriptor_Value_Length, FMOD_TAGDATATYPE_BINARY, false);
                            if (result != FMOD_OK) {
                                return result;
                            }
                            break;
                        }
                        case 0x0003:
                        case 0x0004:
                        case 0x0005: {
                            result = this->metaData(FMOD_TAGTYPE_ASF, tmp, Descriptor_Value, Descriptor_Value_Length, FMOD_TAGDATATYPE_INT, false);
                            if (result != FMOD_OK) {
                                return result;
                            }
                            break;
                        }
                        default: {
                            result = this->metaData(FMOD_TAGTYPE_ASF, tmp, Descriptor_Value, Descriptor_Value_Length, FMOD_TAGDATATYPE_STRING, false);
                            if (result != FMOD_OK) {
                                return result;
                            }
                            break;
                        }
                        }

                        FMOD_Memory_Free(Descriptor_Value);
                    }
                }

                suboffset += subchunk.size;
            }
        }

        offset += chunk.size;

    } while (offset < size && chunk.size);

    return FMOD_OK;
}

FMOD_RESULT CodecTag::readTags() {
    FMOD_RESULT result = FMOD_OK;
    int32_t wavdatapos = 0;
    char header[16];
    uint32_t filepos;
    uint32_t itemsread;

    wavdatapos = 0;
    for (;;) {
        result = this->mFile->seek(wavdatapos - 128, SEEK_END);
        if (result != FMOD_OK) {
            break;
        }

        result = this->mFile->read(header, 1, 3, &itemsread);
        if (result != FMOD_OK) {
            return result;
        }
        if (itemsread != 3) {
            return FMOD_ERR_FILE_BAD;
        }

        if (!FMOD_strncmp(header, "TAG", 3)) {
            result = this->readID3v1();
            if (result != FMOD_OK) {
                return result;
            }

            wavdatapos -= 128;

            result = this->mFile->tell(&filepos);
            if (result != FMOD_OK) {
                return result;
            }
            if (filepos <= 128) {
                break;
            }
        } else {
            result = this->mFile->seek(wavdatapos - 10, SEEK_END);
            if (result != FMOD_OK) {
                if (result == FMOD_ERR_FILE_COULDNOTSEEK) {
                    break;
                }
                return result;
            }

            result = this->mFile->read(header, 1, 3, &itemsread);
            if (result != FMOD_OK) {
                return result;
            }
            if (itemsread != 3) {
                return FMOD_ERR_FILE_BAD;
            }

            if (!FMOD_strncmp(header, "3DI", 3)) {
                result = this->readID3v2FromFooter();
                if (result != FMOD_OK) {
                    return result;
                }

                result = this->mFile->tell(&filepos);
                if (result != FMOD_OK) {
                    return result;
                }
                wavdatapos = filepos;
            } else {
                break;
            }
        }
    }

    wavdatapos = 0;
    result = this->mFile->seek(wavdatapos, SEEK_SET);
    if (result != FMOD_OK) {
        return result;
    }

    for (;;) {
        result = this->mFile->read(header, 1, 16, &itemsread);
        if (result != FMOD_OK) {
            return result;
        }
        if (itemsread != 16) {
            return FMOD_ERR_FILE_BAD;
        }

        if (!FMOD_strncmp(header, "TAG", 3)) {
            result = this->mFile->seek(-13, SEEK_CUR);
            if (result != FMOD_OK) {
                return result;
            }

            result = this->readID3v1();
            if (result != FMOD_OK) {
                return result;
            }
        } else if (!FMOD_strncmp(header, "ID3", 3)) {
            result = this->mFile->seek(-13, SEEK_CUR);
            if (result != FMOD_OK) {
                return result;
            }

            result = this->readID3v2();
            if (result != FMOD_OK) {
                return result;
            }
        } else if (!memcmp(header, &ASF_Header_Object, 16)) {
            result = this->readASF();
            if (result != FMOD_OK) {
                return result;
            }

            result = this->mFile->seek(wavdatapos, SEEK_SET);
            if (result != FMOD_OK) {
                return result;
            }

            break;
        } else {
            result = this->mFile->seek(wavdatapos, SEEK_SET);
            if (result != FMOD_OK) {
                return result;
            }

            break;
        }

        result = this->mFile->tell(&filepos);
        if (result != FMOD_OK) {
            return result;
        }
        wavdatapos = filepos;
    }

    return FMOD_OK;
}

FMOD_RESULT CodecTag::openInternal(FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO* userexinfo) {
    FMOD_RESULT result = FMOD_OK;
    uint32_t startoffset;
    uint32_t filepos;

    this->init(FMOD_SOUND_TYPE_TAG);

    if (usermode & FMOD_IGNORETAGS) {
        return FMOD_ERR_FORMAT;
    }

    result = this->mFile->seek(0, SEEK_SET);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->readTags();
    if (result == FMOD_OK) {
        result = this->mFile->tell(&filepos);
        if (result != FMOD_OK) {
            return result;
        }
        result = this->mFile->getStartOffset(&startoffset);
        if (result != FMOD_OK) {
            return result;
        }

        result = this->mFile->setStartOffset(startoffset + filepos);
        if (result != FMOD_OK) {
            return result;
        }
    }

    result = this->mFile->seek(0, SEEK_SET);
    if (result != FMOD_OK) {
        return result;
    }

    return FMOD_ERR_FORMAT;
}

FMOD_RESULT CodecTag::closeInternal() {
    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK CodecTag::closeCallback(FMOD_CODEC_STATE* codec) {
    auto tag = static_cast<CodecTag*>(codec);
    return tag->closeInternal();
}

FMOD_RESULT F_CALLBACK CodecTag::readCallback(FMOD_CODEC_STATE* codec, void* buffer, uint32_t sizebytes, uint32_t* bytesread) {
    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK CodecTag::setPositionCallback(FMOD_CODEC_STATE* codec, int32_t subsound, uint32_t position, FMOD_TIMEUNIT postype) {
    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK CodecTag::openCallback(FMOD_CODEC_STATE* codec, FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO* userexinfo) {
    auto tag = static_cast<CodecTag*>(codec);
    return tag->openInternal(usermode, userexinfo);
}

FMOD_CODEC_DESCRIPTION_EX* CodecTag::getDescriptionEx() {
    FMOD_memset(&tagcodec, 0, sizeof(FMOD_CODEC_DESCRIPTION_EX));
    tagcodec.name = "FMOD Tag Reader Codec";
    tagcodec.version = 0x10100;
    tagcodec.timeunits = FMOD_TIMEUNIT_PCM;

    tagcodec.open = CodecTag::openCallback;
    tagcodec.close = CodecTag::closeCallback;
    tagcodec.read = CodecTag::readCallback;
    tagcodec.setposition = CodecTag::setPositionCallback;

    tagcodec.mType = FMOD_SOUND_TYPE_TAG;
    tagcodec.mSize = sizeof(CodecTag);

    return &tagcodec;
}

} // namespace FMOD
