#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#ifdef _WIN32
// windows
#else
// posix
//#include <unistd.h>
#endif

#ifdef _WIN32
/* Simple POSIX-like getline replacement for Windows */
ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
	if (!lineptr || !n || !stream) return -1;

	if (*lineptr == NULL || *n == 0) {
		*n = 128;
		*lineptr = malloc(*n);
		if (!*lineptr) return -1;
	}

	size_t pos = 0;

	for (;;) {
		int c = fgetc(stream);

		if (c == EOF) {
			if (pos == 0) return -1;
			break;
		}

		if (pos + 1 >= *n) {
			size_t new_size = (*n) * 2;
			char *new_ptr = realloc(*lineptr, new_size);
			if (!new_ptr) return -1;

			*lineptr = new_ptr;
			*n = new_size;
		}

		(*lineptr)[pos++] = (char)c;

		if (c == '\n')
			break;
	}

	(*lineptr)[pos] = '\0';
	return (ssize_t)pos;
}
#endif

int g_argc;
char** g_args;
bool verbose=0;

#define VERSION "1.0.1"

// Basic colored logging macros
#define LOG_ERROR(msg, ...)   fprintf(stderr, "\x1b[31m[ERROR]\x1b[0m " msg "\n", ##__VA_ARGS__)    // Red
#define LOG_WARNING(msg, ...) fprintf(stderr, "\x1b[33m[WARNING]\x1b[0m " msg "\n", ##__VA_ARGS__)  // Yellow
#define LOG_SUCCESS(msg, ...) fprintf(stdout, "\x1b[32m[SUCCESS]\x1b[0m " msg "\n", ##__VA_ARGS__) // Green
#define LOG_INFO(msg, ...)    fprintf(stdout, "\x1b[36m[INFO]\x1b[0m " msg "\n", ##__VA_ARGS__)    // Cyan/blue
#define LOG_STATUS(msg, ...)  fprintf(stdout, "\x1b[34m[STATUS]\x1b[0m " msg "\n", ##__VA_ARGS__)  // Blue

// COD TYPES
typedef struct {
	uint8_t h;
	uint8_t m;
	uint8_t s;
	uint16_t ms;
} t_time;

typedef int64_t t_timems;

// COD TIME
typedef t_timems sub_time;
typedef struct {
	sub_time from;
	sub_time to;
} sub_timespan;

typedef struct {
	int index;
	sub_timespan span;
	char* text;
} sub_line;

t_timems convert_time_to_ms(t_time* data) {
	return data->ms + data->s*1000 + data->m*1000*60 + data->h*1000*60*60;
}

t_time convert_ms_to_time(t_timems data) {
    t_time span;
    span.h  = data / 3600000;      data %= 3600000;  // 1000*60*60
    span.m  = data / 60000;        data %= 60000;    // 1000*60
    span.s  = data / 1000;         data %= 1000;
    span.ms = data;
    return span;
}

#define START_SIZE 1024

typedef enum {
	om_unknown,
	om_copy,
	om_remove,
	om_offset,
	om_linear_offset
} operation_mode;

// PARAM
int param_get_index(char* name) {
	char** aEnum=g_args;
	int index=0;
	while (*aEnum) {
		if (strcmp(*aEnum, name) == 0)
			return index;
		index++;
		aEnum++;
	}
	return -1;
}
bool param_exists(char* name) {
	char** aEnum=g_args;
	while (*aEnum) {
		if (strcmp(*aEnum, name) == 0)
			return 1;
		aEnum++;
	}
	return 0;
}
char* param_get_value(char* name) {
	char** aEnum=g_args;
	while (*aEnum) {
		if (strcmp(*aEnum, name) == 0) {
			if (*(aEnum+1))
				return *(aEnum+1);
			return NULL;
		}
		aEnum++;
	}
	return NULL;
}

// STR ARRAY
bool array_str_contains(char** array, int size, char* subject) {
	for (int i=0; i<size; i++)
		if (strcmp(subject, array[i]) == 0)
			return true;
	return false;
}
bool narray_str_contains(char** array, char* subject) {
	while (*array) {
		if (strcmp(subject, *array) == 0)
			return true;
		array++;
	}
	return false;
}

// STR
void str_toupper(char* subject) {
	while (*subject) {
		*subject = toupper((unsigned char)*subject);
		subject++;
	}
}
void str_ntoupper(char* subject, int n) {
	while (*subject && n > 0) {
		*subject = toupper((unsigned char)*subject);
		subject++;
		n--;
	}
}

void str_tolower(char* subject) {
	while (*subject) {
		*subject = tolower((unsigned char)*subject);
		subject++;
	}
}
void str_ntolower(char* subject, int n) {
	while (*subject && n > 0) {
		*subject = tolower((unsigned char)*subject);
		subject++;
		n--;
	}
}

bool str_contains_char(char* haystack, char needle) {
	while (*haystack) {
		if (*haystack == needle) return true;
		haystack++;
	}
	return false;
}

bool str_contains(char* haystack, char* needle) {
	return strstr(haystack, needle) != NULL;
}

bool str_starts_with(const char* haystack, const char* needle) {
    while (*needle) {
        if (*haystack != *needle) return false;
        haystack++;
        needle++;
    }
    return true;
}

bool str_ends_with(const char* haystack, const char* needle) {
    size_t hlen = strlen(haystack);
    size_t nlen = strlen(needle);
    if (nlen > hlen) return false;      // needle longer than haystack → can't match
    return strcmp(haystack + hlen - nlen, needle) == 0;
}

// CHAR
bool char_in_set(char needle, char* haystack) {
	while (*haystack) {
		if (*haystack == needle) return true;
		haystack++;
	}
	return false;
}

// STR
#define TRIM_SYMBOLS "\n\t" // cannot contain \0 :P

void str_trim_left(char* subject) {
    if (!subject) return;

    char* start = subject;

    while (*start && char_in_set(*start, TRIM_SYMBOLS))
        start++;

    if (start != subject)
        memmove(subject, start, strlen(start) + 1);
}

void str_trim_right(char* subject) {
	if (!*subject) return; // empty ahh
	char* reference = subject;
	subject += strlen(subject)-1;
	do {
		if (char_in_set(*subject, TRIM_SYMBOLS))
			*subject = '\0';
		else
			break;
		subject--;
	} while (subject >= reference);
}

// APP
void print_header() {
	printf("Codrut's Subtitle Editor\n");
	printf("----------------------\n");
	printf("Copyright (c) Codrut Software\n");
}
void print_usage() {
	printf("Usage:\n");
	printf(" --help, -h\n");
	printf("    display help information and commands\n");
	printf(" --usage\n");
	printf("    display commands and examples\n");
	printf(" -version, -v\n");
	printf("    show version information\n");
	printf(" --verbose\n");
	printf("    show verbose info\n");
	printf("\n");
	printf(" -i, --in <file path>\n");
	printf("    file to read from\n");
	printf(" -o, --out <file path>\n");
	printf("    file to write to\n");
	printf("    if not provided, will overwrite input file\n");
	printf(" -y, --yes\n");
	printf("    do not ask for confirmations\n");
	printf("\n");
	printf("OPERATION MODES:\n");
	printf(" -oc, --operation-copy\n");
	printf("    copy subtitle to destination (DEFAULT)\n");
	printf(" -or, --operation-remove\n");
	printf("    remove subtitle range\n");
	printf(" -oo, --operation-offset\n");
	printf("    offset subtitles in range\n");
	printf(" -olo, --operation-linear-offset\n");
	printf("    offset subtitle range linearly based on the time value\n");
	printf("\n");
	printf("DATA PARAMETERS:\n");
	printf(" RANGE:\n");
	printf(" --from <time>\n");
	printf(" --to <time>\n");
	printf(" --from-index <integer>\n");
	printf(" --to-index <integer>\n");
	printf(" --range-full\n");
	printf("\n");
	printf(" --offset <time>\n");
	printf("\n");
	printf("    used only for linear offset");
	printf(" --offset-from <time>\n");
	printf(" --offset-to <time>\n");
	
	printf("\n");
	printf("OUTPUT:\n");
	printf(" -nix, --no-rebuild-index\n");
	printf("    do not re-build the index of each subtitle after modification\n");
	printf(" --fix-order\n");
	printf("    sort the data read from the input file to ensure indexes are cronological\n");
	printf("    this would only need to be done if a file is not conformed to the SRT standard\n");
	printf("\n");
	printf("UTILS:\n");
	printf(" --backup\n");
	printf("    creates a backup in the temp folder\n");
	printf("\n");
	printf("Accepted data types:\n");
	printf(" <time>:\n");
	printf("   X.Xs - seconds (float)\n");
	printf("   X.Xm - minutes (float)\n");
	printf("   X.Xh - hours (float)\n");
	printf("   hh:mm\n");
	printf("   hh:mm:ss\n");
	printf("   hh:mm:ss,zz - time\n");
}
void print_help() {
	print_header();
	printf("\n");
	print_usage();
}
void print_examples() {
	print_header();
	printf("\n");
	print_usage();
	printf("\n");
	printf("Examples:\n");
	printf("\teditstr --help\n");
	printf("\teditstr -i movie.srt -o out.srt\n");
	printf("\t\tThis creates a copy of the subtitles\n");
	printf("\teditstr -i movie.srt -o out.srt --operation-offset --offset 12s\n");
	printf("\t\tThis offsets the entire subtitle track by 12 seconds\n");
	
}
void print_version() {
	print_header();
	printf("\n");
	printf("Version "VERSION"\n");
}

bool ask_confirm(char* prompt, bool defaultToNone, bool defaultToInvalid) {
	printf("\x1b[1;34m::\x1b[0m %s [%c/%c] ", prompt, 
		defaultToNone ? 'Y' : 'y', 
		defaultToNone ? 'n' : 'N'
		);
		
	size_t n;
	int char_cnt;
	char *result = NULL;
	char_cnt = getline(&result, &n, stdin);
	str_trim_right(result);
	char_cnt = strlen(result);
	
	switch (char_cnt) {
		//
		case 0: return defaultToNone;
		
		// Y/N
		case 1: {
			if (*result == 'y' || *result == 'Y') {
				free(result);
				return true;
			}
			if (*result == 'n' || *result == 'N') {
				free(result);
				return false;
			}
		} break;

		// NO
		case 2: {
			str_toupper(result);
			if (strcmp(result, "NO") == 0) {
				free(result);
				return false;
			}
		} break;

		// YES
		case 3: {
			str_toupper(result);
			printf("Read '%s'", result);
			if (strcmp(result, "YES") == 0) {
				free(result);
				return true;
			}
		}
	}
	free(result);
	return defaultToInvalid;
}
bool ask_confirm_standard(char* prompt, bool defaultToNone) {
	return ask_confirm(prompt, defaultToNone, false);
}

bool try_read(sub_line* to, FILE* f) {
	int index;
	int r;

	// Read index
	while ((r=fscanf(f, "%d", &index)) != 1) {
		if (r == EOF) {
				if (verbose) LOG_INFO("Reached EOF for input file"); // VB
				return false;
		}

		//
		if (verbose) printf("Failed to read index on line\n"); // VB
		fgetc(f);
	}
	//if (verbose) printf("Read dialogue index %d\n", index); // VB

	to->index = index;

	// Read spans
	t_time s_from, s_to;
	if (fscanf(f, "%hhu:%hhu:%hhu,%hu --> %hhu:%hhu:%hhu,%hu", 
		&s_from.h, &s_from.m, &s_from.s, &s_from.ms,
		&s_to.h, &s_to.m, &s_to.s, &s_to.ms
		) != 8) {
		if (verbose) printf("Failed to read time span\n"); // VB
	}
	fgetc(f); // fix stupid glitch

	to->span.from = convert_time_to_ms(&s_from);
	to->span.to = convert_time_to_ms(&s_to);

	// Read dialogue untill blank line
	char* s = NULL;
	size_t b;
	int read_cnt;

	// 
	size_t text_len = 0;
	size_t text_capacity = 128;
	to->text = malloc(text_capacity);
	if (!to->text) exit(1);
	
	while ((read_cnt = getline(&s, &b, f)) != -1) {
	    while (text_len + read_cnt + 1 >= text_capacity) {
	        text_capacity *= 2;
	        to->text = realloc(to->text, text_capacity);
	        if (!to->text) exit(1);
	    }
	
	    // Remove trailing newline
	    while (read_cnt > 0 && (s[read_cnt-1]=='\n'||s[read_cnt-1]=='\r'))
	        read_cnt--;
	
	    if (read_cnt == 0) break; // blank line
	
	    memcpy(to->text + text_len, s, read_cnt);
	    text_len += read_cnt;
	
	    to->text[text_len++] = '\n';
	}
	
	if (text_len > 0) text_len--; // remove last '\n'
	to->text[text_len] = 0;
	
	// free
	free(s);
	s = NULL;

	return true;
}


void sub_sort(sub_line *v, int n) {
	int s,i;
	sub_line aux;
	do{
		s = 0;
		for (i = 1; i < n; i++)
		{      
			if (v[i - 1].index > v[i].index)
			{             
				aux = v[i - 1];
				v[i - 1] = v[i];
				v[i] = aux;
				s = 1;
			}
		}
	} while (s);
}


bool strparsetime(char* source, t_timems* dest) {
    char* ptr = source;
    *dest = 0;

    // Skip leading spaces
    while (isspace(*ptr)) ptr++;

    // Check for hh:mm[:ss][,ms] or mm:ss[,ms]
    char* sep = strchr(ptr, ':');
    if (sep) {
        long h = 0, m = 0, s = 0, ms = 0;

        char* next = NULL;
        h = strtol(ptr, &next, 10);
        if (*next != ':') {
            h = 0; // hh missing, so treat as mm:ss
            next = ptr;
        } else {
            ptr = next + 1;
        }

        m = strtol(ptr, &next, 10);
        ptr = next;

        if (*ptr == ':') {
            ptr++; // skip colon
            s = strtol(ptr, &next, 10);
            ptr = next;
        } else {
            s = 0;
        }

        if (*ptr == ',' || *ptr == '.') {
            ptr++; // skip , or .
            ms = strtol(ptr, &next, 10);
            // Normalize ms to 0-999
            if (ms < 10) ms *= 100;
            else if (ms < 100) ms *= 10;
            ptr = next;
        }

        *dest = h*3600000 + m*60000 + s*1000 + ms;
        return true;
    }

    // Check for float with unit: X.Xs, X.Xm, X.Xh
    char* end;
    double val = strtod(ptr, &end);
    if (end == ptr) return false; // no number

    while (isspace(*end)) end++;

    switch (*end) {
        case 's': *dest = (t_timems)(val * 1000); break;
        case 'm': *dest = (t_timems)(val * 60000); break;
        case 'h': *dest = (t_timems)(val * 3600000); break;
        default:  return false; // unknown unit
    }

    return true;
}

void sub_delete_range(sub_line* list, unsigned int* count, unsigned int aFrom, unsigned int aTo) {
    if (!count || *count == 0 || aFrom > aTo || aTo >= *count) return;

    unsigned int deleteCount = aTo - aFrom + 1;

    // Free the text in the range
    for (unsigned int i = aFrom; i <= aTo; i++)
        free(list[i].text);

    // Shift the remaining elements down
    for (unsigned int i = aTo + 1; i < *count; i++) {
        list[i - deleteCount] = list[i];
    }

    // Update count
    *count -= deleteCount;
}

#ifndef _WIN32
void create_backup(const char* fn_in) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);

    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);

    char backup_path[512];
    snprintf(backup_path, sizeof(backup_path), "/tmp/srt_%s.bak", timestamp);

    FILE* src = fopen(fn_in, "rb");
    if (!src) {
        LOG_ERROR("Failed to open source file for backup.");
        return;
    }

    FILE* dst = fopen(backup_path, "wb");
    if (!dst) {
        fclose(src);
        LOG_ERROR("Failed to create backup file.");
        return;
    }

    char buffer[8192];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, n, dst);
    }

    fclose(src);
    fclose(dst);

    LOG_STATUS("Backup created: %s", backup_path);
}
#endif


// MAIN
int main(int argc, char** argv) {
	g_argc = argc;
	g_args = argv;
	char* knownParameters[] = {
	    "--help",
	    "--usage",
	    "--version",
	    "--verbose",
	    
	    "--in",
	    "--out",
	    "--yes",
	    
	    "--operation-copy",
	    "--operation-remove",
	    "--operation-offset",
	    "--operation-linear-offset",
	    
	    "--from",
	    "--to",
	    "--from-index",
	    "--to-index",
	    "--range-full",
	    
	    "--offset",
	    "--offset-from",
	    "--offset-to",
	    
	    "--no-rebuild-index",
	    "--fix-order",
	    
	    "--backup",

	    "-h",
	    "-v",

	    "-i",
	    "-o",
	    "-y",

	    "-oc",
	    "-or",
	    "-oo",
	    "-olo"
	    ,
	    "-nix",

	    NULL
	};

	// Santize parameters
	for (int i=1; i<argc; i++) {
		// Is param? --X \\ -X, non digit
		if (!str_starts_with(argv[i], "--") && !(strlen(argv[i]) >= 2 && argv[i][0] == '-' && !isdigit(argv[i][1])))
			continue;

		// Check
		if (!narray_str_contains(knownParameters, argv[i])) {
			LOG_ERROR("Unrecognized parameter \"%s\". Call with --help for usage", argv[i]);
			exit(0x02);
		}
	}

	// Read options
	bool optionYesDefault =  param_exists("--yes") || param_exists("-y");
	bool optionNoRebuildIndex =  param_exists("--no-rebuild-index") || param_exists("-nix");
	bool optionFixOrder =  param_exists("--fix-order");
	bool optionBackup =  param_exists("--backup");
	verbose = param_exists("--verbose");

	// Information operation mode
	if (param_exists("--help") || param_exists("-h")) {
		print_help();
		exit(0);
	}
	if (param_exists("--usage")) {
		print_examples();
		exit(0);
	}
	if (param_exists("--version") || param_exists("-v")) {
		print_version();
		exit(0);
	}

	// Get file paths
	char* fn_in;
	char* fn_out;
	if ((fn_in = param_get_value("--in")) == NULL)
		 fn_in = param_get_value("-i");
	if (!fn_in) {
		LOG_ERROR("Missing input file. Try --help for help");
		exit(0x01);
	}
	if ((fn_out = param_get_value("--out")) == NULL)
		if ((fn_out = param_get_value("-o")) == NULL) {
			if (!optionYesDefault && !ask_confirm_standard("Do you want to use the input file as the outpus as well?", false))
				exit(0);
			fn_out = fn_in; // no need to use strdup, these are fromn argv, thus are not freed in main()
			LOG_INFO("Using input file as output as well");
		}
	
	// IN
	FILE* f_in;
	if (verbose) LOG_STATUS("Opening input file \"%s\"", fn_in); // VB
	if ((f_in = fopen(fn_in, "r")) == NULL) {
		perror("Input buffer error");
		exit(0xB1);
	}

	// Reading
	uint32_t data_buffsize=START_SIZE;
	uint32_t data_count=0;
	sub_line* data=malloc(data_buffsize * sizeof(sub_line));
	
	while (try_read(&data[data_count], f_in))  {
		data_count++;

		// Re-alloc
		if (data_count >= data_buffsize) {
			data_buffsize *= 2;
			sub_line* prev=data;
			data = realloc(data, data_buffsize * sizeof(sub_line));

			// Failure
			if (!data) {
				free(prev);
				fclose(f_in);
				perror("Buffer allocation error");
				exit(0xA1);
			}
		}
	}

	if (verbose) LOG_INFO("Successfully read %d lines of dialogue", data_count); // VB

	// Reading - Done
	if (verbose) LOG_SUCCESS("Closing input file \"%s\"", fn_in); // VB
	fclose(f_in);

	// Sort
	if (verbose) printf("⚙️ Sorting to fix order%s\n", !optionFixOrder ? " (SKIPPED)" : "..."); // VB
	if (optionFixOrder) {
		sub_sort(data, data_count);
	}

	// Operation
	operation_mode operation=om_copy;
	// 0 -> copy
	// 1 -> remove
	// 2 -> offset
	// 3 -> linear offset
	if (param_exists("-or") || param_exists("--operation-remove")) operation = om_remove;
	if (param_exists("-oo") || param_exists("--operation-offset")) operation = om_offset;
	if (param_exists("-olo") || param_exists("--operation-linear-offset")) operation = om_linear_offset;
	if (verbose) {
		printf("Selected operation: ");
		switch (operation) {
			case om_copy: printf("COPY"); break;
			case om_remove: printf("REMOVE"); break;
			case om_offset: printf("OFFSET"); break;
			case om_linear_offset: printf("LINEAR OFFSET"); break;

			default: printf("UNKNOWN"); break;
		}
		printf("\n");
	}

	// Range
	int32_t range_from=-1;
	int32_t range_to=-1;
	if (operation != om_copy) {
		char* s;

		if (param_exists("--range-full")) {
			range_from = 0;
			range_to = data_count-1;
		}

		// FROM
		s = param_get_value("--from");
		if (s) {
			t_timems ms;

			if (!strparsetime(s, &ms)) {
				free(data);
				
				LOG_ERROR("Invalid time format");
				exit(0xE2);
			}
			range_from = 0;
			while ((uint32_t)range_from < data_count && data[range_from].span.to < ms)
				range_from++;
		}
		s = param_get_value("--from-index");
		if (s) {
			range_from = atol(s);
			if (range_from == 0 || abs(range_from-1) >= data_count) {
				free(data);
				
				LOG_ERROR("Invalid from range");
				exit(0xE1);
			}
			range_from--; // make internal index
		}
	
		// TO
		s = param_get_value("--to");
		if (s) {
			t_timems ms;

			if (!strparsetime(s, &ms)) {
				free(data);
				
				LOG_ERROR("Invalid time format");
				exit(0xE2);
			}
			range_to = data_count-1;
			while (range_to > 0 && data[range_to].span.from > ms)
				range_to--;
		}
		s = param_get_value("--to-index");
		if (s) {
			range_to = atol(s);
			if (range_to == 0 || abs(range_to-1) >= data_count) {
				free(data);
				
				LOG_ERROR("Invalid to range");
				exit(0xE1);
			}
			range_to--; // make internal index
		}

		// ASK
		if (range_from == -1) {
			if (!optionYesDefault && !ask_confirm_standard("No start range specified. Start from beggining of file?", true))
				exit(0);
			range_from = 0;
		}
		if (range_to == -1) {
			if (!optionYesDefault && !ask_confirm_standard("No destination range specified. Finish at the from end of file?", true))
				exit(0);
			range_to = data_count-1;
		}
		if (range_to < range_from) {
			free(data);
		
			LOG_ERROR("Invalid range. Destination cannot have an smaller index than the start (%d > %d)", range_from, range_to);
			exit(0xE1);
		}

		// VB
		if (verbose) printf("Range:\nFrom\tTo\n%d\t%d\n\n", 
			range_from+1,
			range_to+1); // VB
		if (verbose) printf("Blocks:\nTotal\tModify\tUnchanged\n%d\t%d\t%d\n\n", 
			data_count,
			range_to-range_from+1,
			data_count-(range_to-range_from+1)); // VB
	}

	// Offset
	t_timems offset=0;
	
	if (operation == om_offset) {
		char* s;
		
		// FROM
		s = param_get_value("--offset");
		if (!s) {
			free(data);
			
			LOG_ERROR("Missing offset value");
			exit(0xE7);
		}
		if (!strparsetime(s, &offset)) {
			free(data);
			
			LOG_ERROR("Invalid time format");
			exit(0xE2);
		}

		// Verbose
		if (verbose) {
			t_time t = convert_ms_to_time(offset);

			printf("Offset:\t%02hhu:%02hhu:%02hhu,%03hu\n", 
					t.h, t.m, t.s, t.ms); // VB
		}
	}

	// Linear
	t_timems linearoffset_from=-1;
	t_timems linearoffset_to=-1;
	t_timems linearoffset_cmp_subtract;
	t_timems linearoffset_cmp_divisor;

	if (operation == om_linear_offset) {
		char* s;
		
		// FROM
		s = param_get_value("--offset-from");
		if (!s) {
			free(data);
			
			LOG_ERROR("Missing linear offset from");
			exit(0xE7);
		}
		if (!strparsetime(s, &linearoffset_from)) {
			free(data);
			
			LOG_ERROR("Invalid time format");
			exit(0xE2);
		}

		// TO
		s = param_get_value("--offset-to");
		if (!s) {
			free(data);
			
			LOG_ERROR("Missing linear offset to");
			exit(0xE7);
		}
		if (!strparsetime(s, &linearoffset_to)) {
			free(data);
			
			LOG_ERROR("Invalid time format");
			exit(0xE2);
		}

		// Divisor and subtractor
		linearoffset_cmp_subtract = data[range_from].span.from;
		linearoffset_cmp_divisor = data[range_to].span.from - data[range_from].span.from; // use .from instead of .to as ".from" is used for the percentage calculation
		if (linearoffset_cmp_divisor == 0) {
		    LOG_ERROR("Range start and end have same timestamp, linear progression will not work.");
		    exit(0xE3);
		}

		// Verbose
		if (verbose) {
			t_time t_from = convert_ms_to_time(linearoffset_from);
			t_time t_to = convert_ms_to_time(linearoffset_to);

			printf("Linear offset:\nStart\t\tEnd\t\tSegment divisor\n%02hhu:%02hhu:%02hhu,%03hu\t%02hhu:%02hhu:%02hhu,%03hu\t%d\n\n", 
					t_from.h, t_from.m, t_from.s, t_from.ms,
					t_to.h, t_to.m, t_to.s, t_to.ms,
					data_count-(range_to-range_from+1)); // VB
		}
	}

	// OPERATION
	switch (operation) {
		case om_copy: break;
		
		case om_remove: {
			int deleteCount = range_to-range_from+1;
			if (verbose) printf("[OPERATION] Removing %d items\n", deleteCount);

			sub_delete_range(data, &data_count, range_from, range_to);
		} break;

		case om_offset: {
			if (verbose) printf("[OPERATION] Offseting %d items\n", range_to-range_from+1);

			for (int i=range_from; i<=range_to; i++) {
				data[i].span.from += offset;
				data[i].span.to += offset;
				
				if (data[i].span.from < 0) data[i].span.from = 0;
				if (data[i].span.to   < 0) data[i].span.to   = 0;
			}
		} break;

		case om_linear_offset: {
			if (verbose) printf("[OPERATION] Offseting progressively %d items\n", range_to-range_from+1);

			for (int i=range_from; i<=range_to; i++) {
				t_timems off = linearoffset_from + 
					(linearoffset_to-linearoffset_from) * (data[i].span.from-linearoffset_cmp_subtract) / linearoffset_cmp_divisor;

				data[i].span.from += off;
				data[i].span.to += off;
							
				if (data[i].span.from < 0) data[i].span.from = 0;
				if (data[i].span.to   < 0) data[i].span.to   = 0;						
			}
		} break;

		default: {
			LOG_ERROR("Unknown operation mode.");
			exit(0xFF);
		}
	}

	// Re-build index
	if (verbose) printf("⚙️ Rebuilding index%s\n", optionNoRebuildIndex ? " (SKIPPED)" : "..."); // VB
	if (!optionNoRebuildIndex)
		for (uint32_t i=0; i<data_count; i++)
			data[i].index = i+1;

	// Backup
	if (optionBackup) {
		#ifdef _WIN32
		LOG_ERROR("Backup not supported on Windows.")
		#else
		LOG_STATUS("Creating backup...");
		create_backup(fn_in);
		#endif
	}

	// OUT
	FILE* f_out;
	if (verbose) LOG_STATUS("Opening output file \"%s\"", fn_out); // VB
	if ((f_out = fopen(fn_out, "w")) == NULL) {
		free(data);

		perror("Output buffer error");
		exit(0xB2);
	}
	
	// Writing
	for (uint32_t i=0; i<data_count; i++) {
		fprintf(f_out, "%d\n", data[i].index);

		t_time s_from, s_to;
		s_from = convert_ms_to_time(data[i].span.from);
		s_to = convert_ms_to_time(data[i].span.to);

		fprintf(
			f_out, "%02hhu:%02hhu:%02hhu,%03hu --> %02hhu:%02hhu:%02hhu,%03hu\n", 
			s_from.h, s_from.m, s_from.s, s_from.ms,
			s_to.h, s_to.m, s_to.s, s_to.ms
			);
		
		fprintf(f_out, "%s\n\n", data[i].text);
	}
	
	// Writing - Done
	fclose(f_out);
	if (verbose) LOG_SUCCESS("Closing output file \"%s\"", fn_out); // VB

	// All done, cleanup operation
	for (uint32_t i = 0; i < data_count; i++)
	    free(data[i].text);
	free(data);
}
