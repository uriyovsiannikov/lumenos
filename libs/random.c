#define RAND_MAX 32767
static unsigned long next = 1;
int rand(void) {
    next = next * 1103515245 + 12345;
    return (unsigned int)(next / 65536) % (RAND_MAX + 1);
}
void srand(unsigned int seed) {
    next = seed;
}
int rand_range(int min, int max) {
    if (min > max) {
        int temp = min;
        min = max;
        max = temp;
    }
    return min + (rand() % (max - min + 1));
}
float rand_float(void) {
    return (float)rand() / (float)RAND_MAX;
}
float rand_float_range(float min, float max) {
    if (min > max) {
        float temp = min;
        min = max;
        max = temp;
    }
    return min + (rand_float() * (max - min));
}