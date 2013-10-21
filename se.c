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

#define VERSION "1.0-git"
#define OUT_COLUMNS 5
#define NORMAL_LIST 0
#define PREF_LIST 1
#define COLOR__DEFAULT CYAN
#define COLOR__SPECIAL_HOST PURPLE 
#define COLOR__PREFERED_SERVERS ORANGE
#define SSH_BIN "/usr/bin/ssh"
#define TERMINAL_DEFAULT_TITLE "Terminal"
#define MAX_PREF_LETTER 26

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
#ifdef DEBUG
#define FREE(pointer) do { if(pointer) { free(pointer); pointer = NULL; } else { fprintf(stderr, "FREE error on line %d\n", __LINE__);  }} while(0)
#else
#define FREE(pointer) do { if(pointer) { free(pointer); pointer = NULL; } } while(0)
#endif

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

char* extract_hostname(char* hostname)
{
	short i = 0;
	char* idx = NULL;
	char* tmp_hostname = NULL;

	if(NULL == (idx = strchr(hostname, '@')))
	{
		tmp_hostname = strdup(hostname);
		return tmp_hostname;
	}

	idx++; // skip '@'
	if( NULL ==  (tmp_hostname = malloc(sizeof(char) * 64)))
		ALLOC_FAILURE();
	
	while(*idx != '\0')
	{
		if(*idx == '.')
			break;
		tmp_hostname[i] = *idx;
		idx++;
		i++;
	}
	tmp_hostname[i] = '\0';
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
		FREE(list->hostname);
		FREE(list);
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
		FREE(ch1);
		FREE(ch2);

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


	if(NULL == (hostname = alloca(sizeof(char) * MAX_LENGTH_STRING)))
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
	FREE(line);
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
	char* new_hostname = NULL;
	int i = 0;

	if( NULL == (new_hostname = alloca(sizeof(s->hostname)) ) )
		ALLOC_FAILURE();

	while((char) s->hostname[i] != '\0')
	{
		if ((char) s->hostname[i] == '.')
		{
			break;
		}
		new_hostname[i] = s->hostname[i];
		i++;
	}
	new_hostname[i] = '\0';

	switch(show_type)
	{
		case NORMAL_LIST:
			printf("%s%5d) %-16.16s", s->my == 1 ? COLOR__SPECIAL_HOST : COLOR__DEFAULT, number, new_hostname);
			break;
		case PREF_LIST:
			printf("%s%5s) %-16.16s", COLOR__PREFERED_SERVERS, (char*) &number, new_hostname);
			break;
		default:
			fprintf(stderr, "W00t\n");
			exit(EXIT_FAILURE);
	}
}

void display_list(slist list)
{
	int i = 1;
	slist p_server = list;
	while(p_server != NULL)
	{
		if(p_server->def == 1 || p_server->my == 1)
		{
			display_host(p_server, i, NORMAL_LIST);
			if(i % OUT_COLUMNS == 0)
				printf("\n");
			i++;
		}
		p_server = p_server->next;
	}
	printf("%s\n", CNORMAL);
}
void display_pref_list(slist list)
{
	int i = 1;
	char c = 'A';
	slist p_server = list;
	while(p_server != NULL)
	{
		if(p_server->pref == 1)
		{
			display_host(p_server, c, PREF_LIST);
			if(i % OUT_COLUMNS == 0)
				printf("\n");
			i++;
			c++;
		}
		p_server = p_server->next;
	}
	printf("%s\n", CNORMAL);
}

void free_list(slist list)
{
	slist tmp;
	while(list != NULL)
	{
		tmp = list->next;
		FREE(list->hostname);
		FREE(list);
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
	int i = 0, j = 0, is_root = 0, id_char = 0, server_char = 0;
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
			FREE(tmp);
		}
		p_char++;
	}

	if(NULL == (p_ss = malloc(sizeof(score_server) * MAX_SS)) )
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
		//TODO : I can code better :) 
		if(input[id_char] == p_list->hostname[server_char]) // first letter OK
		{
			if(NULL != strstr(p_list->hostname, server_num ))
			{
				ss.hostname = strdup(p_list->hostname);
				ss.score++;
				while(input[id_char] != '\0' && input[id_char] == p_list->hostname[server_char])
				{
					ss.score++;
					id_char++;
					server_char++;
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

	for(i = 0; i < j; i++)
	{
		if(ss.score < p_ss[i].score)
			ss = p_ss[i];
	}
	FREE(p_ss);


	if(is_root == 1)
	{
		tmp = malloc(strlen(ss.hostname) * sizeof(char) + (sizeof(char) * 6));
		strcpy(tmp, "root@");
		strlcat(tmp, ss.hostname, strlen(ss.hostname) + 6);
		FREE(ss.hostname);
		return tmp;
	}

	return ss.hostname;
}

char* scan_input(slist list)
{
	char *input = NULL, *tmp_hostname = NULL;
	slist p = list;
	int server_id = 0, id = 1;

	if( NULL == (input = malloc( sizeof(char) * 4) )) 
		ALLOC_FAILURE();

	while(scanf("%4s", input) == 0);

	// case digit
	if(sscanf(input, "%d", (int*) &server_id ) == 1)
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
	else if(strlen(input) == 1 && sscanf(input, "%1c", (char*) &server_id ) == 1)
	{
		if((input[0] >= 'a' && input[0] <= 'z' ) || (input[0] >= 'A' && input[0] <= 'Z'))
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
		FREE(input);
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
		FREE(command_arg[j]);
	
	terminal_title(TERMINAL_DEFAULT_TITLE);
}


int main(int argc, char **argv)
{
	slist list_server = NULL;
	char* hostname = NULL;
	int c, option_index = 0;
	char* ssh_config_file = NULL;
	char* ssh_bin = NULL;

	static struct option long_options[] = {
		{"bin", required_argument, 0, 'b'},
		{"config", required_argument, 0, 'c'},
		{"help", no_argument, 0, 'h'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	while((c = getopt_long(argc, argv, "b:c:hv", long_options, &option_index)) != -1)
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
				printf("HELP\n");
				exit(EXIT_FAILURE);
			case 'v':
				printf("se %s\n", VERSION);
				exit(EXIT_FAILURE);
			default:
				abort();
		}
	}

	if(ssh_config_file == NULL)
	{
		if(NULL == (ssh_config_file = malloc(sizeof(char) * MAX_LENGTH_STRING)))
			ALLOC_FAILURE();
		snprintf(ssh_config_file, sizeof(char) * MAX_LENGTH_STRING, "%s/.ssh/config", getenv("HOME"));
	}

	if(ssh_bin == NULL)
		ssh_bin = strdup(SSH_BIN);

	list_server = load_config(ssh_config_file);

	display_list(list_server);
	printf("\n");
	display_pref_list(list_server);

	hostname = scan_input(list_server);
	ssh(hostname, ssh_bin, ssh_config_file);

	// Valgrind loves me :)
	FREE(hostname);
	FREE(ssh_config_file);
	FREE(ssh_bin);
	free_list(list_server);

	return 0;
}
