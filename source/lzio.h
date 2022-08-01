/*
** $Id: lzio.h,v 1.31.1.1 2017/04/19 17:20:42 roberto Exp $
** Buffered streams
** See Copyright Notice in lua.h
*/

#ifndef lzio_h
#define lzio_h

#include "lua.h"

#include "lmem.h"


typedef struct Zio ZIO;

namespace lua {

constexpr int EOZ = -1; /* end of stream */

class Buffer {
public:
    void Init(lua_State* state)
    {
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

}

// @TODO: cleanup
using lua::Buffer;
using lua::EOZ;

#define zgetc(z) (((z)->n--) > 0 ? cast_uchar(*(z)->p++) : luaZ_fill(z))

LUAI_FUNC void luaZ_init(lua_State* L, ZIO* z, lua_Reader reader,
    void* data);
LUAI_FUNC size_t luaZ_read(ZIO* z, void* b, size_t n); /* read next n bytes */

/* --------- Private Part ------------------ */

struct Zio {
    size_t n;          /* bytes still unread */
    const char* p;     /* current position in buffer */
    lua_Reader reader; /* reader function */
    void* data;        /* additional data */
    lua_State* L;      /* Lua state (for reader) */
};

LUAI_FUNC int luaZ_fill(ZIO* z);

#endif
