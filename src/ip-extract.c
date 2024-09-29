#include <ctype.h>
#include <endian.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STR(A) #A

#define HEADER_SIZE 0x100

#ifndef IS_BIG_ENDIAN
#error "No definitions to check if system is Big Endian"
#endif

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

  union
  {
    char raw[8];
    struct
    {
      char year[4];
      char month[2];
      char day[2];
    };
  } release_date;

  char device_information[8];

  char area_symbols[10];
  char _spaces[6];

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

void
reverse_bytes (void *data, size_t size)
{
  char *bytes = (char *)data;
  for (size_t i = 0; i < size / 2; ++i)
    {
      char temp = bytes[i];
      bytes[i] = bytes[size - i - 1];
      bytes[size - i - 1] = temp;
    }
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
print_regions (const char device_infos[8])
{
  static const char JP[] = "Japan";
  static const char ASIA[] = "Asia";
  static const char AMERICA[] = "America";
  static const char PAL[] = "PAL";

  int ret = 1;
  int pos = 0;
  char buffer[128] = { 0 };

  for (int i = 0; i < 8; ++i)
    {
      if (device_infos[i] == ' ')
        continue;

      if (pos != 0)
        {
          sprintf (buffer + pos, ", ");
          pos += 2;
        }

      switch (device_infos[i])
        {
        case 'J':
          sprintf (buffer + pos, JP);
          pos += sizeof (JP) - 1;
          break;

        case 'T':
          sprintf (buffer + pos, ASIA);
          pos += sizeof (ASIA) - 1;
          break;

        case 'U':
          sprintf (buffer + pos, AMERICA);
          pos += sizeof (AMERICA) - 1;
          break;

        case 'E':
          sprintf (buffer + pos, PAL);
          pos += sizeof (PAL) - 1;
          break;

        default:
          DEFER (0, "");
        }
    }
  printf ("Regions: %s\n", buffer);

defer:
  return ret;
}

int
print_compatible_peripheral (const char peripherals[16])
{
  static const char CONTROL_PAD[] = "Control Pad";
  static const char ANALOG_PAD[] = "Analog Controller";
  static const char MOUSE[] = "Mouse";
  static const char KBD[] = "Keyboard";
  static const char STEERING_PAD[] = "Steering Controller";
  static const char MULTITAP[] = "Multitap";

  int ret = 1;
  int pos = 0;
  char buffer[512] = { 0 };

  for (int i = 0; i < 16; ++i)
    {
      if (peripherals[i] == ' ')
        continue;

      if (pos != 0)
        {
          sprintf (buffer + pos, ", ");
          pos += 2;
        }

      switch (peripherals[i])
        {
        case 'J':
          sprintf (buffer + pos, CONTROL_PAD);
          pos += sizeof (CONTROL_PAD) - 1;
          break;

        case 'A':
        case 'E':
          sprintf (buffer + pos, ANALOG_PAD);
          pos += sizeof (ANALOG_PAD) - 1;
          break;

        case 'M':
          sprintf (buffer + pos, MOUSE);
          pos += sizeof (MOUSE) - 1;
          break;

        case 'K':
          sprintf (buffer + pos, KBD);
          pos += sizeof (KBD) - 1;
          break;

        case 'S':
          sprintf (buffer + pos, STEERING_PAD);
          pos += sizeof (STEERING_PAD) - 1;
          break;

        case 'T':
          sprintf (buffer + pos, MULTITAP);
          pos += sizeof (MULTITAP) - 1;
          break;

        default:
          DEFER (0, "");
        }
    }
  printf ("Compatible peripherals: %s\n", buffer);

defer:
  return ret;
}

int
check_system_id (const SystemID *system_id)
{
  static const char THIRD_PARTY[] = "SEGA TP ";

  int ret = 0;
  int cd_number, total_cds;

  if (strncmp (system_id->hardware_identifier, "SEGA SEGASATURN ",
               sizeof (system_id->hardware_identifier))
      != 0)
    {
      DEFER (EXIT_FAILURE, "Invalid hardware identifier");
    }
  printf ("Hardware identifier: %.16s\n", system_id->hardware_identifier);

  if (strncmp (system_id->maker_id, "SEGA ENTERPRISES",
               sizeof (system_id->maker_id))
      != 0)
    {
      if (strncmp (system_id->maker_id, THIRD_PARTY, sizeof (THIRD_PARTY) - 1)
          != 0)
        {
          DEFER (EXIT_FAILURE, "Invalid maker ID");
        }
    }
  printf ("Maker ID: %.16s\n", system_id->maker_id);

  if (check_version_format (system_id->product_version) == 0)
    {
      DEFER (EXIT_FAILURE, "Invalid product version");
    }
  printf ("Version: %.6s\n", system_id->product_version);

  if (check_date_format (system_id->release_date.raw) == 0)
    {
      DEFER (EXIT_FAILURE, "Invalid release date format");
    }
  printf ("Release date: %.4s/%.2s/%.2s\n", system_id->release_date.year,
          system_id->release_date.month, system_id->release_date.day);

  if (sscanf (system_id->device_information, "CD-%d/%d", &cd_number,
              &total_cds)
          != 2
      || total_cds < cd_number)
    {
      DEFER (EXIT_FAILURE, "Invalid device information format");
    }
  printf ("CD: %d/%d\n", cd_number, total_cds);

  if (print_regions (system_id->area_symbols) == 0)
    {
      DEFER (EXIT_FAILURE, "Invalid area symbols");
    }

  if (print_compatible_peripheral (system_id->compatible_peripherals) == 0)
    {
      DEFER (EXIT_FAILURE, "Invalid compatible peripherals");
    }

  if (IS_BIG_ENDIAN == 0)
    reverse_bytes ((void *)&(system_id->ip_size), sizeof (system_id->ip_size));
  printf ("IP Size %d (%x)\n", system_id->ip_size, system_id->ip_size);

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

  DEFER (check_system_id (&system_id), "");

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
