
#include <stdio.h>
#include <cf_base.h>

cf_verbose_t verbose_level;

void
cf_set_verbose (cf_verbose_t level) {
	verbose_level = level;
}

cf_verbose_t
cf_get_verbose () {
	return verbose_level;
}

