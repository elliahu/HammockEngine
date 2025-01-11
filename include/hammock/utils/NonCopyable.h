#pragma once
namespace hammock {
    class NonCopyable {
    protected:
        // Protected default constructor and destructor
        // Allows instantiation by derived classes, but not directly
        NonCopyable() = default;
        ~NonCopyable() = default;

        // Deleted copy constructor and copy assignment operator
        NonCopyable(const NonCopyable &) = delete;
        NonCopyable &operator=(const NonCopyable &) = delete;
    };
}