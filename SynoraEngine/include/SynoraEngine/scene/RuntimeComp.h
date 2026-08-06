#pragma once
#include "RuntimeCompManager.h"

#include <assert.h>
#include <cstring>
#include <string_view>
#include <vector>

namespace SYN {
    struct RuntimeComponent {
        std::vector<char> data{};
        CompDesc description{};

        void* getFieldPtr(std::string_view name) {
            for (int i = 0; i < description.fieldCount; ++i) {
                if (strcmp(description.fields[i].name, name.data()) == 0) {
                    return data.data() + description.fields[i].offset;
                }
            }
            return nullptr;
        }
        template<typename T>
        T &getField(const char *fieldName) {
            void *ptr = getFieldPtr(fieldName);
            if (ptr != nullptr) {
                return *reinterpret_cast<T *>(ptr);
            }
            return nullptr;
        }
    };
}
