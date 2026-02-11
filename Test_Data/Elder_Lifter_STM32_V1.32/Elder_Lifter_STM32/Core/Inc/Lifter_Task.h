// Definition ---------------------------------------------------------
#define MAX_WAYPOINT	10
#define WAYPOINT_SIZE	7

#define RESET_STATE				0
#define SEARCH_WAYPOINT_STATE 	1
#define IDLE_STATE				2
#define RUNNING_STATE			3
#define START_STATE				4
#define FINISH_STATE			5
#define MOVE_UP_STATE			6
#define MOVE_DOWN_STATE			7
#define PAUSE_STATE				8
#define UP_RUNNING_STATE		9
#define DOWN_RUNNING_STATE		10
#define UP_PAUSE_STATE			11
#define DOWN_PAUSE_STATE		12

#define REPEAT_RUN_STATE			20
#define REPEAT_HOME_STATE			21
#define REPEAT_TO_HOME_STATE		22
#define REPEAT_HOME_RUNNING_STATE	23
#define REPEAT_HOME_PAUSE_STATE		24
#define REPEAT_HOME_FINISH_STATE	25
#define REPEAT_UP_STATE				26
#define REPEAT_UP_RUNNING_STATE		27
#define REPEAT_UP_PAUSE_STATE		28
#define REPEAT_UP_FINISH_STATE		29
#define REPEAT_DOWN_STATE			30
#define REPEAT_DOWN_RUNNING_STATE	31
#define REPEAT_DOWN_PAUSE_STATE		32
#define REPEAT_DOWN_FINISH_STATE	33
#define REPEAT_STAND_STATE			34
#define REPEAT_STAND_WAITING_STATE	35
#define REPEAT_STAND_FINISH_STATE	36
#define REPEAT_FINISH_STATE			37

#define MOVE_HOME_STATE				40
#define MOVE_HOME_RUNNING_STATE		41
#define MOVE_HOME_PAUSE_STATE		42
#define MOVE_HOME_FINISH_STATE		43


typedef struct
{
	uint8_t id;
	uint8_t target_speed;
	uint16_t target_V;
	uint16_t target_H;
	uint16_t target_T;
} Waypoint_TypeDef;

typedef struct
{
	uint8_t state;
	uint8_t	pointer;
	uint8_t buffer[MAX_WAYPOINT*WAYPOINT_SIZE+10];
	uint8_t total_waypoint;
	Waypoint_TypeDef waypoint;
	uint8_t up_speed;
	uint8_t down_speed;
	uint8_t last_action;
	uint8_t last_waypoint;
	uint8_t repeat_remain_times;
	uint8_t repeat_flag;
	uint16_t repeat_wait_timer;
	uint8_t repeat_stand_time;
	uint8_t volume;
} Task_TypeDef;

void Task_Init(void);
void Task_Load_Job(void);
Waypoint_TypeDef Task_Search_Waypoint(uint16_t vertical_position, uint8_t dir);
uint8_t Task_Execute_Handler(void);
void Task_Move_Up(void);
void Task_Move_Down(void);
void Task_Move_Stop(void);
void Task_Clear_Waypoint(void);
void Task_Push_Waypoint(uint8_t speed, uint16_t vertical, uint16_t horizontal, uint16_t tilt);
void Task_Set_Speed(uint8_t up_speed, uint8_t down_speed);
uint8_t Task_Up_Speed(void);
uint8_t Task_Down_Speed(void);
void Task_Repeat_Timer_Handler(void);
void Task_Repeat(void);
void Task_Repeat_Pause(void);
void Task_Set_Repeat(uint8_t repeat);
void Task_SOS(void);
void Task_Move_Home(void);
uint8_t Task_Get_Remain_Repeat_Times(void);
void Task_Set_StandTime(uint8_t time);
void Task_Set_Volume(uint8_t vol);


