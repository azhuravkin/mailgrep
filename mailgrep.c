#include <stdio.h>
#include <stdlib.h>
#include <pcre.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define MAX_ID_LEN 32

typedef struct list {
	char		id[MAX_ID_LEN];
	char		*msg;
	struct list	*next;
} list_t;

void show_line(void);
FILE *opening(const char *);
int do_line(const char *, const char *, list_t **);
void print_list(list_t **, pcre *);
void print_other(list_t *);
void free_list(list_t *);
void usage(char *);
char separator[256];
int debug = 0;
int color = 1;

void show_line(void) {
	if (color)
		printf("\033[34;10m%s\033[0m\n", separator);
	else
		printf("%s\n", separator);
}

void free_list(list_t *head) {
	list_t *cur, *next;

	for (cur = head; cur; cur = next) {
		next = cur->next;
		free(cur->msg);
		free(cur);
	}
}

void print_other(list_t *head) {
	list_t *cur;

	for (cur = head; cur; cur = cur->next) {
		show_line();
		printf("+++ %s not completed +++\n%s", cur->id, cur->msg);

		// Если список стал пустым, рисуем последнюю строку разделителя
		if (cur->next == NULL)
			show_line();
	}
}

void print_list(list_t **head, pcre *re) {
	int ovector[8];
	list_t *cur, *prev = NULL;

	for (cur = *head; cur; cur = cur->next) {
		if (pcre_exec(re, NULL, (char *) cur->msg, strlen(cur->msg), 0, 0, ovector, 8) > 0) {
			// Показываем все записи, имеющие статус завершённых и удаляем их из списка
			show_line();
			printf("%s", cur->msg);

			if (prev == NULL)
				*head = cur->next;
			else
				prev->next = cur->next;

			free(cur->msg);
			free(cur);
		} else
			prev = cur;
	}

	// Если список стал пустым, рисуем последнюю строку разделителя
	if (*head == NULL)
		show_line();
}

int do_line(const char *file, const char *pattern, list_t **head) {
	char buffer[2][1024];
	pcre *re[3];
	const char *error;
	int erroffset;
	int ovector[8];
	char regexp[256];
	char id[MAX_ID_LEN];
	list_t *cur = NULL;
	list_t *prev = NULL;
	int exist;
	int count = 0;
	FILE *fd;

	fd = opening(file);

	if ((re[0] = pcre_compile(pattern, PCRE_CASELESS, &error, &erroffset, NULL)) == NULL)
		return -1;

	sprintf(regexp,
		"^[a-z]{3} [ 0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2} [a-z0-9\\-.]+ [a-z0-9\\-_./\\[\\]]+: "
		"([a-z0-9]{11}:|[a-z0-9]{14}:|[a-z0-9]{6}-[a-z0-9]{6}-[a-z0-9]{2}) .*$");

	if ((re[1] = pcre_compile((char *) regexp, PCRE_CASELESS, &error, &erroffset, NULL)) == NULL)
		return -1;

	sprintf(regexp, "stat=|status=|reject=|completed|rejected");

	if ((re[2] = pcre_compile((char *) regexp, PCRE_CASELESS, &error, &erroffset, NULL)) == NULL)
		return -1;

	if (debug) printf("---- Firts pass ----\n");

	while (fgets(buffer[0], sizeof(buffer[0]), fd)) {
		// Проверяем, есть ли в строке pattern
		if (pcre_exec(re[0], NULL, (char *) buffer[0], strlen(buffer[0]), 0, 0, ovector, 8) > 0) {
			if (debug) printf("\nString: %s\t=> match pattern (%s)\n", buffer[0], pattern);

			// Вычитываем из записи id письма
			if (pcre_exec(re[1], NULL, (char *) buffer[0], strlen(buffer[0]), 0, 0, ovector, 8) > 0) {
				strncpy(id, buffer[0] + ovector[2], ovector[3] - ovector[2]);
				id[ovector[3] - ovector[2]] = '\0';

				if (debug) printf("\tMessage ID: %s\n", id);

				// Смотрим, есть ли уже структура list_t, относящаяся к этому id
				for (cur = *head, exist = 0; cur; cur = cur->next) {
					if (!strcmp(cur->id, id)) {
						exist = 1;
						break;
					}
				}

				if (debug) {
					if (exist)
						printf("\tStruct (id: %s) already exist. Skeep it.\n", id);
					else
						printf("\tStruct (id: %s) not exist. Create it.\n", id);
				}

				// Если такой структуры нету - создаём и сохраняем в ней id
				if (!exist) {
					// Не слишком ли много строк совпало?
					if (count > 10000) {
						fprintf(stderr, "Too many match\n");
						return -1;
					}
					if ((cur = malloc(sizeof(list_t))) == NULL) {
						fprintf(stderr, "Error allocated memory!\n");
						return -1;
					}

					count++;

					if (*head == NULL)
						*head = cur;
					if (prev != NULL)
						prev->next = cur;

					cur->msg = NULL;
					cur->next = NULL;
					strcpy(cur->id, id);

					prev = cur;
				}
			}
		}
	} // end while

	// Если список пуст - выходим
	if (*head == NULL) {
		printf("Pattern %s not found.\n", pattern);
		return 0;
	}
	// Переходим в начало файла
	pclose(fd);
	fd = opening(file);

	if (debug) printf("---- Second pass ----\n");

	while (fgets(buffer[0], sizeof(buffer[0]), fd)) {
		// Вычитываем из записи id письма
		if (pcre_exec(re[1], NULL, (char *) buffer[0], strlen(buffer[0]), 0, 0, ovector, 8) > 0) {
			strncpy(id, buffer[0] + ovector[2], ovector[3] - ovector[2]);
			id[ovector[3] - ovector[2]] = '\0';

			if (debug) printf("\nString: %s", buffer[0]);

			// Смотрим, есть ли у нас структура с таким id
			for (cur = *head; cur; cur = cur->next) {
				// Если есть - записываем в неё строку
				if (!strcmp(cur->id, id)) {
					if (debug) printf("\tThis struct (%s) exist.\n", id);

					// Цветовое выделение искомого pattern
					if (color && (pcre_exec(re[0], NULL, (char *) buffer[0], strlen(buffer[0]), 0, 0, ovector, 8) > 0)) {
						memset(buffer[1], '\0', sizeof(buffer[1]));
						strncpy(buffer[1], buffer[0], ovector[0]);
						strcat(buffer[1], "\033[41;2m");
						strncat(buffer[1], buffer[0] + ovector[0], ovector[1] - ovector[0]);
						strcat(buffer[1], "\033[0m");
						strcat(buffer[1], buffer[0] + ovector[1]);
						strcpy(buffer[0], buffer[1]);
					}

					if (cur->msg) {
						if (debug) printf("\tReallocated memory for this entry.\n");

						if ((cur->msg = realloc(cur->msg, strlen(cur->msg) + strlen(buffer[0]) + 1)) == NULL) {
							fprintf(stderr, "Error reallocated memory!\n");
							return -1;
						}
						strcat(cur->msg, buffer[0]);
					} else {
						if (debug) printf("\tAllocated memory for this entry.\n");

						if ((cur->msg = malloc(strlen(buffer[0]) + 1)) == NULL) {
							fprintf(stderr, "Error allocated memory!\n");
							return -1;
						}
						strcpy(cur->msg, buffer[0]);
					}
					break;
				}
			}
			if (debug && cur == NULL)
				printf("\tThis struct (%s) not exist. Skeep it.\n", id);
		}
	} // end while

	if (!debug) print_list(head, re[2]);

	pclose(fd);

	return 0;
}

void usage(char *name) {
	fprintf(stderr, "Usage: %s [-cdh] <regexp> <logfile>\n", name);
	fprintf(stderr, "\t-c: no Color\n");
	fprintf(stderr, "\t-d: Debug\n");
	fprintf(stderr, "\t-h: this (Help) message\n");

	exit(1);
}

FILE *opening(const char *file) {
    FILE *fd;
    char *ext;
    char cmd[256];

    ext = strrchr(file, '.');

    if (ext && !strcmp(ext, ".gz")) {
	snprintf(cmd, sizeof(cmd), "gzip -dc %s", file);
    } else if (ext && !strcmp(ext, ".bz2")) {
	snprintf(cmd, sizeof(cmd), "bzip2 -dc %s", file);
    } else {
	snprintf(cmd, sizeof(cmd), "cat %s", file);
    }

    if (!(fd = popen(cmd, "r"))) {
	fprintf(stderr, "Error opening pipe: %s\n", cmd);
	exit(1);
    }

    return fd;
}

int main(int argc, char **argv) {
	list_t *head = NULL;
	struct winsize ws;
	int opt;

	while ((opt = getopt(argc, argv, "cdh")) != EOF) {
		switch (opt) {
		case 'c':
			color = 0;
			break;
		case 'd':
			debug = 1;
			break;
		default:
			usage(argv[0]);
			break;
		}
	}

	// Если обязательных параметра не 2
	if (argc - optind != 2)
		usage(argv[0]);

	// Формируем линию разделителя по ширине экрана
	if (ioctl(1, TIOCGWINSZ, &ws) == EOF) {
		ws.ws_col = 80;
		color = 0;
	}

	if (ws.ws_col >= sizeof(separator))
		ws.ws_col = sizeof(separator) - 1;

	memset(separator, '-', ws.ws_col);
	separator[ws.ws_col] = '\0';

	nice(19);

	if (do_line(argv[optind + 1], argv[optind], &head) < 0) {
		free_list(head);
		fprintf(stderr, "Unknown error\n");

		return 1;
	}

	if (!debug) print_other(head);
	free_list(head);

	return 0;
}
