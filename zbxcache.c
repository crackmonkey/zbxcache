/* Hax and stuff
 */

#include <stdio.h>
#include <string.h>

#include <libmemcached-1.0/memcached.h>

#define MAXBUF 64

const char *config_string= "--SERVER=localhost";

memcached_st *st;

#define ERROR(...) fprintf(stderr,  __VA_ARGS__)

#if 0
#define TRACE(...) fprintf(stderr,  __VA_ARGS__)
#else
#define TRACE
#endif

char *trim(char *src)
{
	while (*src == ' ' || *src == '\t') {
		src++;
	}

	char c = src[strlen(src)-1];

	while (c == ' ' || c == '\n' || c == '\r') {
		src[strlen(src)-1] = 0;
		c = src[strlen(src)-1];
	}
	return src;
}

// Run the source script and inject the output into memcached
// The output must be in the format "key: val\n"
void run_script(char *script, int maxage)
{
	memcached_return_t ret;
	char valbuf[MAXBUF];
	int vallen, splitidx;
	char *key, *val;
	FILE *output;

	output = popen(script, "r");
	if (output == NULL) {
		ERROR("popen failed for \"%s\"\n", script);
		return;
	}
	while (!feof(output)) {
		char *line;
		size_t buflen;
		int len;
		line = (char *) malloc(MAXBUF);
		len = getline(&line, &buflen, output);
		if (len < 1) {
			continue;
		}
		key = strtok(line, ": ");
		val = trim(strtok(NULL, "\n"));
		TRACE("TRACE: Caching %s=%s\n", key, val);
		ret = memcached_set(st, key, strlen(key), val, strlen(val), maxage, 0);

		free(line);
	}
	pclose(output);
}

int main(int argc, char *argv[])
{
	memcached_return_t ret;
	size_t vallen;
	uint32_t flags;
	char *script, *key;
	int maxage;

	if (argc < 3) {
		ERROR("zbxcache <srcscript> <maxage> <key>\n");
		return 1;
	}

	script = argv[1];
	sscanf(argv[2], "%i", &maxage);
	key = argv[3];


	// init the memcached connection	
	st = memcached(config_string, strlen(config_string)); ;

	char *val = memcached_get(st, key, strlen(key), &vallen, &flags, &ret);

	TRACE("TRACE: getting key \"%s\": %s\n", key, memcached_last_error_message(st));

	if (val != NULL) {
		// return the value
		puts(val);
	} else {
		TRACE("TRACE: key %s not found, running script \"%s\"\n", key, script);
		
		run_script(script, maxage);
		// try again after running the script
		val = memcached_get(st, key, strlen(key), &vallen, &flags, &ret);

		TRACE("TRACE: getting key \"%s\": %s\n", key, memcached_last_error_message(st));

		if (val != NULL) {
			puts(val);
		} else {
			puts("ZBX_NOTSUPPORTED");
		}
	}

	return 0;
}

