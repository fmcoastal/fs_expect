
#include <stdint.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>   // for memset()
#include <stdio.h>    // for printf()
#include <fcntl.h>    // for open()
#include <pthread.h>  // for thread Items
#include <stdlib.h>  // for itoa()
#include <getopt.h>
#include "fln_serial.h"
#include "ffile.h"
#include "frbuff.h"

uint64_t g_serial_debug = 0;
#define  SERAIL_DBG(x) if((g_serial_debug & x) == x)
//#define  SERAIL_DBG_MEASUREMENT_PARMS    0x08
//#define  SERAIL_DBG_ERROR_CALC           0x04
#define  SERIAL_DBG_GETSTRINGPOINTERS      0x10000

#define WAI()   
//#define WAI()  printf("%3d:%s-%s\n",__LINE__,__FILE__,__FUNCTION__); 

#define TAB 0x0b
#define CR 0x0d
#define LF 0x0a
#define MIN(x,y) (x<y?x:y)
#define MAX(x,y) (x>y?x:y)


#define EYE_MENU_EXIT 28




#define FILE_NAME_SIZE  256 
char g_TestLaneConfigFilename[FILE_NAME_SIZE] = {0};;




//----------------------------------
pthread_t g_tid[2];             // posix thread array
frbuff *  g_pTxSerial = NULL;   // Serial Buffer for talking to Tx
frbuff *  g_pRxSerial = NULL;   // Serial Buffer to catch RXx
uint64_t  g_Debug = 0;
uint64_t  g_Verbose = 0;
int       g_Menu = 0;           // if set, execute a CLI menu;

int  g_RxThreadDisableRBuff = 0 ;   // makes Rx thread not write to RxBuff. 
int  g_RxThreadPrint        = 0 ;   // makes Rx thread print to StdOut
int  g_RxThreadLog          = 1 ;   // makes Rx thread log to a file if file handle open

int   g_LogSerialCom = 0; 
FILE *g_fpOut        = NULL;
char outFileName[] = {"log.txt"};


uint64_t fatox64(int8_t* string);
int   RunScriptFile(char * filename);
 
void print_usage(void)
{
   printf("fs_expect - App /src doe to enable  \n");
   printf("-s <serialport>       EG: /dev/ttyS0  (default)\n");
   printf("-l                    open/append log file serial communications\n");
   printf("-t <ScriptFileName>  \n");
   printf("-v                    Verbose Flag  \n");
   printf("-d <0xDebugFlags>     Debug Flags  \n");
   printf("-m                    CLI menu   \n");
   printf("example: \n");
   printf("./fs_expect  -s /dev/ttyS0 -t test  -d9090 -v \n");
   printf("./fs_expect  -m \n");
   printf("\n");
}


// the struct below is passed to the two threads. 
typedef struct 
{
   int done;
   int fd;

}datablock;


int tx(void* arg)
{
    datablock * pdb =(datablock *)arg;
    char c;
    char * pc = &c ;
    int r;

    printf("Starting Tx Thread\n");
    printf("  Serial Handle: %d\n",pdb->fd);
    while( pdb->done == 0 )
    {
#ifdef KEYBOARD
       c = getchar();
//       printf("%c",c);   // local echo??
       write (pdb->fd, &c, 1);           // send 7 character greeting
       if( c == 0x1b) pdb->done = 1;
#else
       r = RBuffFetch( g_pTxSerial,(void **) &pc); //fetch value out of ring buffer */
       if ( r == 0)
       {
           if(g_Debug != 0)
           {
                printf("tx %c  0x%x\n",*pc,*pc);           
           }
           write (pdb->fd, pc, 1);           // send 7 character greeting
       }
       else
       {
           usleep(500);    // let the OS have the machine if noting to go out.
       }
       
#endif
    }
    printf("Ending Tx Thread\n");
    return 1;
}


int rx(void* arg)
{
    int  n;
    int  i;
    int  r;
    char buf [100];
    datablock * pdb =(datablock *)arg;
    printf("Starting Rx Thread\n");

    while( pdb->done == 0) // this will be set by teh back door
    {
       n = read (pdb->fd, buf, sizeof(buf));  // read up to 100 characters if ready to read
       for( i = 0 ; i < n ; i++)
       {
          if ( g_RxThreadDisableRBuff == 0)
          {
              r =  RBuffPut(g_pRxSerial, (void *) (&buf[i]) );    /* put value int ring buffer */
              if ( r != 0)
              {     
                  printf("%d:%s-%s   Error writing Rx Serial Buff :%d \n",__LINE__,__FILE__,__FUNCTION__,r);
              }
          }
          if(g_RxThreadPrint == 1)
          {
              printf("%c",buf[i]);
          }
          if((g_RxThreadLog == 1) && ( g_fpOut != NULL))
          {
              fprintf(g_fpOut,"%c",buf[i]);
          }
 
       }
    }
    printf("End of  Rx Thread\n");
    return 1;
}



void *WorkerThread(void *arg)
{
pthread_t id = pthread_self();
 
   if(pthread_equal(id,g_tid[0]))
   {
       // tx
       tx(arg);
   }
   else
  {
      // rx
      rx(arg);
  }
  return NULL;

}



#define error_message printf
#define llu long long unsigned 

void cleanup()
{
    if(g_fpOut != NULL)  
    {
       fflush(g_fpOut); // flush any lingering data
       fclose(g_fpOut);
    }
    RbuffClose(g_pTxSerial);
    RbuffClose(g_pRxSerial);
}


void doMenu(void);

//
//
//
int main (int argc, char ** argv)
{
    int  fd;
    char portname[80]; 
    char script_file[80] = {0}; 
    int i = 0;
    int err;
    datablock db;    
    int option;
    int result = 0;


// Initialize Default Values
    memset((void*)&db,0,sizeof(datablock));
    strcpy(portname, "/dev/ttyUSB0");
    strcpy(portname, "/dev/ttyS0");
//    strcpy(g_TestLaneConfigFilename , "TestLanes.txt");


while ((option = getopt(argc, argv,"mvls:d:t:")) != -1) {
       switch (option) {
            case 'm' : g_Menu=1 ;          // configure to bind w/ SO_REUSEADDR
                break;
            case 'v' : g_Verbose=1 ;       // configure to bind w/ SO_REUSEADDR
                break;
            case 'l' : g_LogSerialCom = 1; // write/append all serial communications to log 
                break;
            case 's' : strcpy(portname,optarg);      // Serial Port
                break;
            case 't' : strcpy(script_file,optarg);     // Test File
                break;
            case 'd' : g_Debug = atoi(optarg);// #of times to send File Data
                break;
            case 'h' :                               // Print Help
            default:
                print_usage();
//                exit(EXIT_FAILURE);
                return(1);
       }
   }

   if((g_Menu == 0) && (script_file[0] == 0))
   {
      print_usage();
      return 2;
   }



   printf("Command Line Arguments:\n");
   printf("       %16s SerialPort\n",portname);
   printf("       %16s Script File Name\n",script_file);
   printf("       %16d Verbose\n",(int)g_Verbose);
   printf("  0x%016llx   Debug\n",(llu)g_Debug);


   if ( g_LogSerialCom != 0)
   {
//
//  Open an Output LOG FIle if give a name  
//
       g_fpOut = fopen (outFileName, "a");
       if (g_fpOut == NULL)
       {
           printf ("Unable to open file %s\n", outFileName);
           cleanup();
           return (-5);
       }
   }  

// open the test File  and init the 

//  Create Buffers for the Serial port
     RbuffInitialize ( &g_pTxSerial, &result, sizeof(char),4096, 32);  //
     if( result != 0)
     {
         printf("Failed to allocate Tx Buffer  %d \n",result);
         cleanup();
         return -1;
     }
     RbuffInitialize ( &g_pRxSerial, &result, sizeof(char),4096, 32);  //
     if( result != 0)
     {
         printf("Failed to allocate Rx Buffer  %d \n",result);
         cleanup();
         return -1;
     }
 

//   Opent the Serial Port
    fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        error_message ("error %d opening %s: %s", errno, portname, strerror (errno));
        return -1;
    }
    db.fd = fd;  //  handle for the threads to access serial port.   

    set_interface_attribs (fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
    set_blocking (fd, 0);                // set no blocking


//  Launch Tx and Rx Threads
    while(i < 2)
    {
        err = pthread_create(&(g_tid[i]), NULL, &WorkerThread, &db);
        if (err != 0)
            printf("\ncan't create thread :[%s]", strerror(err));
        else
            printf("\n Thread created successfully\n");
        i++;
    }

    usleep(100000);  // for now sleep till threads run.  future, rework and have thread signal it is running!    

    if(g_Menu == 1)
    {
        doMenu();
    }
    else
    {
        RunScriptFile(script_file);
    }

    db.done = 1;   // flag the threads to terminate 
//    while(db.done == 0) {}    

    sleep(5);    // swag hope the treads shut down in 5 seconds

    cleanup();   // Cleanup the rest
    close(fd);  // the handle to the serail port
    return 0;

}

int GetFirstString(char * String, char * FirstArg);
int do_SerialCmd(char * pline);
int do_HexByte(char * pline);
int do_SearchResponse(char * pline, char ** buf, int * bufsz);
int do_Pause(char * pline);
int do_PrintMessage(char * pline);

#define PAUSE_TIME 100000


// wait for end of menu after command
int StatWaitFor_MainMenu(void)
{
    char *pRxLine = NULL;
    int  RxLineSz = 0;
    char CmdLine[256];

    sprintf(CmdLine,"SEARCH_RESPONSE    SG  \"Main menu\"  \"Wait for response\"");
    do_SearchResponse(CmdLine,&pRxLine  ,&RxLineSz  );
//       printf("found: %s\n",pRxLine);
    if(pRxLine != NULL) { free (pRxLine); pRxLine = NULL;}
    return 0;
}

// wait for end of Main Menu - look fo rReboot
int StatWaitFor_RebootMenu(void)
{
    char *pRxLine = NULL;
    int  RxLineSz = 0;
    char CmdLine[256];

    sprintf(CmdLine,"SEARCH_RESPONSE    SG  \"Reboot\"  \"Wait for response\"");
    do_SearchResponse(CmdLine,&pRxLine  ,&RxLineSz  );
//       printf("found: %s\n",pRxLine);
    if(pRxLine != NULL) { free (pRxLine); pRxLine = NULL;}
    return 0;
}




// wait for end of menu after command
int StatWaitFor_string(char * string)
{
    char *pRxLine = NULL;
    int  RxLineSz = 0;
    char CmdLine[256];

    sprintf(CmdLine,"SEARCH_RESPONSE    SG  \"%s\"  \"Wait for response\"",string);
    do_SearchResponse(CmdLine,&pRxLine  ,&RxLineSz  );
//       printf("found: %s\n",pRxLine);
    if(pRxLine != NULL) { free (pRxLine); pRxLine = NULL;}
    return 0;
}




int StatSend(int cmd)
{
    char *pRxLine = NULL;
    int  RxLineSz = 0;
    char CmdLine[256];

    WAI();

    sprintf(CmdLine,"SERIAL_CMD  SG  1  %d  0  0  0   \"Select lane\"",cmd);
 //      printf("%s\n",CmdLine);
    do_SerialCmd(CmdLine);
    sprintf(CmdLine,"SEARCH_RESPONSE    SG  \"%d\"  \"Wait for response\"",cmd);
    do_SearchResponse(CmdLine,&pRxLine  ,&RxLineSz  );
//       printf("found: %s\n",pRxLine);
    if(pRxLine != NULL) { free (pRxLine); pRxLine = NULL;}
    usleep(PAUSE_TIME);
    return 0;
}




extern int64_t g_ffDebug;
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

uint64_t  g_error = 0;
// need to use the escape key to send special charactes
// this is because getchar does not return Ctrl Sequences
#define STATE_NORMAL  0
#define STATE_ESC     1
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
int doTerminalServer(void)
{
    char c;
    int r = 0;
    int d = 0;
    int state = STATE_NORMAL;

    RBuffFlush(g_pRxSerial);    // flush the Rx Buffer
    g_RxThreadDisableRBuff = 1; // have Rx thread not stuff RX Buffer
    g_RxThreadPrint = 1;        // have Rx thread print responses from the target
    while( d == 0 )
    {
       c = getchar();
//       printf("%c",c);   // local echo??
       
       switch (state) {
       case STATE_NORMAL:
           if( c == 0x1b)
           {  
               state = STATE_ESC ;
           }      
           else
           {
              r =  RBuffPut(g_pTxSerial, (void *) &c );    /* put value int ring buffer */
              if( r != 0)
              {
                 g_error = r;
              }
           }
           break;

       case STATE_ESC:
           if( c == 0x1b)
           {  
               d = 1;
           }      
           else if ( c == 'q' ) // ctrl - Q  -> upper case Q 
           {
              c = 0x0d; 
              r =  RBuffPut(g_pTxSerial, (void *) &c );    /* put value int ring buffer */
           }
           state = STATE_NORMAL;
  
           break;
       } // end state switch 

   } //end while

    usleep(250000);           // wait a little
    RBuffFlush(g_pRxSerial);  // flush the Rx Buffer
    g_RxThreadPrint = 0;      // disable  Rx tread print responses from the target
    g_RxThreadDisableRBuff = 0; // have Rx thread not stuff RX Buffer
    return 0;
}



////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
void doMenuMenu(void)
{
printf("\n");
printf("f  <filename>        - Open a LogFile\n");
printf("fc                   - Close Current Log File\n");
//printf("l  <value>           - loop count   now=%d \n",g_StatLoops);
printf("q                    - quit program \n");
printf("s  <filename>        - run a script file \n");
printf("t                    - terminal Server \n");
printf("\n");
}
#define ARG_SIZE   64    // max size of an Individual ARGC
#define MAX_ARGS   5    // max number of Command Line Arguments.

int g_argc = 0;
char g_arg0[ARG_SIZE];
char g_arg1[ARG_SIZE];
char g_arg2[ARG_SIZE];
char g_arg3[ARG_SIZE];
char g_arg4[ARG_SIZE];

char g_MenuPrompt[] ={"Yes?> "};

#define CMD_BUFFER_SIZE 256
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
void doMenu(void)
{
int r;
int d = 0;
char CmdLine[CMD_BUFFER_SIZE] = { 0 };
//int  GetLineResults ;
int Cmd;
#define FILE_STRING_LENGTH 64
char logFileName[FILE_STRING_LENGTH];



    doMenuMenu();

    while (d == 0)
    {
       printf("%s",g_MenuPrompt);
       fgets(CmdLine,CMD_BUFFER_SIZE,stdin);
       g_arg0[0] = 0x00;  // if g_argc == 0 we will run last cmd. - stop this behavior
       g_arg1[0] = 0x00;  // make sure arg1 is null if not updated by input
       g_arg2[0] = 0x00;  // make sure arg1 is null if not updated by input
       g_argc = sscanf(CmdLine,"%s%s%s%s%s",g_arg0,g_arg1,g_arg2,g_arg3,g_arg4);

       Cmd = g_arg0[0];
       switch(Cmd) {

       case 'c':
       case 'C':
           printf("Canned Test \n");
//           if(g_arg0[1]      =='1')  doStatistics(); 
//           else if(g_arg0[1] =='2')  doStatistics_Equalize_EYE(); 
//           else printf("unrecognezed statistic script\n");
           // do script file
           break;

       case 'f':
       case 'F':
           if((g_arg0[1] == 'c') || ( g_arg0[1] == 'C'))
           {
               printf("Close Log File \n");
               if(g_fpOut != NULL) 
               {
                   fflush(g_fpOut); // flush any lingering data
                   fclose(g_fpOut); // close the old, then open the new
                   g_fpOut = NULL;
               }
           }
           else
           {
               printf("Create new Log File \n");
               if(g_fpOut != NULL) // see if file currently open
               {
                   fflush(g_fpOut); // flush any lingering data
                   fclose(g_fpOut); // close the old, then open the new
                   g_fpOut = NULL;
               }
               r = ffMakeDatedFileName( "log", ".txt", &(logFileName[0]) ,FILE_STRING_LENGTH);
               if ( r  == 0)
               {
                   g_fpOut = fopen( logFileName,"w");
                   if( g_fpOut == NULL)
                   {
                       printf("Unable to open Log File error %s \n",logFileName);
                       break;
                   }
                   printf("Created %s\n",logFileName);
               }
               else
               {
                   printf("%d:%s-%s  Unable to create Log File Name %d \n",__LINE__,__FILE__,__FUNCTION__,r);
               }
           }
           break;
 
       case 'l':    // set number of loops
       case 'L':
           if(g_argc >=2)
           {
//               g_StatLoops = atoi(g_arg1);
           }
           else
           {  
               printf("Must enter with command for now \n");
           }
 //          printf("Number of Loops %d \n",g_StatLoops);
           break;
 
       case 'q':
       case 'Q':
           d = 1;
           break;
       case 'r':
       case 'R':
           printf("Test Random Number \n");
            printf("New Line %d\n", (rand() % 4 ) );
            printf("New QLM  %d\n", (rand() % 5 ) );
 
           // do script file
           break;
       case 's':
       case 'S':
           printf("Do Script File : not implemented \n");
           if ( g_argc >= 2 )
              RunScriptFile(g_arg1);
           break;
       case 't':
       case 'T':  
           printf("  Entering  Terminal Server: \n");
           printf("        exit               <ESC ESC Enter> \n");
           printf("        exit prbs monitor  <ESC q  Enter> \n");
           doTerminalServer();
           printf("\n--Exiting  Terminal Server\n\n");
           break;
       default:
           doMenuMenu();
           break;
       } // end switch
    }  // end while 

}


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
int  RunScriptFile(char * filename)
{
    int r;

    char   *buf = NULL;         // buffer where file is read into
    int    bufsz = 0;           // size of the buffer
    char  *buf_ptr_allocated;   // copy of buffer pointer the file was read into.


    ffGetLineHndl FileData;     // structure for handling Get Line functions
    ffGetLineHndl * pFileData;  // pointer to the above structure
    char * pCmdLine;            // pointer filled in by get next line function
    int    CmdLineSz;           // size of line if you want to copy out the line data
    int    CmdLineCount;        // counter for the number of non-comment lines
    pFileData = &FileData;

    char FirstArg[80];          // string for holding the First LIne
    char * pRxLine = NULL;      // pointer filled in by get next line function
    int    RxLineSz = 0;        // size of line if you want to copy out the line data
 

//
//   Open the test file
//

      // input should be a FileName
      r = ffReadFileToBuffer( filename, &buf, &bufsz);
      if( r != 0)
      {
          printf(" Unable to open Test File  %s  Error:%d\n",filename,r);
          return -1;
      }
      buf_ptr_allocated = buf ;

     printf("Test File Size:%d\n",bufsz);


    //  Init the get line function
    //  int ffInitGetLine(ffGetLineHndl * ffHndl,  char * Buf,int BufLen,cha    r * CommentChars);
      r = ffInitGetLine(  pFileData ,  buf, bufsz ,"#");
      if( r != 0)
      {
          printf(" Unable to int ffInitGetLine   Error:%d\n",r );
          free (buf_ptr_allocated);
          return -3;
      }

      // process the work
      CmdLineCount = 0 ;
      while (1)
      {
         // initialize the ggGetLine file
         r =  ffGetLine( pFileData,  &pCmdLine, &CmdLineSz);
         if ( r == FF_END_OF_BUFFER)
         {
             printf(" End of trace file :%d\n",r );
             free (buf_ptr_allocated);
             buf_ptr_allocated = NULL;
             return 0;
         }
         CmdLineCount++;
         // Prccess the line data here
         if( g_Verbose != 0)  printf("%d  %s\n",CmdLineCount,pCmdLine);
         // the first field must be what the record type is.  The function below return the
         //     first string found in the line. 
         GetFirstString(pCmdLine, FirstArg);

         // based on the record type, do the work.
         if( strcmp(FirstArg,"SERIAL_CMD") == 0 )
         {
             printf("First Arg: %s :",FirstArg);
             do_SerialCmd( pCmdLine);
             
         }
         else if( strcmp(FirstArg,"SEND_HEX_BYTE") == 0 )
         {
             printf("First Arg: %s :",FirstArg);
             do_HexByte( pCmdLine);
              
         }
 
 
         else if( strcmp(FirstArg,"SEARCH_RESPONSE") == 0 )
         {
             printf("First Arg: %s :",FirstArg);
             if(pRxLine != NULL) free (pRxLine) ;
             pRxLine = NULL ;  // force do_SearchResult to allocate memoryi
             RxLineSz = 0;
             do_SearchResponse(pCmdLine,&pRxLine  ,&RxLineSz  );
             printf("found: %s\n",pRxLine); 
              
         }
         else if( strcmp(FirstArg,"PAUSE") == 0 )
         {
             printf("First Arg: %s :",FirstArg);
             do_Pause(pCmdLine);
         }

         else if( strcmp(FirstArg,"FLUSH_RX") == 0 )
         {
             printf("First Arg: %s \n",FirstArg);
             RBuffFlush(g_pRxSerial);
         }

         else if( strcmp(FirstArg,"PRINT_MESSAGE") == 0 )
         {
             do_PrintMessage(pCmdLine);
             RBuffFlush(g_pRxSerial);
         }

      } // end while done == 0
      // clean up;
      if(pRxLine != NULL) free (pRxLine) ;
      return 0;
}
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//  Print Message  Cmd
//  Pause   <time>   <comment>
int do_PrintMessage(char * pline)
{
   char ** pStr = NULL;  // I should only be getting 14 arguments.
   int    StrCnt = 25;


   GetStringPointers(pline, &pStr,&StrCnt);
   printf("%s\n",pStr[1]);
   if( pStr != NULL) { free(pStr);}
   return 0;
}



////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//  Pause Cmd
//  Pause   <time>   <comment>
int do_Pause(char * pline)
{
   char str0[64];
   char str1[64];
   char str2[64];
   int  r;
   int  PauseTime;

   r = sscanf(pline,"%s%s%s",str0,str1,str2);
   if( r != 0)
   {
       g_error = r;
   }
 
   printf("%s us\n",str1);
   PauseTime = atoi(str1);
   
   usleep(PauseTime);

   return 0;
}


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//  SendHexByte
//  SEND_HEX_BYTE  SG  byte   "comment"
int do_HexByte(char * pline)
{
   char str0[64];
   char str1[64];
   char str2[64];
   int  r;
   int  cmd;

   r = sscanf(pline,"%s%s%x%s",str0,str1,&cmd,str2);

   if ( r >3)
   {
       printf("   Send hex Byte: 0x%02x \n",cmd);  // print the hex byte we are sending

       r =  RBuffPut(g_pTxSerial, (void *) (&cmd) );    /* put value int ring buffer */
       if( r != 0)
       {
       printf("%d:%s-%s   Error writing tx Serial Buff \n",__LINE__,__FILE__,__FUNCTION__);
       }
   }
   else
   {
      printf("%d:%s-%s  Error Sending Hex Byte - TOo Few Argumets! \n",__LINE__,__FILE__,__FUNCTION__);
   }
   return 0;
}








////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//  Serial Cmd
//  SERIAL_CMD    SG  "command string"
int do_SerialCmd(char * pline)
{
   int r;
   int i;
   char** pStr = NULL;
   int    nstr = 0;
   char cmdstr[512];
 

//  parse the command line into an array of pointers to strings "pStr[]" 
    GetStringPointers( pline,  &pStr ,&nstr);

    if (g_Verbose != 0)
    {    
        printf("\n");  
        for( i = 0 ; i < nstr ; i++)  
        {
            printf("   string %d in PLINE: %s\n",i,pStr[i]);
        }
    }

   if( nstr >= 3)  
   {
       printf("      Send: %s \n",pStr[2]);  // print the command we are sending
       sprintf(cmdstr,"%s \n",pStr[2]);
       for( i = 0 ; i < strlen(cmdstr) ; i++)
       {
          r =  RBuffPut(g_pTxSerial, (void *) ((cmdstr+i)) );    /* put value int ring buffer */
          if( r != 0)
          {
              printf("%d:%s-%s   Error writing tx Serial Buff \n",__LINE__,__FILE__,__FUNCTION__);
          }
       }
   }
   else
   {
      printf("%d:%s-%s  Serial CMD  - Too few Arguments \n",__LINE__,__FILE__,__FUNCTION__);
   }

   if(pStr != NULL) free(pStr);

   return 0;
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//  Serial Cmd
//  SERIAL_CMD         SG  1 2 0 0 0   "Select the Measure command"
int do_SerialCmd_EYE(char * pline)
{
   char str0[64];
   char str1[64];
   char str2[64];
   char str3[64];
   char str4[64];
   char str5[64];
   char str6[64];
   char str7[64];
   int  r;
   int  cmd;
   char cmdstr[512];
   int i;

   r = sscanf(pline,"%s%s%s%s%s%s%s%s",str0,str1,str2,str3,str4,str5,str6,str7);

   cmd = atoi(str2);
   if( cmd == 1)  
   {
       printf(">Send: %s \n",str3);  // print the command we are sending
       sprintf(cmdstr,"%s \n",str3);
       for( i = 0 ; i < strlen(cmdstr) ; i++)
       {
          r =  RBuffPut(g_pTxSerial, (void *) ((cmdstr+i)) );    /* put value int ring buffer */
          if( r != 0)
          {
              printf("%d:%s-%s   Error writing tx Serial Buff \n",__LINE__,__FILE__,__FUNCTION__);
          }
       }
   }
   else
   {
      printf("%d:%s-%s  I only know how ot send 1 command for now!! \n",__LINE__,__FILE__,__FUNCTION__);
   }
   return 0;
}



// This will return the first out of the
int GetFirstString(char * String, char * FirstArg)
{
  while ((*String != 0x00) && (*String != 0x20))
  {
     *FirstArg++ = *String++;
  }
  *FirstArg = 0x00; // Terminate the String
  return(0);
}



#define GET_LINE         0
#define CHECK_FOR_STRING 1
#define ACTIVE_PATTERN   2
#define LINE_BUFFER_SIZE  1024*8
//SEARCH_RESPONSE    SG  "Max run of Zeros(time):"  "Find the number of p zeros"
// Menu choice []:
//Max run of zeros(time): 38 is at voltage(row): 29
//Max run of zeros(Voltage): 11 is at time(col): 21
//   in pline   the "SEARCH_RESPONSE" line above.
//   out lbuff   - the line "argV2" is found in.
//                 if *lbuff is null, will allocate and fill in size 
//   out lbufsize - the Size of the Line.
int do_SearchResponse(char * pline ,char ** lbuf, int * lbufsz)
{
    char c;
    char * pc = &c ;
    int r,i;
    int done = 0 ;
    char** pStr = NULL;
    int    nstr = 0;
    int LineIndex ;
    char LineBuffer[LINE_BUFFER_SIZE];
    int  LineCount;
    int  LineTooLongFlag = 0;

//  parse the command line into an array of pointers to strings "pStr[]" 
    GetStringPointers( pline,  &pStr ,&nstr);

    if (g_Verbose != 0)
    {  
        printf("\n");  
        for( i = 0 ; i < nstr ; i++)  
        {
            printf("   string %d in PLINE: %s\n",i,pStr[i]);
        }
    }

    if( nstr < 4)
         printf("%d:%s-%s Not enough arguments in SEARCH_RESPONCE command %d \n",__LINE__,__FILE__,__FUNCTION__,nstr);

    //printf("looking for: \"%s\"\n",pStr[2]);
    printf(" looking for: >%s< \n",pStr[2]);

    done = 0;
    memset(LineBuffer,0,LINE_BUFFER_SIZE); //          
    LineIndex = 0;
    LineCount = 0;        
    // first characters in buffer up to <CR> are from the previous cmd 
    printf(" Cmd Buffer echo:>"); 
    while( done == 0 )

    {
       // get a char;
       do{
          r = RBuffFetch( g_pRxSerial,(void **) &pc); //fetch character  out of ring buffer */
          if (( r != 0) && ( r != FRBUFF_NO_DATA_AVAILABLE))
          {
             printf("%d:%s-%s  Bad response from Reading Rx Buffer. \n",__LINE__,__FILE__,__FUNCTION__);
          }  
       }
       while (r != 0);
      
       if( g_Verbose > 0) printf("%c",*pc);
      // got a character
       // r == 0  must have data.

       if( *pc != '\n')
       {
          LineBuffer[LineIndex++] = *pc;

          if (CheckStringMatch(LineBuffer,pStr[2]) == 0)
          {
              WAI();
              done = 1;
              if ( *lbuf == NULL)
              {
                   *lbuf = (char *)malloc(LineIndex);
                   if(*lbuf == NULL)
                   {
                       printf("%d:%s-%s Error allocating lbuf. Malloc failed \n",__LINE__,__FILE__,__FUNCTION__);
                   } 
                   else
                   {
                      *lbufsz = LineIndex;
                      strcpy(*lbuf,LineBuffer);
                   }
              }
              else
              {
                   printf("\n%s\n",LineBuffer);
              }
          }
       }
       else
       {
          LineBuffer[LineIndex++] = 0x00;  // null terminate the line of text 
          printf("\r LineCount: %-3d ",LineCount); // print the relative number of lines we have scanned in.
//          if(LineCount >= 1531) 
//                  printf("\n%s\n",LineBuffer);
          if (CheckStringMatch(LineBuffer,pStr[2]) == 0)
          { 
              done = 1;
              if ( *lbuf == NULL)
              {
                   *lbuf = (char *)malloc(LineIndex);
                   if(*lbuf == NULL)
                   {
                       printf("%d:%s-%s Error allocating lbuf. Malloc failed \n",__LINE__,__FILE__,__FUNCTION__);
                   } 
                   else
                   {
                      *lbufsz = LineIndex;
                      strcpy(*lbuf,LineBuffer);
                   }
              }
              else
              {
                   printf("\n%s\n",LineBuffer);
              }
          }
          memset(LineBuffer,0,LINE_BUFFER_SIZE); //          
          LineIndex = 0;
          LineCount++;         
       }
       if(LineIndex >= (LINE_BUFFER_SIZE-1))
       {
            // the line is bigger than my 8K buffer. Flag the event. 
            // print an error at et end of the routine.
            //
            // For now, back the index (this will drop characters on the Floor)
            //   analyze what happened at a later time. 
            LineTooLongFlag = 1;
            LineIndex--;
       }
    }  // end While

    if(LineTooLongFlag == 1)
    {
       if((g_RxThreadLog == 1) && ( g_fpOut != NULL))
          {
              fprintf(g_fpOut,"Serial input -- Line length greater than %d\n",LINE_BUFFER_SIZE);
          }
        printf("Serial input -- Line length greater than %d\n",LINE_BUFFER_SIZE);
    }

//    printf("\nEnd Search Response");
    if(pStr != NULL) free(pStr);
    printf("\n");
    return 0;
}


uint64_t fatox64(int8_t* string)
{
    uint64_t result = 0;
    int64_t i = 0;
    int64_t done = 0;
    uint8_t in;
    int64_t length = (int32_t)strlen((char *)string) + 1;

    while( (done == 0) && (i < length))
    {
        in = *(string+i);
        if( in == 0x00)
        {
            done = 1;
        }
        else
        {
            if (( in >= '0') &&( in <= '9'))
            {
                result <<= 4;
                result += in & 0x0f;
            }
            else if( (( in >= 'a') &&( in <= 'f')) ||
                (( in >= 'A') &&( in <= 'F')) )
            {
                result <<= 4;
                result += ((in & 0x0f) + 9);
            }
            else if( ( in == 'x') || ( in == 'X'))
            {

            }
            else
            {
                done = 1; // unrecognized character
            }
            i++;
        }
    }
    return result;
}


//////////////////////////////////////////////////////////
//





