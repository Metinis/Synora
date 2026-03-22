#pragma once

#include <algorithm>

namespace SYN::VK {

template <typename T>
concept HandleConcept = requires(T t) {
    { t.id } -> std::convertible_to<uint32_t>;
};

template <HandleConcept Handle, typename T> struct SlotMapIterator {
    std::vector<T>::iterator it;
    std::vector<T>::iterator end;
    std::vector<uint8_t> &occupied;
    uint32_t index;

    SlotMapIterator &operator++() {
        if (it != end) {
            // to skip slots that arent occupied
            do {
                it++;
                index++;
            } while (it != end && !occupied[index]);
        }

        return *this;
    }

    std::pair<Handle, T &> operator*() { return {Handle{.id = index}, *it}; }

    bool operator!=(const SlotMapIterator &other) const {
        return it != other.it;
    }
};

template <HandleConcept Handle, typename T> class SlotMap {
  public:
    Handle insert(T value) {
        Handle handle{};
        if (!m_SlotFreeList.empty()) {
            handle = m_SlotFreeList.back();
            m_SlotFreeList.pop_back();
            m_Data[handle.id] = std::move(value);
            m_Occupied[handle.id] = true;
        } else {
            handle = Handle{.id = static_cast<uint32_t>(m_Data.size())};
            m_Data.emplace_back(std::move(value));
            m_Occupied.emplace_back(true);
        }

        assert(m_Data.size() == m_Occupied.size());
        return handle;
    }

    void remove(Handle handle) {
        assert(handle.id < m_Data.size());
        assert(m_Data.size() == m_Occupied.size());

        m_Data[handle.id] = {};

        m_SlotFreeList.emplace_back(handle.id);
        m_Occupied[handle.id] = false;
    }
    const T &at(Handle handle) const {
        assert(handle.id < m_Data.size());
        return m_Data[handle.id];
    }

    bool contains(Handle handle) {
        if (handle.id < m_Occupied.size()) {
            return m_Occupied[handle.id];
        }
        return false;
    }

    SlotMapIterator<Handle, T> begin() {
        auto it{m_Data.begin()};
        uint32_t index{};
        while (it != m_Data.end() && !m_Occupied[index]) {
            it++;
            index++;
        }

        return {
            .it = it,
            .end = m_Data.end(),
            .occupied = m_Occupied,
            .index = index,
        };
    }

    SlotMapIterator<Handle, T> end() {
        return {
            .it = m_Data.end(),
            .end = m_Data.end(),
            .occupied = m_Occupied,
            .index = static_cast<uint32_t>(m_Data.size()),
        };
    }

    T &operator[](Handle handle) {
        assert(handle.id < m_Data.size());
        assert(m_Occupied[handle.id] == true);
        return m_Data[handle.id];
    }

    const T &operator[](Handle handle) const {
        assert(handle.id < m_Data.size());
        assert(m_Occupied[handle.id] == true);
        return m_Data[handle.id];
    }

  private:
    std::vector<Handle> m_SlotFreeList;
    std::vector<uint8_t> m_Occupied;
    std::vector<T> m_Data;
};

} // namespace SYN::VK
