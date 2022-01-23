#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "cJSON.h"

void main() {
	int fd;
	char filename[32];

	char json_string[1024];
	char *pc;
	char *out;

	cJSON *json;

	int numsteps = 0, numops = 0;

	printf("Enter json filename -> ");
	scanf("%s", filename);

	if((fd = open(filename, O_RDONLY)) < 0)
            printf("Error opening %s\n", filename);
        else  {
	  printf("Parsing %s\n\n", filename);
	  pc = json_string;

	  while (read(fd, pc, 1) != '\0')
		  pc++;

/*	  printf("%s\n", json_string); */

	  json = cJSON_Parse(json_string);

	  if (!json) 
	    printf("Error before: [%s]\n",cJSON_GetErrorPtr());
          else
          {
		  printf("No errors in json file\n");
                  out=cJSON_Print(json);

		  /* attempt to walk the resulting tree */
		  printf("Title = %s\n", cJSON_GetObjectItem(json, "Title")->valuestring);
		  printf("Description = %s\n", cJSON_GetObjectItem(json, "Description")->valuestring);
		  printf("Duration = %s\n", cJSON_GetObjectItem(json, "Duration")->valuestring);
		  printf("Prerequisite = %s\n", cJSON_GetObjectItem(json, "Prerequisite")->valuestring);
	          
		  cJSON *steps = cJSON_GetObjectItem(json, "Steps");
		  numsteps = cJSON_GetArraySize(steps);
		  printf("There are %d steps in this sequence\n", numsteps);

		  for(int j = 0; j < numsteps; j++) {
		    cJSON *step = cJSON_GetArrayItem(steps,j);
		    printf("steps[%d] Title = %s\n", j, cJSON_GetObjectItem(step, "Title")->valuestring);

		    cJSON *ops = cJSON_GetObjectItem(step, "Operations");
		    numops = cJSON_GetArraySize(ops);
		    printf("There are %d operations in step %d\n", numops, j);

		    for(int i = 0; i < numops; i++)  {
                      cJSON *op = cJSON_GetArrayItem(ops, i);
		      printf("Operations[%d] Title = %s\n", i, cJSON_GetObjectItem(op, "Command")->valuestring);
		    }

                  }

                  cJSON_Delete(json);
                  printf("%s\n",out);
                  free(out);
          }
	}
}
