#pragma once
#include <stdint.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t vector_t;
typedef uint8_t byte_t;

//-------------------------------------------------------------------------------------------------
typedef struct midi_t midi_t;
struct midi_t
//-------------------------------------------------------------------------------------------------
{
    vector_t frame;
    byte_t status;
    byte_t nbytes;
    byte_t data[12];
};

//-------------------------------------------------------------------------------------------------
typedef struct wpn_midibuffer_t midibuffer_t;
struct wpn_midibuffer_t
// midibuffer_t would have the same length in bytes as an audio buffer for one channel
// it being 16 bytes, we may store 256 elements for a 512 frames graph buffer size
//-------------------------------------------------------------------------------------------------
{
    atomic_ushort capacity;
    atomic_ushort nelem;
    midi_t data[];
};

//-------------------------------------------------------------------------------------------------
midibuffer_t*
wpn_midibuffer_alloc(vector_t capacity);

//-------------------------------------------------------------------------------------------------
void
wpn_midibuffer_push(midibuffer_t* buffer, midi_t* mt);

//-------------------------------------------------------------------------------------------------
midi_t*
wpn_midibuffer_at(midibuffer_t* buffer, vector_t index);

//-------------------------------------------------------------------------------------------------
void
wpn_midibuffer_clear(midibuffer_t* buffer);


#ifdef __cplusplus
}
#endif
