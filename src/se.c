/*
 * se
 *
 * @licence: GPLv2
 * @author: Emilien Mantel
 * @website: http://www.debianiste.org
 * @github: https://github.com/HanXHX
 *
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#if defined(__linux__)
#include <bsd/stdlib.h>
#include <bsd/string.h>
#endif

#define OUT_COLUMNS 5
#define NORMAL_LIST 0
#define PREF_LIST 1
#define COLOR__DEFAULT CYAN
#define COLOR__SPECIAL_HOST PURPLE
#define COLOR__PREFERED_SERVERS ORANGE
#define SSH_BIN "/usr/bin/ssh"
#define TERMINAL_DEFAULT_TITLE "Terminal"
#define MAX_PREF_LETTER 26
#define HOSTNAME_LENGTH 24

/* Colors :) */
#define WHITE "\033[0m"
#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define ORANGE "\033[33m"
#define BLUE "\033[34m"
#define PURPLE "\033[35m"
#define CYAN "\033[36m"
#define GREY "\033[37m"
#define CNORMAL "\x1B[0m"


/*
 * Internal Macros
 */
#define ALLOC_FAILURE() do { fprintf(stderr, "Can't allocate memory on line %d!\n", __LINE__); exit(EXIT_FAILURE); } while(0)
#define HOP() do { printf("HOP! %d \n", __LINE__); } while(0)

typedef struct server {
	char* hostname;
	short def;
	short my;
	short pref;
	struct server* next;
} server;

typedef server* slist;
short split_domain = 0;
short hostname_length = HOSTNAME_LENGTH;

char* extract_hostname(char* hostname)
{
	char* idx = NULL;
	char* tmp_hostname = NULL;

	if(NULL == (idx = strchr(hostname, '@')))
	{
		tmp_hostname = strdup(hostname);
	}
	else
	{
		tmp_hostname = strdup(++idx);
	}

	return tmp_hostname;
}

slist pop_list_server(slist list, char* hostname)
{
	server* p_tmp = NULL;

	if(list == NULL)
		return NULL;

	if(strcmp(list->hostname, hostname) == 0)
	{
		p_tmp = list->next;
		free(list->hostname);
		free(list);
		p_tmp = pop_list_server(p_tmp, hostname);
		return p_tmp;
	}
	else
	{
		list->next = pop_list_server(list->next, hostname);
		return list;
	}
}


slist push_list_server(slist list, char* hostname, short def, short pref, short my)
{
	int cmp;
	server* element = NULL;
	server* csl = list;
	server* tmp = NULL;

	char* ch1 = NULL;
	char* ch2 = NULL;

	if(NULL == (element = malloc(sizeof(server))))
		ALLOC_FAILURE();

	element->hostname = strdup(hostname);
	element->def = def;
	element->pref = pref;
	element->my = my;
	element->next = NULL;


	// No element in list, we return the new one
	if(list == NULL)
	{
		return element;
	}

	// loop list
	while(csl != NULL)
	{
		ch1 = extract_hostname(hostname);
		ch2 = extract_hostname(csl->hostname);
		cmp = strcmp(ch1, ch2);
		free(ch1);
		free(ch2);

		if(cmp < 0)
			break;
		tmp = csl;
		csl = csl->next;
	}

	element->next = csl;

	if(tmp)
	{
		// Insert next
		tmp->next = element;
	}
	else
	{
		// Head insert
		list = element;
	}
	return list;
}

#define MAX_LENGTH_STRING 128
slist load_config(const char* ssh_config_file)
{
	slist list = NULL;
	char* hostname = NULL;
	FILE *fp = NULL;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	short max_pref = 0;

	if(getenv("HOME") == NULL)
	{
		fprintf(stderr, "Error! Can't get *HOME* environment var...\n");
		exit(EXIT_FAILURE);
	}


	if(NULL == (hostname = calloc(MAX_LENGTH_STRING, sizeof(char))))
		ALLOC_FAILURE();

	if(NULL == (fp = fopen(ssh_config_file, "r")))
	{
		fprintf(stderr, "Can't open file: %s\n", ssh_config_file);
		exit(EXIT_FAILURE);
	}

	while((read = getline(&line, &len, fp)) != -1)
	{
		// Dirty optimization :D
		if(read < 5)
			continue;

		if(read > 128)
		{
			fprintf(stderr, "Big config line size! %d read...\n", (int) read);
			exit(EXIT_FAILURE);
		}

		if(sscanf(line, "Host %s", hostname) == 1 && strcmp("*", hostname) != 0)
		{
			list = push_list_server(list, hostname, 1, 0, 0);
		}
		else if(sscanf(line, "#MYHOST %s", hostname) == 1)
		{
			list = push_list_server(list, hostname, 0, 0, 1);
		}
		else if(sscanf(line, "#PREFHOST %s", hostname) == 1)
		{
			if(max_pref < MAX_PREF_LETTER)
			{
				max_pref++;
				list = push_list_server(list, hostname, 0, 1, 0);
			}
			else
			{
				fprintf(stderr, "%d letter in alphabet...\n", MAX_PREF_LETTER);
				exit(EXIT_FAILURE);
			}
		}
		else if(sscanf(line, "#EXCLUDE %s", hostname) == 1)
		{
			list = pop_list_server(list, hostname);
		}
	}
	free(line);
	free(hostname);
	fclose(fp);

	if(list == NULL)
	{
		fprintf(stderr, "No data in config file...\n");
		exit(EXIT_FAILURE);
	}

	return list;
}


void display_host(server* s, int number, const int show_type)
{
	int i = 0;
	char* new_hostname = strdup(s->hostname);

	if(split_domain == 1)
	{
		while((char) new_hostname[i] != '\0')
		{
			if((char) new_hostname[i] == '.')
			{
				new_hostname[i] = '\0';
				break;
			}
			i++;
		}
	}

	switch(show_type)
	{
		case NORMAL_LIST:
			printf("%s%5d) %-*.*s", s->my == 1 ? COLOR__SPECIAL_HOST : COLOR__DEFAULT, number, hostname_length, hostname_length, new_hostname);
			break;
		case PREF_LIST:
			printf("%s%5s) %-*.*s", COLOR__PREFERED_SERVERS, (char*) &number, hostname_length, hostname_length, new_hostname);
			break;
		default:
			fprintf(stderr, "W00t\n");
			exit(EXIT_FAILURE);
	}

	free(new_hostname);
}

void display_list(slist list, int modulo_display)
{
	int i = 1;
	slist p_server = list;
	while(p_server != NULL)
	{
		if(p_server->def == 1 || p_server->my == 1)
		{
			display_host(p_server, i, NORMAL_LIST);
			if(i % modulo_display == 0 && p_server->next != NULL)
				printf("\n");
			i++;
		}
		p_server = p_server->next;
	}
	if(i > 1)
		printf("%s\n", CNORMAL);
}
void display_pref_list(slist list, int modulo_display)
{
	int i = 1;
	char c = 'A';
	slist p_server = list;
	while(p_server != NULL)
	{
		if(p_server->pref == 1)
		{
			display_host(p_server, c, PREF_LIST);
			if(i % modulo_display == 0 && p_server->next != NULL)
				printf("\n");
			i++;
			c++;
		}
		p_server = p_server->next;
	}
	if(i > 1)
		printf("%s\n", CNORMAL);
}

void free_list(slist list)
{
	slist tmp;
	while(list != NULL)
	{
		tmp = list->next;
		free(list->hostname);
		free(list);
		list = tmp;
	}
}

#define MAX_SS 64
char* ia_get_server(slist list, char* input)
{
	typedef struct {
		char* hostname;
		unsigned short score;
	} score_server;
	slist p_list = list;
	int i = 0, j = 0, is_root = 0, id_char = 0, id_char2 = 0;
	char* p_char = NULL;
	char* tmp = NULL;
	char server_num[4] = "";
	const char root_special_chars[] = { '-', ',', ';', ':', '!', '/', '*', '+' };

	score_server* p_ss = NULL;
	score_server ss;


	p_char = input;
	while(*p_char != '\0')
	{
		if(*p_char >= '0' && *p_char <= '9')
		{
			tmp = strndup(p_char, sizeof(char));
			strlcat(server_num, tmp, sizeof(server_num));
			free(tmp);
		}
		p_char++;
	}

	if(NULL == (p_ss = calloc(MAX_SS, sizeof(score_server))))
		ALLOC_FAILURE();

	// Check if root
	for(i = 0; i < (int) (sizeof(root_special_chars) / sizeof(char)); i++)
	{
		if(input[0] == root_special_chars[i])
		{
			is_root = 1;
			id_char++;
			break;
		}
	}

	while(p_list != NULL)
	{
		if(p_list->def == 0)
		{
			p_list = p_list->next;
			continue;
		}

		ss.score = 0;
		id_char = 0;
		id_char2 = 0;

		if(is_root)
			id_char2++;

		//TODO : I can code better :)
		if(input[id_char2] == p_list->hostname[id_char]) // first letter OK
		{
			if(NULL != strstr(p_list->hostname, server_num)) // we have the good number
			{
				ss.hostname = strdup(p_list->hostname);
				ss.score++;
				while(input[id_char2] != '\0')
				{
					if(input[id_char2] == p_list->hostname[id_char])
					{
						ss.score++;
					}
					id_char++;
					id_char2++;
				}
				p_ss[j] = ss;
				j++;
			}
		}
		p_list = p_list->next;
	}

	if(j == 0)
	{
		fprintf(stderr, "Host not found!\n");
		exit(EXIT_FAILURE);
	}

	ss.score = 0;
	ss.hostname = NULL;

	// We search the better hostname
	for(i = 0; i < j; i++)
	{
		if(ss.score < p_ss[i].score)
			ss = p_ss[i];
	}
	free(p_ss);


	if(is_root == 1)
	{
		tmp = calloc(strlen(ss.hostname) + 6, sizeof(char));
		strcpy(tmp, "root@");
		strlcat(tmp, ss.hostname, strlen(ss.hostname) + 6);
		free(ss.hostname);
		return tmp;
	}

	return ss.hostname;
}

char* scan_input(slist list, char* input)
{
	char *tmp_hostname = NULL;
	slist p = list;
	int server_id = 0, id = 1;

	// case digit
	if(sscanf(input, "%d", (int*) &server_id) == 1)
	{
		while(p != NULL)
		{
			if(p->def == 1 || p->my == 1)
			{
				if(server_id == id)
				{
					tmp_hostname = strdup(p->hostname);
					break;
				}
				id++;
			}
			p = p->next;
		}
	}
	// case 1 letter
	else if(strlen(input) == 1 && sscanf(input, "%1c", (char*) &server_id) == 1)
	{
		if((input[0] >= 'a' && input[0] <= 'z') || (input[0] >= 'A' && input[0] <= 'Z'))
		{
			id = 'a';
			while(p != NULL)
			{
				if(p->pref == 1)
				{
					if(tolower(input[0]) == id)
					{
						tmp_hostname = strdup(p->hostname);
						break;
					}
					id++;
				}
				p = p->next;
			}
		}
	}
	// case AI :p
	else
	{
		tmp_hostname = ia_get_server(list, input);
	}

	if(tmp_hostname != NULL)
	{
		return tmp_hostname;
	}

	fprintf(stderr, "Invalid input...\n");
	exit(EXIT_FAILURE);
	return NULL;
}

void terminal_title(char* title)
{
	setbuf(stdout, NULL);
	printf("\033]0;%s\007", title);
}


void ssh(const char* hostname, const char* ssh_bin, const char* ssh_config_file)
{
	char* command_arg[64];
	char terminal[64] = "";
	int i = 0, j;
	pid_t child_pid;

	command_arg[i++] = strdup("ssh");
	command_arg[i++] = strdup("-F");
	command_arg[i++] = strdup(ssh_config_file);
	command_arg[i++] = strdup(hostname);
	command_arg[i] = NULL;

	strlcat(terminal, "ssh://", sizeof(terminal));
	strlcat(terminal, hostname, sizeof(terminal));
	terminal_title(terminal);

	printf("--------------------------------------\n");
	printf("Connecting to %s%s%s\n", ORANGE, hostname, CNORMAL);
	printf("--------------------------------------\n\n");

	child_pid = fork();
	if(child_pid < 0)
	{
		fprintf(stderr, "Cannot fork!\n");
		exit(EXIT_FAILURE);
	}
	else if(child_pid == 0)
	{
		// child process
		if(execv(ssh_bin, command_arg) == -1)
		{
			fprintf(stderr, "Cannot exec SSH_BIN: %s\n", ssh_bin);
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS);
	}

	wait(NULL);

	for(j = 0; j < i; j++)
		free(command_arg[j]);

	terminal_title(TERMINAL_DEFAULT_TITLE);
}


int main(int argc, char **argv)
{
	slist list_server = NULL;
	char* hostname = NULL;
	char* input = NULL;
	int c, option_index = 0, out_columns = OUT_COLUMNS;
	char* ssh_config_file = NULL;
	char* ssh_bin = NULL;

	static struct option long_options[] = {
		{"bin", required_argument, 0, 'b'},
		{"config", required_argument, 0, 'c'},
		{"help", no_argument, 0, 'h'},
		{"out-columns", required_argument, 0, 'o'},
		{"hostname-length", required_argument, 0, 'o'},
		{"split-name", required_argument, 0, 's'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	while((c = getopt_long(argc, argv, "b:c:ho:l:sv", long_options, &option_index)) != -1)
	{
		switch(c)
		{
			case 'b':
				ssh_bin = strdup(optarg);
				break;
			case 'c':
				ssh_config_file = strdup(optarg);
				break;
			case 'h':
				printf("Usage:\n");
				printf("\tse [OPTION]...\n");
				printf("Options:\n");
				printf("\t-b, --binary BINARY\n\t\tFull path to SSH binary (default: /usr/bin/ssh)\n");
				printf("\t-c, --config config-file\n\t\tUse an alternate SSH config file (default: ~/.ssh/config)\n");
				printf("\t-h, --help\n\t\tDisplay help and exit\n");
				printf("\t-l, --hostname-length\n\t\tHostname length\n");
				printf("\t-o, --out-columns number\n\t\tNumber of columns to display (default: 5)\n");
				printf("\t-s, --split-name\n\t\tsplit domain (myhost.domain.tld -> myhost)\n");
				printf("\t-v, --version\n\t\tOutput version information and exit\n");
				exit(EXIT_FAILURE);
			case 'l':
				hostname_length = atoi(optarg);
				break;
			case 'o':
				out_columns = atoi(optarg);
				break;
			case 's':
				split_domain = 1;
				break;
			case 'v':
				printf("se %s\n", VERSION);
				exit(EXIT_FAILURE);
			default:
				exit(EXIT_FAILURE);
		}
	}

	if(ssh_config_file == NULL)
	{
		if(NULL == (ssh_config_file = calloc(MAX_LENGTH_STRING, sizeof(char))))
			ALLOC_FAILURE();
		snprintf(ssh_config_file, sizeof(char) * MAX_LENGTH_STRING, "%s/.ssh/config", getenv("HOME"));
	}

	if(ssh_bin == NULL)
		ssh_bin = strdup(SSH_BIN);

	list_server = load_config(ssh_config_file);

	display_list(list_server, out_columns);
	display_pref_list(list_server, out_columns);

	if(NULL == (input = calloc(255, sizeof(char))))
		ALLOC_FAILURE();

	if(argc > 1)
	{
		input = argv[argc - 1];
	}
	else
	{
		while(scanf("%255s", input) == 0);
	}

	hostname = scan_input(list_server, input);
	ssh(hostname, ssh_bin, ssh_config_file);

	// Valgrind loves me :)
	free(hostname);
	free(ssh_config_file);
	free(ssh_bin);
	free_list(list_server);

	return 0;
}
