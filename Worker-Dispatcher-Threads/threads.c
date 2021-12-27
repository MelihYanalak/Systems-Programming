
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
#include <pthread.h>


typedef struct students{
  char name[256];
  int quality;
  int speed;
  int price;
  int isSleeping;
  sem_t *studentSemaphore;
  int moneyMade;
  int homeworkDone;

}students;
int totalHomeworkNumber;
int workerThreadNumber = 0;
int money ;
pthread_t producerThreadId,mainThreadId,*studentThreadIds;
int fd,fd2;
int **pipeFd;
sig_atomic_t exitThread = 0;
students *studentInfo;
sem_t *mutex,*studentDoneSem,*isFinished,*full,*empty,*mutex2,*awakeStudents;
char bufferGlobal[100];
int bufferSizeGlobal = 0;
void tasiTaragiTopla();
void *producerThread();
void mainThread();
void *studentThread(void* studentIndex);
int chooseBestStudent(char homeworkType);
void my_sem_wait(sem_t * semaphore);
void my_sem_post(sem_t * semaphore);
void signalhandler(int signum, siginfo_t *info, void *ptr);
char* itoa(int val, int base);
int minCost;
int main(int argc, char *argv[]) {
  struct stat st;
  struct sigaction new_action;
  mode_t modesOfSemaphores = S_IRUSR | S_IWUSR | S_IXUSR;
  int flagsOfSemaphores = O_CREAT | O_EXCL ;
  int flags = O_RDWR;
  char c;
  FILE *fp;
  char * line = NULL;
  char* splitted;
  size_t len = 0;
  setbuf(stdout,NULL);



  if (argv[1] == NULL || argv[2] == NULL || argv[3] == NULL) {
    printf("INVALID ARGUMENTS,PROGRAM TERMINATING\n" );
    exit(0);
  }

  new_action.sa_sigaction = signalhandler;
  sigemptyset(&new_action.sa_mask);
  new_action.sa_flags = SA_RESTART;

  sigaction(SIGINT, &new_action, NULL);


  money = atoi(argv[3]);

  if ((fd = open(argv[2],flags)) == -1){
    perror("Error while opening file\n");
    exit(0);
  }
  if ((fd2 = open(argv[1],flags)) == -1){
    perror("Error while opening file\n");
    exit(0);
  }
  fstat(fd2, &st);
  totalHomeworkNumber = st.st_size;
  totalHomeworkNumber--;


  fp = fdopen(fd, "w+");
  while ((c = getc(fp)) != EOF){
    if (c == '\n')
        workerThreadNumber = workerThreadNumber + 1;
  }
  if (lseek(fd,0, SEEK_SET) == -1 ) {
    perror("Error in lseek");
  }

  studentInfo = (students*)calloc(workerThreadNumber,sizeof(students));
  for (int i = 0; i < workerThreadNumber; i++) {
    studentInfo[i].studentSemaphore = calloc(1,sizeof(sem_t));
  }
  studentThreadIds = (pthread_t*)calloc(workerThreadNumber,sizeof(pthread_t));
  fp = fdopen(fd, "w+");

  for (int i = 0; i < workerThreadNumber; i++) {
       getline(&line, &len, fp);
       splitted = strtok(line," ");
       strcpy(studentInfo[i].name,splitted);
       splitted = strtok (NULL, " ");
       studentInfo[i].quality = atoi(splitted);
       splitted = strtok (NULL, " ");
       studentInfo[i].speed = atoi(splitted);
       splitted = strtok (NULL, " ");
       studentInfo[i].price = atoi(splitted);
       studentInfo[i].isSleeping = 0;
       sem_init(studentInfo[i].studentSemaphore, 0,0);
       studentInfo[i].moneyMade = 0;
       studentInfo[i].homeworkDone = 0;
   }
   printf("%d students-for-hire threads have been created.\nName Q S C\n",workerThreadNumber);
   minCost = studentInfo[0].price;
   for (int i = 0; i < workerThreadNumber; i++) {
        printf("%s %d %d %d\n",studentInfo[i].name,studentInfo[i].quality,studentInfo[i].speed,studentInfo[i].price);
        if (minCost > studentInfo[i].price ) {
          minCost = studentInfo[i].price;
        }
    }



   pipeFd = calloc(workerThreadNumber,sizeof(int*));
   for (int k = 0; k < workerThreadNumber; k++) {
     pipeFd[k] = calloc(2,sizeof(int));
   }

   for (int k = 0; k < workerThreadNumber; k++) {
     if(pipe(pipeFd[k]) < 0){
 			printf("Error pipe creation,program terminating \n");
       exit(0);
 		}
   }

   if ((mutex = sem_open("/midterm_mutex",flagsOfSemaphores,modesOfSemaphores,1)) == SEM_FAILED) {
     printf("Program could not open the semaphore named /midterm_mutex\n" );
     exit(0);
   }


   if ((studentDoneSem = sem_open("/midterm_student_done",flagsOfSemaphores,modesOfSemaphores,0)) == SEM_FAILED) {
     printf("Program could not open the semaphore named /midterm_isFinished\n" );
     exit(0);
   }


   if ((isFinished = sem_open("/midterm_isFinished",flagsOfSemaphores,modesOfSemaphores,0)) == SEM_FAILED) {
     printf("Program could not open the semaphore named /midterm_isFinished\n" );
     exit(0);
   }
   if ((full = sem_open("/midterm_full",flagsOfSemaphores,modesOfSemaphores,0)) == SEM_FAILED) {
     printf("Program could not open the semaphore named /midterm_isFinished\n" );
     exit(0);
   }
   if ((empty = sem_open("/midterm_empty",flagsOfSemaphores,modesOfSemaphores,100)) == SEM_FAILED) {
     printf("Program could not open the semaphore named /midterm_isFinished\n" );
     exit(0);
   }
   if ((mutex2 = sem_open("/midterm_mutex2",flagsOfSemaphores,modesOfSemaphores,1)) == SEM_FAILED) {
     printf("Program could not open the semaphore named /midterm_isFinished\n" );
     exit(0);
   }
   if ((awakeStudents = sem_open("/midterm_awakeStudents",flagsOfSemaphores,modesOfSemaphores,workerThreadNumber)) == SEM_FAILED) {
     printf("Program could not open the semaphore named /midterm_isFinished\n" );
     exit(0);
   }

  for (int i = 0; i < workerThreadNumber; i++) {
    int *arg = calloc(1,sizeof(*arg));
    if ( arg == NULL ) {
        fprintf(stderr, "Couldn't allocate memory for thread arg.\n");
        exit(EXIT_FAILURE);
    }
    *arg = i;
     pthread_create(&studentThreadIds[i], NULL, studentThread, arg);
   }

   if (pthread_create(&producerThreadId, NULL, producerThread, NULL) != 0) {
     printf("PTHREAD_CREATE ERROR\n" );
   }
   mainThread();
  //pthread_create(&mainThreadId, NULL, mainThread, NULL);


  pthread_join(producerThreadId, NULL);
  //pthread_join(mainThreadId, NULL);
  for (int i = 0; i < workerThreadNumber; i++) {
      pthread_join(studentThreadIds[i], NULL);
    }
  printf("geldi\n" );
  tasiTaragiTopla();
  for (int k = 0; k < workerThreadNumber; k++) {
    free(pipeFd[k]);
  }
  free(pipeFd);
  free(studentInfo);
  free(studentThreadIds);
  if (close(fd) == -1) {
    perror("Error when closing file\n");
  }


  return 0;
}

void *producerThread(){
  char bufferTemp[1];
  int currentHomeworkNumber = 0;
  while (currentHomeworkNumber < totalHomeworkNumber) {
    my_sem_wait(empty);
    my_sem_wait(mutex);
    if (money < minCost) {
      my_sem_post(mutex);
      my_sem_post(full);
      break;
    }
    if (read(fd2,bufferTemp,1) <= 0) {
      printf("Producer okuma hatası\n" );
    }
    printf("H has a new homework %c; remaining money is %d\nc",bufferTemp[0],money);
    bufferGlobal[bufferSizeGlobal] = bufferTemp[0];
    bufferSizeGlobal = bufferSizeGlobal+1;

    my_sem_post(mutex);
    my_sem_post(full);
    currentHomeworkNumber++;
  }
  printf("No more homeworks left or coming in, closing.\n" );
  if (close(fd2) == -1) {
    perror("Error when closing file\n");
  }
  pthread_exit(0);
}

void mainThread(){
  int totalMoneyMade = 0;
  int totalHomeworkDone = 0;
  int studentToChoose;
  int currentHomeworkNumber = 0;
  char readHomework;
  while (currentHomeworkNumber < totalHomeworkNumber){
    my_sem_wait(awakeStudents);
    my_sem_wait(full);
    my_sem_wait(mutex);
    if (currentHomeworkNumber == totalHomeworkNumber) {
      printf("No more homeworks left or coming in, closing.\n" );
      printf("Homeworks solved and money made by the students:\n" );
      for (int i = 0; i < workerThreadNumber; i++) {
        if (studentInfo[i].moneyMade > 0) {
          printf("%s %d %d\n",studentInfo[i].name,studentInfo[i].homeworkDone,studentInfo[i].moneyMade);
          totalMoneyMade = totalMoneyMade + studentInfo[i].moneyMade;
          totalHomeworkDone = totalHomeworkDone + studentInfo[i].homeworkDone;
        }

       }
       printf("Total cost for %d homeworks %dTL\n", totalHomeworkDone,totalMoneyMade);
       printf("Money left at G’s account: %dTL\n",money );
      exit(0);
    }
    readHomework = bufferGlobal[0];
    for(int i = 1; i < bufferSizeGlobal; i++){
      bufferGlobal[i-1] = bufferGlobal[i];
    }
    bufferSizeGlobal = bufferSizeGlobal-1;

    studentToChoose = chooseBestStudent(readHomework);
    if (money >= studentInfo[studentToChoose].price) {
      money = money - studentInfo[studentToChoose].price;
    }
    else{
        printf("H has no more money for homeworks, terminating.\n");
        break;
    }
    studentInfo[studentToChoose].moneyMade = studentInfo[studentToChoose].moneyMade + studentInfo[studentToChoose].price;
    studentInfo[studentToChoose].homeworkDone = studentInfo[studentToChoose].homeworkDone + 1;
    currentHomeworkNumber = currentHomeworkNumber + 1;

    printf("%s is solving homework %c for %d, H has %d left.\n",studentInfo[studentToChoose].name,readHomework,studentInfo[studentToChoose].price,money );
    my_sem_post(studentInfo[studentToChoose].studentSemaphore);
    my_sem_wait(studentDoneSem);

    if (money < minCost) {
      printf("H has no more money for homeworks, terminating.\n");
      printf("Money is over, closing.\n");
      printf("Homeworks solved and money made by the students:\n" );
      for (int i = 0; i < workerThreadNumber; i++) {
        if (studentInfo[i].moneyMade > 0) {
          printf("%s %d %d\n",studentInfo[i].name,studentInfo[i].homeworkDone,studentInfo[i].moneyMade);
          totalMoneyMade = totalMoneyMade + studentInfo[i].moneyMade;
          totalHomeworkDone = totalHomeworkDone + studentInfo[i].homeworkDone;
        }

       }
       printf("Total cost for %d homeworks %dTL\n", totalHomeworkDone,totalMoneyMade);
       printf("Money left at G’s account: %dTL\n",money );
      exit(0);
    }

    my_sem_post(mutex);
    my_sem_post(empty);
  }
  printf("Homeworks solved and money made by the students:\n" );
  for (int i = 0; i < workerThreadNumber; i++) {
    if (studentInfo[i].moneyMade > 0) {
      printf("%s %d %d\n",studentInfo[i].name,studentInfo[i].homeworkDone,studentInfo[i].moneyMade);
      totalMoneyMade = totalMoneyMade + studentInfo[i].moneyMade;
      totalHomeworkDone = totalHomeworkDone + studentInfo[i].homeworkDone;
    }

   }
   printf("Total cost for %d homeworks %dTL\n", totalHomeworkDone,totalMoneyMade);
   printf("Money left at G’s account: %dTL\n",money );
  exit(0);
}

void* studentThread(void* studentIndex){
  int studentIndexNum = *((int *) studentIndex);
  //free(studentIndex);
  while (1) {

    printf("%s is waiting for a homework\n",studentInfo[studentIndexNum].name );
    my_sem_wait(studentInfo[studentIndexNum].studentSemaphore);


    studentInfo[studentIndexNum].isSleeping = 1;
    my_sem_post(studentDoneSem);
    sleep(6-studentInfo[studentIndexNum].speed);
    my_sem_post(awakeStudents);
    studentInfo[studentIndexNum].isSleeping = 0;
  }
  return (void*)1;
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

        printf("Termination signal received, closing.\n");

        exit(0);
    }
}

void tasiTaragiTopla(){
  if ( (sem_close(mutex)) < 0) {
    perror("sem_close error");
  }
  if ( (sem_close(studentDoneSem)) < 0) {
    perror("sem_close error");
  }
  if ( (sem_close(isFinished)) < 0) {
    perror("sem_close error");
  }
  if ( (sem_close(full)) < 0) {
    perror("sem_close error");
  }
  if ( (sem_close(empty)) < 0) {
    perror("sem_close error");
  }
  if ( (sem_close(mutex2)) < 0) {
    perror("sem_close error");
  }
  if ( (sem_close(awakeStudents)) < 0) {
    perror("sem_close error");
  }

  if ( (sem_unlink("/midterm_mutex")) < 0) {
    perror("sem_unlink error");
  }
  if ( (sem_unlink("/midterm_student_done")) < 0) {
    perror("sem_unlink error");
  }
  if ( (sem_unlink("/midterm_isFinished")) < 0) {
    perror("sem_unlink error");
  }
  if ( (sem_unlink("/midterm_full")) < 0) {
    perror("sem_unlink error");
  }
  if ( (sem_unlink("/midterm_empty")) < 0) {
    perror("sem_unlink error");
  }
  if ( (sem_unlink("/midterm_mutex2")) < 0) {
    perror("sem_unlink error");
  }
  if ( (sem_unlink("/midterm_awakeStudents")) < 0) {
    perror("sem_unlink error");
  }
  return;

}

int chooseBestStudent(char homeworkType){
  int *indexes;
  int temp;
  students *tempStudents;
  students tempStudent;
  tempStudents = calloc(workerThreadNumber,sizeof(students));
  indexes = calloc(workerThreadNumber,sizeof(int));
  for (int i = 0; i < workerThreadNumber; i++) {
    tempStudents[i].price =  studentInfo[i].price;
    tempStudents[i].speed =  studentInfo[i].speed;
    tempStudents[i].quality =  studentInfo[i].quality;
    tempStudents[i].isSleeping = studentInfo[i].isSleeping;
  }
  for (int i = 0; i < workerThreadNumber; i++) {
    indexes[i] = i;
  }
  if (homeworkType == 'C') {
    for (int i = 0; i < workerThreadNumber-1; i++){
     for (int j = 0; j < workerThreadNumber-i-1; j++){
         if (tempStudents[j].price  > tempStudents[j+1].price){
            tempStudent = tempStudents[j];
            tempStudents[j] = tempStudents[j+1];
            tempStudents[j+1] = tempStudent;
            temp = indexes[j];
            indexes[j] = indexes[j+1];
            indexes[j+1] = temp;
          }
      }
    }

    for (int i = 0; i < workerThreadNumber; i++) {
      if (studentInfo[indexes[i]].isSleeping == 0 && money >= studentInfo[indexes[i]].price) {
        temp = indexes[i];
        free(indexes);
        return (temp);
      }
    }
  }
  if (homeworkType == 'Q') {
    for (int i = 0; i < workerThreadNumber-1; i++){
     for (int j = 0; j < workerThreadNumber-i-1; j++){
         if (tempStudents[j].quality  < tempStudents[j+1].quality){
           tempStudent = tempStudents[j];
           tempStudents[j] = tempStudents[j+1];
           tempStudents[j+1] = tempStudent;
           temp = indexes[j];
           indexes[j] = indexes[j+1];
           indexes[j+1] = temp;
          }
      }
    }

    for (int i = 0; i < workerThreadNumber; i++) {
      if (studentInfo[indexes[i]].isSleeping == 0 && money >= studentInfo[indexes[i]].price) {
        temp = indexes[i];
        free(indexes);
        return (temp);
      }
    }
  }
  if (homeworkType == 'S') {
    for (int i = 0; i < workerThreadNumber-1; i++){
     for (int j = 0; j < workerThreadNumber-i-1; j++){
         if (tempStudents[j].speed  < tempStudents[j+1].speed){
           tempStudent = tempStudents[j];
           tempStudents[j] = tempStudents[j+1];
           tempStudents[j+1] = tempStudent;
           temp = indexes[j];
           indexes[j] = indexes[j+1];
           indexes[j+1] = temp;
          }
      }
    }

    for (int i = 0; i < workerThreadNumber; i++) {
      if (studentInfo[indexes[i]].isSleeping == 0 && money >= studentInfo[indexes[i]].price) {
        temp = indexes[i];
        free(indexes);
        return (temp);
      }
    }
  }
  return 1;


}

char* itoa(int val, int base){

	static char buf[32] = {0};

	int i = 30;

	for(; val && i ; --i, val /= base)

		buf[i] = "0123456789abcdef"[val % base];

	return &buf[i+1];

}
