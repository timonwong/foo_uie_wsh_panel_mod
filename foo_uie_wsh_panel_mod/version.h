#pragma once

#define WSPM_VERSION_NUMBER_MAIN "1.3.3"
#define WSPM_TEST_VERSION 0

#ifdef _DEBUG
#	define WSPM_DEBUG_SUFFIX " (DEBUG)"
#else
#	define WSPM_DEBUG_SUFFIX ""
#endif

#if WSPM_TEST_VERSION == 1
#	define WSPM_TEST_VERSION_SUFFIX "Beta 1"
#	define WSPM_VERSION_NUMBER WSPM_VERSION_NUMBER_MAIN " " WSPM_TEST_VERSION_SUFFIX WSPM_DEBUG_SUFFIX
#	include <time.h>
#else
#	define WSPM_VERSION_NUMBER WSPM_VERSION_NUMBER_MAIN WSPM_DEBUG_SUFFIX
#endif

#if WSPM_TEST_VERSION == 1
/* NOTE: Assume that date is following this format: "Jan 28 2010" */
static bool is_expired(char const * date)
{
    char s_month[4] = {0};
    int month, day, year;
    tm t = {0};
    const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

    sscanf_s(date, "%3s %2d %4d", s_month, _countof(s_month), &day, &year);

    const char * month_pos = strstr(month_names, s_month);
    month = (month_pos - month_names) / 3;

    t.tm_mon = month;
    t.tm_mday = day;
    t.tm_year = year - 1900;
    t.tm_isdst = FALSE;

    time_t start = mktime(&t);
    time_t end = time(NULL);

    // expire in ~14 days
    const double secs = 15 * 60 * 60 * 24;
    return (difftime(end, start) > secs);
}

#define IS_EXPIRED(date) (is_expired(__DATE__))
#else

#define IS_EXPIRED(date) (false)
#endif
