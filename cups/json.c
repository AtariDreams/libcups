//
// JSON API implementation for CUPS.
//
// Copyright © 2022 by OpenPrinting.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "cups-private.h"
#include "json.h"
#include <sys/stat.h>


//
// Private types...
//

struct _cups_json_s			// JSON node
{
  cups_jtype_t	type;			// Node type
  cups_json_t	*parent,		// Parent node, if any
		*sibling;		// Next sibling node, if any
  union
  {
    cups_json_t	*child;			// First child node
    double	number;			// Number value
    char	*string;		// String value
  }		value;			// Value, if any
};


//
// Local functions...
//

static void	free_json(cups_json_t *json);


//
// 'cupsJSONDelete()' - Delete a JSON node and all of its children.
//

void
cupsJSONDelete(cups_json_t *json)	// I - JSON node
{
  cups_json_t	*child,			// Child node
		*sibling;		// Sibling node


  // Range check input...
  if (!json)
    return;

  // Remove the node from its parent...
  if (json->parent)
  {
    if ((child = json->parent->value.child) == json)
    {
      // This is the first child of the parent...
      json->parent->value.child = json->sibling;
    }
    else
    {
      // Find this node in the list of children...
      while (child)
      {
        if (child->sibling == json)
        {
          child->sibling = json->sibling;
          break;
        }

        child = child->sibling;
      }
    }
  }

  // Free the value(s)
  if (json->type == CUPS_JTYPE_ARRAY || json->type == CUPS_JTYPE_OBJECT)
  {
    for (child = json->value.child; child && child != json; child = sibling)
    {
      if ((child->type == CUPS_JTYPE_ARRAY || child->type == CUPS_JTYPE_OBJECT) && child->value.child)
      {
        // Descend into child nodes...
        sibling = child->value.child;
      }
      else if ((sibling = child->sibling) == NULL)
      {
        // No more silbings, ascend unless the parent is the original node...
	sibling = child->parent;
        free_json(child);

	while (sibling && sibling != json)
        {
	  cups_json_t *temp = sibling;	// Save the current pointer

          if (sibling->sibling)
          {
            // More siblings at this level, free the parent...
            sibling = sibling->sibling;
            free_json(temp);
            break;
          }
          else
          {
            // No more siblings, continue upwards...
            sibling = sibling->parent;
            free_json(temp);
          }
        }
      }
      else
      {
        // Free the memory for this node
        free_json(child);
      }
    }
  }

  free_json(json);
}



//
// 'cupsJSONFind()' - Find the value(s) associated with a given key.
//

cups_json_t *				// O - JSON value or `NULL`
cupsJSONFind(cups_json_t *json,		// I - JSON object node
             const char  *key)		// I - Object key
{
  cups_json_t	*current;		// Current child node


  // Range check input...
  if (!json || json->type != CUPS_JTYPE_OBJECT)
    return (NULL);

  // Search for the named key...
  for (current = json->value.child; current; current = current->sibling)
  {
    if (current->type == CUPS_JTYPE_KEY && !strcmp(key, current->value.string))
      return (current->sibling);
  }

  // If we get here there was no match...
  return (NULL);
}


//
// 'cupsJSONGetChild()' - Get the first child node of an array or object node.
//

cups_json_t *				// O - First child node or `NULL`
cupsJSONGetChild(cups_json_t *json,	// I - JSON array or object node
                 size_t      n)		// I - Child node number (starting at `0`)
{
  cups_json_t	*current;		// Current child node


  // Range check input...
  if (!json || (json->type != CUPS_JTYPE_ARRAY && json->type != CUPS_JTYPE_OBJECT))
    return (NULL);

  // Search for the Nth child...
  for (current = json->value.child; n > 0 && current; current = current->sibling)
    n --;

  // Return the child node pointer...
  return (current);
}


//
// 'cupsJSONGetCount()' - Get the number of child nodes.
//

size_t					// O - Number of child nodes
cupsJSONGetCount(cups_json_t *json)	// I - JSON array or object node
{
  cups_json_t	*current;		// Current child node
  size_t	n;			// Number of child nodes

  // Range check input...
  if (!json || (json->type != CUPS_JTYPE_ARRAY && json->type != CUPS_JTYPE_OBJECT))
    return (0);

  // Count the child nodes...
  for (current = json->value.child, n = 0; current; current = current->sibling)
    n ++;

  // Return the count...
  return (n);
}


//
// 'cupsJSONGetKey()' - Get the key string, if any.
//
// This function returns the key string for a JSON key node or `NULL` if
// the node is not a key.
//

const char *				// O - String value
cupsJSONGetKey(cups_json_t *json)	// I - JSON string node
{
  return (json && json->type == CUPS_JTYPE_KEY ? json->value.string : NULL);
}


//
// 'cupsJSONGetParent()' - Get the parent node, if any.
//

cups_json_t *				// O - Parent node or `NULL` if none
cupsJSONGetParent(cups_json_t *json)	// I - JSON node
{
  return (json ? json->parent : NULL);
}


//
// 'cupsJSONGetSibling()' - Get the next sibling node, if any.
//

cups_json_t *				// O - Sibling node or `NULL` if none
cupsJSONGetSibling(cups_json_t *json)	// I - JSON node
{
  return (json ? json->sibling : NULL);
}


//
// 'cupsJSONGetNumber()' - Get the number value, if any.
//
// This function returns the number value for a JSON number node or `0.0` if
// the node is not a number.
//

double					// O - Number value
cupsJSONGetNumber(cups_json_t *json)	// I - JSON number node
{
  return (json && json->type == CUPS_JTYPE_NUMBER ? json->value.number : 0.0);
}


//
// 'cupsJSONGetString()' - Get the string value, if any.
//
// This function returns the string value for a JSON string node or `NULL` if
// the node is not a string.
//

const char *				// O - String value
cupsJSONGetString(cups_json_t *json)	// I - JSON string node
{
  return (json && json->type == CUPS_JTYPE_STRING ? json->value.string : NULL);
}


//
// 'cupsJSONGetType()' - Get the type of a JSON node.
//

cups_jtype_t				// O - JSON node type
cupsJSONGetType(cups_json_t *json)	// I - JSON node
{
  return (json ? json->type : CUPS_JTYPE_NULL);
}


//
// 'cupsJSONLoadFile()' - Load a JSON object file.
//

cups_json_t *				// O - Root JSON object node
cupsJSONLoadFile(const char *filename)	// I - JSON filename
{
  cups_json_t	*json;			// Root JSON object node
  int		fd;			// JSON file
  struct stat	fileinfo;		// JSON file information
  char		*s;			// Allocated string containing JSON file
  ssize_t	bytes;			// Bytes read


  // Range check input...
  if (!filename)
    return (NULL);

  // Try opening the file...
  if ((fd = open(filename, O_RDONLY)) < 0)
  {
    _cupsSetError(IPP_STATUS_ERROR_INTERNAL, strerror(errno), 0);
    return (NULL);
  }
  else if (fstat(fd, &fileinfo))
  {
    _cupsSetError(IPP_STATUS_ERROR_INTERNAL, strerror(errno), 0);
    close(fd);
    return (NULL);
  }
  else if (fileinfo.st_size > 16777216)
  {
    // Don't support JSON files over 16MiB
    _cupsSetError(IPP_STATUS_ERROR_INTERNAL, _("JSON file too large."), 1);
    close(fd);
    return (NULL);
  }

  // Allocate memory for the JSON file...
  if ((s = malloc((size_t)fileinfo.st_size + 1)) == NULL)
  {
    _cupsSetError(IPP_STATUS_ERROR_INTERNAL, strerror(errno), 0);
    close(fd);
    return (NULL);
  }

  // Read the file into the allocated string...
  if ((bytes = read(fd, s, (size_t)fileinfo.st_size)) < 0)
  {
    _cupsSetError(IPP_STATUS_ERROR_INTERNAL, strerror(errno), 0);
    close(fd);
    free(s);
    return (NULL);
  }

  s[bytes] = '\0';
  close(fd);

  // Load the resulting string
  json = cupsJSONLoadString(s);

  // Free the string and return...
  free(s);

  return (json);
}


//
// 'cupsJSONLoadString()' - Load a JSON object from a string.
//

cups_json_t *				// O - Root JSON object node
cupsJSONLoadString(const char *s)	// I - JSON string
{
  cups_json_t	*json,			// Root JSON object node
		*parent,		// Current parent node
		*prev = NULL,		// Previous node
		*current;		// Current node
  size_t	count;			// Number of children
  struct lconv	*loc;			// Locale data
  static const char *sep = ",]} \n\r\t";// Separator chars


  DEBUG_printf(("cupsJSONLoadString(s=\"%s\")", s));

  // Range check input...
  if (!s || *s != '{')
  {
    DEBUG_puts("2cupsJSONLoadString: Doesn't start with '{'.");
    _cupsSetError(IPP_STATUS_ERROR_INTERNAL, _("Invalid JSON data."), 1);
    return (NULL);
  }

  // Create the root node...
  if ((json = cupsJSONNew(NULL, NULL, CUPS_JTYPE_OBJECT)) == NULL)
  {
    DEBUG_puts("2cupsJSONLoadString: Unable to create root object.");
    return (NULL);
  }

  // Parse until we get to the end...
  parent = json;
  count  = 0;
  loc    = localeconv();
  s ++;

  while (*s)
  {
    // Skip leading whitespace/separators
    while (*s && isspace(*s & 255))
      s ++;

    if (parent->type == CUPS_JTYPE_ARRAY)
    {
      // Arrays can have multiple values separated by commas and whitespace...
      if (*s == ',')
      {
	// Comma (value) separator...
	if (!parent->value.child)
	{
	  // Cannot have a comma here
          DEBUG_puts("2cupsJSONLoadString: Unexpected comma.");
	  goto invalid;
	}

	s ++;
	while (*s && isspace(*s & 255))
	  s ++;
      }
    }
    else
    {
      // Objects have colons between key and value and commas between key/value
      // pairs...
      if (*s == ',')
      {
	// Comma (value) separator...
	if (!parent->value.child || (count & 1))
	{
	  // Cannot have a comma here
          DEBUG_puts("2cupsJSONLoadString: Unexpected comma.");
	  goto invalid;
	}

	s ++;
	while (*s && isspace(*s & 255))
	  s ++;
      }
      else if (*s == ':')
      {
        if (!parent->value.child || !(count & 1))
        {
	  // Cannot have a colon here
          DEBUG_puts("2cupsJSONLoadString: Unexpected colon.");
	  goto invalid;
	}

	s ++;
	while (*s && isspace(*s & 255))
	  s ++;
      }
      else if (count & 1)
      {
        // Missing colon...
	DEBUG_puts("2cupsJSONLoadString: Missing colon.");
	goto invalid;
      }

      if (!(count & 1) && *s != '\"' && *s != '}')
      {
        // Need a key string here...
	DEBUG_puts("2cupsJSONLoadString: Missing key string.");
	goto invalid;
      }
    }

    // Parse the key/value
    if (*s == '\"')
    {
      // String
      size_t		len;		// Length of value
      const char	*start;		// Start of value
      char		*ptr;		// Pointer into string

      // Find the end of the string...
      for (s ++, start = s, len = 1; *s && *s != '\"'; s ++, len ++)
      {
        if (*s == '\\')
        {
          // Ensure escaped character is valid...
          s ++;
          if (!*s || !strchr("\"\\/bfnrtu", *s))
          {
	    DEBUG_printf(("2cupsJSONLoadString: Bad escape '\\%c'.", *s));
            goto invalid;
          }
          else if (*s == 'u' && (!isxdigit(s[1] & 255) || !isxdigit(s[2] & 255) || !isxdigit(s[3] & 255) || !isxdigit(s[4] & 255)))
          {
	    DEBUG_printf(("2cupsJSONLoadString: Bad escape '\\%s'.", s));
            goto invalid;
          }
        }
        else if ((*s & 255) < ' ')
        {
          // Control characters are not allowed in a string...
	  DEBUG_printf(("2cupsJSONLoadString: Bad control character 0x%02x in string.", *s));
          goto invalid;
        }
      }

      if (!*s)
      {
        // Missing close quote...
	DEBUG_puts("2cupsJSONLoadString: Missing close quote.");
        goto invalid;
      }

      // Allocate and copy the string over...
      if (parent->type == CUPS_JTYPE_OBJECT && !(count & 1))
        current = cupsJSONNew(parent, prev, CUPS_JTYPE_KEY);
      else
        current = cupsJSONNew(parent, prev, CUPS_JTYPE_STRING);

      if (!current)
      {
	DEBUG_puts("2cupsJSONLoadString: Unable to allocate key/string node.");
        goto error;
      }
      else if ((current->value.string = malloc(len)) == NULL)
      {
        _cupsSetError(IPP_STATUS_ERROR_INTERNAL, strerror(errno), 0);
        goto error;
      }

      for (s = start, ptr = current->value.string; *s && *s != '\"'; s ++)
      {
        if (*s == '\\')
        {
          // Copy quoted character...
          s ++;

          if (strchr("\\\"/", *s))
          {
            // Backslash, quote, or slash...
            *ptr++ = *s;
          }
          else if (*s == 'b')
          {
            // Backspace
            *ptr++ = '\b';
          }
          else if (*s == 'f')
          {
            // Formfeed
            *ptr++ = '\f';
          }
          else if (*s == 'n')
          {
            // Linefeed
            *ptr++ = '\n';
          }
          else if (*s == 'r')
          {
            // Carriage return
            *ptr++ = '\r';
          }
          else if (*s == 't')
          {
            // Tab
            *ptr++ = '\t';
          }
          else if (*s == 'u')
          {
            // Unicode character
            int ch,			// Unicode character
		digit;			// Current digit

            // Convert hex digits to a 16-bit Unicode character
            for (ch = 0, digit = 0; digit < 4; digit ++)
            {
              s ++;
              ch <<= 4;
              if (isdigit(*s))
                ch |= *s - '0';
              else
                ch |= tolower(*s) - 'a' + 10;
            }

            // Convert 16-bit Unicode character to UTF-8...
            if (ch < 0x80)
            {
              // ASCII
              *ptr++ = (char)ch;
            }
            else if (ch < 0x800)
            {
              // 2-byte UTF-8
              *ptr++ = (char)(0xc0 | (ch >> 6));
              *ptr++ = (char)(0x80 | (ch & 0x3f));
            }
            else
            {
              // 3-byte UTF-8
              *ptr++ = (char)(0xe0 | (ch >> 12));
              *ptr++ = (char)(0x80 | ((ch >> 6) & 0x3f));
              *ptr++ = (char)(0x80 | (ch & 0x3f));
            }
          }
        }
        else
        {
          // Copy literal character...
          *ptr++ = *s;
        }
      }

      *ptr = '\0';
      if (*s == '\"')
        s ++;

      count ++;
      prev = current;

      DEBUG_printf(("3cupsJSONLoadString: Added %s '%s'.", current->type == CUPS_JTYPE_KEY ? "key" : "string", current->value.string));
    }
    else if (strchr("0123456789-", *s))
    {
      // Number
      if ((current = cupsJSONNew(parent, prev, CUPS_JTYPE_NUMBER)) == NULL)
        goto error;

      current->value.number = _cupsStrScand(s, (char **)&s, loc);
      count ++;
      prev = current;

      DEBUG_printf(("3cupsJSONLoadString: Added number %g.", current->value.number));
    }
    else if (*s == '{')
    {
      // Start object
      if ((parent = cupsJSONNew(parent, prev, CUPS_JTYPE_OBJECT)) == NULL)
      {
        DEBUG_puts("2cupsJSONLoadString: Unable to allocate object.");
        goto error;
      }

      count = 0;
      prev  = NULL;
      s ++;

      DEBUG_puts("3cupsJSONLoadString: Opened object.");
    }
    else if (*s == '}')
    {
      // End object
      if (parent->type != CUPS_JTYPE_OBJECT)
      {
        // Not in an object, so this is unexpected...
        DEBUG_puts("2cupsJSONLoadString: Got '}' in an array.");
	goto invalid;
      }

      DEBUG_puts("3cupsJSONLoadString: Closed object.");

      if ((parent = parent->parent) == NULL)
        break;

      count = cupsJSONGetCount(parent);
      prev  = NULL;
      s ++;
    }
    else if (*s == '[')
    {
      // Start array
      if ((parent = cupsJSONNew(parent, prev, CUPS_JTYPE_ARRAY)) == NULL)
      {
        DEBUG_puts("2cupsJSONLoadString: Unable to allocate array.");
        goto error;
      }

      count = 0;
      prev  = NULL;
      s ++;

      DEBUG_puts("3cupsJSONLoadString: Opened array.");
    }
    else if (*s == ']')
    {
      // End array
      if (parent->type != CUPS_JTYPE_ARRAY)
      {
        // Not in an array, so this is unexpected...
        DEBUG_puts("2cupsJSONLoadString: Got ']' in an object.");
	goto invalid;
      }

      DEBUG_puts("3cupsJSONLoadString: Closed array.");

      parent = parent->parent;
      count  = cupsJSONGetCount(parent);
      prev   = NULL;
      s ++;
    }
    else if (!strncmp(s, "null", 4) && strchr(sep, s[4]))
    {
      // null value
      if ((prev = cupsJSONNew(parent, prev, CUPS_JTYPE_NULL)) == NULL)
      {
        DEBUG_puts("2cupsJSONLoadString: Unable to allocate null value.");
        goto error;
      }

      count ++;
      s += 4;

      DEBUG_puts("3cupsJSONLoadString: Added null value.");
    }
    else if (!strncmp(s, "false", 5) && strchr(sep, s[5]))
    {
      // false value
      if ((prev = cupsJSONNew(parent, prev, CUPS_JTYPE_FALSE)) == NULL)
      {
        DEBUG_puts("2cupsJSONLoadString: Unable to allocate false value.");
        goto error;
      }

      count ++;
      s += 5;

      DEBUG_puts("3cupsJSONLoadString: Added false value.");
    }
    else if (!strncmp(s, "true", 4) && strchr(sep, s[4]))
    {
      // true value
      if ((prev = cupsJSONNew(parent, prev, CUPS_JTYPE_TRUE)) == NULL)
      {
        DEBUG_puts("2cupsJSONLoadString: Unable to allocate true value.");
        goto error;
      }

      count ++;
      s += 4;

      DEBUG_puts("3cupsJSONLoadString: Added true value.");
    }
    else
    {
      // Something else we don't understand...
      DEBUG_printf(("2cupsJSONLoadString: Unexpected '%s'.", s));
      goto invalid;
    }
  }

  if (*s != '}')
  {
    DEBUG_puts("2cupsJSONLoadString: Missing '}' at end.");
    goto invalid;
  }

  DEBUG_printf(("3cupsJSONLoadString: Returning %p (%u children)", (void *)json, (unsigned)cupsJSONGetCount(json)));

  return (json);

  // If we get here we saw something we didn't understand...
  invalid:

  _cupsSetError(IPP_STATUS_ERROR_INTERNAL, _("Invalid JSON data."), 1);

  // and here if we have another error that is already recorded...
  error:

  cupsJSONDelete(json);

  DEBUG_puts("3cupsJSONLoadString: Returning NULL.");

  return (NULL);
}


//
// 'cupsJSONNew()' - Create a new JSON node.
//

cups_json_t *				// O - JSON node
cupsJSONNew(cups_json_t  *parent,	// I - Parent JSON node or `NULL` for a root node
            cups_json_t  *after,	// I - Previous sibling node or `NULL` to append to the end
            cups_jtype_t type)		// I - JSON node type
{
  cups_json_t	*node;			// JSON node


  // Range check input...
  if (parent && parent->type != CUPS_JTYPE_ARRAY && parent->type != CUPS_JTYPE_OBJECT)
    return (NULL);

  // Allocate the node...
  if ((node = calloc(1, sizeof(cups_json_t))) != NULL)
  {
    node->parent = parent;
    node->type   = type;

    if (parent)
    {
      // Add node to parent...
      cups_json_t	*current;	// Current child node

      if (after)
      {
        // Append after the specified sibling...
        node->sibling  = after->sibling;
        after->sibling = node;
      }
      else if ((current = parent->value.child) != NULL)
      {
        // Find the last child...
	while (current && current->sibling)
	  current = current->sibling;

	current->sibling = node;
      }
      else
      {
        // This is the first child...
	parent->value.child = node;
      }
    }
  }

  return (node);
}


//
// 'cupsJSONNewKey()' - Create a new JSON key node.
//

cups_json_t *				// O - JSON node
cupsJSONNewKey(cups_json_t *parent,	// I - Parent JSON node or `NULL` for a root node
	       cups_json_t  *after,	// I - Previous sibling node or `NULL` to append to the end
               const char  *value)	// I - Key string
{
  cups_json_t	*node;			// JSON node
  char		*s = strdup(value);	// Key string


  if (!s)
    return (NULL);

  if ((node = cupsJSONNew(parent, after, CUPS_JTYPE_KEY)) != NULL)
    node->value.string = s;
  else
    free(s);

  return (node);
}


//
// 'cupsJSONNewNumber()' - Create a new JSON number node.
//

cups_json_t *				// O - JSON node
cupsJSONNewNumber(cups_json_t *parent,	// I - Parent JSON node or `NULL` for a root node
		  cups_json_t  *after,	// I - Previous sibling node or `NULL` to append to the end
                  double      value)	// I - Number value
{
  cups_json_t	*node;			// JSON node


  if ((node = cupsJSONNew(parent, after, CUPS_JTYPE_NUMBER)) != NULL)
    node->value.number = value;

  return (node);
}


//
// 'cupsJSONNewString()' - Create a new JSON string node.
//

cups_json_t *				// O - JSON node
cupsJSONNewString(cups_json_t *parent,	// I - Parent JSON node or `NULL` for a root node
		  cups_json_t  *after,	// I - Previous sibling node or `NULL` to append to the end
		  const char  *value)	// I - String value
{
  cups_json_t	*node;			// JSON node
  char		*s = strdup(value);	// String value


  if (!s)
    return (NULL);

  if ((node = cupsJSONNew(parent, after, CUPS_JTYPE_STRING)) != NULL)
    node->value.string = s;
  else
    free(s);

  return (node);
}


//
// 'cupsJSONSaveFile()' - Save a JSON node tree to a file.
//

bool					// O - `true` on success, `false` on failure
cupsJSONSaveFile(cups_json_t *json,	// I - JSON root node
                 const char  *filename)	// I - JSON filename
{
  char	*s;				// JSON string
  int	fd;				// JSON file


  DEBUG_printf(("cupsJSONSaveFile(json=%p, filename=\"%s\")", (void *)json, filename));

  // Get the JSON as a string...
  if ((s = cupsJSONSaveString(json)) == NULL)
    return (false);

  // Create the file...
  if ((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0)
  {
    _cupsSetError(IPP_STATUS_ERROR_INTERNAL, strerror(errno), 0);
    free(s);
    return (false);
  }

  if (write(fd, s, strlen(s)) < 0)
  {
    _cupsSetError(IPP_STATUS_ERROR_INTERNAL, strerror(errno), 0);
    close(fd);
    unlink(filename);
    free(s);
    return (false);
  }

  close(fd);
  free(s);

  return (true);
}


//
// 'cupsJSONSaveString()' - Save a JSON node tree to a string.
//
// This function saves a JSON node tree to an allocated string.  The resulting
// string must be freed using the `free` function.
//

char *					// O - JSON string or `NULL` on error
cupsJSONSaveString(cups_json_t *json)	// I - JSON root node
{
  cups_json_t	*current;		// Current node
  size_t	length;			// Length of JSON data as a string
  char		*s,			// JSON string
		*ptr;			// Pointer into string
  const char	*value;			// Pointer into string value
  struct lconv	*loc;			// Locale data


  DEBUG_printf(("cupsJSONSaveString(json=%p)", (void *)json));

  // Range check input...
  if (!json)
  {
    _cupsSetError(IPP_STATUS_ERROR_INTERNAL, strerror(EINVAL), 0);
    DEBUG_puts("3cupsJSONSaveString: Returning NULL.");
    return (NULL);
  }

  // Figure out the necessary space needed in the string
  current = json;
  length  = 1;				// nul

  while (current)
  {
    if (current->parent && current->parent->value.child != current)
      length ++;			// Comma or colon separator

    switch (current->type)
    {
      case CUPS_JTYPE_NULL :
      case CUPS_JTYPE_TRUE :
          length += 4;
          break;

      case CUPS_JTYPE_FALSE :
          length += 5;
          break;

      case CUPS_JTYPE_ARRAY :
      case CUPS_JTYPE_OBJECT :
          length += 2;			// Brackets/braces
          break;

      case CUPS_JTYPE_NUMBER :
          length += 32;
          break;

      case CUPS_JTYPE_KEY :
      case CUPS_JTYPE_STRING :
          length += 2;			// Quotes
          for (value = current->value.string; *value; value ++)
          {
	    if (strchr("\\\"\b\f\n\r\t", *value))
	      length += 2;		// Simple escaped char
            else if ((*value & 255) < ' ')
              length += 6;		// Worst case for control char
	    else
	      length ++;		// Literal char
          }
          break;
    }

    // Get next node...
    if ((current->type == CUPS_JTYPE_ARRAY || current->type == CUPS_JTYPE_OBJECT) && current->value.child)
    {
      // Descend
      current = current->value.child;
    }
    else if (current->sibling)
    {
      // Visit silbling
      current = current->sibling;
    }
    else
    {
      // Ascend and continue...
      current = current->parent;
      while (current)
      {
        if (current->sibling)
	{
	  current = current->sibling;
	  break;
	}
	else
        {
          current = current->parent;
	}
      }
    }
  }

  DEBUG_printf(("2cupsJSONSaveString: length=%u", (unsigned)length));

  // Allocate memory and fill it up...
  if ((s = malloc(length)) == NULL)
  {
    _cupsSetError(IPP_STATUS_ERROR_INTERNAL, strerror(errno), 0);
    DEBUG_puts("3cupsJSONSaveString: Returning NULL.");
    return (NULL);
  }

  current = json;
  ptr     = s;
  loc     = localeconv();

  while (current)
  {
    if (current->parent && current->parent->value.child != current)
    {
      // Add separator
      if (current->type == CUPS_JTYPE_KEY || current->parent->type == CUPS_JTYPE_ARRAY)
        *ptr++ = ',';
      else
        *ptr++ = ':';
    }

    switch (current->type)
    {
      case CUPS_JTYPE_NULL :
          memcpy(ptr, "null", 4);
          ptr += 4;
          break;

      case CUPS_JTYPE_TRUE :
          memcpy(ptr, "true", 4);
          ptr += 4;
          break;

      case CUPS_JTYPE_FALSE :
          memcpy(ptr, "false", 5);
          ptr += 5;
          break;

      case CUPS_JTYPE_ARRAY :
          *ptr++ = '[';
          break;

      case CUPS_JTYPE_OBJECT :
          *ptr++ = '{';
          break;

      case CUPS_JTYPE_NUMBER :
          _cupsStrFormatd(ptr, s + length, current->value.number, loc);
          ptr += strlen(ptr);
          break;

      case CUPS_JTYPE_KEY :
      case CUPS_JTYPE_STRING :
          *ptr++ = '\"';

          for (value = current->value.string; *value; value ++)
          {
            // Quote/escape as needed...
	    if (*value == '\\' || *value == '\"')
	    {
	      *ptr++ = '\\';
	      *ptr++ = *value;
	    }
	    else if (*value == '\b')
	    {
	      *ptr++ = '\\';
	      *ptr++ = 'b';
	    }
	    else if (*value == '\f')
	    {
	      *ptr++ = '\\';
	      *ptr++ = 'f';
	    }
	    else if (*value == '\n')
	    {
	      *ptr++ = '\\';
	      *ptr++ = 'n';
	    }
	    else if (*value == '\r')
	    {
	      *ptr++ = '\\';
	      *ptr++ = 'r';
	    }
	    else if (*value == '\t')
	    {
	      *ptr++ = '\\';
	      *ptr++ = 't';
	    }
            if ((*value & 255) < ' ')
            {
              snprintf(ptr, length - (size_t)(ptr - s), "\\u%04x", *value);
              ptr += 6;
	    }
	    else
	    {
	      *ptr++ = *value;
	    }
          }

          *ptr++ = '\"';
          break;
    }

    // Get next node...
    if ((current->type == CUPS_JTYPE_ARRAY || current->type == CUPS_JTYPE_OBJECT) && current->value.child)
    {
      // Descend
      current = current->value.child;
    }
    else if (current->sibling)
    {
      // Visit silbling
      current = current->sibling;
    }
    else if ((current = current->parent) != NULL)
    {
      // Ascend and continue...
      if (current->type == CUPS_JTYPE_ARRAY)
        *ptr++ = ']';
      else
        *ptr++ = '}';

      while (current)
      {
        if (current->sibling)
	{
	  current = current->sibling;
	  break;
	}
	else if ((current = current->parent) != NULL)
	{
	  if (current->type == CUPS_JTYPE_ARRAY)
	    *ptr++ = ']';
	  else
	    *ptr++ = '}';
	}
      }
    }
  }

  *ptr = '\0';

  DEBUG_printf(("3cupsJSONSaveString: Returning \"%s\".", s));

  return (s);
}


//
// 'free_json()' - Free the JSON node.
//

static void
free_json(cups_json_t *json)		// I - JSON node
{
  if (json->type == CUPS_JTYPE_KEY || json->type == CUPS_JTYPE_STRING)
    free(json->value.string);

  free(json);
}

