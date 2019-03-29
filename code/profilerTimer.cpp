#pragma once 

#define TIMER_INFO_SIZE 100

struct TimerInfo {
	bool initialised;
	bool stringsSaved;

	char* file;
	char* function;
	char* name;
	int line, line2;
	uint type;

	Vec3 color;
};

enum TimerType {
	TIMER_TYPE_BEGIN,
	TIMER_TYPE_END,
};

#pragma pack(push,1)
struct TimerSlot {
	char type;
	uint threadId;
	char timerIndex;

	u64 cycles;
};
#pragma pack(pop)

struct ProfilerTimer {
	TimerInfo* timerInfos;
	int timerInfoCount;
	int timerInfoSize;

	TimerSlot* timerBuffer;
	int bufferIndex;
	int bufferSize;
	
	int lastBufferIndex;

	void init(int bufferSize);
	void addSlot(int infoIndex, int type);
	void addSlotAndInfo(int infoIndex, int type, char* file, char* function, int line, char* name = "");
	void update();
};

void ProfilerTimer::init(int bufferSize) {
	this->timerInfoCount = TIMER_INFO_SIZE;
	timerInfos = getPArray(TimerInfo, TIMER_INFO_SIZE);

	this->bufferSize = bufferSize;
	timerBuffer = getPArray(TimerSlot, bufferSize);
}

void ProfilerTimer::addSlot(int infoIndex, int type) {
	// -10 so we have some room to patch things up at the end
	// if we run out of space.
	if(bufferIndex > bufferSize-10) return; 

	int slotIndex = InterlockedIncrement((LONG*)(&bufferIndex));
	slotIndex--;
	TimerSlot* slot = timerBuffer + slotIndex;
	slot->cycles = __rdtsc();
	slot->type = type;
	slot->threadId = GetCurrentThreadId();
	slot->timerIndex = infoIndex;
}

void ProfilerTimer::addSlotAndInfo(int infoIndex, int type, char* file, char* function, int line, char* name) {

	TimerInfo* info = timerInfos + infoIndex;
	if(!info->initialised) {
		info->initialised = true;
		info->file = file;
		info->function = function;
		info->line = line;
		info->type = type;
		info->name = name;
	}

	addSlot(infoIndex, type);
}

void ProfilerTimer::update() {
	for(int i = 0; i < timerInfoCount; i++) {
		TimerInfo* info = timerInfos + i;

		// Set colors.
		float ss = i%(timerInfoCount/2) / ((float)timerInfoCount/2);
		float h = i < timerInfoCount/2 ? 0.1f : -0.1f;
		info->color = hslToRgb(360*ss, 0.5f, 0.5f+h);

		if(!info->initialised || info->stringsSaved) continue;
		
		info->file = getPString(info->file);
		info->function = getPString(info->function);
		info->name = getPString(info->name);

		info->stringsSaved = true;
	}
}

extern ProfilerTimer* theTimer;

//

void addTimerSlot(int infoIndex, int type) { return theTimer->addSlot(infoIndex, type); }
void addTimerSlotAndInfo(int index, int type, char* file, char* function, int line, char* name = "") {
	return theTimer->addSlotAndInfo(index, type, file, function, line, name);
}

struct TimerBlock {
	int codeIndex;

	TimerBlock(int codeIndex, char* file, char* function, int line, char* name = "") {
		this->codeIndex = codeIndex;
		addTimerSlotAndInfo(codeIndex, TIMER_TYPE_BEGIN, file, function, line, name);
	}

	~TimerBlock() {
		addTimerSlot(codeIndex, TIMER_TYPE_END);
	}
};

#define TIMER_BLOCK() \
	TimerBlock timerBlock##__LINE__(__COUNTER__, __FILE__, __FUNCTION__, __LINE__);

#define TIMER_BLOCK_NAMED(name) \
	TimerBlock timerBlock##__LINE__(__COUNTER__, __FILE__, __FUNCTION__, __LINE__, name);

#define TIMER_BLOCK_BEGIN(ID) \
	const int timerCounter##ID = __COUNTER__; \
	addTimerSlotAndInfo(timerCounter##ID, TIMER_TYPE_BEGIN, __FILE__, __FUNCTION__, __LINE__); 

#define TIMER_BLOCK_BEGIN_NAMED(ID, name) \
	const int timerCounter##ID = __COUNTER__; \
	addTimerSlotAndInfo(timerCounter##ID, TIMER_TYPE_BEGIN, __FILE__, __FUNCTION__, __LINE__, name); 

#define TIMER_BLOCK_END(ID) \
	addTimerSlot(timerCounter##ID, TIMER_TYPE_END);
