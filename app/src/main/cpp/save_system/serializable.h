#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <cstring>
#include <android/log.h>

#define LOG_TAG_SER "Serializable"
#define SER_LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_SER, __VA_ARGS__)
#define SER_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_SER, __VA_ARGS__)

// ============================================================================
// BinaryWriter - Writes primitive types to a byte buffer
// ============================================================================

class BinaryWriter {
public:
    BinaryWriter() = default;

    // Primitive writes
    void writeBool(bool v) { writeRaw(&v, sizeof(v)); }
    void writeInt8(int8_t v) { writeRaw(&v, sizeof(v)); }
    void writeUint8(uint8_t v) { writeRaw(&v, sizeof(v)); }
    void writeInt16(int16_t v) { writeRaw(&v, sizeof(v)); }
    void writeUint16(uint16_t v) { writeRaw(&v, sizeof(v)); }
    void writeInt32(int32_t v) { writeRaw(&v, sizeof(v)); }
    void writeUint32(uint32_t v) { writeRaw(&v, sizeof(v)); }
    void writeInt64(int64_t v) { writeRaw(&v, sizeof(v)); }
    void writeUint64(uint64_t v) { writeRaw(&v, sizeof(v)); }
    void writeFloat(float v) { writeRaw(&v, sizeof(v)); }
    void writeDouble(double v) { writeRaw(&v, sizeof(v)); }

    // String write (length-prefixed)
    void writeString(const std::string& s) {
        uint32_t len = static_cast<uint32_t>(s.size());
        writeUint32(len);
        if (len > 0) {
            writeRaw(s.data(), len);
        }
    }

    // Vec3 write
    void writeVec3(const glm::vec3& v) {
        writeFloat(v.x);
        writeFloat(v.y);
        writeFloat(v.z);
    }

    // Raw bytes
    void writeRaw(const void* data, size_t size) {
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        buffer_.insert(buffer_.end(), bytes, bytes + size);
    }

    // Access buffer
    const std::vector<uint8_t>& getBuffer() const { return buffer_; }
    size_t getSize() const { return buffer_.size(); }
    void clear() { buffer_.clear(); }

private:
    std::vector<uint8_t> buffer_;
};

// ============================================================================
// BinaryReader - Reads primitive types from a byte buffer
// ============================================================================

class BinaryReader {
public:
    BinaryReader(const uint8_t* data, size_t size)
        : data_(data), size_(size), pos_(0) {}

    // Primitive reads
    bool readBool() { return readRaw<bool>(); }
    int8_t readInt8() { return readRaw<int8_t>(); }
    uint8_t readUint8() { return readRaw<uint8_t>(); }
    int16_t readInt16() { return readRaw<int16_t>(); }
    uint16_t readUint16() { return readRaw<uint16_t>(); }
    int32_t readInt32() { return readRaw<int32_t>(); }
    uint32_t readUint32() { return readRaw<uint32_t>(); }
    int64_t readInt64() { return readRaw<int64_t>(); }
    uint64_t readUint64() { return readRaw<uint64_t>(); }
    float readFloat() { return readRaw<float>(); }
    double readDouble() { return readRaw<double>(); }

    // String read (length-prefixed)
    std::string readString() {
        uint32_t len = readUint32();
        if (len == 0) return "";
        if (pos_ + len > size_) {
            SER_LOGE("BinaryReader: string read out of bounds");
            return "";
        }
        std::string result(reinterpret_cast<const char*>(data_ + pos_), len);
        pos_ += len;
        return result;
    }

    // Vec3 read
    glm::vec3 readVec3() {
        float x = readFloat();
        float y = readFloat();
        float z = readFloat();
        return glm::vec3(x, y, z);
    }

    // Skip bytes
    void skip(size_t bytes) {
        if (pos_ + bytes > size_) {
            SER_LOGE("BinaryReader: skip out of bounds");
            return;
        }
        pos_ += bytes;
    }

    // State
    size_t getPosition() const { return pos_; }
    size_t getSize() const { return size_; }
    bool isEOF() const { return pos_ >= size_; }
    bool isValid() const { return data_ != nullptr && size_ > 0; }

private:
    const uint8_t* data_;
    size_t size_;
    size_t pos_;

    template<typename T>
    T readRaw() {
        if (pos_ + sizeof(T) > size_) {
            SER_LOGE("BinaryReader: read out of bounds at pos %zu", pos_);
            return T{};
        }
        T value;
        std::memcpy(&value, data_ + pos_, sizeof(T));
        pos_ += sizeof(T);
        return value;
    }
};

// ============================================================================
// ISaveable - Interface for serializable game systems
// ============================================================================

class ISaveable {
public:
    virtual ~ISaveable() = default;

    // Serialize system state to binary writer
    virtual void serialize(BinaryWriter& writer) const = 0;

    // Deserialize system state from binary reader
    virtual bool deserialize(BinaryReader& reader) = 0;

    // Get system identifier (for debugging)
    virtual const char* getSystemName() const = 0;
};

// ============================================================================
// Serialization helpers for STL containers
// ============================================================================

namespace save_util {

// Vector of ISaveable-compatible types
template<typename T>
void writeVector(BinaryWriter& writer, const std::vector<T>& vec) {
    writer.writeUint32(static_cast<uint32_t>(vec.size()));
    for (const auto& item : vec) {
        writer.writeUint32(item);
    }
}

template<typename T>
void readVector(BinaryReader& reader, std::vector<T>& vec) {
    uint32_t count = reader.readUint32();
    vec.resize(count);
    for (uint32_t i = 0; i < count; ++i) {
        vec[i] = static_cast<T>(reader.readUint32());
    }
}

// String vector
inline void writeStringVector(BinaryWriter& writer, const std::vector<std::string>& vec) {
    writer.writeUint32(static_cast<uint32_t>(vec.size()));
    for (const auto& s : vec) {
        writer.writeString(s);
    }
}

inline void readStringVector(BinaryReader& reader, std::vector<std::string>& vec) {
    uint32_t count = reader.readUint32();
    vec.resize(count);
    for (uint32_t i = 0; i < count; ++i) {
        vec[i] = reader.readString();
    }
}

// Map<uint32_t, T>
template<typename T>
void writeUint32Map(BinaryWriter& writer, const std::unordered_map<uint32_t, T>& map) {
    writer.writeUint32(static_cast<uint32_t>(map.size()));
    for (const auto& [key, value] : map) {
        writer.writeUint32(key);
        writer.writeFloat(value);
    }
}

template<typename T>
void readUint32Map(BinaryReader& reader, std::unordered_map<uint32_t, T>& map) {
    uint32_t count = reader.readUint32();
    map.clear();
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t key = reader.readUint32();
        T value = static_cast<T>(reader.readFloat());
        map[key] = value;
    }
}

// Map<string, float>
inline void writeStringFloatMap(BinaryWriter& writer, const std::unordered_map<std::string, float>& map) {
    writer.writeUint32(static_cast<uint32_t>(map.size()));
    for (const auto& [key, value] : map) {
        writer.writeString(key);
        writer.writeFloat(value);
    }
}

inline void readStringFloatMap(BinaryReader& reader, std::unordered_map<std::string, float>& map) {
    uint32_t count = reader.readUint32();
    map.clear();
    for (uint32_t i = 0; i < count; ++i) {
        std::string key = reader.readString();
        float value = reader.readFloat();
        map[key] = value;
    }
}

} // namespace save_util
