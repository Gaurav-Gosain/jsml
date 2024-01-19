# JSML - JSON Manipulation Library

JSML is a lightweight C library for parsing and manipulating JSON data.

## Features
- Parse JSON data from a string or a file.
- Access JSON values by key or index.
- Helper functions for accessing nested JSON values and printing JSON data (along with datatypes).
- Free up memory used by JSON data.

## Usage

### Parsing JSON

#### Parse from String Literal

> Note: The unescape_json_string_literal function is only needed if you are parsing a JSON string literal i.e. a string that is hardcoded and escaped. If you are parsing a JSON string, you can skip this step.

```C
char* json_string = "{\"key\": \"value\"}";
const json* parsed_json = json_parse_utf8(unescape_json_string_literal(json_string));
if (parsed_json) {
  json_print(parsed_json, 0);
}
json_free(parsed_json);
```

#### Parse from File

```C
const json* parsed_json_file = json_parse_file_utf8("example.json");
if (parsed_json_file) {
  json_print(parsed_json_file, 0);
}
json_free(parsed_json_file);
```
## Accessing JSON Values
You can access JSON values by key or index using the json_get and json_item functions. Here is an example:

> For this example, we will use the following JSON data:
```JSON
{
  "int": 195,
  "array": [3, 5.1, -7, "nine"],
  "bool": true,
  "double": -1e-4,
  "null-value": null,
  "hello": "world!",
  "obj": {
    "key": "val",
    "double": 1e4
  },
  "nested": [
    {
      "a": "b"
    },
    {
      "a": 69
    },
    {
      "a": [4, 2, 0]
    }
  ]
}
```

> NOTE: Please assert that the value returned by json_get or json_item is not NULL before accessing the value. For the sake of brevity, we have omitted the assertions in the following examples.

```C
const json* parsed_json = json_parse_file_utf8("example.json");

/* Access values */
const json *int_node = json_get(parsed_json, "int");
assert(int_node->type == JSON_INTEGER);
printf("int: %lld\n", int_node->json_integer);

printf("bool: %s\n",
        json_get(parsed_json, "bool")->json_bool ? "true" : "false");
printf("double: %f\n", json_get(parsed_json, "double")->json_double);
printf("some-null: %s\n",
        json_get(parsed_json, "null-value")->type == JSON_NULL ? "null"
                                                              : "not null");
printf("hello: %s\n", json_get(parsed_json, "hello")->json_string);
printf("obj.key: %s\n",
        json_get(json_get(parsed_json, "obj"), "key")->json_string);
printf("obj.double: %f\n\n",
        json_get(json_get(parsed_json, "obj"), "double")->json_double);

/* Nested */
printf("nested[0].a: %s\n",
        json_get(json_item(json_get(parsed_json, "nested"), 0), "a")
            ->json_string);
printf("nested[1].a: %lld\n",
        json_get(json_item(json_get(parsed_json, "nested"), 1), "a")
            ->json_integer);
printf("nested[2].a[0]: %lld\n",
        json_item(
            json_get(json_item(json_get(parsed_json, "nested"), 2), "a"), 0)
            ->json_integer);
printf("nested[2].a[1]: %lld\n",
        json_item(
            json_get(json_item(json_get(parsed_json, "nested"), 2), "a"), 1)
            ->json_integer);
printf("nested[2].a[2]: %lld\n\n",
        json_item(
            json_get(json_item(json_get(parsed_json, "nested"), 2), "a"), 2)
            ->json_integer);

/* Nested helper */
printf("(Helper) obj.key: %s\n",
        json_get_nested(parsed_json, "obj.key")->json_string);
printf("(Helper) obj.double: %f\n",
        json_get_nested(parsed_json, "obj.double")->json_double);

printf("--------------------\n\n");

json_free(parsed_json_file);
```

Output

```output
int: 195
bool: true
double: -0.000100
some-null: null
hello: world!
obj.key: val
obj.double: 10000.000000

nested[0].a: b
nested[1].a: 69
nested[2].a[0]: 4
nested[2].a[1]: 2
nested[2].a[2]: 0

(Helper) obj.key: val
(Helper) obj.double: 10000.000000
--------------------
```

Array Example

```c
// access array
const json *array = json_get(parsed_json, "array");
assert(array->type == JSON_ARRAY);

// access array elements
for (size_t i = 0; i < array->length; i++) {
  const json *element = json_item(array, i);
  switch (element->type) {
  case JSON_INTEGER:
    printf("array[%zu]: %lld (int)\n", i, element->json_integer);
    break;
  case JSON_DOUBLE:
    printf("array[%zu]: %f (double)\n", i, element->json_double);
    break;
  case JSON_STRING:
    printf("array[%zu]: %s (string)\n", i, element->json_string);
    break;
  case JSON_BOOL:
    printf("array[%zu]: %s (bool)\n", i,
            element->json_bool ? "true" : "false");
    break;
  case JSON_NULL:
    printf("array[%zu]: null (NULL)\n", i);
    break;
  default:
    break;
  }
}
```

Output

```output
array[0]: 3 (int)
array[1]: 5.100000 (double)
array[2]: -7 (int)
array[3]: nine (string)
```

## Printing JSON
You can print JSON data using the json_print function. Here is an example:

```c
if (parsed_json) {
  json_print(parsed_json);
}
```
Output

```output
┼── int: 195 (int)
┼── array: ARRAY
┼──┼── 3 (int)
┼──┼── 5.100000 (double)
┼──┼── -7 (int)
┼──┼── nine (string)
┼── bool: true (bool)
┼── double: -0.000100 (double)
┼── null-value: NULL
┼── hello: world! (string)
┼── obj: OBJECT
┼──┼── key: val (string)
┼──┼── double: 10000.000000 (double)
┼── nested: ARRAY
┼──┼── OBJECT
┼──┼──┼── a: b (string)
┼──┼── OBJECT
┼──┼──┼── a: 69 (int)
┼──┼── OBJECT
┼──┼──┼── a: ARRAY
┼──┼──┼──┼── 4 (int)
┼──┼──┼──┼── 2 (int)
┼──┼──┼──┼── 0 (int)
```

## Freeing JSON Data
You can free up memory used by JSON data using the json_free function. Here is an example:

```c
/* PLEASE FREE TO AVOID MEMORY LEAKS */
json_free(parsed_json_file);
json_free(parsed_json);
```

## API Reference
For more details, please refer to the jsml.h header file.

```h
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

/* String Parse Functions */
const json *json_parse(char *text, json_unicode_encoder encoder);
const json *json_parse_file(char *file_path, json_unicode_encoder encoder);

/* File Parse Functions */
const json *json_parse_utf8(char *text);
const json *json_parse_file_utf8(char *file_path);

/* JSON Access Functions */
const json *json_get(const json *json,
                     const char *key); // get object's property by key
const json *json_item(const json *json, int idx); // get array element by index
const json *json_get_nested(const json *parsed_json,
                            const char *key); // get nested object/array

/* Extra Helper Functions */
char *unescape_json_string_literal(const char *str);
void json_print(const json *parsed_json);
void json_recursive_print(const json *parsed_json, int depth);
void json_free(const json *js);
```

## Examples
You can find examples of how to use this library in the examples directory.

- [example.c](examples/example.c) - Example of parsing JSON from a string literal and a file.
  - Usage: `gcc -o example examples/example.c; ./example`
- [api_example.c](examples/api_example.c) - Example of using the API functions.
  - Usage: `gcc -o api_example examples/api_example.c -lcurl; ./a.out`

## License
This project is licensed under the MIT License. See the LICENSE file for details.
