#include "ringbuf.h"
#include "../../libs/print.h"
#include "../../libs/string.h"
#include "../../modules/mm/mm.h"
#include <stddef.h>
ringbuf_t *ringbuf_create(size_t size, bool overwrite) {
    if (size == 0) {
        print_error("Ringbuf: Zero size requested\n");
        return NULL;
    }
    ringbuf_t *rb = (ringbuf_t *)malloc(sizeof(ringbuf_t));
    if (!rb) {
        print_error("Ringbuf: Failed to allocate struct\n");
        return NULL;
    }
    rb->buffer = (uint8_t *)malloc(size);
    if (!rb->buffer) {
        print_error("Ringbuf: Failed to allocate buffer\n");
        free(rb);
        return NULL;
    }
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    rb->overwrite = overwrite;
    rb->is_static = false;  // Динамически выделенный буфер
    memset(rb->buffer, 0, size);
    return rb;
}
ringbuf_t *ringbuf_create_static(uint8_t *buffer, size_t size, bool overwrite) {
    if (!buffer || size == 0) {
        print_error("Ringbuf: Invalid static buffer parameters\n");
        return NULL;
    }
    ringbuf_t *rb = (ringbuf_t *)malloc(sizeof(ringbuf_t));
    if (!rb) {
        print_error("Ringbuf: Failed to allocate struct for static buffer\n");
        return NULL;
    }
    rb->buffer = buffer;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    rb->overwrite = overwrite;
    rb->is_static = true;  // Статический буфер
    
    return rb;
}
void ringbuf_destroy(ringbuf_t *rb) {
    if (!rb) return;
    if (!rb->is_static && rb->buffer) {
        free(rb->buffer);
        rb->buffer = NULL;
    }
    free(rb);
}
void ringbuf_clear(ringbuf_t *rb) {
    if (!rb) return;
    
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    if (rb->buffer) {
        memset(rb->buffer, 0, rb->size);
    }
}
bool ringbuf_push(ringbuf_t *rb, uint8_t data) {
    if (!rb) {
        print_error("Ringbuf: NULL pointer in push\n");
        return false;
    }
    
    if (!rb->buffer) {
        print_error("Ringbuf: Buffer is NULL\n");
        return false;
    }
    
    if (ringbuf_is_full(rb)) {
        if (!rb->overwrite) {
            print_warning("Ringbuf: Buffer full, overwrite disabled\n");
            return false;  // Буфер полен и перезапись запрещена
        }
        rb->tail = (rb->tail + 1) % rb->size;
        rb->count--;
    }
    
    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) % rb->size;
    rb->count++;
    
    return true;
}
bool ringbuf_push_bytes(ringbuf_t *rb, const uint8_t *data, size_t len) {
    if (!rb || !data || len == 0) {
        print_error("Ringbuf: Invalid parameters in push_bytes\n");
        return false;
    }
    if (len > rb->size && !rb->overwrite) {
        print_warning("Ringbuf: Cannot push, buffer too small\n");
        return false;
    }
    
    for (size_t i = 0; i < len; i++) {
        if (!ringbuf_push(rb, data[i])) {
            return false;
        }
    }
    
    return true;
}
bool ringbuf_pop(ringbuf_t *rb, uint8_t *data) {
    if (!rb || !data) {
        print_error("Ringbuf: NULL pointer in pop\n");
        return false;
    }
    
    if (ringbuf_is_empty(rb)) {
        return false;
    }
    
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->size;
    rb->count--;
    
    return true;
}
size_t ringbuf_pop_bytes(ringbuf_t *rb, uint8_t *data, size_t len) {
    if (!rb || !data || len == 0) {
        return 0;
    }
    
    size_t popped = 0;
    while (popped < len && !ringbuf_is_empty(rb)) {
        if (!ringbuf_pop(rb, &data[popped])) {
            break;
        }
        popped++;
    }
    
    return popped;
}
bool ringbuf_is_empty(const ringbuf_t *rb) {
    return !rb || rb->count == 0;
}
bool ringbuf_is_full(const ringbuf_t *rb) {
    return rb && rb->count == rb->size;
}
size_t ringbuf_available(const ringbuf_t *rb) {
    if (!rb) return 0;
    return rb->size - rb->count;
}
size_t ringbuf_used(const ringbuf_t *rb) {
    if (!rb) return 0;
    return rb->count;
}
size_t ringbuf_capacity(const ringbuf_t *rb) {
    if (!rb) return 0;
    return rb->size;
}
bool ringbuf_peek(const ringbuf_t *rb, uint8_t *data, size_t offset) {
    if (!rb || !data || offset >= rb->count) {
        return false;
    }
    
    size_t index = (rb->tail + offset) % rb->size;
    *data = rb->buffer[index];
    
    return true;
}
size_t ringbuf_peek_bytes(const ringbuf_t *rb, uint8_t *data, size_t offset, size_t len) {
    if (!rb || !data || offset >= rb->count) {
        return 0;
    }
    
    size_t to_copy = len;
    if (offset + to_copy > rb->count) {
        to_copy = rb->count - offset;
    }
    
    for (size_t i = 0; i < to_copy; i++) {
        size_t index = (rb->tail + offset + i) % rb->size;
        data[i] = rb->buffer[index];
    }
    
    return to_copy;
}
int ringbuf_find(const ringbuf_t *rb, uint8_t value) {
    if (!rb) return -1;
    
    for (size_t i = 0; i < rb->count; i++) {
        size_t index = (rb->tail + i) % rb->size;
        if (rb->buffer[index] == value) {
            return i;  // Возвращаем смещение от начала
        }
    }
    
    return -1;  // Не найдено
}
int ringbuf_find_bytes(const ringbuf_t *rb, const uint8_t *pattern, size_t pattern_len) {
    if (!rb || !pattern || pattern_len == 0 || pattern_len > rb->count) {
        return -1;
    }
    
    for (size_t i = 0; i <= rb->count - pattern_len; i++) {
        bool found = true;
        for (size_t j = 0; j < pattern_len; j++) {
            size_t index1 = (rb->tail + i + j) % rb->size;
            if (rb->buffer[index1] != pattern[j]) {
                found = false;
                break;
            }
        }
        if (found) {
            return i;
        }
    }
    return -1;
}
bool ringbuf_push_string(ringbuf_t *rb, const char *str) {
    if (!rb || !str) {
        print_error("Ringbuf: NULL pointer in push_string\n");
        return false;
    }
    size_t len = strlen(str);
    return ringbuf_push_bytes(rb, (const uint8_t *)str, len + 1);
}
size_t ringbuf_pop_string(ringbuf_t *rb, char *buffer, size_t buffer_size) {
    if (!rb || !buffer || buffer_size == 0) {
        return 0;
    }
    int null_pos = ringbuf_find(rb, '\0');
    if (null_pos == -1) {
        return 0;  // Нет завершенной строки
    }
    size_t bytes_to_read = null_pos + 1;
    if (bytes_to_read > buffer_size) {
        bytes_to_read = buffer_size;
    }
    size_t read = ringbuf_pop_bytes(rb, (uint8_t *)buffer, bytes_to_read);
    if (read > 0 && read <= buffer_size) {
        buffer[read - 1] = '\0';
    }
    return read;
}
bool ringbuf_has_string(const ringbuf_t *rb) {
    return ringbuf_find(rb, '\0') != -1;
}
void ringbuf_print_info(const ringbuf_t *rb) {
    if (!rb) {
        print("Ringbuf: NULL pointer\n", WHITE);
        return;
    }
    print("Ring Buffer Info:\n", LIGHT_CYAN);
    print("  Size: ", WHITE); print_dec(rb->size, WHITE); print(" bytes\n", WHITE);
    print("  Used: ", WHITE); print_dec(rb->count, WHITE); print(" bytes\n", WHITE);
    print("  Free: ", WHITE); print_dec(rb->size - rb->count, WHITE); print(" bytes\n", WHITE);
    print("  Head: ", WHITE); print_dec(rb->head, WHITE); print("\n", WHITE);
    print("  Tail: ", WHITE); print_dec(rb->tail, WHITE); print("\n", WHITE);
    print("  Overwrite: ", WHITE); print(rb->overwrite ? "enabled" : "disabled", WHITE); print("\n", WHITE);
    print("  Type: ", WHITE); print(rb->is_static ? "static" : "dynamic", WHITE); print("\n", WHITE);
}