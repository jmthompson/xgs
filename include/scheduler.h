extern long cpu_cycle_count;

extern int schedulerInit(void);
extern void schedulerStart(void);
extern void schedulerStop(void);
extern long schedulerGetTime(void);
extern float schedulerGetActualSpeed(void);
extern float schedulerGetTargetSpeed(void);
extern void schedulerSetTargetSpeed(float);
extern void schedulerTick(void);
