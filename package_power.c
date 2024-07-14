/*
gcc -o package_power package_power.c -lm
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include <sys/syscall.h>
#include <linux/perf_event.h>

#define AMD_MSR_PWR_UNIT 0xC0010299
#define AMD_MSR_PACKAGE_ENERGY 0xC001029B

#define AMD_ENERGY_UNIT_MASK 0x1F00

int open_msr(int core) {
    char msr_filename[BUFSIZ];
	int fd;

	sprintf(msr_filename, "/dev/cpu/%d/msr", core);
	fd = open(msr_filename, O_RDONLY);
	if ( fd < 0 ) {
		if ( errno == ENXIO ) {
			fprintf(stderr, "rdmsr: No CPU %d\n", core);
			exit(2);
		} else if ( errno == EIO ) {
			fprintf(stderr, "rdmsr: CPU %d doesn't support MSRs\n",
					core);
			exit(3);
		} else {
			perror("rdmsr:open");
			fprintf(stderr,"Trying to open %s\n",msr_filename);
			exit(127);
		}
	}

	return fd;
}

static long long read_msr(int fd, unsigned int which) {

	uint64_t data;

	if ( pread(fd, &data, sizeof data, which) != sizeof data ) {
		perror("rdmsr:pread");
		exit(127);
	}

	return (long long) data;
}

int main(int argc, char **argv) {
    int fd = open_msr(0);

    int core_energy_units = read_msr(fd, AMD_MSR_PWR_UNIT);
    unsigned int energy_unit = (core_energy_units & AMD_ENERGY_UNIT_MASK) >> 8;
    double energy_unit_d = pow(0.5, (double) energy_unit);

    int package_raw = read_msr(fd, AMD_MSR_PACKAGE_ENERGY) * energy_unit_d;
    usleep(100000);
    int package_raw_delta = read_msr(fd, AMD_MSR_PACKAGE_ENERGY) * energy_unit_d;

    printf("%g", ((double) package_raw_delta - (double) package_raw) * 10);
	
	return 0;
}
