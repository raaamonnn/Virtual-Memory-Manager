#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define PAGES 256
#define PAGE_SIZE 256
#define FRAME_SIZE 256

signed char main_mem[65536];
signed char* storePosition;

unsigned getpage(unsigned x) { return (0xff00 & x) >> 8; }
unsigned getoffset(unsigned x) { return (0xff & x); }

void getpage_offset(unsigned x) {
	unsigned page = getpage(x);
	unsigned offset = getoffset(x);

	printf("x is: %u, page: %u, offset:%u, address: %u, paddress: %u\n", x, page, offset,
		(page << 8) | getoffset(x), page * 256 + offset);
}

void open_files(FILE** fadd, FILE** fcorr, FILE** foutput) {
	*fadd = fopen("addresses.txt", "r");
	if (*fadd == NULL) { fprintf(stderr, "Could not open file: 'addresses.txt'\n"); exit(0); }
	*fcorr = fopen("correct.txt", "r");
	if (*fcorr == NULL) { fprintf(stderr, "Could not open file: 'correct.txt'\n"); exit(0); }
	*foutput = fopen("output.txt", "w");
	if (*foutput == NULL) { fprintf(stderr, "Could not open file: 'foutput.txt'\n"); exit(0); }
}

int main(int argc, const char* argv[])
{
	char correctBuf[256];
	unsigned   virt_add, phys_add, correctValue;  // read from file correct.txt
	int tables[PAGES];

	for (int i = 0; i < PAGES; i++) { tables[i] = -2; }

	int access_count = 0, pageFault = 0;
	const char* file_name = "BACKING_STORE.bin";

	int backStoreFile = open(file_name, O_RDONLY);
	storePosition = mmap(0, 65536, PROT_READ, MAP_PRIVATE, backStoreFile, 0);

	FILE* fadd, * fcorr, * foutput;
	open_files(&fadd, &fcorr, &foutput);

	int logical_addr;
	unsigned char freePage = 0;
	while (fscanf(fadd, "%d", &logical_addr) != EOF)
	{
		fscanf(fcorr, "%s %s %d %s %s %d %s %d", correctBuf, correctBuf, &virt_add,
			correctBuf, correctBuf, &phys_add, correctBuf, &correctValue);

		unsigned page = getpage(logical_addr);
		int offset = getoffset(logical_addr);
		int frame = tables[page];
		access_count++;

		if (frame == -2) {
			pageFault++;

			frame = freePage;
			freePage++;

			memcpy(main_mem + frame * PAGE_SIZE, storePosition + page * PAGE_SIZE, PAGE_SIZE);

			tables[page] = frame;
		}

		int physicall_addr = (frame << 8) | offset;

		int physical_add = frame * FRAME_SIZE + offset;
		signed char value = main_mem[physical_add];

		printf("logical: %5u (page: %3u, offset: %3u) ---> physical: %5u -> value: %4d \n", logical_addr, page, offset,
			physical_add, value);
		fprintf(foutput, "Logical address: %d physicall address: %d Value: %d\n", logical_addr, physicall_addr, value);

		assert(value == correctValue);
		assert(phys_add == physical_add);
	}

	printf("Addresses Accessed = %d\n", access_count);
	printf("Page fault cout->%d rate->%.1f\n", pageFault, (pageFault / (access_count * 1.)) * 100);
	printf("Free Pages->%d\n", freePage);
	return 0;
}