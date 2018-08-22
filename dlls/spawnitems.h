#ifndef SPAWNITEMS_H
#define SPAWNITEMS_H

struct SpawnItem
{
	const char* name;
	float value;
};

extern SpawnItem gSpawnItems[39];

int ChooseRandomSpawnItem(const int items[], int itemCount, const float* pointsLeft, const float *playerNeeds);
int CountSpawnItems(const int items[], int maxCount);

void EvaluatePlayersNeeds(float* playerNeeds);

#endif
