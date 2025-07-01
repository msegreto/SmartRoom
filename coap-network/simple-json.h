#include <stdio.h>
#include <string.h>


static int json_start_object(char *buffer, int pos, int max_len) {
  if(pos >= max_len - 1) return -1;
  buffer[pos] = '{';
  return pos + 1;
}

static int json_end_object(char *buffer, int pos, int max_len) {
  if(pos >= max_len - 1) return -1;
  buffer[pos] = '}';
  return pos + 1;
}

static int json_add_string(char *buffer, int pos, int max_len, const char *key, const char *value) {
  int needed = snprintf(buffer + pos, max_len - pos, "\"%s\":\"%s\"", key, value);
  if(needed >= max_len - pos) return -1;
  return pos + needed;
}

static int json_add_number(char *buffer, int pos, int max_len, const char *key, int value) {
  int needed = snprintf(buffer + pos, max_len - pos, "\"%s\":%d", key, value);
  if(needed >= max_len - pos) return -1;
  return pos + needed;
}

static int json_add_comma(char *buffer, int pos, int max_len) {
  if(pos >= max_len - 1) return -1;
  buffer[pos] = ',';
  return pos + 1;
}

/* Example usage for sensor registration */
static int build_registration_json(char *buffer, int max_len, const char *sensor_type, int period) {
  int pos = 0;
  
  pos = json_start_object(buffer, pos, max_len);
  if(pos < 0) return -1;
  
  pos = json_add_string(buffer, pos, max_len, "n", sensor_type);
  if(pos < 0) return -1;
  
  pos = json_add_comma(buffer, pos, max_len);
  if(pos < 0) return -1;
  
  pos = json_add_number(buffer, pos, max_len, "t", period);
  if(pos < 0) return -1;
  
  pos = json_end_object(buffer, pos, max_len);
  if(pos < 0) return -1;
  
  buffer[pos] = '\0';
  return pos;
}
