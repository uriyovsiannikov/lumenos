#include "event.h"
#include "../../libs/print.h"
#include "../../libs/string.h"
#include "../../modules/syslogger/syslogger.h"
#include "../../modules/timer/timer.h"
#include <stddef.h>
typedef struct {
    event_t events[EVENT_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    int pending_immediate;
} event_queue_t;
static event_queue_t event_queue;
static event_subscription_t *subscriptions = NULL;
static int system_initialized = 0;
static uint32_t event_counter = 0;
void event_system_init(void) {
    print("Initializing event system...", WHITE);
    if (system_initialized) return;
    event_queue.head = 0;
    event_queue.tail = 0;
    event_queue.count = 0;
    event_queue.pending_immediate = 0;
    subscriptions = NULL;
    system_initialized = 1;
    print(" [OK]", GREEN);
    log_message("Event system initialized", LOG_INFO);
}
void event_system_shutdown(void) {
    if (!system_initialized) return;
    event_queue.head = event_queue.tail = event_queue.count = 0;
    event_subscription_t *sub = subscriptions;
    while (sub) {
        event_subscription_t *next = sub->next;
        free(sub);
        sub = next;
    }
    subscriptions = NULL;
    
    system_initialized = 0;
    log_message("Event system shutdown", LOG_INFO);
}
event_t *event_create(event_type_t type, void *sender, void *data, uint32_t data_size) {
    event_t *event = (event_t *)malloc(sizeof(event_t));
    if (!event) return NULL;
    event->type = type;
    event->timestamp = get_ticks();
    event->sender = sender;
    event->data_size = data_size;
    if (data && data_size > 0) {
        event->data = malloc(data_size);
        if (event->data) {
            memcpy(event->data, data, data_size);
        } else {
            event->data = NULL;
            event->data_size = 0;
        }
    } else {
        event->data = NULL;
        event->data_size = 0;
    }
    return event;
}
void event_destroy(event_t *event) {
    if (!event) return;
    
    if (event->data) {
        free(event->data);
    }
    free(event);
}
static int event_enqueue(event_t *event) {
    if (!system_initialized || !event) return 0;
    if (event_queue.count >= EVENT_QUEUE_SIZE) {
        log_message("Event queue overflow", LOG_WARNING);
        event_destroy(event);
        return 0;
    }
    event_queue.events[event_queue.tail] = *event;
    event_queue.tail = (event_queue.tail + 1) % EVENT_QUEUE_SIZE;
    event_queue.count++;
    event_counter++;
    free(event);
    return 1;
}
static event_t *event_dequeue(void) {
    if (!system_initialized || event_queue.count == 0) {
        return NULL;
    }
    event_t *event = (event_t *)malloc(sizeof(event_t));
    if (!event) return NULL;
    *event = event_queue.events[event_queue.head];
    event_queue.head = (event_queue.head + 1) % EVENT_QUEUE_SIZE;
    event_queue.count--;
    return event;
}
int event_post(event_type_t type, void *sender, void *data, uint32_t data_size) {
    if (!system_initialized) return 0;
    event_t *event = event_create(type, sender, data, data_size);
    if (!event) return 0;
    return event_enqueue(event);
}
int event_post_immediate(event_type_t type, void *sender, void *data, uint32_t data_size) {
    if (!system_initialized) return 0;
    event_t *event = event_create(type, sender, data, data_size);
    if (!event) return 0;
    event_subscription_t *sub = subscriptions;
    while (sub) {
        if (sub->type == type || sub->type == EVENT_NONE) {
            sub->handler(event);
        }
        sub = sub->next;
    }
    event_destroy(event);
    return 1;
}
void event_subscribe(event_type_t type, event_handler_t handler, void *user_data) {
    if (!system_initialized || !handler) return;
    event_subscription_t *sub = (event_subscription_t *)malloc(sizeof(event_subscription_t));
    if (!sub) return;
    sub->type = type;
    sub->handler = handler;
    sub->user_data = user_data;
    sub->next = subscriptions;
    subscriptions = sub;
    char log_msg[64];
    snprintf(log_msg, sizeof(log_msg), "Subscribed to event type %d", type);
    log_message(log_msg, LOG_INFO);
}
void event_unsubscribe(event_type_t type, event_handler_t handler) {
    if (!system_initialized || !handler) return;
    event_subscription_t *prev = NULL;
    event_subscription_t *current = subscriptions;
    while (current) {
        if (current->type == type && current->handler == handler) {
            if (prev) {
                prev->next = current->next;
            } else {
                subscriptions = current->next;
            }
            free(current);
            char log_msg[64];
            snprintf(log_msg, sizeof(log_msg), "Unsubscribed from event type %d", type);
            log_message(log_msg, LOG_INFO);
            return;
        }
        prev = current;
        current = current->next;
    }
}
void event_process(void) {
    if (!system_initialized) return;
    while (event_queue.count > 0) {
        event_t *event = event_dequeue();
        if (!event) break;
        event_subscription_t *sub = subscriptions;
        while (sub) {
            if (sub->type == event->type || sub->type == EVENT_NONE) {
                sub->handler(event);
            }
            sub = sub->next;
        }
        event_destroy(event);
    }
    event_queue.pending_immediate = 0;
}
int event_pending(void) {
    return system_initialized ? event_queue.count : 0;
}
const char *event_type_to_string(event_type_t type) {
    switch (type) {
        case EVENT_NONE:           return "EVENT_NONE";
        case EVENT_SYSTEM_START:   return "EVENT_SYSTEM_START";
        case EVENT_SYSTEM_SHUTDOWN:return "EVENT_SYSTEM_SHUTDOWN";
        case EVENT_SYSTEM_TICK:    return "EVENT_SYSTEM_TICK";
        case EVENT_KEY_PRESS:      return "EVENT_KEY_PRESS";
        case EVENT_KEY_RELEASE:    return "EVENT_KEY_RELEASE";
        case EVENT_MOUSE_MOVE:     return "EVENT_MOUSE_MOVE";
        case EVENT_MOUSE_CLICK:    return "EVENT_MOUSE_CLICK";
        case EVENT_TIMER_EXPIRED:  return "EVENT_TIMER_EXPIRED";
        case EVENT_FILE_OPEN:      return "EVENT_FILE_OPEN";
        case EVENT_FILE_CLOSE:     return "EVENT_FILE_CLOSE";
        case EVENT_FILE_WRITE:     return "EVENT_FILE_WRITE";
        case EVENT_FILE_READ:      return "EVENT_FILE_READ";
        case EVENT_PROCESS_CREATE: return "EVENT_PROCESS_CREATE";
        case EVENT_PROCESS_EXIT:   return "EVENT_PROCESS_EXIT";
        case EVENT_PROCESS_SWITCH: return "EVENT_PROCESS_SWITCH";
        default:                   return "EVENT_UNKNOWN";
    }
}
static void event_logger_handler(event_t *event) {
    if (!event) return;
    char log_msg[128];
    const char *type_str = event_type_to_string(event->type);
    snprintf(log_msg, sizeof(log_msg), "Event: %s (timestamp: %u)", 
             type_str, event->timestamp);
    log_message(log_msg, LOG_INFO);
}
void event_test(void) {
    print("=== Event System Test ===\n", WHITE);
    event_subscribe(EVENT_NONE, event_logger_handler, NULL);
    print("Test 1: Posting events...\n", WHITE);
    EVENT_POST(EVENT_SYSTEM_TICK, NULL, NULL);
    EVENT_POST(EVENT_KEY_PRESS, NULL, NULL);
    EVENT_POST(EVENT_FILE_OPEN, NULL, NULL);
    print("Events in queue: ", WHITE);
    print_dec(event_pending(), CYAN);
    print("\n", WHITE);
    print("Test 2: Posting event with data...\n", WHITE);
    char test_data[] = "Hello Event System!";
    EVENT_POST_SIZE(EVENT_USER_CUSTOM, NULL, test_data, sizeof(test_data));
    print("Test 3: Immediate event...\n", WHITE);
    event_post_immediate(EVENT_SYSTEM_TICK, NULL, NULL, 0);
    print("Test 4: Processing event queue...\n", WHITE);
    event_process();
    print("Events remaining: ", WHITE);
    print_dec(event_pending(), CYAN);
    print("\n", WHITE);
    event_unsubscribe(EVENT_NONE, event_logger_handler);
    print("=== Event System Test Complete ===\n\n", WHITE);
}