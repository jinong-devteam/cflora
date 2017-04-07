#include <stdio.h>

#include <cf_base.h>
#include <cf_msgbuf.h>

#include "test.h"

#define BUFF "abcdefghijklmnopgrstuwxyzabcdefghijklmnopgrstuwxyzabcdefghijklmnopgrstuwxyzabcdefghijklmnopgrstuwxyzabcdefghijklmnopgrstuwxyzabcdefghijklmnopgrstuwxyzabcdefghijklmnopgrstuwxyzabcdefghijklmnopgrstuwxyz"

cf_ret_t
test_msgbuf () {
	cf_msgbuf_t msgbuf;
	int len, size;
	char *tbuf;

	ASSERT_OK (cf_init_msgbuf (&msgbuf), "fail to init");
	ASSERT_OK (cf_append_msgbuf (&msgbuf, BUFF, strlen (BUFF)), "fail to append");

	len = cf_getlength_msgbuf (&msgbuf);
	tbuf = cf_getbuf_msgbuf (&msgbuf);
	ASSERT (len == strlen(tbuf), "length is not matched");

	cf_setused_msgbuf (&msgbuf, 3);
	tbuf = cf_getbuf_msgbuf (&msgbuf);
	ASSERT ('d' == *tbuf, "the character is not 'd'");

	ASSERT_OK (cf_append_msgbuf (&msgbuf, BUFF, strlen (BUFF)), "fail to append");

	len = cf_getlength_msgbuf (&msgbuf);
	tbuf = cf_getbuf_msgbuf (&msgbuf);
	ASSERT (len == strlen(tbuf), "length is not matched");

	size = cf_getsize_msgbuf (&msgbuf);
	fprintf (stderr, "current size is %d\n", size);
	ASSERT_OK (cf_resize_msgbuf (&msgbuf, 100), "fail to resize");
	ASSERT (100 <= cf_getsize_msgbuf (&msgbuf), "size is smaller than 100");

	ASSERT_OK (cf_resize_msgbuf (&msgbuf, 1000), "fail to resize");
	ASSERT (1000 == cf_getsize_msgbuf (&msgbuf), "size is not 1000");

	cf_release_msgbuf (&msgbuf);

	return CF_OK;
}

int
main () {
	TEST (test_msgbuf ());
	return 0;
}
