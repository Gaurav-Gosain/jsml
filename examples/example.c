#include <assert.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <sys/stat.h>

#include "../jsml.h"

int main() {
  /**
   * Parse JSON file
   */
  const json *parsed_json_file = json_parse_file_utf8("examples/example.json");

  if (parsed_json_file) {
    json_print(parsed_json_file);
  }

  printf("--------------------\n\n");

  /**
   *   ```json
   *   {
   *     "int": 195,
   *     "array": [3, 5.1, -7, "nine"],
   *     "bool": true,
   *     "double": -1e-4,
   *     "null-value": null,
   *     "hello": "world!",
   *     "obj": {
   *       "key": "val",
   *       "double": 1e4
   *     },
   *     "nested": [
   *       {
   *         "a": "b"
   *       },
   *       {
   *         "a": 69
   *       },
   *       {
   *         "a": [4, 2, 0]
   *       }
   *     ]
   *   }
   *   ```
   */
  char *json_string =
      "{\"int\": 195,\"array\": [3, 5.1, -7, \"nine\"],\"bool\": "
      "true,\"double\": -1e-4,\"null-value\": null,\"hello\": "
      "\"world!\",\"obj\": {\"key\": \"val\",\"double\": 1e4},\"nested\": "
      "[{\"a\": \"b\"}, {\"a\": 69}, {\"a\": [4, 2, 0]}]}";

  const json *parsed_json =
      json_parse_utf8(unescape_json_string_literal(json_string));
  if (parsed_json) {
    json_print(parsed_json);

    printf("--------------------\n\n");

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
  }

  /* PLEASE FREE TO AVOID MEMORY LEAKS */
  json_free(parsed_json_file);
  json_free(parsed_json);

  return 0;
}
