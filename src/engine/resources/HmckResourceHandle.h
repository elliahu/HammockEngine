#pragma once
#include <cstdint>

namespace Hmck {
    template<typename ResourceType>
    class ResourceHandle {
    public:
        explicit ResourceHandle(int32_t id = 0) : id_(id) {
        }

        int32_t id() const { return id_; }

        bool isValid() const { return id_ != -1; }

        bool operator==(const ResourceHandle &other) const { return id_ == other.id_; }
        bool operator!=(const ResourceHandle &other) const { return id_ != other.id_; }

    private:
        uint32_t id_;
    };
}
