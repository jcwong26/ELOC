// Pin defs
#define CURR_SENSE_PIN 5
#define V_INV_SENSE_PIN 6

#define V_PER_V_INV 20
#define R_SHUNT_OHMS 0.01

#define V_24V_DIV_RATIO 0.0507614
#define MAX_ADC_OUTPUT 4095

#define V_TO_I_SHUNT(volts) ((volts/V_PER_V_INV)/R_SHUNT_OHMS)

void init_adcs(void);

void current_monitoring_task(void*);
void supply_voltage_monitor_task(void*);