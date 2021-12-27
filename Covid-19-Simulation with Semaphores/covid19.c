
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <semaphore.h>
#include <assert.h>
#include <time.h>
#include <sys/mman.h>
#include <signal.h>

typedef struct sharedMem
{

   int totalFileSize;
   int totalConsumedSize ;
   int lastCallingVaccinatorNum;
   pid_t lastCallingVaccinatorPid;
   int remainingCitizen;
   int numberOfVaccinators;
   int citizenPrinted;
   int nextCitizen;
   int numberOfCitizens;
   int v1;
   int v2;

} sharedMem;

int fd;
int **pipeFd;
sharedMem *bufferInfo;
int *vaccinatorInfo;
int *citizenInfo;
sem_t *mutex,*citizenDoneSem,*isFinished,*empty,*v1v2;
void nurse(int fd,int i);
int vaccinator(int i);
void citizen(int citizenNumber,int t);
//void signal_handler(int signo);
void my_sem_wait(sem_t * semaphore);
void my_sem_post(sem_t * semaphore);
void signalhandler(int signum, siginfo_t *info, void *ptr);
sig_atomic_t exitThread = 0;

int main(int argc, char *argv[]) {

  struct sigaction new_action;

  struct stat st;
  int sharedDescriptor,sharedDescriptor2,sharedDescriptor3;
  pid_t  wpid;
  mode_t modesOfSemaphores = S_IRUSR | S_IWUSR | S_IXUSR;
  int flagsOfSemaphores = O_CREAT | O_EXCL ;
  int status = 0;
  int flags = O_RDWR;
  int pid;
  char c;
  int sizeOfTheFile;
  int index = 0;
  char inputFilePath[255];
  void* sharedMemory,*sharedMemory2,*sharedMemory3;
  int numberOfNurses,numberOfVaccinators,numberOfCitizens,bufferSize,howManyTimes;
  setbuf(stdout,NULL);


  while((c = getopt(argc, argv, "n:v:c:b:t:i:")) != -1)
  {
      switch(c)
      {
          case 'n':
              if (atoi(optarg) < 2) {
                printf("Invalid Number Of Nurses Argument,program terminating\n" );
                exit(0);
              }
              numberOfNurses = atoi(optarg);
              break;
          case 'v':
              if (atoi(optarg) < 2) {
                printf("Invalid Number Of Vaccinators Argument,program terminating\n" );
                exit(0);
              }
              numberOfVaccinators = atoi(optarg);
              break;
          case 'c':
              if (atoi(optarg) < 3) {
                printf("Invalid Number Of Citizens Argument,program terminating\n" );
                exit(0);
              }
              numberOfCitizens = atoi(optarg);
              break;
          case 'b':
              bufferSize = atoi(optarg);
              break;
          case 't':
              if (atoi(optarg) < 1) {
                printf("Invalid Vaccine Times Argument,program terminating\n" );
                exit(0);
              }
              howManyTimes = atoi(optarg);
              break;
          case 'i':
              strcpy(inputFilePath,optarg);
              break;
          default:
              printf("unknown option, program terminating\n");
              return(-1);
      }
  }
  if (bufferSize < (howManyTimes * numberOfCitizens) + 1) {
    printf("Invalid Buffer Size Argument,program terminating\n" );
    exit(0);
  }


  if ((fd = open(inputFilePath,flags)) == -1){
    perror("Error while opening file\n");
    exit(0);
  }

  fstat(fd, &st);
  sizeOfTheFile = st.st_size;
  if (sizeOfTheFile-1 < 2*howManyTimes*numberOfCitizens) {
    printf("There is not enough vaccines in input file\n" );
    exit(0);
  }

  sharedDescriptor = shm_open("/midterm_buffer_info", O_RDWR | O_CREAT , S_IRWXU);  // UNLINK ETMEYI UNUTMA
	if (sharedDescriptor == -1)
	{
		perror("open");
		exit(0);
	}

  if (ftruncate(sharedDescriptor, sizeof(sharedMem)) == -1) {
    perror("ftruncate");
    exit(0);
  }


	sharedMemory = mmap(NULL, (1*sizeof(sharedMem)),(PROT_READ | PROT_EXEC | PROT_WRITE ), MAP_SHARED, sharedDescriptor, 0);
	if (sharedMemory == MAP_FAILED)
	{
		perror("mmap");
		exit(0);
	}
  bufferInfo = sharedMemory;
  bufferInfo->totalConsumedSize = 0;
  bufferInfo->totalFileSize = 2*numberOfCitizens*howManyTimes ;
  bufferInfo->remainingCitizen = numberOfCitizens;
  bufferInfo->numberOfVaccinators = numberOfVaccinators;
  bufferInfo->citizenPrinted = 0;
  bufferInfo->nextCitizen = 0;
  bufferInfo->numberOfCitizens = numberOfCitizens;
  bufferInfo->v1 = 0;
  bufferInfo->v2 = 0;

  sharedDescriptor3 = shm_open("/midterm_citizenInfo", O_RDWR | O_CREAT , S_IRWXU);  // UNLINK ETMEYI UNUTMA
	if (sharedDescriptor3 == -1)
	{
		perror("shm_open");
		exit(0);
	}

  if (ftruncate(sharedDescriptor3, (numberOfCitizens*sizeof(int))) == -1) {
    perror("ftruncate");
    exit(0);
  }

	sharedMemory3 = mmap(NULL, (numberOfCitizens*sizeof(int)),(PROT_READ | PROT_EXEC | PROT_WRITE ), MAP_SHARED, sharedDescriptor3, 0);
	if (sharedMemory3 == MAP_FAILED)
	{
		perror("mmap");
		exit(0);
	}
  citizenInfo = sharedMemory3;


  sharedDescriptor2 = shm_open("/midterm_vaccinatorInfo", O_RDWR | O_CREAT , S_IRWXU);  // UNLINK ETMEYI UNUTMA
	if (sharedDescriptor2 == -1)
	{
		perror("open");
		exit(0);
	}

  if (ftruncate(sharedDescriptor2, (2*numberOfVaccinators*sizeof(int))) == -1) {
    perror("ftruncate");
    exit(0);
  }

	sharedMemory2 = mmap(NULL, (2*numberOfVaccinators*sizeof(int)),(PROT_READ | PROT_EXEC | PROT_WRITE ), MAP_SHARED, sharedDescriptor2, 0);
	if (sharedMemory2 == MAP_FAILED)
	{
		perror("mmap");
		exit(0);
	}
  vaccinatorInfo = sharedMemory2;






  pipeFd = calloc(numberOfCitizens,sizeof(int*));
  for (int k = 0; k < numberOfCitizens; k++) {
    pipeFd[k] = calloc(2,sizeof(int));
  }

  for (int k = 0; k < numberOfCitizens; k++) {
    if(pipe(pipeFd[k]) < 0){
			printf("Error pipe creation,program terminating \n");
      exit(0);
		}
  }

  if ((mutex = sem_open("/midterm_mutex",flagsOfSemaphores,modesOfSemaphores,1)) == SEM_FAILED) {
    printf("Program could not open the semaphore named /midterm_mutex\n" );
    exit(0);
  }


  if ((citizenDoneSem = sem_open("/midterm_citizen_done",flagsOfSemaphores,modesOfSemaphores,0)) == SEM_FAILED) {
    printf("Program could not open the semaphore named /midterm_isFinished\n" );
    exit(0);
  }


  if ((isFinished = sem_open("/midterm_isFinished",flagsOfSemaphores,modesOfSemaphores,0)) == SEM_FAILED) {
    printf("Program could not open the semaphore named /midterm_isFinished\n" );
    exit(0);
  }
  if ((empty = sem_open("/midterm_empty",flagsOfSemaphores,modesOfSemaphores,bufferSize)) == SEM_FAILED) {
    printf("Program could not open the semaphore named /midterm_isFinished\n" );
    exit(0);
  }

  if ((v1v2 = sem_open("/midterm_v1v2",flagsOfSemaphores,modesOfSemaphores,0)) == SEM_FAILED) {
    printf("Program could not open the semaphore named /midterm_isFinished\n" );
    exit(0);
  }
  printf("Welcome to the GTU344 clinic. Number of citizens to vaccinate c=%d with t=%d doses.\n",numberOfCitizens,howManyTimes );
  new_action.sa_sigaction = signalhandler;
  sigemptyset(&new_action.sa_mask);
  new_action.sa_flags = SA_RESTART;

  sigaction(SIGINT, &new_action, NULL);
  for (int i = 0; i < (numberOfNurses+numberOfCitizens+numberOfVaccinators); i++) {
    switch (pid = fork()) {
      case -1:
        printf("Fork error\n" );
        exit(0);
        break;
      case 0:
        if (i < numberOfNurses) {
          nurse(fd,i+1);

        }
        else if (i >= numberOfNurses && i < numberOfNurses+numberOfCitizens) {
          citizenInfo[i-numberOfNurses] = getpid();
          citizen(i-numberOfNurses+1,howManyTimes);

        }
        if (i >= numberOfNurses+numberOfCitizens) {
          vaccinatorInfo[i-(numberOfNurses+numberOfCitizens)] = vaccinator(i-(numberOfNurses+numberOfCitizens)+1);

        }
        sem_close(mutex);
        sem_close(citizenDoneSem);
        sem_close(isFinished);
        sem_close(empty);
        sem_close(v1v2);
        for (int k = 0; k < numberOfCitizens; k++) {
          free(pipeFd[k]);
        }
        free(pipeFd);
        exit(0);
        break;
      default:
        break;

    }
  }

  while ((wpid = wait(&status)) > 0);

  index = 0;
  if (exitThread != -1) {
    printf("All citizens have been vaccinated .\n" );
  }
if (exitThread != -1) {
  while (index < numberOfVaccinators) {
    printf("Vaccinator %d (pid=%d) vaccinated %d doses. ",index+1,vaccinatorInfo[2*numberOfVaccinators-index-1],vaccinatorInfo[index]);
    index++;
  }
}
  if (exitThread != -1) {
    printf("The clinic is now closed. Stay healthy.\n");
  }
  if ( (sem_close(mutex)) < 0) {
    perror("sem_close error");
  }

  if ( (sem_close(citizenDoneSem)) < 0) {
    perror("sem_close error");
  }
  if ( (sem_close(isFinished)) < 0) {
    perror("sem_close error");
  }
  if ( (sem_close(empty)) < 0) {
    perror("sem_close error");
  }
  if ( (sem_close(v1v2)) < 0) {
    perror("sem_close error");
  }



  for (int k = 0; k < numberOfCitizens; k++) {
    free(pipeFd[k]);
  }
  free(pipeFd);
  if ( (sem_unlink("/midterm_mutex")) < 0) {
    perror("sem_unlink error");
  }

  if ( (sem_unlink("/midterm_citizen_done")) < 0) {
    perror("sem_unlink error");
  }
  if ( (sem_unlink("/midterm_isFinished")) < 0) {
    perror("sem_unlink error");
  }
  if ( (sem_unlink("/midterm_empty")) < 0) {
    perror("sem_unlink error");
  }
  if ( (sem_unlink("/midterm_v1v2")) < 0) {
    perror("sem_unlink error");
  }

  if ( (shm_unlink("/midterm_buffer_info")) < 0) {
    perror("shm_unlink error");
  }

  if ( (shm_unlink("/midterm_vaccinatorInfo")) < 0) {
    perror("shm_unlink error");
  }

  if ( (shm_unlink("/midterm_citizenInfo")) < 0) {
    perror("shm_unlink error");
  }


  if (close(fd) == -1) {
    perror("Error when closing file\n");
  }
  return 0;
}

void nurse(int fd,int i){
  char buffer[1];
  while (exitThread != -1) {
    my_sem_wait(empty);
    my_sem_wait(mutex);

    if (exitThread == -1) {
      my_sem_post(v1v2);
      my_sem_post(mutex);
    }

    if (read(fd,buffer,1) <= 0) {
      my_sem_post(mutex);
      break;
    }
    if (buffer[0] == '1') {
      if (bufferInfo->v1 < bufferInfo->v2 ) {
        my_sem_post(v1v2);
      }
      bufferInfo->v1 = bufferInfo->v1+1;
    }
    else if (buffer[0] == '2'){
      if (bufferInfo->v1 > bufferInfo->v2 ) {
        my_sem_post(v1v2);
      }
      bufferInfo->v2 = bufferInfo->v2 +1;
    }
    else{
        if (exitThread != -1) {
          printf("Nurses have carried all vaccines to the buffer, terminating.\n");
        }
        my_sem_post(mutex);
        my_sem_post(v1v2);
        return;
    }
    if (exitThread != -1) {
      printf("Nurse %d (pid=%d) has brought vaccine %s: the clinic has %d vaccine 1 and %d vaccine 2.\n",i,getpid(),buffer,bufferInfo->v1,bufferInfo->v2 );
    }
    my_sem_post(mutex);

  }
  return;
}

int vaccinator(int i){
  int returnNum = 0;
  int isFinishedNum;
  //int v1Num,v2Num;
  char *temp = "1";
  vaccinatorInfo[(2*bufferInfo->numberOfVaccinators)-(i-1)-1] = getpid();
  while (exitThread != -1){
    if (exitThread == -1) {
      for (int i = 0; i < bufferInfo->numberOfCitizens; i++) {
        close(pipeFd[i][0]);
    		if(write(pipeFd[i][1], temp, 1)<0) {  //pipe'ın icine en son totalsize'ı parentta hesaplayabilmek icin yazdi
    			perror("write-- ");
    		}

      }
      my_sem_post(empty);
      my_sem_post(mutex);
    }
    my_sem_wait(v1v2);
    my_sem_wait(mutex);

    sem_getvalue(isFinished,&isFinishedNum);

    if (isFinishedNum > 0) {
      bufferInfo->v1 = bufferInfo->v1+1;
      bufferInfo->v2 = bufferInfo->v2 +1;
      my_sem_post(mutex);
      my_sem_post(v1v2);
      for (int i = 0; i < bufferInfo->numberOfCitizens; i++) {
          close(pipeFd[i][1]);
      }
      return returnNum;
    }
    bufferInfo->v1 = bufferInfo->v1-1;
    bufferInfo->v2 = bufferInfo->v2-1;
    my_sem_post(empty);
    my_sem_post(empty);
    bufferInfo->totalConsumedSize = bufferInfo->totalConsumedSize +2;
    if (exitThread != -1) {
      printf("Vaccinator %d (pid=%d) is inviting citizen pid=%d to the clinic.\n",i,getpid(),citizenInfo[bufferInfo->nextCitizen % bufferInfo->numberOfCitizens]);
    }
    close(pipeFd[bufferInfo->nextCitizen % bufferInfo->numberOfCitizens][0]);
		if(write(pipeFd[bufferInfo->nextCitizen % bufferInfo->numberOfCitizens][1], temp, 1)<0) {
			perror("write-- ");
		}
    my_sem_wait(citizenDoneSem);

    returnNum++;
    if (bufferInfo->totalConsumedSize >= bufferInfo->totalFileSize) {
      my_sem_post(isFinished);
      bufferInfo->v1 = bufferInfo->v1+1;
      bufferInfo->v2 = bufferInfo->v2+1;
      my_sem_post(mutex);
      my_sem_post(v1v2);
      for (int i = 0; i < bufferInfo->numberOfCitizens; i++) {
          //close(pipeFd[i][1]);
          if (close(pipeFd[i][1]) < 0) {
            perror("closePipe");
          }
      }
      break;
    }
    if (exitThread == -1) {
      for (int i = 0; i < bufferInfo->numberOfCitizens; i++) {
        close(pipeFd[i][0]);
    		if(write(pipeFd[i][1], temp, 1)<0) {
    			perror("write-- ");
    		}

      }
      my_sem_post(empty);
      my_sem_post(mutex);
    }

    my_sem_post(mutex);
  }



  return returnNum;
}

void citizen(int citizenNumber,int t){
  int i = 1;
  pid_t pid;
  char temp[1];
  pid = getpid();
  while (i <= t && exitThread != -1) {

    close(pipeFd[citizenNumber-1][1]);

		if(read(pipeFd[citizenNumber-1][0], temp,1)<0) {
			perror("readPipe ");
		}
    if (exitThread != -1) {
      if (i == t) {
        bufferInfo->remainingCitizen = bufferInfo->remainingCitizen -1 ;
        printf("Citizen %d (pid=%d) is vaccinated for the %d time: the clinic has %d vaccine 1 and %d vaccine 2. The citizen is leaving. Remaining citizens to vaccinate: %d\n",citizenNumber,pid,i,bufferInfo->v1,bufferInfo->v2,bufferInfo->remainingCitizen);
      }
      else{
        printf("Citizen %d (pid=%d) is vaccinated for the %d time: the clinic has %d vaccine 1 and %d vaccine 2. \n",citizenNumber,pid,i,bufferInfo->v1,bufferInfo->v2 );
      }
    }

    bufferInfo->nextCitizen = bufferInfo->nextCitizen + 1;
    my_sem_post(citizenDoneSem);
    i++;


  }
  close(pipeFd[citizenNumber-1][0]);


  return;
}

void my_sem_wait(sem_t * semaphore) {
  if (sem_wait(semaphore) < 0) {
    perror("Sem_wait Error");
    exit(0);
  }
}

void my_sem_post(sem_t * semaphore) {
  if (sem_post(semaphore) < 0) {
    perror("Sem_post Error");
    exit(0);
  }
}

void signalhandler(int signum, siginfo_t *info, void *ptr)
{

    if (signum == SIGINT)
    {

        printf("\n\n-USER PRESSED CTRL-C,PROGRAM TERMINATING GRACEFULLY-\n\n");

        if (exitThread != -1)
        {
            exitThread = -1;
        }
    }
}
