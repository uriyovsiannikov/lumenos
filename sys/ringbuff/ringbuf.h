#ifndef RINGBUF_H
#define RINGBUF_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
typedef struct {
    uint8_t *buffer;
    size_t size;
    size_t head;
    size_t tail;
    size_t count;
    bool overwrite;
    bool is_static;
} ringbuf_t;
ringbuf_t *ringbuf_create(size_t size, bool overwrite);
ringbuf_t *ringbuf_create_static(uint8_t *buffer, size_t size, bool overwrite);
void ringbuf_destroy(ringbuf_t *rb);
void ringbuf_clear(ringbuf_t *rb);
bool ringbuf_push(ringbuf_t *rb, uint8_t data);
bool ringbuf_push_bytes(ringbuf_t *rb, const uint8_t *data, size_t len);
bool ringbuf_pop(ringbuf_t *rb, uint8_t *data);
size_t ringbuf_pop_bytes(ringbuf_t *rb, uint8_t *data, size_t len);
bool ringbuf_is_empty(const ringbuf_t *rb);
bool ringbuf_is_full(const ringbuf_t *rb);
size_t ringbuf_available(const ringbuf_t *rb);
size_t ringbuf_used(const ringbuf_t *rb);
size_t ringbuf_capacity(const ringbuf_t *rb);
bool ringbuf_peek(const ringbuf_t *rb, uint8_t *data, size_t offset);
size_t ringbuf_peek_bytes(const ringbuf_t *rb, uint8_t *data, size_t offset, size_t len);
int ringbuf_find(const ringbuf_t *rb, uint8_t value);
int ringbuf_find_bytes(const ringbuf_t *rb, const uint8_t *pattern, size_t pattern_len);
bool ringbuf_push_string(ringbuf_t *rb, const char *str);
size_t ringbuf_pop_string(ringbuf_t *rb, char *buffer, size_t buffer_size);
bool ringbuf_has_string(const ringbuf_t *rb);
void ringbuf_print_info(const ringbuf_t *rb);
#endif // RINGBUF_H