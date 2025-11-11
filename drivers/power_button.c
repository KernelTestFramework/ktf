#include <acpi_ktf.h>
#include <drivers/power_button.h>

static void default_handler(void *);

/** power button press handler */
static pb_handler_t pb_handler = default_handler;
/** context passed to the power button press handler */
static void *pb_context = NULL;

#ifdef KTF_ACPICA
static UINT32 button_handler(void *Context) {
    if (ACPI_FAILURE(AcpiClearEvent(ACPI_EVENT_POWER_BUTTON)))
        panic("PWRB: Failed to clear power button event");

    if (pb_handler)
        pb_handler(pb_context);

    return ACPI_INTERRUPT_HANDLED;
}
#endif

static void default_handler(void *notused) {
#ifdef KTF_ACPICA
    acpi_power_off();
#endif
}

void pb_set_handler(pb_handler_t handler, void *context) {
    unsigned long flags = interrupts_disable_save();
    pb_handler = handler;
    pb_context = context;
    interrupts_restore(flags);
}

bool init_power_button() {
#ifdef KTF_ACPICA
    ACPI_TABLE_FADT *fadt = acpi_find_table(ACPI_SIG_FADT);

    if (!(fadt->Flags & ACPI_FADT_POWER_BUTTON)) {
        printk("PWRB: Configuring ACPI 'fixed' power button handling\n");

        if (ACPI_FAILURE(AcpiClearEvent(ACPI_EVENT_POWER_BUTTON))) {
            warning("PWRB: Failed to clear power button event");
            return false;
        }

        if (ACPI_FAILURE(AcpiInstallFixedEventHandler(ACPI_EVENT_POWER_BUTTON,
                                                      button_handler, NULL))) {
            warning("PWRB: Failed to install power button handler");
            return false;
        }
    }
    else {
        warning("PWRB: Non-fixed power button not implemented\n");
        return false;
    }

    return true;
#else
    warning("PWRB: Power button without ACPICA not implemented\n");

    return false;
#endif
}
