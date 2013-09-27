#include <app/message/system_message.h>
#include <message.h>
#include <panic.h>
#include <stdlib.h>

#include "headset_private.h"
#include "test_headset.h"
#include "test_utils.h"

static TaskData testTask;

/* Register the test task  */
void test_init(void) {
    testTask.handler = handle_msg_from_host;
    if (MessageHostCommsTask(&testTask)) {Panic();}
}

/**************************************************
   VM2HOST
 **************************************************/

/* HS State notification */
void vm2host_send_state(headsetState state) {
    HS_STATE_T message;
    message.state = state;
    test_send_message(HS_STATE, (Message)&message, sizeof(HS_STATE_T), 0, NULL);
}

/* HS Event Notification */
void vm2host_send_event(headsetEvents_t event) {
    HS_EVENT_T message;
    message.event = event;
    test_send_message(HS_EVENT, (Message)&message, sizeof(HS_EVENT_T), 0, NULL);
}

/**************************************************
   HOST2VM
 **************************************************/

/* HS host messages handler */
void handle_msg_from_host(Task task, MessageId id, Message message) {
    HEADSET_FROM_HOST_MSG_T *tmsg = (HEADSET_FROM_HOST_MSG_T *)message;

    switch (tmsg->funcId) {
        case HEADSET_EVENT_MSG:
            MessageSend(
                &theHeadset.task,
                tmsg->headset_from_host_msg.headset_event_msg.event,
                NULL
            );
            break;
    }
}
