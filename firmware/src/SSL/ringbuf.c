#include <sdk_common.h>
#include <nrf_assert.h>
#include <string.h>
#include "ringbuf.h"

#define BUF_EMPTY 0
#define BUF_FULL  2
//-----------------------------------------------------------------------------
//   PRIVATE FUNCTIONS PROTOTYPES
//-----------------------------------------------------------------------------
static uint8_t *modulo(ringbuf_t const *p_ringbuf, uint8_t *ptr);

//-----------------------------------------------------------------------------
//   PUBLIC FUNCTIONS
//-----------------------------------------------------------------------------
void ringbufInit(ringbuf_t const * p_ringbuf)
{
  ASSERT(p_ringbuf);
  ASSERT(p_ringbuf->bufsize > 1);
  ringbufReset(p_ringbuf);
}

//-----------------------------------------------------------------------------
size_t ringbufGetFree(ringbuf_t const *p_ringbuf)
{
  ASSERT(p_ringbuf);
  if (p_ringbuf->data->tail == p_ringbuf->data->head)
  {
    if (p_ringbuf->data->state == BUF_EMPTY)
      return p_ringbuf->bufsize;
    else
      return 0;
  }
  if (p_ringbuf->data->tail > p_ringbuf->data->head)
    return p_ringbuf->data->tail - p_ringbuf->data->head;
  else
    return p_ringbuf->data->tail + p_ringbuf->bufsize - p_ringbuf->data->head;
}

//-----------------------------------------------------------------------------
size_t ringbufGetFull(ringbuf_t const *p_ringbuf)
{
  ASSERT(p_ringbuf);
    if (p_ringbuf->data->tail == p_ringbuf->data->head)
  {
    if (p_ringbuf->data->state == BUF_FULL)
      return p_ringbuf->bufsize;
    else
      return 0;
  }
  if (p_ringbuf->data->head > p_ringbuf->data->tail)
    return p_ringbuf->data->head - p_ringbuf->data->tail;
  else
    return p_ringbuf->data->head + p_ringbuf->bufsize - p_ringbuf->data->tail;
}

//-----------------------------------------------------------------------------
size_t ringbufPut(ringbuf_t const * p_ringbuf, uint8_t *p_data, size_t length)
{
  ASSERT(p_ringbuf);
  ASSERT(p_data);
  size_t qtt = ringbufGetFree(p_ringbuf);
  size_t processed = 0;
  if (length < qtt)   qtt = length;
  if (qtt == 0) return 0;
  uint8_t *bufEnd =  p_ringbuf->p_buffer + p_ringbuf->bufsize;
  do
  {
    size_t  partSize = bufEnd - p_ringbuf->data->head;
    partSize = MIN(partSize, qtt);
    memcpy(p_ringbuf->data->head, p_data + processed, partSize);
    processed += partSize;
    qtt -= partSize;
    p_ringbuf->data->head += partSize;
    p_ringbuf->data->head = modulo(p_ringbuf, p_ringbuf->data->head);

    if (p_ringbuf->data->head == p_ringbuf->data->tail)
      p_ringbuf->data->state = BUF_FULL;

  } while (qtt > 0);

  return processed;
}

//-----------------------------------------------------------------------------
ret_code_t ringbufGet(ringbuf_t const *p_ringbuf, uint8_t *p_data, size_t *p_size)
{
  ASSERT(p_ringbuf);
  ASSERT(p_data);
  ASSERT(p_size);

  if (*p_size == 0)  return NRF_SUCCESS;

  if (p_ringbuf->data->openedSize)
    return NRF_ERROR_BUSY;

  size_t qtt = ringbufGetFull(p_ringbuf);
  size_t processed = 0;
  qtt = MIN(*p_size, qtt);
  if (qtt == 0)
  {
   *p_size = 0;
    return NRF_SUCCESS;
  }
  uint8_t *bufEnd =  p_ringbuf->p_buffer + p_ringbuf->bufsize;
  do
  {
    size_t  partSize = bufEnd - p_ringbuf->data->tail;
    partSize = MIN(partSize, qtt);
    memcpy(p_data + processed, p_ringbuf->data->tail , partSize);
    processed += partSize;
    qtt -= partSize;
    p_ringbuf->data->tail += partSize;
    p_ringbuf->data->tail = modulo(p_ringbuf, p_ringbuf->data->tail);

    if (p_ringbuf->data->head == p_ringbuf->data->tail)
      p_ringbuf->data->state = BUF_EMPTY;

  } while (qtt > 0);

  *p_size = processed;
  return NRF_SUCCESS; 
}

//-----------------------------------------------------------------------------
ret_code_t ringbufOpenDirectRead(ringbuf_t const *p_ringbuf, uint8_t **pp_data, size_t *p_size)
{
  ASSERT(p_ringbuf);
  ASSERT(pp_data);
  ASSERT(p_size);
  
  if (p_ringbuf->data->openedSize)
    return NRF_ERROR_BUSY;

  size_t qtt = ringbufGetFull(p_ringbuf);

  uint8_t *bufEnd =  p_ringbuf->p_buffer + p_ringbuf->bufsize;
  size_t  partSize = bufEnd - p_ringbuf->data->tail;

  *p_size = p_ringbuf->data->openedSize = MIN(partSize, qtt);
  *pp_data = p_ringbuf->data->tail;
  return NRF_SUCCESS; 
}

//-----------------------------------------------------------------------------
void ringbufApplyRead(ringbuf_t const *p_ringbuf, size_t size)
{
  ASSERT(p_ringbuf);
  ASSERT(p_ringbuf->data);
  ASSERT(size <= p_ringbuf->data->openedSize);

  p_ringbuf->data->openedSize = 0;

  p_ringbuf->data->tail = modulo(p_ringbuf, p_ringbuf->data->tail + size);
  if (p_ringbuf->data->head == p_ringbuf->data->tail)
      p_ringbuf->data->state = BUF_EMPTY;
}

//-----------------------------------------------------------------------------
void ringbufReset(ringbuf_t const *p_ringbuf)
{
  p_ringbuf->data->head = p_ringbuf->data->tail = p_ringbuf->p_buffer;
  p_ringbuf->data->state = BUF_EMPTY;
  p_ringbuf->data->openedSize = 0;
}

//-----------------------------------------------------------------------------
//   PRIVATE FUNCTIONS
//-----------------------------------------------------------------------------
static uint8_t *modulo(ringbuf_t const *p_ringbuf, uint8_t *ptr)
{
  ASSERT(p_ringbuf);
  ASSERT(ptr);
  uint8_t *bufEnd =  p_ringbuf->p_buffer + p_ringbuf->bufsize;
  return (ptr >= bufEnd) ? ptr - p_ringbuf->bufsize : ptr;   
}
