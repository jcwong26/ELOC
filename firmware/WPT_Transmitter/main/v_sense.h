// Pin defs
#define CURR_SENSE_PIN 5
#define V_INV_SENSE_PIN 6

#define V_PER_V_INV 20
#define R_SHUNT_OHMS 0.01

#define V_24V_DIV_RATIO 0.0507614
#define MAX_ADC_OUTPUT 4095

#define V_TO_I_SHUNT(volts) ((volts/V_PER_V_INV)/R_SHUNT_OHMS)
#define CONVERT_V24(volts) (volts/V_24V_DIV_RATIO)

#define ADC_CONV_MODE           ADC_CONV_SINGLE_UNIT_1
#define ADC_OUTPUT_TYPE         ADC_DIGI_OUTPUT_FORMAT_TYPE1

#define V_24V_CHANNEL ADC_CHANNEL_5
#define I_SENSE_CHANNEL ADC_CHANNEL_4

void init_adcs(void);

void current_monitoring_task(void*);
void supply_voltage_monitor_task(void*);