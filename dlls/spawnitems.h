#ifndef SPAWNITEMS_H
#define SPAWNITEMS_H

struct SpawnItem
{
	const char* name;
	float value;
};

extern SpawnItem gSpawnItems[22];

int ChooseRandomSpawnItem(const int items[], int itemCount, const float* pointsLeft);
int CountSpawnItems(const int items[], int maxCount);

#endif
