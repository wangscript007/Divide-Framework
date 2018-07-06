#ifndef MANGOSSERVER_WORLDPACKET_H
#define MANGOSSERVER_WORLDPACKET_H

#include "Platform/Platform/Headers/ByteBuffer.h"
#include "OPCodesTpl.h"
#include <boost/serialization/base_object.hpp>

namespace Divide {

class WorldPacket : public ByteBuffer {
   public:
    // just container for later use
    WorldPacket() : WorldPacket(OPCodes::MSG_NOP, 0)
    {
    }

    explicit WorldPacket(OPCodes::ValueType opcode, 
                         size_t res = 200)
        : ByteBuffer(res),
          m_opcode(opcode)
    {
    }

    // copy constructor
    WorldPacket(const WorldPacket& packet)
        : ByteBuffer(packet),
          m_opcode(packet.m_opcode)
    {
    }

    void Initialize(U16 opcode, size_t newres = 200) {
        clear();
        _storage.reserve(newres);
        m_opcode = opcode;
    }

    OPCodes::ValueType opcode() const { return m_opcode; }
    void opcode(OPCodes::ValueType opcode) { m_opcode = opcode; }

   private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ACKNOWLEDGE_UNUSED(version);

        ar & boost::serialization::base_object<ByteBuffer>(*this);
        ar & m_opcode;
    }

   protected:
    OPCodes::ValueType m_opcode;
};

};  // namespace Divide

#endif