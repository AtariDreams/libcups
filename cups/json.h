//
// JSON API definitions for CUPS.
//
// Copyright © 2022 by OpenPrinting.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#ifndef _CUPS_JSON_H_
#  define _CUPS_JSON_H_
#  include "base.h"
#  ifdef __cplusplus
extern "C" {
#  endif /* __cplusplus */


//
// Types...
//

typedef enum cups_jtype_e		// JSON node type
{
  CUPS_JTYPE_NULL,			// Null value
  CUPS_JTYPE_FALSE,			// Boolean false value
  CUPS_JTYPE_TRUE,			// Boolean true value
  CUPS_JTYPE_NUMBER,			// Number value
  CUPS_JTYPE_STRING,			// String value
  CUPS_JTYPE_ARRAY,			// Array value
  CUPS_JTYPE_OBJECT,			// Object value
  CUPS_JTYPE_KEY			// Object key (string)
} cups_jtype_t;

typedef struct _cups_json_s cups_json_t;// JSON node


//
// Functions...
//

extern void		cupsJSONDelete(cups_json_t *json) _CUPS_PUBLIC;

extern cups_json_t	*cupsJSONFind(cups_json_t *json, const char *key) _CUPS_PUBLIC;

extern cups_json_t	*cupsJSONGetChild(cups_json_t *json, size_t n) _CUPS_PUBLIC;
extern size_t		cupsJSONGetCount(cups_json_t *json) _CUPS_PUBLIC;
extern const char	*cupsJSONGetKey(cups_json_t *json) _CUPS_PUBLIC;
extern cups_json_t	*cupsJSONGetParent(cups_json_t *json) _CUPS_PUBLIC;
extern cups_json_t	*cupsJSONGetSibling(cups_json_t *json) _CUPS_PUBLIC;
extern double		cupsJSONGetNumber(cups_json_t *json) _CUPS_PUBLIC;
extern const char	*cupsJSONGetString(cups_json_t *json) _CUPS_PUBLIC;
extern cups_jtype_t	cupsJSONGetType(cups_json_t *json) _CUPS_PUBLIC;

extern cups_json_t	*cupsJSONLoadFile(const char *filename) _CUPS_PUBLIC;
extern cups_json_t	*cupsJSONLoadString(const char *s) _CUPS_PUBLIC;

extern cups_json_t	*cupsJSONNew(cups_json_t *parent, cups_json_t *after, cups_jtype_t type) _CUPS_PUBLIC;
extern cups_json_t	*cupsJSONNewKey(cups_json_t *parent, cups_json_t *after, const char *value) _CUPS_PUBLIC;
extern cups_json_t	*cupsJSONNewNumber(cups_json_t *parent, cups_json_t *after, double number) _CUPS_PUBLIC;
extern cups_json_t	*cupsJSONNewString(cups_json_t *parent, cups_json_t *after, const char *value) _CUPS_PUBLIC;

extern bool		cupsJSONSaveFile(cups_json_t *json, const char *filename) _CUPS_PUBLIC;
extern char		*cupsJSONSaveString(cups_json_t *json) _CUPS_PUBLIC;


#  ifdef __cplusplus
}
#  endif /* __cplusplus */
#endif // !_CUPS_JSON_H_
