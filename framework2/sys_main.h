#define SYS_MAIN_CYCLE (5)

struct sys_timer {
	uint64_t u64_time;
	bool bl_state;
};

extern void sys_call_timer_start(struct sys_timer *);
extern void sys_call_timer_stop(struct sys_timer *);
extern bool sys_call_timer_check(struct sys_timer *, uint64_t);
extern uint64_t sys_call_timer_diff(struct sys_timer *, uint64_t);
extern bool sys_call_timer_isrun(struct sys_timer *);
extern void sys_call_disable_interrupts();
extern void sys_call_enable_interrupts();
extern void sys_call_sleep_request();
