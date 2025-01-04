#pragma once

#include <iostream>
#include <utility>
#include <stdexcept>

namespace Hammock {
    class ScopedMemory {
    public:
        // Constructor to take ownership of the pointer
        explicit ScopedMemory(const void* ptr = nullptr)
            : memory_(ptr) {}

        // Destructor automatically frees the memory
        ~ScopedMemory() {
            clear();
        }

        // Move constructor to allow transferring ownership
        ScopedMemory(ScopedMemory&& other) noexcept
            : memory_(other.memory_) {
            other.memory_ = nullptr; // Release ownership from the source
        }

        // Move assignment operator for ownership transfer
        ScopedMemory& operator=(ScopedMemory&& other) noexcept {
            if (this != &other) {
                clear(); // Free any existing memory
                memory_ = other.memory_;
                other.memory_ = nullptr; // Release ownership from the source
            }
            return *this;
        }

        // Deleted copy constructor and copy assignment to prevent copying
        ScopedMemory(const ScopedMemory&) = delete;
        ScopedMemory& operator=(const ScopedMemory&) = delete;

        // Free the memory manually (if needed)
        void clear() {
            if (memory_) {
                stbi_image_free(const_cast<void*>(memory_));
                memory_ = nullptr;
            }
        }

        // Retrieve the pointer
        const void* get() const {
            return memory_;
        }

        // Access the pointer with [] syntax (useful for arrays)
        const void* operator[](size_t index) const {
            return static_cast<const char*>(memory_) + index;
        }

    private:
        const void* memory_; // Pointer to the managed memory
    };
}