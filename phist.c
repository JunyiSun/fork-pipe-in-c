#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>

void output_generator(char *outfile, int *length_list){
  /* generate output list*/
  if (outfile != NULL){
    FILE *output_file;
    int error;
    output_file = fopen(outfile, "w");
    if (output_file == NULL) {
    }
    fprintf(output_file,  "");
    error = fclose(output_file);
    if (error != 0) {
        fprintf(stderr, "fclose failed\n");
    }
  }

  for (int j = 1; j < 46; j++){
    if(outfile != NULL){
      FILE *output_file;
      int error;
      output_file = fopen(outfile, "a");
      if (output_file == NULL) {
          fprintf(stderr, "Error opening file\n");
      }
      fprintf(output_file,  "%d, %d\n", j, length_list[j]);
      error = fclose(output_file);
      if (error != 0) {
          fprintf(stderr, "fclose failed\n");
      }
    }
    else{
      /*print to console*/
      fprintf(stdout, "%d, %d\n", j, length_list[j]);
    }
  }
}

void file_count(char *infolder, int *c_ptr){
  /*count number of files in the folder */
  DIR *d;
  struct dirent *dir;
  d = opendir (infolder);
  if (d) {
      while ((dir = readdir(d)) != NULL) {
          if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0){
            *c_ptr = *c_ptr + 1 ;
          }
      }
      closedir (d);
  }
  else{
    fprintf(stderr, "This folder is not in the current directory\n");
    exit(1);
  }
}

void file_list_generator(char *infolder, int size, char file_list[size][100]){
  /* create an array of file names from the input folder */
  DIR *d;
  struct dirent *dir;
  int index = 0;
  d = opendir (infolder);
  if (d) {
      while ((dir = readdir(d)) != NULL) {
          if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0){
            strcpy(file_list[index], dir->d_name);
            index++ ;
          }
      }
      closedir (d);
  }
}

void length_list_generator(int *length_list, int bound){
  /*initialize length list */
  for(int i= 0; i < bound; i++){
    length_list[i] = 0;
  }
}

void path_generator(char *path, char *infolder, char *filename){
  /*create string of file path */
  path[0] = '\0';
  strcat(path, infolder);
  strcat(path, "/");
  strcat(path, filename);
}

void distribute_length_of_a_file(char *path, int *length_list){
  /*process one file : count number of occurance of different word length*/
  FILE *inputfile;
  int error;
  char line[1024];
  inputfile = fopen(path, "r");
  if (inputfile == NULL) {
      fprintf(stderr, "Error opening file\n");
  }

  while (fgets(line, 1024, inputfile) != NULL) {
      char * pch;
      char delimiter[] = " ,.-!?\"|':;+";
      // trim out white space in the end
      line[strcspn(line, "\r\n")] = '\0';  
      pch = strtok (line,delimiter);
      while (pch != NULL) {
           length_list[strlen(pch)]++;
           pch = strtok (NULL,delimiter);
      }
  }
  error = fclose(inputfile);
  if (error != 0) {
      fprintf(stderr, "fclose failed\n");
  }
}

void process_sub_length_list(int *sub_length_list, int process_num, 
  int i, int count, int per_chunk, char *infolder, char file_list[count][100]){
   length_list_generator(sub_length_list, 46);
   /* process a sub pile of files in each process */
       
       int lower, upper;
       lower = i * per_chunk;
       if(i == (process_num - 1)){
        upper = count;
       }
       else{
        upper = (i + 1) * per_chunk;
       }

       for (int n = lower ; n < upper ; n++){
          char path[200];
          path_generator(path, infolder, file_list[n]);
          distribute_length_of_a_file(path, sub_length_list);
        }
}

void starttiming(struct timeval *start_p){
  /* start recording time*/
          if ((gettimeofday(start_p, NULL)) == -1) {
              perror("gettimeofday");
              exit(1);
          }
}

void endtiming(struct timeval *start_p, struct timeval *end_p){
  /*end recording time and print to stderr */
  double timediff;
  if ((gettimeofday(end_p, NULL)) == -1) {
      perror("gettimeofday");
      exit(1);
  }

  timediff = (end_p->tv_sec - start_p->tv_sec) +
      (end_p->tv_usec - start_p->tv_usec) / 1000000.0;
  fprintf(stderr, "%.4f\n", timediff);
}


int main(int argc, char *argv[]) {
    struct timeval starttime, endtime;
    struct timeval *start_ptr = &starttime;
    struct timeval *end_ptr = &endtime;
    starttiming(start_ptr);

    int ch;
    int process_num = 1;

    char *infolder = NULL, *outfile = NULL;

    /* read in arguments */
    while ((ch = getopt(argc, argv, "d:n:o:")) != -1) {
        switch(ch) {
            case 'd':
                infolder = optarg;
                break;
            case 'n':
                process_num = atoi(optarg);
                if(process_num == 0){
                  fprintf(stderr, "Invalid process number. Please try again.\n");
                  exit(1);
                }
                break;
            case 'o':
                outfile = optarg;
                break;
            default :
                fprintf(stderr, "Usage: -d <input directory name>  -n <process number> "
                              "-o <output file name>\n");
                exit(1);
        }
    }

/*initialization of variables */
    int count = 0;
    int *count_ptr = &count;
    file_count(infolder, count_ptr);

    char file_list[count][100];
    file_list_generator(infolder, count, file_list);

    int length_list[46];
    length_list_generator(length_list, 46);

if (process_num < 2){  
  /* when process number is only 1*/
    for (int n = 0; n < count ; n++){

      char path[200];
      path_generator(path, infolder, file_list[n]);
      distribute_length_of_a_file(path, length_list);
    }
}

/* when process number is more than 1*/
else{
    if (process_num > count){
      process_num = count;
    }
    int per_chunk = count / process_num;
    
    int fd[process_num][2];
    int r;

     /* create n pipes, and also fork n children*/
    for (int i = 0; i < process_num; i++){
      if((pipe(fd[i])) == -1){
        perror("pipe");
        exit(1);
      }
    
     
      r = fork();
      
      if(r == 0){    //child process
        
        //only write, close read
        if (close(fd[i][0]) == -1) {
          perror("close");
          exit(1);
        }

        for(int child_no = 0; child_no < i; child_no++){
          close(fd[child_no][0]);
        }
       /* process each sub pile of files */
       int sub_length_list[46];
       process_sub_length_list(sub_length_list, process_num, i, count, per_chunk, infolder, file_list);
      

       int input = 0;
       for(int m = 0; m < 46; m++){
         input = sub_length_list[m];
         write(fd[i][1], &input, sizeof(int));
       }
       // close write of this pipe
       if (close(fd[i][1]) == -1) {
          perror("close");
          exit(1);
        }
       exit(0);
      }     

      else if (r < 0) {
        perror("fork");
        exit(1);
      }
       
    }
       
      for(int i= 0; i < process_num; i++){   //parent process
        int status;
        if (wait(&status) != -1) {
          if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) == 0) {
              //only read, close write  
              if (close(fd[i][1]) == -1) {
                perror("close");
                exit(1);
              }  
              /*read from the pipe, and update the main word length list*/
              int contribution = 0;
              for(int m = 0; m < 46; m++){ 
                read(fd[i][0], &contribution, sizeof(int));
                length_list[m] += contribution;
              }
              /*close read of this pipe*/
              if (close(fd[i][0]) == -1) {
                perror("close");
                exit(1);
              }
            }
            else{
              fprintf(stderr, "child process terminated pre-maturely\n");
              exit(1);
             }
           }
          else{
            fprintf(stderr, "child process did not exit\n");
            exit(1);
            }
          }
          else{
            perror("wait");
            exit(1);
          }
         
     }
  }
    
    output_generator(outfile,length_list);
    endtiming(start_ptr, end_ptr);
    return 0;
}

