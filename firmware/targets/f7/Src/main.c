#include <furi.h>
#include <furi_hal.h>
#include <flipper.h>
#include <alt_boot.h>
#include <semphr.h>
#include <update_util/update_operation.h>

#define TAG "Main"

static const osThreadAttr_t init_thread_attr = {
    .name = "Init",
    .stack_size = 4096,
};

void init_task() {
    // Flipper FURI HAL
    furi_hal_init();

    // Init flipper
    flipper_init();

    osThreadExit();
}

int main() {
    // Initialize FURI layer
    furi_init();

    // Flipper critical FURI HAL
    furi_hal_init_early();

#ifdef FURI_RAM_EXEC
    osThreadNew(init_task, NULL, &init_thread_attr);
#else
    furi_hal_light_sequence("RGB");

    // Delay is for button sampling
    furi_hal_delay_ms(100);

    FuriHalRtcBootMode boot_mode = furi_hal_rtc_get_boot_mode();
    if(boot_mode == FuriHalRtcBootModeDfu || !furi_hal_gpio_read(&gpio_button_left)) {
        furi_hal_light_sequence("rgb WB");
        furi_hal_rtc_set_boot_mode(FuriHalRtcBootModeNormal);
        flipper_boot_dfu_exec();
        furi_hal_power_reset();
    } else if(boot_mode == FuriHalRtcBootModeUpdate) {
        furi_hal_light_sequence("rgb BR");
        flipper_boot_update_exec();
        // if things go nice, we shouldn't reach this point.
        // But if we do, abandon to avoid bootloops
        update_operation_disarm();
        furi_hal_power_reset();
    } else {
        furi_hal_light_sequence("rgb G");
        osThreadNew(init_task, NULL, &init_thread_attr);
    }
#endif

    // Run Kernel
    furi_run();

    furi_crash("Kernel is Dead");
}

void Error_Handler(void) {
    furi_crash("ErrorHandler");
}

void abort() {
    furi_crash("AbortHandler");
}
