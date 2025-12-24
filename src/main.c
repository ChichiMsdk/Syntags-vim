#define SUB_CONSOLE
#include "chihab.h"
#include "cm_io.c"
#include "cm_win32.c"

#define BEGIN_CAPACITY_BUFFER 32000

typedef struct Highlight
{
  char *buffer;
  u64  size, capacity;
} Highlight;

bool
search_string_with_sentinel(char* str, u32 l1, char sentinel, char *find, u32 l2)
{
  u32 i, j;

  i = 0;
  while (i < l1 && str[i] != sentinel)
  {
    j = 0;
    if (str[i] == find[0])
    {
      while (i < l1 && j < l2 && str[i] != sentinel && str[i] == find[j])
      { j++; i++; }
      if (j == l2) return true;
    }
    i++;
  }
  return false;
}

static inline bool
is_white_space(char c)
{
  return (c == '\t' || c == ' ' || c == '\v' || c == '\n' || c == '\r');
}

static inline void
will_it_fit(Highlight *hl, u64 added)
{
  cm_assert(hl->capacity > 0);
  if ( hl->size + added >= hl->capacity )
  {
    hl->capacity = (2 * hl->capacity) + added;
    heap_realloc_dz(sizeof(char) * hl->capacity, hl->buffer, hl->buffer);
  }
}

inline static void
highlight_buffer_fill(Highlight *hl, char *str, u64 len)
{
  will_it_fit(hl, len);
  memcpy(&hl->buffer[hl->size], str, len);
  hl->size += len;
}

bool
highlight_entry_add(Highlight *hl, char *view, u64 view_size)
{
  u64   i, l1, l2;
  char  *vim_begin, *vim_end;

  i = 0;
  vim_end   = "\\>\"\n";
  vim_begin = "syn match cType \"\\<";
  l2        = strlen(vim_end);
  l1        = strlen(vim_begin);
  highlight_buffer_fill(hl, vim_begin, l1);
  while (i < view_size)
  {
    if (is_white_space(view[i]))
    {
      highlight_buffer_fill(hl, vim_end, l2);
      break;
    }
    highlight_buffer_fill(hl, &view[i++], 1);
  }
  return true;
}

Highlight
highlight_get(char *path, char *type)
{
  u64       i, view_size, type_size;
  File      file;
  char      *temp, *view;
  Highlight highlight;

  if ( file_exist_open_map_ro(path, &file) )
  {
    printf("File %s could not be open somehow..\n", path);
    exit(EXIT_FAILURE);
  }

  heap_alloc_dz(sizeof(char) * BEGIN_CAPACITY_BUFFER, highlight.buffer);
  highlight.size     = 0;
  highlight.capacity = BEGIN_CAPACITY_BUFFER;

  i         = 0;
  view      = file.buffer.view;
  view_size = file.buffer.size;
  type_size = strlen(type);
  while (i < view_size)
  {
    /* @Note: Find the "type" */
    if ( search_string_with_sentinel(&view[i], view_size - i, '\n', type, type_size) )
    {
      highlight_entry_add(&highlight, &view[i], view_size - i);
    }
    /* @Note: Move to the next line */
    while (i < view_size && view[i] != '\n') i++;
    i++;
  }

  if ( file_close(&file) ) printf("File %s could not be closed\n", file.path);
  return highlight;
}

i32
main(i32 argc, char **argv)
{
  char      *typeref, *path, *defaut, *syntax;
  Highlight highlight;

  syntax  = "syntax.vim";
  defaut  = "C:\\vc_env\\useful_tags\\combined_tags.txt";
  typeref = "\tt\ttyperef:struct";
  path    = (argc < 2) ? defaut : argv[1];

  highlight = highlight_get(path, typeref);
  if (!file_dump(syntax, highlight.buffer, highlight.size))
    printf("Created %s and dumped content\n", syntax);

  heap_free_dz(highlight.buffer);
  return 0;
}

ENTRY
{
  i32           return_value;
  _Command_Line cl;
  
  if (!command_line_args_ansi(&cl)) 
  { 
    printf("Failed retrieving command line\n"); 
    RETURN_FROM_MAIN(EXIT_FAILURE);
  }
  return_value = main(cl.argc, cl.argv);
  RETURN_FROM_MAIN((u32) return_value);
}
