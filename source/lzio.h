#pragma once

#include "lua.h"

#include "lmem.h"

namespace lua {

constexpr char EOZ = -1; /* end of stream */

class Buffer {
public:
    void Init(lua_State* state)
    {
        (void)state;
        m_buffer = nullptr;
        m_capacity = 0;
    }

    void Remove(size_t i)
    {
        m_length -= i;
    }

    void Reset()
    {
        m_length = 0;
    }

    void Resize(lua_State* state, size_t size)
    {
        m_buffer = luaM_reallocvchar(state, m_buffer, m_capacity, size);
        m_capacity = size;
    }

    void Free(lua_State* state)
    {
        Resize(state, 0);
    }

    char* GetBuffer() const
    {
        return m_buffer;
    }

    size_t GetCapacity() const
    {
        return m_capacity;
    }

    size_t GetLength() const
    {
        return m_length;
    }

    void SetLength(size_t length)
    {
        m_length = length;
    }

private:
    char* m_buffer;
    size_t m_length;
    size_t m_capacity;
};

struct Zio {
    size_t m_unread;      /* bytes still unread */
    const char* m_cursor; /* current position in buffer */
    lua_Reader m_reader;  /* reader function */
    void* m_data;         /* additional data */
    lua_State* m_state;   /* Lua state (for reader) */

    void Init(lua_State* state, lua_Reader reader, void* data);
    /* read next n bytes */
    size_t Read(void* b, size_t n);

    int Fill();

    int GetChar();
};

}

// @TODO: cleanup
using lua::Buffer;
using lua::EOZ;
using lua::Zio;

#define zgetc(z) (((z)->n--) > 0 ? cast_uchar(*(z)->p++) : luaZ_fill(z))
