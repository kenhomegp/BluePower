#ifndef _TEST_HEADSET_H_
#define _TEST_HEADSET_H_

#include <message.h>
#include "headset_states.h"
#include "headset_events.h"

/* Register the main task  */
void test_init(void);

#define HS_MESSAGE_BASE   0x2000

/**************************************************
   VM2HOST
 **************************************************/
typedef enum {
    HS_STATE = HS_MESSAGE_BASE,
    HS_EVENT
} vm2host_headset;

typedef struct {
    uint16 state;    /*!< The HeadSet state. */
} HS_STATE_T;

typedef struct {
    uint16 event;   /*!< The HeadSet event. */
} HS_EVENT_T;

/* HS State notification */
void vm2host_send_state(headsetState state);

/* HS Event notification */
void vm2host_send_event(headsetEvents_t event);

/**************************************************
   HOST2VM
 **************************************************/
typedef enum {
    HEADSET_EVENT_MSG = HS_MESSAGE_BASE + 0x80
} host2vm_headset;

typedef struct {
    uint16 event;
} HEADSET_EVENT_MSG_T;

typedef struct {
    uint16 length;
    uint16 bcspType;
    uint16 funcId;

    union {
        HEADSET_EVENT_MSG_T headset_event_msg;
    } headset_from_host_msg;
} HEADSET_FROM_HOST_MSG_T;

/* HS host messages handler */
void handle_msg_from_host(Task task, MessageId id, Message message);

#endif
