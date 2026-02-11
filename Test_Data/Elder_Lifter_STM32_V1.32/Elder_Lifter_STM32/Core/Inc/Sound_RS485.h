void Sound_RS485_Init(void);
void Sound_RS485_Play (uint8_t sound);
void Sound_RS485_Volume(uint8_t volume);

#define VOLUME_DEFAULT	15 //8
#define VOLUME_SOS		23	//20

#define SOUND_I_AM_READY 			1
#define SOUND_READY_START			2
#define SOUND_TRAINING_COMPLETE		3
#define SOUND_HELP					4
#define SOUND_E_STOP				5
#define SOUND_UP					6
#define SOUND_DOWN					7
#define SOUND_PAUSE					8
#define SOUND_END_POINT				9
#define SOUND_START_POINT			10
#define SOUND_HOME_POINT			10
#define SOUND_UP_FINISH				11
#define SOUND_STAND_FINISH			12
