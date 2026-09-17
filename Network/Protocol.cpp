#include "Network/Protocol.h"

#include <cstring>

namespace Protocol
{
namespace
{

class Writer
{
public:
    explicit Writer(MessageType type) { m_bytes.push_back(static_cast<uint8_t>(type)); }

    void putU8(uint8_t v) { m_bytes.push_back(v); }
    void putBool(bool v) { putU8(v ? 1 : 0); }
    void putI32(int32_t v) { putRaw(&v, sizeof(v)); }
    void putFloat(float v) { putRaw(&v, sizeof(v)); }
    void putBlockType(BlockType v) { putU8(static_cast<uint8_t>(v)); }

    template <typename E>
    void putEnum(E v)
    {
        putU8(static_cast<uint8_t>(v));
    }

    std::vector<uint8_t> take() { return std::move(m_bytes); }

private:
    void putRaw(const void* data, size_t size)
    {
        const auto* bytes = static_cast<const uint8_t*>(data);
        m_bytes.insert(m_bytes.end(), bytes, bytes + size);
    }

    std::vector<uint8_t> m_bytes;
};

class Reader
{
public:
    explicit Reader(const std::vector<uint8_t>& bytes)
        : m_bytes(bytes)
        , m_offset(1) // skip the leading MessageType byte
    {
    }

    uint8_t getU8() { return m_bytes[m_offset++]; }
    bool getBool() { return getU8() != 0; }
    int32_t getI32()
    {
        int32_t v;
        getRaw(&v, sizeof(v));
        return v;
    }
    float getFloat()
    {
        float v;
        getRaw(&v, sizeof(v));
        return v;
    }
    BlockType getBlockType() { return static_cast<BlockType>(getU8()); }

    template <typename E>
    E getEnum()
    {
        return static_cast<E>(getU8());
    }

private:
    void getRaw(void* out, size_t size)
    {
        std::memcpy(out, m_bytes.data() + m_offset, size);
        m_offset += size;
    }

    const std::vector<uint8_t>& m_bytes;
    size_t m_offset;
};

} // namespace

std::vector<uint8_t> encode(const BoardSnapshotMsg& msg)
{
    Writer w(MessageType::BoardSnapshot);
    w.putU8(static_cast<uint8_t>(msg.playerIndex));
    for (int row = 0; row < Board::kHeight; ++row) {
        for (int col = 0; col < Board::kWidth; ++col) {
            w.putBlockType(msg.cells[static_cast<size_t>(row)][static_cast<size_t>(col)]);
        }
    }
    return w.take();
}

std::vector<uint8_t> encode(const LiveStateMsg& msg)
{
    Writer w(MessageType::LiveState);
    for (const PieceStateMsg& p : msg.players) {
        w.putBool(p.gameOver);
        w.putBlockType(p.type);
        w.putI32(p.position.x);
        w.putI32(p.position.y);
        w.putI32(p.rotationState);
        w.putI32(p.generation);
        w.putBlockType(p.nextType);
        w.putI32(p.score);
        w.putI32(p.snowEnergy);
    }

    w.putU8(static_cast<uint8_t>(msg.inFlightAttacks.size()));
    for (const InFlightAttackMsg& a : msg.inFlightAttacks) {
        w.putEnum(a.type);
        w.putI32(a.power);
        w.putI32(a.sourceLinesCleared);
        w.putU8(static_cast<uint8_t>(a.targetPlayerIndex));
        w.putFloat(a.elapsedSeconds);
        w.putFloat(a.durationSeconds);
    }
    return w.take();
}

std::vector<uint8_t> encode(const LinesClearedFxMsg& msg)
{
    Writer w(MessageType::LinesClearedFx);
    w.putU8(static_cast<uint8_t>(msg.playerIndex));
    w.putU8(static_cast<uint8_t>(msg.clearedLines.size()));
    for (const Board::ClearedLine& line : msg.clearedLines) {
        w.putI32(line.row);
        for (int col = 0; col < Board::kWidth; ++col) {
            w.putBlockType(line.cells[static_cast<size_t>(col)]);
        }
    }
    return w.take();
}

std::vector<uint8_t> encode(const AttackLandedFxMsg& msg)
{
    Writer w(MessageType::AttackLandedFx);
    w.putU8(static_cast<uint8_t>(msg.targetPlayerIndex));
    w.putEnum(msg.attack.type);
    w.putI32(msg.attack.power);
    w.putI32(msg.attack.sourceLinesCleared);
    return w.take();
}

std::vector<uint8_t> encodeMatchReset()
{
    Writer w(MessageType::MatchReset);
    return w.take();
}

std::vector<uint8_t> encode(const InputStateMsg& msg)
{
    Writer w(MessageType::InputState);
    w.putBool(msg.left);
    w.putBool(msg.right);
    w.putBool(msg.down);
    return w.take();
}

std::vector<uint8_t> encode(const InputActionMsg& msg)
{
    Writer w(MessageType::InputAction);
    w.putEnum(msg.action);
    return w.take();
}

MessageType peekType(const std::vector<uint8_t>& bytes)
{
    return static_cast<MessageType>(bytes[0]);
}

BoardSnapshotMsg decodeBoardSnapshot(const std::vector<uint8_t>& bytes)
{
    Reader r(bytes);
    BoardSnapshotMsg msg;
    msg.playerIndex = r.getU8();
    for (int row = 0; row < Board::kHeight; ++row) {
        for (int col = 0; col < Board::kWidth; ++col) {
            msg.cells[static_cast<size_t>(row)][static_cast<size_t>(col)] = r.getBlockType();
        }
    }
    return msg;
}

LiveStateMsg decodeLiveState(const std::vector<uint8_t>& bytes)
{
    Reader r(bytes);
    LiveStateMsg msg;
    for (PieceStateMsg& p : msg.players) {
        p.gameOver = r.getBool();
        p.type = r.getBlockType();
        p.position.x = r.getI32();
        p.position.y = r.getI32();
        p.rotationState = r.getI32();
        p.generation = r.getI32();
        p.nextType = r.getBlockType();
        p.score = r.getI32();
        p.snowEnergy = r.getI32();
    }

    const uint8_t attackCount = r.getU8();
    msg.inFlightAttacks.reserve(attackCount);
    for (uint8_t i = 0; i < attackCount; ++i) {
        InFlightAttackMsg a;
        a.type = r.getEnum<SnowAttackType>();
        a.power = r.getI32();
        a.sourceLinesCleared = r.getI32();
        a.targetPlayerIndex = r.getU8();
        a.elapsedSeconds = r.getFloat();
        a.durationSeconds = r.getFloat();
        msg.inFlightAttacks.push_back(a);
    }
    return msg;
}

LinesClearedFxMsg decodeLinesClearedFx(const std::vector<uint8_t>& bytes)
{
    Reader r(bytes);
    LinesClearedFxMsg msg;
    msg.playerIndex = r.getU8();
    const uint8_t lineCount = r.getU8();
    msg.clearedLines.reserve(lineCount);
    for (uint8_t i = 0; i < lineCount; ++i) {
        Board::ClearedLine line;
        line.row = r.getI32();
        for (int col = 0; col < Board::kWidth; ++col) {
            line.cells[static_cast<size_t>(col)] = r.getBlockType();
        }
        msg.clearedLines.push_back(line);
    }
    return msg;
}

AttackLandedFxMsg decodeAttackLandedFx(const std::vector<uint8_t>& bytes)
{
    Reader r(bytes);
    AttackLandedFxMsg msg;
    msg.targetPlayerIndex = r.getU8();
    msg.attack.type = r.getEnum<SnowAttackType>();
    msg.attack.power = r.getI32();
    msg.attack.sourceLinesCleared = r.getI32();
    return msg;
}

InputStateMsg decodeInputState(const std::vector<uint8_t>& bytes)
{
    Reader r(bytes);
    InputStateMsg msg;
    msg.left = r.getBool();
    msg.right = r.getBool();
    msg.down = r.getBool();
    return msg;
}

InputActionMsg decodeInputAction(const std::vector<uint8_t>& bytes)
{
    Reader r(bytes);
    InputActionMsg msg;
    msg.action = r.getEnum<InputActionType>();
    return msg;
}

} // namespace Protocol
