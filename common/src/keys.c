/*
  vim:ts=4
  vim:sw=4
*/
#include "common.h"

uint8_t keystates[NUMKEYSTATES];

// clear keyboard buffer
void clear_keys()
{
    struct keyboard_event_t e;
    while (kbuf_poll_event(&e)) {}
    clearKeyStates();
}
// return after specific key is pressed
uint8_t wait_for_key(uint8_t key)
{
    struct keyboard_event_t e;
    do {
        while (!kbuf_poll_event(&e)) {}
    } while (e.vkey != key);
    return key;
}
// return after specific key is released
uint8_t wait_for_key_up(uint8_t key)
{
    struct keyboard_event_t e;
    do {
        while (!kbuf_poll_event(&e)) {}
    } while (e.vkey != key && e.isdown);
    return key;
}
// return after key is released
uint8_t wait_for_any_key()
{
    struct keyboard_event_t e;
    int gotkey = 0;
    do {
        int ret = kbuf_poll_event(&e);
        if (ret && !e.isdown) gotkey = 1;
    } while ( gotkey == 0 );

    return e.vkey;
}
// return after key-down event
uint8_t wait_for_any_key_press()
{
    struct keyboard_event_t e;
    int gotkey = 0;
    do {
        int ret = kbuf_poll_event(&e);
        if (ret && e.isdown) gotkey = 1;
    } while ( gotkey == 0 );

    return e.vkey;
}

void clearKeyStates()
{
    memset(keystates, 0, NUMKEYSTATES);
    //for (int k=0;k<128;k++) keystates[k] = false;
}

uint8_t doKeyPoll()
{
    struct keyboard_event_t e;
    if (kbuf_poll_event(&e))
    {
        keystates[e.vkey] = e.isdown;
        return e.vkey;
    }
    return 0;
}

// repeat key poll, will include case where key is released
uint8_t wait_for_keypoll()
{
    uint8_t vkey = 0;
    do {
        vkey = doKeyPoll();
    } while ( vkey == 0 );
    return vkey;
}

uint8_t getKeyState(uint8_t vkey)
{
    return keystates[vkey];
}
