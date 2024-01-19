#ifndef JSML_H
#define JSML_H

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum json_type {
  JSON_NULL,
  JSON_OBJECT,
  JSON_ARRAY,
  JSON_STRING,
  JSON_INTEGER,
  JSON_DOUBLE,
  JSON_BOOL
} json_type;

typedef struct json {
  json_type type;          // type of json node, based on json_type enum
  const char *key;         // key of the property; applicable only for OBJECT
  const char *json_string; // value of STRING node
  long long json_integer;  // value of INTEGER node
  int json_bool;           // value of BOOL node
  double json_double;      // value of DOUBLE node
  int length;              // number of children of OBJECT or ARRAY
  struct json *child;      // points to first child
  struct json *next;       // points to next child
  struct json *last_child;
} json;

typedef int (*json_unicode_encoder)(unsigned int codepoint, char *p,
                                    char **endp);

const json *json_parse(char *text, json_unicode_encoder encoder);
const json *json_parse_file(char *file_path, json_unicode_encoder encoder);
const json *json_parse_utf8(char *text);
const json *json_parse_file_utf8(char *file_path);
const json *json_get(const json *json,
                     const char *key); // get object's property by key
const json *json_item(const json *json, int idx); // get array element by index
const json *json_get_nested(const json *parsed_json,
                            const char *key); // get nested object/array
void json_print(const json *parsed_json);
void json_recursive_print(const json *parsed_json, int depth);
void json_free(const json *js);
char *unescape_json_string_literal(const char *str);
char *read_file(char *filename);

// redefine JSON_CALLOC & JSON_FREE to use custom allocator
#ifndef JSON_CALLOC
#define JSON_CALLOC() calloc(1, sizeof(json))
#define JSON_FREE(json) free((void *)(json))
#endif

// redefine JSON_REPORT_ERROR to use custom error reporting
#ifndef JSON_REPORT_ERROR
#define JSON_REPORT_ERROR(msg, p)                                              \
  fprintf(stderr, "JSML PARSE ERROR (%d): " msg " at %s\n", __LINE__, p)
#endif

#define IS_WHITESPACE(c) ((unsigned char)(c) <= (unsigned char)' ')

static const json dummy = {JSON_NULL};

static json *create_json(json_type type, const char *key, json *parent) {
  json *js = (json *)JSON_CALLOC();
  assert(js);
  js->type = type;
  js->key = key;
  if (!parent->last_child) {
    parent->child = parent->last_child = js;
  } else {
    parent->last_child->next = js;
    parent->last_child = js;
  }
  parent->length++;
  return js;
}

inline void json_free(const json *js) {
  json *p = js->child;
  json *p1;
  while (p) {
    p1 = p->next;
    json_free(p);
    p = p1;
  }
  JSON_FREE(js);
}

static int unicode_to_utf8(unsigned int codepoint, char *p, char **endp) {
  // code from http://stackoverflow.com/a/4609989/697313
  if (codepoint < 0x80)
    *p++ = codepoint;
  else if (codepoint < 0x800)
    *p++ = 192 + codepoint / 64, *p++ = 128 + codepoint % 64;
  else if (codepoint - 0xd800u < 0x800)
    return 0; // surrogate must have been treated earlier
  else if (codepoint < 0x10000)
    *p++ = 224 + codepoint / 4096, *p++ = 128 + codepoint / 64 % 64,
    *p++ = 128 + codepoint % 64;
  else if (codepoint < 0x110000)
    *p++ = 240 + codepoint / 262144, *p++ = 128 + codepoint / 4096 % 64,
    *p++ = 128 + codepoint / 64 % 64, *p++ = 128 + codepoint % 64;
  else
    return 0; // error
  *endp = p;
  return 1;
}

static inline int hex_val(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

static char *unescape_string(char *s, char **end,
                             json_unicode_encoder encoder) {
  char *p = s;
  char *d = s;
  char c;
  while ((c = *p++)) {
    if (c == '"') {
      *d = '\0';
      *end = p;
      return s;
    } else if (c == '\\') {
      switch (*p) {
      case '\\':
      case '/':
      case '"':
        *d++ = *p++;
        break;
      case 'b':
        *d++ = '\b';
        p++;
        break;
      case 'f':
        *d++ = '\f';
        p++;
        break;
      case 'n':
        *d++ = '\n';
        p++;
        break;
      case 'r':
        *d++ = '\r';
        p++;
        break;
      case 't':
        *d++ = '\t';
        p++;
        break;
      case 'u': // unicode
      {
        if (!encoder) {
          // leave untouched
          *d++ = c;
          break;
        }
        char *ps = p - 1;
        int h1, h2, h3, h4;
        if ((h1 = hex_val(p[1])) < 0 || (h2 = hex_val(p[2])) < 0 ||
            (h3 = hex_val(p[3])) < 0 || (h4 = hex_val(p[4])) < 0) {
          JSON_REPORT_ERROR("invalid unicode escape", p - 1);
          return 0;
        }
        unsigned int codepoint = h1 << 12 | h2 << 8 | h3 << 4 | h4;
        if ((codepoint & 0xfc00) ==
            0xd800) { // high surrogate; need one more unicode to succeed
          p += 6;
          if (p[-1] != '\\' || *p != 'u' || (h1 = hex_val(p[1])) < 0 ||
              (h2 = hex_val(p[2])) < 0 || (h3 = hex_val(p[3])) < 0 ||
              (h4 = hex_val(p[4])) < 0) {
            JSON_REPORT_ERROR("invalid unicode surrogate", ps);
            return 0;
          }
          unsigned int codepoint2 = h1 << 12 | h2 << 8 | h3 << 4 | h4;
          if ((codepoint2 & 0xfc00) != 0xdc00) {
            JSON_REPORT_ERROR("invalid unicode surrogate", ps);
            return 0;
          }
          codepoint =
              0x10000 + ((codepoint - 0xd800) << 10) + (codepoint2 - 0xdc00);
        }
        if (!encoder(codepoint, d, &d)) {
          JSON_REPORT_ERROR("invalid codepoint", ps);
          return 0;
        }
        p += 5;
        break;
      }
      default:
        // leave untouched
        *d++ = c;
        break;
      }
    } else {
      *d++ = c;
    }
  }
  JSON_REPORT_ERROR("no closing quote for string", s);
  return 0;
}

static char *parse_key(const char **key, char *p,
                       json_unicode_encoder encoder) {
  // on '}' return with *p=='}'
  char c;
  while ((c = *p++)) {
    if (c == '"') {
      *key = unescape_string(p, &p, encoder);
      if (!*key)
        return 0; // propagate error
      while (*p && IS_WHITESPACE(*p))
        p++;
      if (*p == ':')
        return p + 1;
      JSON_REPORT_ERROR("unexpected chars", p);
      return 0;
    } else if (IS_WHITESPACE(c) || c == ',') {
      // continue
    } else if (c == '}') {
      return p - 1;
    } else {
      JSON_REPORT_ERROR("unexpected chars", p - 1);
      return 0; // error
    }
  }
  JSON_REPORT_ERROR("unexpected chars", p - 1);
  return 0; // error
}

static char *parse_value(json *parent, const char *key, char *p,
                         json_unicode_encoder encoder) {
  json *js;
  while (1) {
    switch (*p) {
    case '\0':
      JSON_REPORT_ERROR("unexpected end of text", p);
      return 0; // error
    case ' ':
    case '\t':
    case '\n':
    case '\r':
    case ',':
      // skip
      p++;
      break;
    case '{':
      js = create_json(JSON_OBJECT, key, parent);
      p++;
      while (1) {
        const char *new_key;
        p = parse_key(&new_key, p, encoder);
        if (!p)
          return 0; // error
        if (*p == '}')
          return p + 1; // end of object
        p = parse_value(js, new_key, p, encoder);
        if (!p)
          return 0; // error
      }
    case '[':
      js = create_json(JSON_ARRAY, key, parent);
      p++;
      while (1) {
        p = parse_value(js, 0, p, encoder);
        if (!p)
          return 0; // error
        if (*p == ']')
          return p + 1; // end of array
      }
    case ']':
      return p;
    case '"':
      p++;
      js = create_json(JSON_STRING, key, parent);
      js->json_string = unescape_string(p, &p, encoder);
      if (!js->json_string)
        return 0; // propagate error
      return p;
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      js = create_json(JSON_INTEGER, key, parent);
      char *pe;
      errno = 0;
      js->json_integer = strtoll(p, &pe, 0);
      if (pe == p || errno == ERANGE) {
        JSON_REPORT_ERROR("invalid number", p);
        return 0; // error
      }
      if (*pe == '.' || *pe == 'e' || *pe == 'E') { // double value
        js->type = JSON_DOUBLE;
        errno = 0;
        js->json_double = strtod(p, &pe);
        if (pe == p || errno == ERANGE) {
          JSON_REPORT_ERROR("invalid number", p);
          return 0; // error
        }
      } else {
        js->json_double = js->json_integer;
      }
      return pe;
    }
    case 't':
      if (!strncmp(p, "true", 4)) {
        js = create_json(JSON_BOOL, key, parent);
        js->json_bool = 1;
        return p + 4;
      }
      JSON_REPORT_ERROR("unexpected chars", p);
      return 0; // error
    case 'f':
      if (!strncmp(p, "false", 5)) {
        js = create_json(JSON_BOOL, key, parent);
        js->json_bool = 0;
        return p + 5;
      }
      JSON_REPORT_ERROR("unexpected chars", p);
      return 0; // error
    case 'n':
      if (!strncmp(p, "null", 4)) {
        create_json(JSON_NULL, key, parent);
        return p + 4;
      }
      JSON_REPORT_ERROR("unexpected chars", p);
      return 0; // error
    default:
      JSON_REPORT_ERROR("unexpected chars", p);
      return 0; // error
    }
  }
}

inline char *unescape_json_string_literal(const char *str) {
  // replace \" with "
  char *unescaped = (char *)malloc(strlen(str) + 1);
  char *p = unescaped;
  while (*str) {
    if (*str == '\\' && *(str + 1) == '\"') {
      *p++ = '"';
      str += 2;
    } else {
      *p++ = *str++;
    }
  }

  *p = '\0';

  return unescaped;
}

inline char *read_file(char *filename) {
  FILE *f = fopen(filename, "rt");
  assert(f);
  fseek(f, 0, SEEK_END);
  long length = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buffer = (char *)malloc(length + 1);
  buffer[length] = '\0';
  fread(buffer, 1, length, f);
  fclose(f);
  return buffer;
}

inline void json_print(const json *parsed_json) {
  json_recursive_print(parsed_json, 0);
}

inline void json_recursive_print(const json *parsed_json, int depth) {
  json *js;

  for (js = parsed_json->child; js; js = js->next) {

    if (depth == 0) {
      printf("┼──");
    } else {
      for (int i = 0; i < depth + 1; i++) {
        printf("┼──");
      }
    }
    int type = (int)js->type;
    if (js->key) {
      printf(" %s: ", js->key);
    } else {
      printf(" ");
    }
    switch (type) {
    case JSON_NULL:
      printf("NULL\n");
      break;
    case JSON_OBJECT:
      printf("OBJECT\n");
      json_recursive_print(js, depth + 1);
      break;
    case JSON_ARRAY:
      printf("ARRAY\n");
      json_recursive_print(js, depth + 1);
      break;
    case JSON_STRING:
      printf("%s (string)\n", js->json_string);
      break;
    case JSON_INTEGER:
      printf("%lld (int)\n", js->json_integer);
      break;
    case JSON_DOUBLE:
      printf("%lf (double)\n", js->json_double);
      break;
    case JSON_BOOL:
      printf("%s (bool)\n", js->json_bool ? "true" : "false");
      break;
    default:
      printf("?\n");
      break;
    }
  }
}

inline const json *json_parse_utf8(char *text) {
  return json_parse(text, unicode_to_utf8);
}

inline const json *json_parse(char *text, json_unicode_encoder encoder) {
  json js = {};
  if (!parse_value(&js, 0, text, encoder)) {
    if (js.child)
      json_free(js.child);
    return 0;
  }
  return js.child;
}

inline const json *json_parse_file_utf8(char *file_path) {
  return json_parse_file(file_path, unicode_to_utf8);
}

inline const json *json_parse_file(char *file_path,
                                   json_unicode_encoder encoder) {
  char *text = read_file(file_path);
  json js = {};
  if (!parse_value(&js, 0, text, encoder)) {
    if (js.child)
      json_free(js.child);
    return 0;
  }
  return js.child;
}

inline const json *json_get(const json *parsed_json, const char *key) {
  if (!parsed_json || !key)
    return &dummy; // never return null
  json *js;
  for (js = parsed_json->child; js; js = js->next) {
    if (js->key && !strcmp(js->key, key))
      return js;
  }
  return &dummy; // never return null
}

inline const json *json_get_nested(const json *parsed_json, const char *key) {
  if (!parsed_json || !key)
    return &dummy; // never return null
  // split key by '.'
  char *key_copy = strdup(key);
  char *token = strtok(key_copy, ".");
  const json *js = parsed_json;
  while (token) {
    js = json_get(js, token);
    if (js == &dummy) {
      return &dummy;
    }
    token = strtok(NULL, ".");
  }
  free(key_copy);
  return js;
}

inline const json *json_item(const json *parsed_json, int idx) {
  if (!parsed_json)
    return &dummy; // never return null
  json *js;
  for (js = parsed_json->child; js; js = js->next) {
    if (!idx--)
      return js;
  }
  return &dummy; // never return null
}

#endif // JSML_H
