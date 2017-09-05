#ifndef _UDPCONTROL_H
#define _UDPCONTROL_H
#include <stdint.h>
int udpcontrol_setup(void);
int udp_send_state(int state, uint32_t offset);
int udp_receive_state(int * state, uint32_t * offset);
int get_ip(void);
#endif
