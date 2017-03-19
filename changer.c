#define _DEFAULT_SOURCE
#include<stdlib.h>
#include<unistd.h>
#include<stdio.h>
#include<linux/limits.h>
#include<limits.h>
#include<sys/types.h>
#include<sys/fcntl.h>
#include<sys/ioctl.h>
#include<time.h>
#include<string.h>
#include<errno.h>


#define IOCTL_CMD _IOW(0xCD, 0x19, char*)

struct ctm_data {
	char* pathname;
	size_t pathsize;
	struct timespec time;
}; 

int main(int argc, char **argv){
	int fd = open("/dev/ctimem", O_RDWR);
	if(fd < 0){
		fprintf(stderr, "Could not open driver's device\n");
		return 1;
	}
	if((argc-1)%2 || argc <= 1){//Two args per path + program path as a first arg
		fprintf(stderr, "Invalid number of arguments\n");
		goto fail;
	}
	for(int i = 1; i < argc;i+=2){
		char resolved[PATH_MAX];
		struct tm tinfo;
		struct ctm_data data;
		if(realpath(argv[i], resolved) != resolved){
			fprintf(stderr, "Invalid path: '%s'\n", argv[i]);
			continue;
		}
		data.pathname = resolved;
		data.pathsize = strlen(resolved) + 1;
		sscanf(argv[i+1], "%u.%u.%u-%u:%u:%u:%ld", &tinfo.tm_mday, &tinfo.tm_mon,
			&tinfo.tm_year, &tinfo.tm_hour, &tinfo.tm_min, &tinfo.tm_sec,
			&data.time.tv_nsec);
		tinfo.tm_year -= 1900; //relative time
		tinfo.tm_mon -= 1;
		data.time.tv_sec = mktime(&tinfo);
		if(data.time.tv_sec < 0 || data.time.tv_nsec < 0 || data.time.tv_nsec >= 100000000L){
			fprintf(stderr, "Invalid time: '%s'\n", argv[i+1]);
			continue;
		}
		if(ioctl(fd, IOCTL_CMD, &data) < 0){
			fprintf(stderr, "Failed to change time of '%s': %s\n", argv[i], strerror(errno));
			continue;
		}
		printf("Changed time of '%s' to %s", argv[i], asctime(&tinfo));
	}
	close(fd);
	return 0;
fail:
	close(fd);
	return 1;
}
