#pragma once

#include <cstdint>
#include <atomic>
#include <cstdlib>
#include <iterator>
#include <memory.h>
#include <cassert>

#ifdef WPN114_DOUBLE_PRECISION
    using sample_t = double;
#else
    using sample_t = float;
#endif

namespace wpn114 {

using byte_t        = uint8_t;
using vector_t      = uint16_t;
using nframes_t     = uint32_t;
using nchannels_t   = byte_t;
using abuffer_t     = sample_t*;
class mbuffer_t;

//-------------------------------------------------------------------------------------------------
struct mempool
// that would be similar to dlang's InSituRegion but with expand capabilities
// note that memmove is used in the case of area overlapping
//-------------------------------------------------------------------------------------------------
{
    mempool(byte_t* sttc, size_t nbytes) noexcept :
        m_data(sttc), m_capacity(nbytes), m_used(0) {}

    //---------------------------------------------------------------------------------------------
    template<typename T> T*
    request(vector_t nbytes, vector_t* count) noexcept
    //---------------------------------------------------------------------------------------------
    {
        if (m_used+nbytes > m_capacity) {
           *count = m_capacity-(m_used+nbytes);
            return nullptr;
        } else {
            *count = nbytes;
             auto ret = reinterpret_cast<T*>(&m_data[m_used]);
             m_used += nbytes;
             return ret;
        }
    }

    //---------------------------------------------------------------------------------------------
    void
    expand(void* source, size_t area_sz, size_t elem_sz, vector_t* count) noexcept
    //---------------------------------------------------------------------------------------------
    {
        auto area = static_cast<byte_t*>(source);

        if (area+area_sz-elem_sz == &m_data[m_used] &&
            m_used+elem_sz < m_capacity) {
            m_used += elem_sz;
            *count += elem_sz;
            return;
        } else {
            size_t offset = area-m_data;
            size_t mvsz = area_sz+m_used-(offset+area_sz);
            memmove(&m_data[m_used], area, mvsz);
            memmove(area, &m_data[m_used], mvsz);
            m_used += elem_sz;
            *count += elem_sz;
        }
    }

private:

    byte_t* m_data;
    size_t m_capacity;
    size_t m_used;
};

//-------------------------------------------------------------------------------------------------
template<typename T>
struct vector
// a container which memory depends on a user-defined memory-pool, passed at construct-time
// useful for having static/stack memory-based vectors, and memory-pool systems
//-------------------------------------------------------------------------------------------------
{
    vector(mempool& memp) : m_mp(memp) {}

    //---------------------------------------------------------------------------------------------
    class iterator : public std::iterator<std::input_iterator_tag, T>
    //---------------------------------------------------------------------------------------------
    {
    public:
        //-----------------------------------------------------------------------------------------
        iterator(T* data) : m_data(data) {}

        //-----------------------------------------------------------------------------------------
        iterator&
        operator++() { m_data++; return *this; }

        //-----------------------------------------------------------------------------------------
        T&
        operator*() { return *m_data; }

        iterator&
        operator+=(size_t index) { m_data += index; return *this; }

        //-----------------------------------------------------------------------------------------
        bool
        operator==(iterator const& rhs) { return m_data == rhs.m_data; }

        //-----------------------------------------------------------------------------------------
        bool
        operator!=(iterator const& rhs) { return !operator==(rhs); }

    private:
        T* m_data = nullptr;
    };

    //---------------------------------------------------------------------------------------------
    iterator
    begin() { return iterator(m_data); }

    //---------------------------------------------------------------------------------------------
    iterator
    end() { return iterator(&m_data[m_nelem]); }

    //---------------------------------------------------------------------------------------------
    uint32_t
    count() const { return m_nelem; }

    void
    reserve(vector_t nelem)
    {
        m_data = m_mp.request<T>(nelem*sizeof(T), nullptr);
        m_capacity = nelem;
    }

    //---------------------------------------------------------------------------------------------
    void
    emplace(vector_t nelem, T ivalue)
    //---------------------------------------------------------------------------------------------
    {
        reserve(nelem);
        m_nelem = nelem;

        for (auto& e : *this)
             e = ivalue;
    }

    //---------------------------------------------------------------------------------------------
    T&
    pull()
    //---------------------------------------------------------------------------------------------
    {
        if (m_nelem < m_capacity) {
            T& r= m_data[m_nelem];
            m_nelem++;
            return r;
        } else {
            vector_t count = 0;
            m_mp.expand(m_data, static_cast<vector_t>(m_nelem*sizeof(T)),
                        sizeof(T), &count);
            assert(count);
            T& r = m_data[m_nelem];
            m_capacity++;
            m_nelem++;
            return r;
        }
    }

    //---------------------------------------------------------------------------------------------
    void
    push(T const& elem)
    //---------------------------------------------------------------------------------------------
    {
        T& target = pull();
        target = elem;
    }

    //---------------------------------------------------------------------------------------------
    T&
    operator[](size_t index) { return m_data[index]; }

    //---------------------------------------------------------------------------------------------
    T&
    last() { return m_data[m_nelem]; }

private:

    T*
    m_data = nullptr;

    uint32_t
    m_nelem = 0;

    uint32_t
    m_capacity = 0;

    mempool&
    m_mp;
};

//-------------------------------------------------------------------------------------------------
template<typename T>
struct vector_ref
//-------------------------------------------------------------------------------------------------
{
    typename wpn114::vector<T>::iterator begin() { return m_begin; }
    typename wpn114::vector<T>::iterator end() { return m_end; }

    T& operator[](size_t index)  { return *(m_begin+=index); }

    uint32_t
    count() const { return nelem; }

private:
    typename wpn114::vector<T>::iterator m_begin;
    typename wpn114::vector<T>::iterator m_end;
    uint32_t nelem = 0;

};

//-------------------------------------------------------------------------------------------------

typedef void
(*int_fn_t)(sample_t, void*);

typedef void
(*prc_fn_t)(vector_ref<abuffer_t>,
            vector_ref<mbuffer_t>,
            vector_t, void*);

//-------------------------------------------------------------------------------------------------
struct midi_t
//-------------------------------------------------------------------------------------------------
{
    vector_t frame;
    byte_t status;
    byte_t nbytes;
    byte_t* data;
};

//-------------------------------------------------------------------------------------------------
class mbuffer_t
// a fixed capacity byte vector holding midi_t events of different sizes
//-------------------------------------------------------------------------------------------------
{

public:
    //---------------------------------------------------------------------------------------------
    mbuffer_t() {}
    mbuffer_t(mempool& mp, size_t nbytes)
    {
        m_data = mp.request<byte_t>(nbytes, nullptr);
    }

    //---------------------------------------------------------------------------------------------
    class iterator : public std::iterator<std::input_iterator_tag, midi_t>
    //---------------------------------------------------------------------------------------------
    {
    public:
        //-----------------------------------------------------------------------------------------
        iterator(byte_t* data) : m_data(data) {}

        //-----------------------------------------------------------------------------------------
        iterator&
        operator++()
        {
            byte_t nbytes = m_data[3];
            m_data += sizeof(midi_t)+nbytes;
            return *this;
        }

        //-----------------------------------------------------------------------------------------
        midi_t&
        operator*() { return *reinterpret_cast<midi_t*>(m_data); }

        //-----------------------------------------------------------------------------------------
        bool
        operator==(iterator const& rhs) { return m_data == rhs.m_data; }

        //-----------------------------------------------------------------------------------------
        bool
        operator!=(iterator const& rhs) { return !operator==(rhs); }

    private:
        byte_t*
        m_data = nullptr;
    };

    //---------------------------------------------------------------------------------------------
    iterator
    begin() { return iterator(m_data); }

    //---------------------------------------------------------------------------------------------
    iterator
    end() { return iterator(&(m_data[m_index.load()])); }

    //---------------------------------------------------------------------------------------------
    void
    allocate(size_t nbytes)
    //---------------------------------------------------------------------------------------------
    {

    }

    //---------------------------------------------------------------------------------------------
    vector_t
    count() const { return m_count.load(); }

    //---------------------------------------------------------------------------------------------
    void
    clear() { m_index.store(0); }

    //---------------------------------------------------------------------------------------------
    midi_t*
    reserve(byte_t nbytes)
    //---------------------------------------------------------------------------------------------
    {
        byte_t msz = sizeof(midi_t);
        size_t idx = m_index.load();
        size_t tmp = idx+msz+nbytes;

        if (tmp > m_capacity.load())
            return nullptr;

        midi_t* mt = reinterpret_cast<midi_t*>(&m_data[idx]);
        memset(mt, 0, msz+nbytes);
        mt->nbytes = nbytes;
        mt->data = &(m_data[idx+msz]);

        m_index.store(tmp);
        m_count++;
        return mt;
    }

    //---------------------------------------------------------------------------------------------
    midi_t*
    reserve(byte_t nbytes, byte_t status, vector_t frame, byte_t* data)
    //---------------------------------------------------------------------------------------------
    {
        midi_t* mt = reserve(nbytes);
        mt->status = status;
        mt->frame = frame;
        memcpy(mt->data, data, nbytes);
        return mt;
    }

    //---------------------------------------------------------------------------------------------
    midi_t*
    reserve(byte_t status, vector_t frame, byte_t b1, byte_t b2)
    //---------------------------------------------------------------------------------------------
    {
        midi_t* mt = reserve(2);
        mt->status = status;
        mt->frame = frame;
        mt->data[0] = b1;
        mt->data[1] = b2;
        return mt;
    }

    // --------------------------------------------------------------------------------------------
    bool
    push(midi_t const& event)
    // --------------------------------------------------------------------------------------------
    {
        if (midi_t* mt = reserve(event.nbytes)) {
            memcpy(mt, &event, sizeof(midi_t)+event.nbytes);
            return true;
        }   else return false;
    }

    //---------------------------------------------------------------------------------------------
    midi_t*
    operator[](vector_t index)
    {
        return nullptr;
    }

private:
    //---------------------------------------------------------------------------------------------
    std::atomic<size_t>
    m_index {0}, m_capacity{0};

    std::atomic<uint16_t>
    m_count {0};

    //---------------------------------------------------------------------------------------------
    byte_t*
    m_data = nullptr;
};
}
