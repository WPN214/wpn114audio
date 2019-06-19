#include "mbuffer.h"
#include <string.h>
#include <malloc.h>

//-------------------------------------------------------------------------------------------------
midibuffer_t*
wpn_midibuffer_alloc(vector_t capacity)
//-------------------------------------------------------------------------------------------------
{
    midibuffer_t*
    mb = malloc(sizeof(midibuffer_t) + sizeof(midi_t)*capacity);

    __c11_atomic_store(&mb->nelem, 0, __ATOMIC_RELAXED);
    __c11_atomic_store(&mb->capacity, capacity, __ATOMIC_RELAXED);

    return mb;
}

//-------------------------------------------------------------------------------------------------
void
wpn_midibuffer_push(midibuffer_t* mb, midi_t* mt)
//-------------------------------------------------------------------------------------------------
{
    vector_t
    nelem       = __c11_atomic_load(&mb->nelem, __ATOMIC_RELAXED),
    capacity    = __c11_atomic_load(&mb->capacity, __ATOMIC_RELAXED);

    if (nelem == capacity) return;
    memcpy(&(mb[nelem]), mt, sizeof(midi_t));

    __c11_atomic_store(&mb->nelem, nelem++, __ATOMIC_RELAXED);
}

//-------------------------------------------------------------------------------------------------
midi_t*
wpn_midibuffer_at(midibuffer_t* mb, vector_t index) { return &mb->data[index]; }

//-------------------------------------------------------------------------------------------------
void
wpn_midibuffer_clear(midibuffer_t* mb)
//-------------------------------------------------------------------------------------------------
{
    __c11_atomic_store(&mb->nelem, 0, __ATOMIC_RELAXED);
}
