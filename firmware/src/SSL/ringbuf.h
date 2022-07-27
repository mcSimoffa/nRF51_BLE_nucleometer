#ifndef RINGBUF_H
#define RINGBUF_H

#include <stdint.h>
#include <stddef.h>
#include "sdk_errors.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Ring buffer instance data.
 * */
typedef struct
{
  uint8_t   *head;
  uint8_t   *tail;
  uint8_t   state;  //0-Empty, 1=full
  size_t    openedSize;
} ringbuf_data_t;

/**
 * @brief Ring buffer instance structure.
 * */
typedef struct
{
    uint8_t         *p_buffer;
    uint32_t        bufsize;
    ringbuf_data_t  *data;
} ringbuf_t;

/**
 * @brief Macro for defining a ring buffer instance.
 *
 * @param _name Instance name.
 * @param _size Size of the ring buffer (must be a power of 2).
 * */
#define RING_BUF_DEF(_name, _size)                \
static const ringbuf_t _name =                    \
{                                                 \
  .p_buffer = (uint8_t*)&(uint8_t[_size]){0},     \
  .bufsize = _size,                               \
  .data = (ringbuf_data_t*)&(ringbuf_data_t){0},  \
}

/**
 * @brief Function for initializing a ring buffer instance.
 *
 * @param p_ringbuf          Pointer to the ring buffer instance.
 * */
void ringbufInit(ringbuf_t const *p_ringbuf);

/**
 * @brief Function for put blob to a ring buffer.
 *
 * @param[in] p_ringbuf      Pointer to the ring buffer instance.
 * @param[in] p_data         Pointer to data to store.
 * @param[in] length         Length of data block to store.
 *
 * @retval Amount of really putted bytes to ringbuf.
 * */
size_t ringbufPut(ringbuf_t const * p_ringbuf, uint8_t *p_data, size_t length);

/**
 * Function for getting data from the ring buffer.
 *
 * @param[in]     p_ringbuf     Pointer to the ring buffer instance.
 * @param[in]     p_data        Pointer to store reading data block
 * @param[in-out] p_size        Amount of wanted to read, Function put here real amount to read.
 *
 * @retval  NFR_ERROR_BUSU - if application have direct access to read and didn't close it.
 * @retval  NFR_SUCCES -     if operation success (but it can write zero at p_size)
  */
ret_code_t ringbufGet(ringbuf_t const *p_ringbuf, uint8_t *p_data, size_t *p_size);

/**
 * Function for open internal buffer to getting data from.
 *
 * @param[in]     p_ringbuf     Pointer to the ring buffer instance.
 * @param[in]     pp_data       Pointer to pointer to the read position in internal buffer.
 *                              Application shouldn't write here !! Only for reading.
 * @param[out] p_size           Function put here real amount to read.
 *
 * @retval  NFR_ERROR_BUSU - if application have direct access to read and didn't close it.
 * @retval  NFR_SUCCES -     if operation success (but it can write zero at p_size)
  */
ret_code_t ringbufOpenDirectRead(ringbuf_t const *p_ringbuf, uint8_t **pp_data, size_t *p_size);

void ringbufApplyRead(ringbuf_t const *p_ringbuf, size_t size);

/**
 * @brief Function for getting free spice in ring buffer.
 *
 * @param[in] p_ringbuf          Pointer to the ring buffer instance.
 *
 * @return  amount of free space.
 * */
size_t ringbufGetFree(ringbuf_t const *p_ringbuf);

/**
 * @brief Function for getting full spice in ring buffer.
 *
 * @param[in] p_ringbuf          Pointer to the ring buffer instance.
 *
 * @return  Amount of full space
 * */
size_t ringbufGetFull(ringbuf_t const *p_ringbuf);

/**
 * @brief Function clear ring buffer content.
 *
 * @param[in] p_ringbuf          Pointer to the ring buffer instance.
 * */
void ringbufReset(ringbuf_t const *p_ringbuf);

#ifdef __cplusplus
}
#endif

#endif //RINGBUF_H
/** @} */
