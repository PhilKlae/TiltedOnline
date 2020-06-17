#pragma once

#if TP_FALLOUT4

#include <Games/Primitives.h>

struct BSFixedString
{
    BSFixedString(const BSFixedString&) = delete;
    BSFixedString& operator=(const BSFixedString&) = delete;

    struct Data
    {
        Data* next;
        uint32_t flags;
        uint32_t length;
        Data* derived;
        char data[1];

        [[nodiscard]] const Data* GetExternData() const noexcept
        {
            auto* pIterator = this;

            while (pIterator->flags & 0x4000)
                pIterator = pIterator->derived;

            return pIterator;
        }

        [[nodiscard]] bool IsAscii() const noexcept
        {
            return (GetExternData()->flags & 0x8000) == 0;
        }

        [[nodiscard]] const char* AsAscii() const
        {
            return reinterpret_cast<const char*>(GetExternData()->data);
        }

        [[nodiscard]] const wchar_t* AsWide() const
        {
            return reinterpret_cast<const wchar_t*>(GetExternData()->data);
        }

    };

    static_assert(offsetof(Data, data) == 0x18);

    // Be careful using this, the destructor does NOT release the memory, call Release manually if you wish to delete it
    BSFixedString(const char* acpData);
    BSFixedString(BSFixedString&& aRhs);
    ~BSFixedString();

    BSFixedString& operator=(BSFixedString&& aRhs);

    void Release();

    [[nodiscard]] bool IsAscii() const noexcept { return m_data->IsAscii(); }
    [[nodiscard]] const char* AsAscii() const { return m_data ? m_data->AsAscii() : nullptr; }
    [[nodiscard]] const wchar_t* AsWide() const { return m_data ? m_data->AsWide() : nullptr; }

    Data* m_data;
};

#endif
