/* Alternative cJSON usage with pre-allocated buffer */
#include "cJSON.h"

static char json_buffer[512]; /* Pre-allocated buffer */

/* Function to safely print JSON to static buffer */
static char* safe_cjson_print(cJSON *item) {
  char *string = cJSON_PrintUnformatted(item);
  if (string == NULL) {
    return NULL;
  }
  
  /* Copy to our static buffer and free the cJSON allocated memory immediately */
  if (strlen(string) < sizeof(json_buffer)) {
    strcpy(json_buffer, string);
    free(string); /* Free immediately */
    return json_buffer;
  } else {
    free(string); /* Free on error too */
    return NULL;
  }
}
