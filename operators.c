#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

int main() {
	// 0  = 00000000
	// 1  = 00000001
	// 1  = 00000010
	// 2  = 00000011
	// 3  = 00000100
	// 4  = 00000101
	// 5  = 00000111
	// 6  = 00001000
	// 7  = 00001001
	// 8  = 00001010
	// 32 = 10000000
	//
	//

	uint32_t a = 32 >> 3; // == 32 / 2^3 == 32 / 8
	// 32 / 8

	printf("%u", a);

	return 0;
}

int uint32_overflow() {
	uint32_t n = 4294957294;

	int max = 0;

	while(true) {
		if (n > max) {
			max = n;
		}
		printf("\r\033[K%u", n);
		fflush(stdout);
		n++;

		if (max < n) {
			printf("\r\033[KMax found!!! %u %u", n, max);
			break;
		}
	}

	return 0;
}

