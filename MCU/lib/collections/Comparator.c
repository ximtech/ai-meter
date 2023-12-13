#include "Comparator.h"

static uint32_t skipLeadingSpacesOrZeroes(const char* text, int32_t *numberOfZeroes);
static int32_t compareRight(const char *one, const char *two);
static int32_t compareEqual(const char *one, const char *two, int32_t numberOfZeroesOne, int32_t numberOfZeroesTwo);

static inline bool isNotDigitAndPunct(char ch) {
    return !isdigit(ch) && ch != '.' && ch != ',';
}


uint32_t strHashCode(const char *key) {  // Returns a hashCode code for the provided string.
    uint32_t hash = 2166136261u;
    uint32_t keyLength = strlen(key);
    for (uint32_t i = 0; i < keyLength; i++) {
        hash ^= (uint8_t) key[i];
        hash *= 16777619;
    }
    return hash;
}

int doubleComparator(double one, double two) {
    if (one < two) {
        return -1;           // Neither val is NaN, 'one' is smaller
    }
    if (one > two) {
        return 1;            // Neither val is NaN, 'one' is larger
    }

    int64_t oneBits = (int64_t) one;
    int64_t twoBits = (int64_t) two;
    return (oneBits == twoBits ?  0 :   // Values are equal
            (oneBits < twoBits ? -1 :   // (-0.0, 0.0) or (!NaN, NaN)
             1));                       // (0.0, -0.0) or (NaN, !NaN)
}

int floatComparator(float one, float two) {
    if (one < two) {
        return -1;           // Neither val is NaN, 'one' is smaller
    }
    if (one > two) {
        return 1;            // Neither val is NaN, 'one' is larger
    }

    int32_t oneBits = (int32_t) one;
    int32_t twoBits = (int32_t) two;
    return (oneBits == twoBits ?  0 :   // Values are equal
            (oneBits < twoBits ? -1 :   // (-0.0, 0.0) or (!NaN, NaN)
             1));                       // (0.0, -0.0) or (NaN, !NaN)
}

int strNaturalSortComparator(const char *one, const char *two) {
    // Only count the number of zeroes leading the last number compared
    int32_t numberOfZeroesOne = 0;
    int32_t numberOfZeroesTwo = 0;

    while (true) {
        one += skipLeadingSpacesOrZeroes(one, &numberOfZeroesOne);
        two += skipLeadingSpacesOrZeroes(two, &numberOfZeroesTwo);

        // Process run of digits
        if (isdigit((int) *one) && isdigit((int) *two)) {
            int bias = compareRight(one, two);
            if (bias != 0) {
                return bias;
            }
        }

        if (*one == '\0' && *two == '\0') {
            // The strings compare the same. Perhaps the caller
            // will want to call strcmp to break the tie.
            return compareEqual(one, two, numberOfZeroesOne, numberOfZeroesTwo);
        }

        if (*one < *two) {
            return -1;
        }

        if (*one > *two) {
            return 1;
        }

        one++;
        two++;
    }
}

static uint32_t skipLeadingSpacesOrZeroes(const char* text, int32_t *numberOfZeroes) {
    uint32_t index = 0;
    while (isspace((int) *text) || *text == '0') {
        if (*text == '0') {
            *numberOfZeroes = *numberOfZeroes + 1;
        } else {
            *numberOfZeroes = 0;
        }
        text++;
        index++;
    }
    return index;
}

static int32_t compareRight(const char *one, const char *two) {
    // The longest run of digits wins. That aside, the greatest
    // value wins, but we can't know that it will until we've scanned
    // both numbers to know that they have the same magnitude, so we
    // remember it in BIAS.
    int32_t bias = 0;
    while (true) {
        if (isNotDigitAndPunct(*one) && isNotDigitAndPunct(*two)) {
            return bias;
        }

        if (isNotDigitAndPunct(*one)) {
            return -1;
        }

        if (isNotDigitAndPunct(*two)) {
            return 1;
        }

        if (*one == '\0' && *two == '\0') {
            return bias;
        }

        if (bias == 0) {
            if (*one < *two) {
                bias = -1;
            } else if (*one > *two) {
                bias = 1;
            }
        }
        one++;
        two++;
    }
}

static int32_t compareEqual(const char *one, const char *two, int32_t numberOfZeroesOne, int32_t numberOfZeroesTwo) {
    int32_t zeroDiff = numberOfZeroesOne - numberOfZeroesTwo;
    if (zeroDiff != 0) {
        return zeroDiff;
    }

    int32_t oneLength = (int32_t) strlen(one);
    int32_t twoLength = (int32_t) strlen(two);
    if (oneLength == twoLength) {
        return strcmp(one, two);
    }

    return oneLength - twoLength;
}