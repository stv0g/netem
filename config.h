/** Global configuration.
 *
 * @author Steffen Vogel <post@steffenvogel.de>
 * @copyright 2014-2017, Steffen Vogel
 * @license GPLv3
 *********************************************************************************/

#ifndef _CONFIG_H_
#define _CONFIG_H_

#define VERSION		"0.1"

struct config {
	int mark;
	int mask;
	int interval;
	int limit;
	int warmup;
	double rate;
	double scaling;
	char *dev;
	enum {
		FORMAT_TC,
		FORMAT_VILLAS
	} format;
};

/* Declared in main.c */
extern struct config cfg;

#endif
