/*
 * aquaauto.h
 */

/*
 * These are the atomic operations that the aquarium automation embedded "server" can execute
 * Aquarium Automation Commands
 */
#define AAC_NULL    0  /* expect this will be handy ... sort of like ping */
#define ACC_TODWAIT 1  /* wait for the specified time of day to continue */
#define ACC_DELAY   2  /* delay this number of mS */
#define AAC_CANCEL  1  /* cancel current operation */
#define AAC_DUMP    2  /* remove water from the system */
#define AAC_FILL    3  /* add water to the system */

/*
 * Acknowledgements and error codes
 * Aquarium Automation Replies
 */
#define AAR_ACK     0  /* successful acknowledge */
#define AAR_ERROR  -1  /* generic failure */
#define AAR_ARG    -2  /* invalid argument */

struct operation {
  int id,
  char label[32],
};

struct operation valid_ops[] = {
  { ACC_NULL, "Null Command" },
  ...
};

/*
 * a structure to describe the physical components that 
 * will be used to manifest the operations in the real world
 */
struct physcomp {
  int id,          /* an integer to use for switching */
  char label[32],  /* a human readable reference for error msg's, etc */
  int *(action()); /* call this function to make it happen */
};

struct physcomp valvesnpumps[] = {
  { DUMP_VALVE, "Dump Valve", opendump() },
  ...
};

/*
 * Do I want to send commands in json format ?
 * Yep ... convert the json using the struct's above so as not to have to do 
 * lots of str compares
 *
 * like ... let's call it a "sequence":
 *
 { 
  "Title": "Water Change",
  "Description": "Execute a water change",
  "Duration": "15m",
  "Prerequisition" : "None",
  "Section" :  {
    "Title": "System Check",
    "Operations" : [
      {
        "Command" : "ACC_TODWAIT",
        "TOD" : "08:00:00",
        "Notification" : {
	  "Method" : "Text",
	  "Number" : "2624098283",
	  "Carrier" : "ATT"
	}
      },
      {
        "Command" : "ACC_DUMP",
	"Valve" : "Dump_Valve",
	"Volume" : "1L",
	"FS Time" : "10m",
	"Notification" : {
	  "Method" : "Tone",
	}
      },
    ],
  }
}


    

