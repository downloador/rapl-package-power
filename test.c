/*
gcc -o test test.c -lm
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

int open_msr(int core) {
    char msr_filename[BUFSIZ]; // String buffer for the filename
    sprintf(msr_filename, "/dev/cpu/%d/msr", core); // Adds the filename string to the buffer
    
    int file_descriptor = open(msr_filename, O_RDONLY); // Get File Descriptor from filename, perform as READONLY
    if (file_descriptor == -1) { // -1 is returned on error
        if (errno == ENXIO) {
            fprintf(stderr, "open-msr: CPU MSR %d not found\n", core);
            exit(2);
        } else if (errno == EIO) {
            fprintf(stderr, "open-msr: CPU MSR %d I/O error, your CPU probably doesn't support MSRs\n", core);
            exit(3);
        } else {
            perror("open-msr");
            fprintf(stderr, "Something went wrong trying to open %s\n", msr_filename);
            exit(127);
        }
    }

    return file_descriptor;
}

static long read_msr(int file_descriptor, unsigned int offset) {
    uint64_t data;

    // Read fd and write to pointer data, with offset
    if (pread(file_descriptor, &data, sizeof data, offset) != sizeof data) {
        perror("read-msr");
        exit(127);
    }

    // I don't know what long long is used for but it was used before so I will too
    // I will actually use long now
    return (long) data;
}

//

#define AMD_MSR_PWR_UNIT 0xC0010299
#define AMD_MSR_PACKAGE_ENERGY 0xC001029B
#define AMD_ENERGY_UNIT_MASK 0x1F00

int get_package_power(int core) {
    int file_descriptor_core = open_msr(core);

    int core_energy_units = read_msr(file_descriptor_core, AMD_MSR_PWR_UNIT);
    unsigned int energy_unit = (core_energy_units & AMD_ENERGY_UNIT_MASK) >> 8;
    double energy_unit_d = pow(0.5, (double) energy_unit);

    int package_raw = read_msr(file_descriptor_core, AMD_MSR_PACKAGE_ENERGY) * energy_unit_d;
    usleep(100000); // Sleep for 100 milliseconds, or 100000 microseconds
    int package_raw_delta = read_msr(file_descriptor_core, AMD_MSR_PACKAGE_ENERGY) * energy_unit_d;

    int package_power = (package_raw_delta - package_raw) * 10;

    return package_power;
}

//

int main(int argc, char **argv) {
    printf("CPU   POWER\n");

    for (int core = 0; core < 12; core++) {
        char core_as_string[2];
        sprintf(core_as_string, "%d", core);
        
        printf("%d", core);
        for (int i = 0; i < 3 - strlen(core_as_string); i++) printf(" ");

        //
        printf("   ");
        //

        int package_power = get_package_power(core);
        char power_as_string[3];
        sprintf(power_as_string, "%d", package_power);

        printf("%d", package_power);
        printf(" W");
        for (int i = 0; i < 3 - strlen(power_as_string); i++) printf(" ");
        printf("\n");
    }

	return 0;
}
