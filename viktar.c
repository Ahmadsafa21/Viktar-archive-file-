#include <stdio.h>
#include <grp.h>
#include <stdint.h>
#include <pwd.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define OPTIONS "xctTf:hv"
#define c 1
#define x 2
#define t 3
#define T 4

#include "viktar.h"


int main(int argc, char *argv[]){

	int opt = 0;
	struct passwd *pid;
	struct group *gp;
	struct tm *lt;
	int ofd = STDOUT_FILENO;
	int ifd = STDIN_FILENO;  //points to stdin
	int action = 0;
	char *fileName = NULL;
	char stuff[100];
	char out[200]; 
	char symode[200];
	
	struct stat sb;
	viktar_header_t data; 
	memset(&data, 0, sizeof(data));



	while( (opt = getopt(argc, argv, OPTIONS)) != -1) {
		switch(opt){
			case 'x':
				action = x;
				break;
			case 'c':
				action = c;
				break;
			case 't':
				action = t;
				break;
			case 'T':
				action = T;
				break;
			case 'f':
			//ask about input vs ouput
				//strcpy(fileName, optarg);
				fileName = optarg;
				break;
			case 'h':
				printf("help text\n");
				printf("./viktar\n");
				printf("Options: xctTf:hv\n");
				printf("-x      extract file/files from archive\n");
				printf("-c		create an archive file\n");
				printf("-t		display a short table of contents of the archive file\n");
				printf("-T		display a long table of contents of the archive file\n");
				printf("Only one of xctT can be specified\n");
				printf("-f filename	use filename as the archive file\n");
				printf("-v		give verbose diagnostic messages\n");
				printf("-h		display this AMAZING help message\n");
				exit(EXIT_SUCCESS);
			case 'v':
				printf("verbose enabled");
				exit(EXIT_SUCCESS);
				break;
			default:
				fprintf(stderr, "Invalid option: %c\n",opt );
				exit(EXIT_FAILURE);
				break;
		}
	}
	//ask about lseek
	
	if (action == t){
		if(fileName != NULL && strcmp(fileName, "stdin") != 0){
			ifd = open(fileName, O_RDONLY);
			if (ifd < 0){
				fprintf(stderr, "cannot open %s for input", fileName);
				exit(EXIT_FAILURE);
			}
		}
		
		if(ifd == STDIN_FILENO){
			fileName = "stdin";
		}

		printf("Contents of viktar file: \"%s\"\n", fileName);

		memset(stuff, 0, 100);
		read(ifd, stuff, strlen(VIKTAR_FILE));
		if(strncmp(stuff, VIKTAR_FILE, strlen(VIKTAR_FILE) ) != 0){
			fprintf(stderr, "invalid viktar file\n");
			exit(EXIT_FAILURE);
		}



		while(read(ifd, &data, sizeof(viktar_header_t)) > 0){
			printf("\tfile name: %s\n", data.viktar_name);
			lseek(ifd, data.st_size,SEEK_CUR);
		}
		close(ifd);
		exit(EXIT_SUCCESS);
	}
	else if(action == T){
		if(fileName != NULL && strcmp(fileName, "stdin") != 0){
			ifd = open(fileName, O_RDONLY);
			if (ifd < 0){
				fprintf(stderr, "cannot open %s for input", fileName);
				exit(EXIT_FAILURE);
			}
		}

		printf("Contents of viktar file: \"%s\"\n", fileName);

		memset(stuff, 0, 100);
		read(ifd, stuff, strlen(VIKTAR_FILE));
		if(strncmp(stuff, VIKTAR_FILE, strlen(VIKTAR_FILE) ) != 0){
			fprintf(stderr, "invalid viktar file\n");
			exit(EXIT_FAILURE);
		}

		while(read(ifd, &data, sizeof(viktar_header_t)) > 0){
			printf("\tfile name: %s\n", data.viktar_name);

			memset(symode, 0, 200);
			switch (data.st_mode & S_IFMT) {
				case S_IFDIR:  strcat(symode, "d");
							   break;
				case S_IFIFO:  strcat(symode, "p");
							   break;
				case S_IFLNK:  strcat(symode, "l");
							   break;
				case S_IFREG:  strcat(symode, "-");
							   break;
				case S_IFSOCK: strcat(symode, "s");
							   break;
				case S_IFCHR:  strcat(symode, "c");
							   break;
				case S_IFBLK:  strcat(symode, "b");
							   break;
				default:       strcat(symode, "-");
							   break;
			}
			strcat(symode, (data.st_mode & S_IRUSR) ? "r" : "-");
			strcat(symode, (data.st_mode & S_IWUSR) ? "w" : "-");
			strcat(symode, (data.st_mode & S_IXUSR) ? "x" : "-");

			strcat(symode, (data.st_mode & S_IRGRP) ? "r" : "-");
			strcat(symode, (data.st_mode & S_IWGRP) ? "w" : "-");
			strcat(symode, (data.st_mode & S_IXGRP) ? "x" : "-");

			strcat(symode, (data.st_mode & S_IROTH) ? "r" : "-");
			strcat(symode, (data.st_mode & S_IWOTH) ? "w" : "-");
			strcat(symode, (data.st_mode & S_IXOTH) ? "x" : "-");
			pid = getpwuid((uintmax_t) data.st_uid);
        	gp = getgrgid(data.st_gid);
			lt = localtime(&data.st_mtime);

			printf("\t\tmode:  %s\n", symode);
			printf("\t\tuser:  %s\n", pid->pw_name);
        	printf("\t\tgroup: %s\n", gp->gr_name); 
			printf("\t\tsize:  %ld\n", (intmax_t) data.st_size);

			strftime(out, 200, "%Y-%m-%d %X %Z", lt);
			printf("\t\tmtime: %s\n", out);

			out[0] = '\0';
			lt = localtime(&data.st_atime);
			strftime(out, 200, "%Y-%m-%d %X %Z", lt);
			printf("\t\tatime: %s\n", out);


			lseek(ifd, data.st_size,SEEK_CUR);
		}
		close(ifd);
		exit(EXIT_SUCCESS);

	}



	else if(action == c){
		size_t temp = strlen(VIKTAR_FILE);
		if(fileName != NULL) {
			ofd = open(fileName
				, O_WRONLY | O_TRUNC | O_CREAT
				, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (ofd < 0){
				fprintf(stderr, "cannot open %s for output", optarg);
				exit(EXIT_FAILURE);
			}
		}
	//    mode_t    st_mode;        /* File type and mode */
	//    uid_t     st_uid;         /* User ID of owner */
	//    gid_t     st_gid;         /* Group ID of owner */
	//    off_t     st_size;        /* Total size, in bytes */
	//
	//    struct timespec st_atim;  /* Time of last access */
	//    struct timespec st_mtim;  /* Time of last modification */
	//    struct timespec st_ctim;  /* Time of last status change */
		write(ofd, VIKTAR_FILE, temp);
		for(int i = optind; i < argc; ++i){
			char buff[100] = {'\0'};
			int bytesRad = 0;
			if (stat(argv[i], &sb) == -1) {
				perror("lstat");
				exit(EXIT_FAILURE);
        	}
			memset(&data, 0, sizeof(data) );
			strncpy(data.viktar_name, argv[i], VIKTAR_MAX_FILE_NAME_LEN);
			data.st_mode = sb.st_mode;
			data.st_uid = sb.st_uid;
			data.st_gid = sb.st_gid;
			data.st_size = sb.st_size;

			data.st_atim = sb.st_atim;
			data.st_mtim = sb.st_mtim;
			data.st_ctim = sb.st_ctim;

			write(ofd, &data, sizeof(viktar_header_t));
			

			ifd = open(argv[i], O_RDONLY);

			while((bytesRad = read(ifd, buff, 100)) != 0){
				write(ofd, buff, bytesRad);
			}
			close(ifd);
		}

			//read(ifd, &data, sizeof(viktar_header_t) );
			//write(ofd, argv[i], VIKTAR_MAX_FILE_NAME_LEN);

//			write(ofd, &sb.st_mode, sizeof(mode_t) );
//			write(ofd, &sb.st_uid, sizeof(sb.st_uid) );
//			write(ofd, &sb.st_gid, sizeof(sb.st_gid) );
//			write(ofd, &sb.st_size, sizeof() );
//
//			write(ofd, &sb.st_atim, sizeof(sb.st_atim) );
//			write(ofd, &sb.st_mtim, sizeof(sb.st_mtim) );
//			write(ofd, &sb.st_ctim, sizeof(sb.st_ctim) );
			//write(ofd, &sb, sizeof(sb));


	}

	else if(action == x){
	}







	return EXIT_SUCCESS;
}

