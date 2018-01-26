/** Main routine.
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2014-2017, Steffen Vogel
 * @license GPLv3
 *********************************************************************************/

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include <errno.h>
#include <error.h>

#include "config.h"

int running = 1;

/* Default settings */
struct config cfg = {
	.probe = {
		.payload = 0,
		.rate = 1,
		.warmup = 200,
		.limit = 100
	},
	.dist = {
		.format = FORMAT_TC,
		.scaling = 1
	},
	.emulate = {
		.mark = 0xCD,
		.mask = 0xFFFFFFFF,
		.dev = "eth0"
	}
};

int probe(int argc, char *argv[]);
int emulate(int argc, char *argv[]);
int dist(int argc, char *argv[]);

void quit(int sig, siginfo_t *si, void *ptr)
{
	printf("Goodbye!\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf( "usage: %s CMD [OPTIONS]\n"
			"  CMD     can be one of:\n\n"
			"    probe IP PORT    Start TCP SYN+ACK RTT probes and write measurements data to STDOUT\n"
			"    emulate          Read measurement data from STDIN and configure Kernel (tc-netem(8)) on-the-fly.\n"
			"                        This mode only uses the mean and standard deviation of of the previous samples\n"
			"                        to configure the netem qdisc. This can be used to interactively replicate a network link.\n"
			"\n"
			"    dist generate    Read measurement data from STDIN and write distribution file to STDOUT (see /usr/lib/tc/*.dist)\n"
			"    dist load        Read measurement data from STDIN and configure Kernel (tc-netem(8))\n"
			"                        These modes generate an inverse cumulated probability function (CDF) from the previously\n"
			"                        recorded measurements. This iCDF can either be used by tc(8) or 'netem table'\n"
			"\n"
			"  OPTIONS:\n\n"
			"    -m  N      apply emulation only to packet buffers with mark N\n"
			"    -M  N      an optional mask for the fw mark\n"
			"    -i  N      update the emulation parameters every N seconds\n"
			"    -r RATE    rate limit used for measurements and updates of network emulation\n"
			"    -l CNT     how many probes should we sent\n"
			"    -w SAMPLES number of probe samples to be collected for estimating histogram boundaries\n"
			"    -d IF      network interface\n"
			"    -s FACTOR  a scaling factor for the dist subcommands\n"
			"    -f FMT     the output format of the distribution tables\n"
			"    -p SZ      payload size for ICMP messages\n"
			"\n"
			"NetPlika %s (built on %s %s)\n"
			" Copyright 2016-2018, Steffen Vogel <post@steffenvogel.de>\n", argv[0], VERSION, __DATE__, __TIME__);

		exit(EXIT_FAILURE);
	}

	/* Setup signals */
	struct sigaction sa_quit = {
		.sa_flags = SA_SIGINFO,
		.sa_sigaction = quit
	};

	sigemptyset(&sa_quit.sa_mask);
	sigaction(SIGTERM, &sa_quit, NULL);
	sigaction(SIGINT, &sa_quit, NULL);

	/* Initialize PRNG for TCP sequence nos */
	srand(time(NULL));

	/* Parse Arguments */
	char c, *endptr;
	while ((c = getopt(argc, argv, "h:m:M:i:l:d:r:s:f:w:p:")) != -1) {
		switch (c) {
			case 'm':
				cfg.emulate.mark = strtoul(optarg, &endptr, 0);
				goto check;
			case 'M':
				cfg.emulate.mask = strtoul(optarg, &endptr, 0);
				goto check;
			case 'w':
				cfg.probe.warmup = strtoul(optarg, &endptr, 0);
				goto check;
			case 'i':
				cfg.emulate.interval = strtoul(optarg, &endptr, 10);
				goto check;
			case 'r':
				cfg.probe.rate = strtof(optarg, &endptr);
				goto check;
			case 'l':
				cfg.probe.limit = strtoul(optarg, &endptr, 10);
				goto check;
			case 'd':
				cfg.emulate.dev = strdup(optarg);
				break;
			case 's':
				cfg.dist.scaling = strtof(optarg, &endptr);
				goto check;
			case 'p':
				cfg.probe.payload = strtoul(optarg, &endptr, 10);
				goto check;
			case 'f':
				if (strcmp(optarg, "villas") == 0)
					cfg.dist.format = FORMAT_VILLAS;
				else if (strcmp(optarg, "tc") == 0)
					cfg.dist.format = FORMAT_TC;
				else {
					error(-1, 0, "Unknown format: %s.", optarg);
					exit(EXIT_FAILURE);
				}
				break;
			case '?':
				if (optopt == 'c')
					error(-1, 0, "Option -%c requires an argument.", optopt);
				else if (isprint(optopt))
					error(-1, 0, "Unknown option '-%c'.", optopt);
				else
					error(-1, 0, "Unknown option character '\\x%x'.", optopt);

				exit(EXIT_FAILURE);
			default:
				abort();
		}

		continue;
check:
		if (optarg == endptr)
			error(-1, 0, "Failed to parse parse option argument '-%c %s'", c, optarg);
	}

	char *cmd = argv[optind];

	if      (!strcmp(cmd, "probe"))
		return probe(argc-optind-1, argv+optind+1);
	else if (!strcmp(cmd, "emulate"))
		return emulate(argc-optind-1, argv+optind+1);
	else if (!strcmp(cmd, "dist"))
		return dist(argc-optind-1, argv+optind+1);
	else
		error(-1, 0, "Unknown command: %s", cmd);

	return 0;
}
