/**
 * @file InterpDelay2.hpp
 * @author Dale Johnson, Valley Audio Soft
 * @brief A more optimised version of the linear interpolating delay.
 */

#pragma once
#include <cstdint>
#include <cassert>
#include "../../Utilities.hpp"

template<typename T = float>
class InterpDelay {
public:
    T input = T(0);
    T output = T(0);

    InterpDelay(uint64_t maxLength = 512, uint64_t initDelayTime = 0) {
        assert(maxLength != 0);
        l = maxLength;
        setDelayTime(initDelayTime);
    }

    void process() {
        if (!buffer) {
            output = T(0);
            return;
        }
        assert(w >= 0);
        assert(w < l);
        buffer[w] = input;
        int64_t r = w - t;
        
        if (r < 0) {
            r += l;
        }

        ++w;
        if (w == l) {
            w = 0;
        }

        int64_t upperR = r - 1;
        if (upperR < 0) {
            upperR += l;
        }

        assert(r >= 0);
        assert(r < l);
        assert(upperR >= 0);
        assert(upperR < l);
        output = linterp(buffer[r], buffer[upperR], f);
    }

    T tap(int64_t i) const {
        if (!buffer) {
            return T(0);
        }

        assert(i < l);
        assert(i >= 0);

        int64_t j = w - i;
        if (j < 0) {
            j += l;
        }
        return buffer[j];
    }

    void setDelayTime(T newDelayTime) {
        if (newDelayTime >= l) {
            newDelayTime = l - 1;
        }
        if (newDelayTime < 0) {
            newDelayTime = 0;
        }
        t = static_cast<int64_t>(newDelayTime);
        f = newDelayTime - static_cast<T>(t);
    }

    void setMemory(T *mem, uint64_t memLength) {
        assert(mem);
        assert(memLength >= static_cast<uint64_t>(l));
        buffer = mem;
        clear();
    }

    uint64_t getMaxLength() const {
        return static_cast<uint64_t>(l);
    }

    void clear() {
        if (!buffer) {
            input = T(0);
            output = T(0);
            return;
        }
        for (int64_t i = 0; i < l; ++i) {
            buffer[i] = T(0);
        }
        input = T(0);
        output = T(0);
    }

private:
    T *buffer = nullptr;
    int64_t w = 0;
    int64_t t = 0;
    T f = T(0);
    int64_t l = 512;
};

