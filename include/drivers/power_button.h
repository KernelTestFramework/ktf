#ifndef KTF_POWER_BUTTON_H
#define KTF_POWER_BUTTON_H

typedef void (*pb_handler_t)(void *);

/**
 * Set a handler for the power button. Useful to control experiments.
 *
 * Note: This function MUST be called from the bsp to ensure consistency!
 */
void pb_set_handler(pb_handler_t handler, void *context);

/**
 * Initialize power button handling. Requires ACPICA library.
 */
extern bool init_power_button();

#endif /* KTF_POWER_BUTTON_H */
