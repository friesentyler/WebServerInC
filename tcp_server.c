#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

struct server_init {
	struct sockaddr_in* server_addr;
	fd_set* readfds;
	int master_fd;
};

struct message {
	char title[100];
	char content[1000];
	struct message *next;
};

// pointer which holds message structs
struct message* messages = NULL;

char* read_html_file(char* file_path) {
	FILE *file;
	file = fopen(file_path, "r");
	if (file == NULL) {
		printf("Not able to open HTML file");
		fflush(stdout);
		char* null_exception = "<html><h1>ERROR 404</h1></html>";
		return null_exception;
	}

	fseek(file, 0L, SEEK_END);
	long int size = ftell(file);
	fseek(file, 0L, SEEK_SET);
	char* file_string = malloc(sizeof(char)*size);
	
	char* curr_line = malloc(sizeof(char)*300);
	while (fgets(curr_line, 300, file)) {
		printf("%s", curr_line);
		strcat(file_string, curr_line);
	}
	free(curr_line);

	fclose(file);
	return file_string;
}

void add_message(const char *title, const char *content) {
	struct message *msg = malloc(sizeof(struct message));
	if (!msg) {
		return;
	}
	strncpy(msg->title, title, sizeof(msg->title)-1);
	strncpy(msg->content, content, sizeof(msg->content)-1);
	msg->title[sizeof(msg->title) - 1] = '\0';
	msg->content[sizeof(msg->content) - 1] = '\0';
	msg->next = messages;
	messages = msg;
}

void handle_http_get_request(int client_fd, char* file_path) {
	char response[8192]; // Assuming a maximum response size of 8192 bytes
	if (strcmp(file_path, "messages") == 0) {
		char *body = malloc(sizeof(char)*8192);
		if (!body) {
			return;
		}
		body[0] = '\0';
		strcat(body, "[");
		struct message *msg = messages;
		while (msg != NULL) {
			char msg_buffer[1150];
			snprintf(msg_buffer, sizeof(msg_buffer), "{\"title\": \"%s\", \"content\": \"%s\"}", msg->title, msg->content);
			strcat(body, msg_buffer);
			if (msg->next != NULL) {
				strcat(body, ",");
			}
			msg = msg->next;
		}
		strcat(body, "]");
		sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n", strlen(body));
		strcat(response, body);
		//free(body);
		printf("%s", response);
		fflush(stdout);
		int bytes_sent = send(client_fd, response, strlen(response), 0);
		if (bytes_sent < 0) {
			printf("Failed to send HTTP response\n");
		}
	} else if (file_path) {
		// Construct HTTP headers
		char* body = read_html_file(file_path);
		sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n", strlen(body));
		// Append HTTP body
		strcat(response, body);


		int bytes_sent = send(client_fd, response, strlen(response), 0);
		if (bytes_sent < 0) {
			printf("Failed to send HTTP response\n");
		}
	} else {
		char* error_response = "HTTP/1.1 404 Not Found\r\nContent-Length: 13\r\nConnection: close\r\n\r\n404 Not Found";
		send(client_fd, error_response, strlen(error_response), 0);
	}
}

void handle_http_post_request(int client_fd, char *request, char *file_path) {
	if (strcmp(file_path, "messages") == 0) {
		char response[1024];
		char *body_start = strstr(request, "\r\n\r\n");
		if (body_start != NULL) {
			body_start += 4;
			char *body_copy[strlen(body_start)];
			strcpy(body_copy, body_start);


			char *title = strstr(body_start, "{\"title\":\"");
			char *content = strstr(body_start, "\"content\":\"");
			title += 10;
			content += 11;
			char *title_end = strstr(title, "\"");
			char *content_end = strstr(content, "\"");
			title_end[0] = '\0';
			content_end[0] = '\0';
			add_message(title, content);


			sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: %lu\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n", strlen(body_copy));
			strcat(response, body_copy);

			int bytes_sent = send(client_fd, response, strlen(response), 0);
			if (bytes_sent < 0) {
				printf("Failed to send HTTP response\n");
			}
		} else {
			printf("Invalid POST request\n");
		}
	}
}

void handle_http_request(int client_fd) {
	char request[2048];
	int bytes_read = read(client_fd, request, sizeof(request)-1);
	if (bytes_read < 0) {
		printf("Failed to read HTTP request\n");
		return;
	}
	request[bytes_read] = '\0';
	printf("Received HTTP request:\n%s\n", request);
	fflush(stdout);

	char method[10];
	char url[256];
	sscanf(request, "%s %s", method, url);

	if (strcmp(method, "GET") == 0) {
		char *trimmed_url = url;
        	if (trimmed_url[0] == '/') {
            		trimmed_url++;
        	}
		handle_http_get_request(client_fd, trimmed_url);
	} else if (strcmp(method, "POST") == 0) {
		char *trimmed_url = url;
        	if (trimmed_url[0] == '/') {
            		trimmed_url++;
        	}
		handle_http_post_request(client_fd, request, trimmed_url);
	} else {
		printf("Unsupported HTTP method\n");
	}
}

struct server_init* create_master_fd(int port) {
	// specify server info
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	fd_set readfds;


	// creates a master file descriptor (server that can handle client requests)
	int master_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct server_init *server = (struct server_init*)malloc(sizeof(struct server_init));
	server->server_addr = &server_addr;
	server->readfds = &readfds;
	server->master_fd = master_fd;
	return server;
}

void bind_master_fd(int master_fd, struct sockaddr* server_addr) {
	// binds the master_fd to a IP and port
	if (bind(master_fd, server_addr, sizeof(struct sockaddr)) == -1) {
		printf("failed to bind socket to address");
	} else {
		printf("master socket created with id: %d\n", master_fd);
		printf("master socket bound to port 2000\n");
	}
}

void listen_master_fd(int master_fd) {
	// sets master_fd to listen for incoming connections
	if (listen(master_fd, 5) < 0) {
		printf("listen failed\n");
	} else {
		printf("master socket listening on port 2000\n");
	}
}

int main() {
	struct server_init* server = create_master_fd(2000);
	bind_master_fd(server->master_fd, (struct sockaddr*)(server->server_addr));
	listen_master_fd(server->master_fd);

	// start the server loop
	while (1) {
		struct sockaddr_in client_addr;
		int client_fd;
		int client_addr_len = sizeof(client_addr);

		// Accept incoming connections and handles them
		client_fd = accept(server->master_fd, (struct sockaddr*)&client_addr, (socklen_t*)&client_addr_len);
		if (client_fd < 0) {
			printf("Accept() failed\n");
			continue;	
		}
		handle_http_request(client_fd);
		close(client_fd);
	}
	free(server->server_addr);
	free(server->readfds);
	free(server);
	close(server->master_fd);
	return 0;
}
