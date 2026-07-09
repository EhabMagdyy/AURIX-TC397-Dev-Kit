#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"

// Shared memory between cores in DSPR
extern volatile uint8  g_cpu0_done;
extern volatile uint32 g_cpu1_result;
extern volatile uint8  g_cpu1_done;
extern IfxCpu_syncEvent cpuSyncEvent;

// The "critical" computation both cores must run identically
static uint32 criticalComputation(uint32 input){
    uint32 result = 0;
    for(uint32 i = 0; i < input; i++)
        result += i * 3;
    return result;
}

void core1_main(void){
    IfxCpu_enableInterrupts();
    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());

    // Sync with CPU0 ONCE before starting
    IfxCpu_emitEvent(&cpuSyncEvent);
    IfxCpu_waitEvent(&cpuSyncEvent, 1);

    while(1){
        // Wait for CPU0 to start its computation first
        while(g_cpu0_done == 0);

        // CPU1 runs the SAME computation independently
        g_cpu1_result = criticalComputation(100);
        g_cpu1_done   = 1;

        // Wait for CPU0 to reset flags before next iteration
        while(g_cpu0_done == 1);
    }
}
