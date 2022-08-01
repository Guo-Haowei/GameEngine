#include "lzio.h"

#include <cstring>

namespace lua {

int Zio::Fill()
{
    size_t size;
    const char* buff;
    lua_unlock(m_state);
    buff = m_reader(m_state, m_data, &size);
    lua_lock(m_state);
    if (buff == NULL || size == 0) {
        return EOZ;
    }
    m_unread = size - 1; /* discount char being returned */
    m_cursor = buff;
    return (unsigned char)(*(m_cursor++));
}

void Zio::Init(lua_State* state, lua_Reader reader, void* data)
{
    m_state = state;
    m_reader = reader;
    m_data = data;
    m_unread = 0;
    m_cursor = nullptr;
}

size_t Zio::Read(void* b, size_t n)
{
    while (n) {
        if (m_unread == 0) {
            if (Fill() == EOZ) {
                return m_unread; /* no more input; return number of missing bytes */
            } else {
                /* luaZ_fill consumed first byte; put it back */
                ++m_unread;
                --m_cursor;
            }
        }
        const size_t m = (n <= m_unread) ? n : m_unread; /* min. between n and z->n */
        memcpy(b, m_cursor, m);
        m_unread -= m;
        m_cursor += m;
        b = (char*)b + m;
        n -= m;
    }
    return 0;
}

int Zio::GetChar()
{
    return m_unread-- > 0 ? *m_cursor++ : Fill();
}

}