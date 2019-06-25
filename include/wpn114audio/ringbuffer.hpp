#pragma once

#include <atomic>
#include <stdio.h>
#include <QObject>

// TODO: use C99 F.A.M instead

namespace wpn114
{

using byte_t = uint8_t;

struct rbuffer_data_t
{
    size_t
    count = 0;

    byte_t*
    data = nullptr;
};

using rbuffer_data_vector = std::array<rbuffer_data_t,2>;

// ================================================================================================
class rbuffer
// this is based on jack's ringbuffer
// using std::atomic instead of mlock for the lock-free algorithm
// ================================================================================================
{

public:

    // --------------------------------------------------------------------------------------------
    rbuffer() {}

    // --------------------------------------------------------------------------------------------
    void
    allocate(size_t nbytes)
    // --------------------------------------------------------------------------------------------
    {
        int p2; // get the closest power of two
        for (p2 = 1; 1 << p2 < nbytes; p2++);

        m_size = 1 << p2;
        m_size_mask = m_size;
        m_size_mask -= 1;

        m_data = new byte_t[m_size]();
    }

    // --------------------------------------------------------------------------------------------
    bool
    can_read(size_t nbytes) const { return can_read() <= nbytes; }

    // --------------------------------------------------------------------------------------------
    size_t
    can_read() const
    // --------------------------------------------------------------------------------------------
    {
        size_t w = w_index, r = r_index;
        if (w > r) return w-r;
        else return (w-r+m_size) & m_size_mask;
    }

    // --------------------------------------------------------------------------------------------
    size_t
    can_write() const
    // --------------------------------------------------------------------------------------------
    {
        size_t w = w_index, r = r_index;

        if (w > r) return ((r-w+m_size) & m_size_mask)-1;
        else if (w < r) return (r-w)-1;
        else return m_size-1;
    }

    bool
    can_write(size_t nbytes) const { return can_write() <= nbytes; }

    // --------------------------------------------------------------------------------------------
    size_t
    read(rbuffer_data_vector& vec, size_t nbytes = 0)
    // this is a no-copy read, see read_copy otherwise
    // --------------------------------------------------------------------------------------------
    {
        size_t free_cnt, cnt2, w, r;
        w = w_index;
        r = r_index;

        if (w > r)
             free_cnt = w-r;
        else free_cnt = (w-r+m_size) & m_size_mask;

        cnt2 = r+free_cnt;

        if (cnt2 > m_size)
        {
            vec[0].data = &(m_data[r]);
            vec[0].count = m_size-r;
            vec[1].data = m_data;
            vec[1].count = cnt2 & m_size_mask;
        }
        else
        {
            vec[0].data = &(m_data[r]);
            vec[0].count = free_cnt;
            vec[1].count = 0;
        }

        return free_cnt;
    }

    // --------------------------------------------------------------------------------------------
    size_t
    read_copy(byte_t* dest, size_t nbytes = 0)
    // --------------------------------------------------------------------------------------------
    {
        size_t free_cnt, cnt2, to_read, n1, n2;
        size_t r = r_index.load(), w = w_index.load();

        if ((free_cnt = can_read()) == 0)
            return 0;

        if (nbytes == 0)
            nbytes = free_cnt;

        to_read = nbytes > free_cnt ? free_cnt : nbytes;
        cnt2 = r+to_read;

        if (cnt2 > m_size) {
            n1 = m_size-r;
            n2 = cnt2 & m_size_mask;
        } else {
            n1 = to_read;
            n2 = 0;
        }

        memcpy(dest, &(m_data[r]), n1);
        r = (r+n1) & m_size_mask;

        if (n2) {
            memcpy(dest+n1, &(m_data[r]), n2);
            r = (r+n2) & m_size_mask;
        }

        r_index.store(r);
        return to_read;
    }

    // --------------------------------------------------------------------------------------------
    size_t
    peek(byte_t* data, size_t nbytes = 0) const
    // unused for now
    // --------------------------------------------------------------------------------------------
    {
        return 0;
    }

    // --------------------------------------------------------------------------------------------
    size_t
    reserve(rbuffer_data_vector& vec, size_t nbytes = 0)
    // non-copy write, see write_copy otherwise
    // --------------------------------------------------------------------------------------------
    {
        size_t free_cnt, cnt2;
        size_t w = w_index.load(), r = r_index.load();

        if (w > r)
            free_cnt = ((r-w+m_size) & m_size_mask) -1;
        else if (w < r)
            free_cnt = (r-w)-1;
        else free_cnt = m_size-1;

        cnt2 = w+free_cnt;

        if (cnt2 > m_size)
        {
            vec[0].data = &(m_data[w]);
            vec[0].count = m_size-w;
            vec[1].data = m_data;
            vec[1].count = cnt2 & m_size_mask;
        }
        else
        {
            vec[0].data = &(m_data[w]);
            vec[0].count = free_cnt;
            vec[1].count = 0;
        }

        return free_cnt;
    }

    // --------------------------------------------------------------------------------------------
    size_t
    write_copy(rbuffer_data_vector& vec)
    // --------------------------------------------------------------------------------------------
    {
        size_t
        nbytes = write_copy(vec[0].data, vec[0].count);

        if (vec[1].count)
            nbytes += write_copy(vec[1].data, vec[1].count);

        return nbytes;
    }

    // --------------------------------------------------------------------------------------------
    size_t
    write_copy(byte_t* source, size_t nbytes = 0)
    // --------------------------------------------------------------------------------------------
    {
        size_t free_cnt, cnt2, to_write, n1, n2;
        size_t w = w_index.load();

        if ((free_cnt = can_write()) == 0)
            return 0;

        if (nbytes == 0)
            nbytes = free_cnt;

        to_write = nbytes > free_cnt ? free_cnt : nbytes;
        cnt2 = w+to_write;

        if (cnt2 > m_size) {
            n1 = m_size-w;
            n2 = cnt2 & m_size_mask;
        } else {
            n1 = to_write;
            n2 = 0;
        }

        memcpy(&(m_data[w]), source, n1);
        w = (w+n1) & m_size_mask;

        if (n2) {
            memcpy(&(m_data[w]), source, n2);
            w = (w+n2) & m_size_mask;
        }

        w_index.store(w);
        return to_write;
    }

    // --------------------------------------------------------------------------------------------
    void
    read_fwd(size_t nbytes = 0)
    // --------------------------------------------------------------------------------------------
    {
        size_t tmp = (r_index+nbytes) & m_size_mask;
        r_index = tmp;
    }

    // --------------------------------------------------------------------------------------------
    void
    write_fwd(size_t nbytes = 0)
    // --------------------------------------------------------------------------------------------
    {
        size_t tmp = (w_index+nbytes) & m_size_mask;
        w_index = tmp;
    }

private:

    // --------------------------------------------------------------------------------------------
    std::atomic<size_t>
    w_index{0}, r_index{0};

    // --------------------------------------------------------------------------------------------
    size_t
    m_size{0}, m_size_mask{0};

    // --------------------------------------------------------------------------------------------
    byte_t*
    m_data = nullptr;

};
}
