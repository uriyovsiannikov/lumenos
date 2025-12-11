#ifndef RANDOM_H
#define RANDOM_H
void srand(unsigned int seed);
int rand(void);
int rand_range(int min, int max);
float rand_float(void);
float rand_float_range(float min, float max);
#endif