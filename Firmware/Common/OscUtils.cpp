#include "OscUtils.h"

class BufferPrinter : public Print {
public:
    uint8_t *buffer;
    int index;

    BufferPrinter(uint8_t *buf) : buffer(buf), index(0) {}

    virtual size_t write(uint8_t b) {
        buffer[index++] = b;
        return 1;
    }
};

bool oscCopyArgs(OSCMessage &src, OSCMessage &dst) {
    for (int i = 0; i < src.size(); i++) {
        if (src.isInt(i)) dst.add(src.getInt(i));
        else if (src.isFloat(i)) dst.add(src.getFloat(i));
        else if (src.isString(i)) {
            char s[64];
            src.getString(i, s, 64);
            dst.add(s);
        }
        else return false;
    }
    return true;
}

int oscSerialize(OSCMessage &msg, uint8_t *buffer, int maxLen) {
    BufferPrinter bp(buffer);
    msg.send(bp);
    int len = bp.index;

    while (len % 4 != 0 && len < maxLen)
        buffer[len++] = 0;

    return len;
}

OSCMessage oscBuildMessage(const char* address, const OSCMessage& src) {
    OSCMessage out(address);
    oscCopyArgs((OSCMessage&)src, out);
    return out;
}
