#include "main.h"
#include "str.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); failures++; } \
} while (0)

int main(void)
{
    char buf[64];
    char s1[64] = "  abc  ";
    char s2[64] = "NeXTstep 3.3";
    char s3[64] = "NeXTstep 3.3";
    char *s;

    CHECK(Str_Trim(s1) == s1 && strcmp(s1, "abc") == 0,
          "Str_Trim trims leading/trailing whitespace");
    Str_ToUpper(s2);
    CHECK(strcmp(s2, "NEXTSTEP 3.3") == 0, "Str_ToUpper");
    Str_ToLower(s3);
    CHECK(strcmp(s3, "nextstep 3.3") == 0, "Str_ToLower");
    CHECK(Str_Copy(buf, "abcdefgh", sizeof(buf)) == 8 &&
          strcmp(buf, "abcdefgh") == 0, "Str_Copy fits buffer");
    CHECK(Str_Copy(buf, "this source string is much, much too long to fit in the sixty four byte destination buffer", sizeof(buf)) < 0,
          "Str_Copy truncates and returns negative on overflow");
    s = Str_Dup("NeXT");
    CHECK(s && strcmp(s, "NeXT") == 0, "Str_Dup duplicates");
    free(s);

    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("test-str: OK\n");
    return 0;
}