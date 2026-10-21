#pragma once

namespace westie::platform::win32 {

class ComApartment {
public:
    ComApartment();
    ~ComApartment();

    ComApartment(const ComApartment&) = delete;
    ComApartment& operator=(const ComApartment&) = delete;
};

}
