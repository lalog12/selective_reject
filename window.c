#include "window.h"



int process_packet_buffer(bManager *manager, )
{

    switch (manager -> current_state)
    {
    case IN_ORDER_STATE:
        inorderStateActive(manager);
        break;
    
    case BUFFERING_STATE:
        bufferingStateActive(manager);
        break;

    case FLUSHING_STATE:
        flushing_StateActive(manager);
        break;
    }
}
