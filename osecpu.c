#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define INSTRUCTION_LIMM 0x76000002
#define INSTRUCTION_OR   0x76000010
#define INSTRUCTION_XOR  0x76000011
#define INSTRUCTION_AND  0x76000012
#define INSTRUCTION_LIDR 0x760000fd

struct Osecpu
{
	int registers[0x40];
	int pregisters[0x40];
	int dregisters[4];
	uint8_t* code;
	long codelen;

	int invalid_instruction_error;
	int invalid_argument_error;
};

int load_b32(struct Osecpu* osecpu, const char* filename)
{
	FILE* fp;
	long len;
	uint8_t* code;

	if (osecpu->code) return -1;

	fp = fopen(filename, "rb");
	if (!fp) return -1;

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// ファイルの長さが8バイト以下(シグネチャ長以下)はエラー
	if (len <= 8) {
		fclose(fp);
		return -1;
	}

	code = (uint8_t*)malloc(len);
	if (!code) {
		fclose(fp);
		return -1;
	}

	if (fread(code, len, 1, fp) < 1) {
		free(code);
		fclose(fp);
		return -1;
	}

	fclose(fp);

	// シグネチャチェック
	if (strcmp(code, "\x05\xe2\x00\xcf\xee\x7f\xf1\x88") != 0) {
		free(code);
		return -1;
	}

	osecpu->code = (uint8_t*)malloc(len);
	if (!osecpu->code) {
		free(code);
		return -1;
	}

	osecpu->codelen = len-8;
	memcpy(osecpu->code, code+8, osecpu->codelen);

	free(code);
	return 0;
}

void coredump(struct Osecpu* osecpu)
{
	int i, j;

	printf("Registers:\n");
	i = 0;
	while (i < 0x40/4) {
		printf("R%02X: %08X\tR%02X: %08X\tR%02X: %08X\tR%02X: %08X\n"
				, i+16*0, osecpu->registers[i+16*0]
				, i+16*1, osecpu->registers[i+16*1]
				, i+16*2, osecpu->registers[i+16*2]
				, i+16*3, osecpu->registers[i+16*3]
			  );
		i++;
	}
	printf("\n");

	printf("Registers(pointer):\n");
	i = 0;
	while (i < 0x40/4) {
		printf("P%02X: %08X\tP%02X: %08X\tP%02X: %08X\tP%02X: %08X\n"
				, i+16*0, osecpu->pregisters[i+16*0]
				, i+16*1, osecpu->pregisters[i+16*1]
				, i+16*2, osecpu->pregisters[i+16*2]
				, i+16*3, osecpu->pregisters[i+16*3]
			  );
		i++;
	}
	printf("\n");

	printf("Registers(debug):\n");
	for (i = 0; i < 4; i++) {
		printf("DR%X: %08X\t", i, osecpu->dregisters[i]);
	}
	printf("\n\n");

	printf("osecpu->code:\n");
	i = 0;
	while (i < osecpu->codelen) {
		printf("%08X: ", i);
		for (j = 0; j < 16; j++) {
			if (i+j > osecpu->codelen-1) break;
			printf("%02X ", osecpu->code[i+j]);
		}
		printf("\n");
		i+=j;
	}
}

int fetch_code(struct Osecpu* osecpu)
{
	const int pc = osecpu->pregisters[0x3f];
	if (pc+4 > osecpu->codelen) return 0;
	osecpu->pregisters[0x3f] += 4;
	return
		(osecpu->code[pc+0] << 24) |
		(osecpu->code[pc+1] << 16) |
		(osecpu->code[pc+2] <<  8) |
		(osecpu->code[pc+3] <<  0);
}

int do_instruction(struct Osecpu* osecpu, const int icode)
{
	switch (icode) {
		case INSTRUCTION_LIMM:
			{
				const int resv = fetch_code(osecpu);
				const int imm = fetch_code(osecpu);
				const int r = fetch_code(osecpu)-0x76000000;
				const int bit = fetch_code(osecpu)-0x76000000;
				if (resv != 0xfffff788) goto invalid_argument_error;
				if (r < 0 || r > 0x3f) goto invalid_argument_error;
				if (bit != 0x20) goto invalid_argument_error;
				osecpu->registers[r] = imm;
			}
			break;
		case INSTRUCTION_LIDR:
			{
				const int resv = fetch_code(osecpu);
				const int imm = fetch_code(osecpu);
				const int dr = fetch_code(osecpu)-0x76000000;
				if (resv != 0xfffff788) goto invalid_argument_error;
				if (dr < 0 || dr > 4) goto invalid_argument_error;
				osecpu->dregisters[dr] = imm;
			}
			break;
		case INSTRUCTION_OR:
			{
				const int r1 = fetch_code(osecpu)-0x76000000;
				const int r2 = fetch_code(osecpu)-0x76000000;
				const int r0 = fetch_code(osecpu)-0x76000000;
				const int bit = fetch_code(osecpu);
				if (r1 < 0 || r1 > 0x3f) goto invalid_argument_error;
				if (r2 < 0 || r2 > 0x3f) goto invalid_argument_error;
				if (r0 < 0 || r0 > 0x3f) goto invalid_argument_error;
				if (bit != 0x20) goto invalid_argument_error;
				osecpu->registers[r0] = osecpu->registers[r1] | osecpu->registers[r2];
			}
			break;
		case INSTRUCTION_XOR:
			{
				const int r1 = fetch_code(osecpu)-0x76000000;
				const int r2 = fetch_code(osecpu)-0x76000000;
				const int r0 = fetch_code(osecpu)-0x76000000;
				const int bit = fetch_code(osecpu);
				if (r1 < 0 || r1 > 0x3f) goto invalid_argument_error;
				if (r2 < 0 || r2 > 0x3f) goto invalid_argument_error;
				if (r0 < 0 || r0 > 0x3f) goto invalid_argument_error;
				if (bit != 0x20) goto invalid_argument_error;
				osecpu->registers[r0] = osecpu->registers[r1] ^ osecpu->registers[r2];
			}
			break;
		case INSTRUCTION_AND:
			{
				const int r1 = fetch_code(osecpu)-0x76000000;
				const int r2 = fetch_code(osecpu)-0x76000000;
				const int r0 = fetch_code(osecpu)-0x76000000;
				const int bit = fetch_code(osecpu);
				if (r1 < 0 || r1 > 0x3f) goto invalid_argument_error;
				if (r2 < 0 || r2 > 0x3f) goto invalid_argument_error;
				if (r0 < 0 || r0 > 0x3f) goto invalid_argument_error;
				if (bit != 0x20) goto invalid_argument_error;
				osecpu->registers[r0] = osecpu->registers[r1] & osecpu->registers[r2];
			}
			break;
		default:
			osecpu->invalid_instruction_error = icode;
			return -1;
	}
	return 0;

invalid_argument_error:
	osecpu->invalid_argument_error = 1;
	return -1;
}

int run_b32(struct Osecpu* osecpu)
{
	int icode;
	while (1) {
		icode = fetch_code(osecpu);
		if (osecpu->codelen <= osecpu->pregisters[0x3f]) break;
		if (do_instruction(osecpu, icode) == -1) return -1;
	}
	return 0;
}


int main(int argc, char** argv)
{
	struct Osecpu* osecpu;

	if (argc < 2) {
		printf("Usage: %s app.ose\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	osecpu = (struct Osecpu*)malloc(sizeof(struct Osecpu));
	if (!osecpu) {
		printf("malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	memset(osecpu, 0, sizeof(struct Osecpu));

	if (load_b32(osecpu, argv[1]) == -1) {
		printf("load_b32() error\n");
		if (osecpu->code) free(osecpu->code);
		free(osecpu);
		exit(EXIT_FAILURE);
	}

	run_b32(osecpu);
	if (osecpu->invalid_instruction_error) {
		fprintf(stderr, "Error: invalid instruction error (%08X)\n", osecpu->invalid_instruction_error);
	} else if (osecpu->invalid_argument_error) {
		fprintf(stderr, "Error: invalid argument error\n");
	}

	coredump(osecpu);

	if (osecpu->code) free(osecpu->code);
	free(osecpu);

	return 0;
}

