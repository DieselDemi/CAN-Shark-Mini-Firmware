#include <stdio.h>
#include "can_bus.h"
#include "comms.h"

void app_main(void)
{
    
    can_bus_init(get_settings());

    while(can_bus_update())
    {
        //Give the CPU a chance to do other things (5ms); 
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    can_bus_cleanup();
}
