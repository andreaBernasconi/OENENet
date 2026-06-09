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


bool parseOscAddress(const char *address,
                     char *prefix, int prefixSize,
                     char *command, int commandSize)
{
    if (!address || address[0] != '/') return false;

    const char *addr = address + 1;
    const char *slash = strchr(addr, '/');
    if (!slash) return false;

    int prefixLen = slash - addr;
    if (prefixLen <= 0 || prefixLen >= prefixSize) return false;

    strncpy(prefix, addr, prefixLen);
    prefix[prefixLen] = '\0';

    strncpy(command, slash, commandSize);
    command[commandSize - 1] = '\0';

    return true;
}


bool oscCopyArgs(OSCMessage &src, OSCMessage &dst) {
    int count = src.size();  

    for (int i = 0; i < count; i++) {
        char t = src.getType(i);   

        switch (t) {

            case 'i':   // int32
                dst.add(src.getInt(i));
                break;

            case 'f':   // float32
                dst.add(src.getFloat(i));
                break;

            case 's': { // string
                char buf[64];
                src.getString(i, buf, sizeof(buf));
                dst.add(buf);
                break;
            }

            default:
                return false;  
        }
    }

    return true;
}



OSCMessage oscBuildMessage(const char* address, const OSCMessage& src) {
    OSCMessage out(address);
    oscCopyArgs((OSCMessage&)src, out);
    return out;
}

int buildOscForEspNow(OSCMessage &msg, uint8_t *buffer, int maxLen) {

    // Serialize OSC message into the provided buffer
    BufferPrinter bp(buffer);
    msg.send(bp);
    int len = bp.index;

    // Apply OSC padding to 4-byte boundary
    while (len % 4 != 0 && len < maxLen) {
        buffer[len++] = 0;
    }

    // Safety checks: invalid or oversized payload
    if (len <= 0) return 0;      // empty or serialization error
    if (len > maxLen) return 0;  // buffer overflow risk

    
    return len;
}
