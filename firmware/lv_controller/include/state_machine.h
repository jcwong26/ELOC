#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

// void state_machine(void);
int to_loading(void);
int to_closed(void);
int to_charging(void);
int to_unlocked(void);
int to_unloading(void);
int to_empty(void);

#endif