/*
 * aquaauto.h
 */

/*
 * These are the atomic operations that the aquarium automation embedded "server" can execute
 */
#define AA_NULL 0x00  /* expect this will be handy */
#define AA_DUMP 0x01  /* remove water from the system */
#define AA_FILL 0x02  /* add water to the system */
