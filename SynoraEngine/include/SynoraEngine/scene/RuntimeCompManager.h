#pragma once

namespace SYN {
struct EngineContext;

enum class FieldType {
    STRING,
    FLOAT,
    INT,
    BOOL
};
template<typename T>
constexpr FieldType DeduceFieldType() {
    return FieldType::STRING;
};

template<>
constexpr FieldType DeduceFieldType<int>() { return FieldType::INT; }

template<>
constexpr FieldType DeduceFieldType<float>() { return FieldType::FLOAT; }

template<>
constexpr FieldType DeduceFieldType<bool>() { return FieldType::BOOL; }

template<>
constexpr FieldType DeduceFieldType<const char*>() { return FieldType::STRING; }

struct FieldDesc {
    const char* name;
    FieldType type;
    size_t offset;
};
struct CompDesc {
    const char* name;
    size_t size;

    FieldDesc* fields;
    size_t fieldCount;
};

#define FIELD(Comp, Member) \
    SYN::FieldDesc { \
        .name = #Member, \
        .type = SYN::DeduceFieldType<decltype(Comp::Member)>(), \
        .offset = offsetof(Comp, Member), \
    }

#define REGISTER_COMPONENT(Manager, Type, ...)                \
    do {                                                       \
        static SYN::FieldDesc fields[] = { __VA_ARGS__ };      \
        Manager->registerComponent(SYN::CompDesc{             \
            .name = #Type,                                     \
            .size = sizeof(Type),                              \
            .fields = fields,                                  \
            .fieldCount = sizeof(fields) / sizeof(fields[0])   \
        });                                                    \
    } while(0)

class RuntimeCompManager {
    public:
    RuntimeCompManager() = default;
    void init(EngineContext* ctx);
    void registerComponent(const CompDesc& comp);
    CompDesc getComponentDesc(const std::string& name);
    void removeComponent(const char* name);
    ~RuntimeCompManager() = default;
private:
    std::unordered_map<const char*, CompDesc> m_Components{};
    EngineContext* m_Ctx{};
};
}

