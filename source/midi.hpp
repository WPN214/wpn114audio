#pragma once

#include <cstdint>
#include <iterator>
#include <atomic>
#include <memory.h>

using vector_t = uint16_t;
using byte_t = uint8_t;

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
class midibuffer_t
// a fixed capacity byte vector holding midi_t events of different sizes
//-------------------------------------------------------------------------------------------------
{

public:
    //---------------------------------------------------------------------------------------------
    midibuffer_t() {}

    //---------------------------------------------------------------------------------------------
    ~midibuffer_t()
    {
        delete[] m_data;
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
        operator*()
        {
            midi_t* mt = reinterpret_cast<midi_t*>(m_data);
            return *mt;
        }

        //-----------------------------------------------------------------------------------------
        bool
        operator==(iterator const& rhs) { return m_data == rhs.m_data; }

        //-----------------------------------------------------------------------------------------
        bool
        operator !=(iterator const& rhs) { return !operator==(rhs); }

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
    allocate(vector_t nbytes)
    //---------------------------------------------------------------------------------------------
    {
        m_data = new byte_t[nbytes]();
        m_capacity.store(nbytes);
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
        size_t idx = m_index.load();
        size_t tmp = idx+sizeof(midi_t)+nbytes;

        if (tmp > m_capacity.load())
            return nullptr;

        midi_t* mt = reinterpret_cast<midi_t*>(&m_data[idx]);
        memset(mt, 0, sizeof(midi_t)+nbytes);
        mt->nbytes = nbytes;
        mt->data = &(m_data[idx+sizeof(midi_t)]);

        m_index.store(tmp);
        m_count++;
        return mt;
    }

    // --------------------------------------------------------------------------------------------
    bool
    push(midi_t const& event)
    // --------------------------------------------------------------------------------------------
    {
        midi_t* mt = reserve(event.nbytes);

        if (mt) {
            memcpy(mt, &event, sizeof(midi_t)+event.nbytes);
            return true;
        }

        else return false;
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
