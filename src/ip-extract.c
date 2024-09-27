#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STR(A) #A

#define HEADER_SIZE 0x100

#define DEFER(code, ...)                                                      \
  do                                                                          \
    {                                                                         \
      fprintf (stderr, __VA_ARGS__);                                          \
      if (errno != 0)                                                         \
        {                                                                     \
          perror ("");                                                        \
          errno = 0;                                                          \
        }                                                                     \
      else                                                                    \
        {                                                                     \
          puts ("");                                                          \
        }                                                                     \
      ret = (code);                                                           \
      goto defer;                                                             \
    }                                                                         \
  while (0)

static const char *program = NULL;

typedef struct SystemID
{
  char hardware_identifier[16];
  char maker_id[16];

  char product_number[10];
  char product_version[6];

  char release_date[8];
  char device_information[8];

  char area_symbols[10];
  char spaces[6];

  char compatible_peripherals[16];

  char game_title[16 * 7];

  int _res1[4];

  int ip_size;
  int _res2;
  int stack_m;
  int stack_s;

  int first_read_addr;
  int first_read_size;
  int res3[2];
} SystemID;

const char *
shift (int *argc, const char ***argv)
{
  const char *arg = NULL;
  if (*argc > 0)
    {
      arg = **argv;
      *argc -= 1;
      *argv += 1;
    }
  return arg;
}

void
usage ()
{
  fprintf (stderr, "Usage:\n\t%s <filename>\n\n", program);
  fprintf (stderr, "<filename> must be the Sega Saturn ripped binary game\n");
  exit (EXIT_FAILURE);
}

int
check_version_format (const char version[6])
{
  return (version[0] == 'V' && isdigit (version[1]) && version[2] == '.'
          && isdigit (version[3]) && isdigit (version[4])
          && isdigit (version[5]));
}

int
check_date_format (const char date[8])
{
  int ret = 1;
  for (int i = 0; i < 8; ++i)
    {
      ret &= (isdigit (date[i]) != 0);
    }
  return ret;
}

int
check_sytem_id (const SystemID *system_id)
{
  int ret = 0;

  if (strncmp (system_id->hardware_identifier, "SEGA SEGASATURN ",
               sizeof (system_id->hardware_identifier))
      != 0)
    {
      DEFER (EXIT_FAILURE, "Invalid hardware identifier");
    }

  if (strncmp (system_id->maker_id, "SEGA ENTERPRISES",
               sizeof (system_id->maker_id))
      != 0)
    {
      if (strncmp (system_id->maker_id, "SEGA TP ",
                   sizeof (system_id->maker_id))
          != 0)
        {
          DEFER (EXIT_FAILURE, "Invalid maker ID");
        }
    }

  if (check_version_format (system_id->product_version) == 0)
    {
      DEFER (EXIT_FAILURE, "Invalid product version");
    }

  if (check_date_format (system_id->release_date) == 0)
    {
      DEFER (EXIT_FAILURE, "Invalid release date format");
    }

defer:
  return ret;
}

int
parse_file (const char *filename)
{
  FILE *file;
  size_t file_size;
  int ret = 0;
  SystemID system_id = { 0 };

  file = fopen (filename, "rb");
  if (file == NULL)
    {
      DEFER (EXIT_FAILURE, "Failed to open file\n");
    }

  fseek (file, 0, SEEK_SET);
  fseek (file, 0, SEEK_END);
  file_size = ftell (file);
  if (file_size < HEADER_SIZE)
    {
      DEFER (EXIT_FAILURE, "File does not respect minimum size of %x bytes",
             HEADER_SIZE);
    }

  /* skip sector synchronization + header */
  /*   |0|1|2|3|4|5|6|7|8|9|A|B|C|D|E|F|  */
  /*   ---------------------------------  */
  /*   |    synchronization    |M S F|M|  */
  /*   ---------------------------------  */
  /* MSF: Minute,Second,Frame (1B each)   */
  /* M: Mode                              */
  fseek (file, 16, SEEK_SET);

  if (fread ((void *)&system_id, sizeof (system_id), 1, file) == 0)
    {
      DEFER (EXIT_FAILURE, "Failed to read System ID");
    }

  DEFER (check_sytem_id (&system_id), "");

defer:
  if (file != NULL)
    fclose (file);
  return ret;
}

int
main (int argc, const char **argv)
{
  const char *filename;

  int return_value = 0;

  program = shift (&argc, &argv);

  filename = shift (&argc, &argv);
  if (filename == NULL)
    usage ();

  return parse_file (filename);
}
