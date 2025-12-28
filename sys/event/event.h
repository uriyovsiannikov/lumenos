#ifndef EVENT_H
#define EVENT_H
#include <stdint.h>
#include <stddef.h>
#define EVENT_QUEUE_SIZE 64
typedef enum {
    EVENT_NONE = 0,
    EVENT_SYSTEM_START,
    EVENT_SYSTEM_SHUTDOWN,
    EVENT_SYSTEM_TICK,
    EVENT_KEY_PRESS,
    EVENT_KEY_RELEASE,
    EVENT_MOUSE_MOVE,
    EVENT_MOUSE_CLICK,
    EVENT_TIMER_EXPIRED,
    EVENT_FILE_OPEN,
    EVENT_FILE_CLOSE,
    EVENT_FILE_WRITE,
    EVENT_FILE_READ,
    EVENT_PROCESS_CREATE,
    EVENT_PROCESS_EXIT,
    EVENT_PROCESS_SWITCH,
    EVENT_USER_CUSTOM = 1000
} event_type_t;
typedef struct {
    event_type_t type;
    uint32_t timestamp;
    void *sender;
    void *data;
    uint32_t data_size;
} event_t;
typedef void (*event_handler_t)(event_t *event);
typedef struct event_subscription {
    event_type_t type;
    event_handler_t handler;
    void *user_data;
    struct event_subscription *next;
} event_subscription_t;
void event_system_init(void);
void event_system_shutdown(void);
int event_post(event_type_t type, void *sender, void *data, uint32_t data_size);
int event_post_immediate(event_type_t type, void *sender, void *data, uint32_t data_size);
void event_subscribe(event_type_t type, event_handler_t handler, void *user_data);
void event_unsubscribe(event_type_t type, event_handler_t handler);
void event_process(void);
int event_pending(void);
event_t *event_create(event_type_t type, void *sender, void *data, uint32_t data_size);
void event_destroy(event_t *event);
const char *event_type_to_string(event_type_t type);
#define EVENT_POST(type, sender, data) event_post((type), (sender), (data), 0)
#define EVENT_POST_SIZE(type, sender, data, size) event_post((type), (sender), (data), (size))
#define EVENT_SUBSCRIBE(type, handler) event_subscribe((type), (handler), NULL)
#define EVENT_SUBSCRIBE_DATA(type, handler, userdata) event_subscribe((type), (handler), (userdata))
#endif // EVENT_H