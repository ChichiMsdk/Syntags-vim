#define SUB_CONSOLE
#include "chihab.h"
#include "cm_io.c"
#include "cm_string.c"
#include "cm_win32.c"
#include "cm_proc_threads.c"

#define BEGIN_CAPACITY_BUFFER 32000

typedef struct Highlight
{
  char *buffer;
  u64  size, capacity;
} Highlight;

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
hl_buffer_fill(Highlight *hl, char *str, u64 len)
{
  will_it_fit(hl, len);
  memcpy(&hl->buffer[hl->size], str, len);
  hl->size += len;
}

bool
hl_entry_add(Highlight *hl, char *view, u64 view_size)
{
  u64   i, l1, l2;
  char  *vim_begin, *vim_end;

  i = 0;
  vim_end   = "\\>\"\n";
  vim_begin = "syn match cType \"\\<";
  l2        = strlen(vim_end);
  l1        = strlen(vim_begin);
  hl_buffer_fill(hl, vim_begin, l1);
  while (i < view_size)
  {
    if (is_white_space(view[i]))
    {
      hl_buffer_fill(hl, vim_end, l2);
      break;
    }
    hl_buffer_fill(hl, &view[i++], 1);
  }
  return true;
}

Highlight
hl_get(char *path, char *type)
{
  u64       i, view_size, type_size;
  File      file;
  char      *temp, *view;
  Highlight hl;

  if ( file_exist_open_map_ro(path, &file) )
  {
    printf("File %s could not be open somehow..\n", path);
    exit(EXIT_FAILURE);
  }

  heap_alloc_dz(sizeof(char) * BEGIN_CAPACITY_BUFFER, hl.buffer);
  hl.size     = 0;
  hl.capacity = BEGIN_CAPACITY_BUFFER;

  i         = 0;
  view      = file.buffer.view;
  view_size = file.buffer.size;
  type_size = strlen(type);
  while (i < view_size)
  {
    /* @Note: Find the "type" */
    if ( search_string_with_sentinel(&view[i], view_size - i, '\n', type, type_size) )
    {
      hl_entry_add(&hl, &view[i], view_size - i);
    }
    /* @Note: Move to the next line */
    while (i < view_size && view[i] != '\n') i++;
    i++;
  }

  if ( file_close(&file) ) printf("File %s could not be closed\n", file.path);
  return hl;
}

bool
print_process_name(PROCESSENTRY32 entry, void *data)
{
  printf("%s\n", entry.szExeFile);
  return true;
}

i32
main(i32 argc, char **argv)
{
  char      *typeref, *path, *defaut, *syntax;
  Highlight hl;

  syntax  = "syntax.vim";
  defaut  = "C:\\vc_env\\useful_tags\\combined_tags.txt";
  typeref = "\tt\ttyperef:struct";
  path    = (argc < 2) ? defaut : argv[1];

  process_list_do(print_process_name, NULL);

  hl = hl_get(path, typeref);
  if (!file_dump(syntax, hl.buffer, hl.size))
    printf("Created %s and dumped content\n", syntax);

  heap_free_dz(hl.buffer);
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
