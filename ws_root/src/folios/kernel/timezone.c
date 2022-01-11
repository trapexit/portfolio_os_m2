/* @(#) timezone.c 96/11/06 1.5 */

/* This value corresponds to the number of seconds from GMT of the time zone
 * of the location where the kernel is built. The value needs to change
 * depending on daylight savings time.
 */

#if 0

/* when in daylight saving time */
int timezone = 7*3600;

#else

/* when in standard time */
int timezone = 8*3600;

#endif
