/* Copyright 2001, 2019 IBM Corporation
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the 
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the 
 * following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the 
 * following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE 
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 /******************************************************************************
 *
 * Source File Name = db2uext2.cdisk 
 *
 * Licensed Materials - Property of IBM
 *
 * (C) COPYRIGHT International Business Machines Corp. 1996.
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 * Function = Sample Log Management User Exit C Source Code
 *
 *****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/* Sample Name: db2uext2.cdisk                                               */
/*                                                                           */
/*                                                                           */
/* Purpose:    This is a sample User Exit utilizing the AIX system copy      */
/*             command to Archive and Retrieve database log files.           */
/*                                                                           */
/*                                                                           */
/* Options:    1. This sample provides an audit trail of calls ( stored in a */
/*                separate file for each option ) including a timestamp and  */
/*                parameters received.  This option can be disabled.         */
/*                                                                           */
/*             2. This sample provides an error trail of calls in error      */
/*                including a timestamp and an error isolation string for    */
/*                problem determination.  This option can be disabled.       */
/*                                                                           */
/*                                                                           */
/* Usage:      1. Copy "db2uext2.cdisk" to "db2uext2.c" and place this file  */
/*                into a working directory.                                  */
/*                                                                           */
/*             2. Modify the "Installation Defined Variables" to suit your   */
/*                environment.  Two example scenarios have been provided     */
/*                below for illustrative purposes.                           */
/*                                                                           */
/*             3. Modify the program logic to suit your environment.  If     */
/*                there are situations where the user exit program fails on  */
/*                archive request and you want DB2 to retry the request, set */
/*                the return code to RC_OPATTN.                              */
/*                                                                           */
/*                *** NOTE *** User Exit must COPY log files from active log */
/*                             path to archive log path. Do not use MOVE     */
/*                             operation that will remove log file from the  */
/*                             active log path.                              */
/*                                                                           */
/*             4. Compile and link "db2uext2.c" with the following command   */
/*                or a functional equivalent:                                */
/*                          cc -o db2uext2 db2uext2.c                        */
/*                                                                           */
/*                Place the resultant "db2uext2" named executable            */
/*                into sqllib/adm.                                           */
/*                                                                           */
/*                *** NOTE ***  Code/command modification may be required    */
/*                              depending on compiler options used and       */
/*                              header file location                         */
/*                                                                           */
/*                *** NOTE ***  On HP compile as follows:                    */
/*                              cc -D_INCLUDE_POSIX_SOURCE -Aa db2uext2.c    */
/*                                 -o db2uext2                               */
/*                                                                           */
/*                *** NOTE ***  On SCO UnixWare or Linux compile as follows: */
/*                              cc -D_INCLUDE_POSIX_SOURCE db2uext2.c        */
/*                                 -o db2uext2                               */
/*                                                                           */
/*                *** NOTE ***  On Sequent (PTX) compile as follows:         */
/*                              c++ -o db2uext2 db2uext2.c -ansi -lseq       */
/*                                                                           */
/*                                                                           */
/*             5. DB2 calls "db2uext2" in the following format -             */
/*                                                                           */
/*                  db2uext2 -OS<os> -RL<release> -RQ<request> -DB<dbname>   */
/*                           -NN<nodenumber> -LP<logpath> -LN<logname>       */
/*                           [-LSlogsize -SPstartingpage]                    */
/*                           [-AP<adsmpasswd>]                               */
/*                                                                           */
/*                  where:  os         = operating system                    */
/*                          release    = DB2 release                         */
/*                          request    = 'ARCHIVE' or 'RETRIEVE'             */
/*                          dbname     = database name                       */
/*                          nodenumber = node number                         */
/*                          logpath    = log file path                       */
/*                          logname    = log file name                       */
/*                          logsize    = log file size (optional)            */
/*                          startingpage = starting offset in 4K page unit   */
/*                                         (optional)                        */
/*                          adsmpasswd = ADSM password (optional)            */
/*                                                                           */
/*                  Note: logsize and startingpage are only used when        */
/*                        logpath is a raw device.                           */
/*                                                                           */
/*             6. Log files are archived and retrieved from disk with the    */
/*                following naming convention:                               */
/*                                                                           */
/*                  archive:  archive path  + database name +                */
/*                                  node number + log file name              */
/*                  retrieve: retrieve path + database name +                */
/*                                  node number + log file name              */
/*                                                                           */
/*                  For example:                                             */
/*                     If the archive path was "/u/usrX/archPath/", the      */
/*                     retrieve path was "/u/usrX/retrPath/", the database   */
/*                     name was "SAMPLE", the node number was NODE0000 and   */
/*                     the log file name was "S0000001.LOG", the log file    */
/*                     would be:                                             */
/*                        archived to -                                      */
/*                           "/u/usrX/archPath/SAMPLE/NODE0000/S0000001.LOG" */
/*                                                                           */
/*                        retrieved from -                                   */
/*                           "/u/usrX/retrPath/SAMPLE/NODE0000/S0000001.LOG" */
/*                                                                           */
/*                  Note: The subdirectory /u/usrX/archPath/SAMPLE/NODE0000/ */
/*                        need to exist for the user exit sample program to  */
/*                        work.  Please create it before turning USEREXIT    */
/*                        on.                                                */
/*                                                                           */
/* Logic Flow: 1. install signal handlers                                    */
/*             2. verify the number of parameters passed                     */
/*             3. verify the action requested                                */
/*             4. start the audit trail ( if requested )                     */
/*             5. if the requested action is to archive a file:              */
/*                . copy the log file from the log path to the archive path  */
/*                . if the log file is not found proceed to point 6          */
/*                if the requested action is to retrieve a file:             */
/*                . copy the log file from the retrieve path to the log path */
/*                . if the log file is not found proceed to point 6          */
/*             6. log errors ( if requested and required )                   */
/*             7. end the audit trail ( if requested )                       */
/*             8. exit with the appropriate return code                      */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

/* HP does not define llseek and offset_t */
#ifdef _INCLUDE_POSIX_SOURCE
# define llseek lseek
  typedef unsigned long offset_t;
#endif

/*===========================================================================*/
/*==                                                                       ==*/
/*== ------------------ INSTALLATION DEFINED VARIABLES ------------------- ==*/
/*==                                                                       ==*/
/*== -------------------- REQUIRES USER MODIFICATION --------------------- ==*/
/*==                                                                       ==*/
/*===========================================================================*/
/*== ARCHIVE_PATH:  Path where log files will be archived to               ==*/
/*==                Notes: 1. the path is concatenated with the database   ==*/
/*==                          name ( the database name will be UPPERCASE ) ==*/
/*==                          and the node number ( the node number will   ==*/
/*==                          be in UPPERCASE ) to form the physical       ==*/
/*==                          archive path                                 ==*/
/*==                       2. the physical archive path must exist ( the   ==*/
/*==                          user exit will not create the path )         ==*/
/*==                       3. the path must end with a back slash          ==*/
/*==                       4. the default path is "/u/"                    ==*/
/*==                                                                       ==*/
/*== RETRIEVE_PATH: Path where log files will be retrieved from            ==*/
/*==                Notes: 1. the path is concatenated with the database   ==*/
/*==                          name ( the database name will be UPPERCASE ) ==*/
/*==                          and the node number ( the node number will   ==*/
/*==                          be in UPPERCASE ) to form the physical       ==*/
/*==                          retrieve path                                ==*/
/*==                       2. the physical retrieve path must exist ( the  ==*/
/*==                          user exit will not create the path )         ==*/
/*==                       3. the path must end with a back slash          ==*/
/*==                       4. the default path is "/u/"                    ==*/
/*==                                                                       ==*/
/*== AUDIT_ACTIVE:  The user may wish an audit trail of the user exit run. ==*/
/*==                Sample audit functions have been provided              ==*/
/*==                ( AuditLogStart() and AuditLogEnd() )                  ==*/
/*==                Notes: 1. enable audit logging by setting AUDIT_ACTIVE ==*/
/*==                          to 1                                         ==*/
/*==                       2. disable audit logging by setting             ==*/
/*==                          AUDIT_ACTIVE to 0                            ==*/
/*==                       3. archive requests will be traced in a file    ==*/
/*==                          named "ARCHIVE.LOG" located in the audit and ==*/
/*==                          error file path                              ==*/
/*==                       4. retrieve requests will be traced in a file   ==*/
/*==                          named "RETRIEVE.LOG" located in the audit    ==*/
/*==                          and error file path                          ==*/
/*==                       5. the default setting is enable audit          ==*/
/*==                          logging                                      ==*/
/*==                                                                       ==*/
/*== ERROR_ACTIVE:  The user may wish an error trail of the user exit run. ==*/
/*==                A sample error log function has been provided          ==*/
/*==                ( ErrorLog() )                                         ==*/
/*==                Notes: 1. enable error logging by setting ERROR_ACTIVE ==*/
/*==                          to 1                                         ==*/
/*==                       2. disable error logging by setting             ==*/
/*==                          ERROR_ACTIVE to 0                            ==*/
/*==                       3. errors will be traced in a file named        ==*/
/*==                          "USEREXIT.ERR" located in the audit and      ==*/
/*==                          error file path                              ==*/
/*==                       4. the default setting is enable error          ==*/
/*==                          logging                                      ==*/
/*==                                                                       ==*/
/*== AUDIT_ERROR_PATH: Path where Audit and Error logs will reside         ==*/
/*==                   Notes: 1. the path must exist ( the user exit will  ==*/
/*==                             not create the path )                     ==*/
/*==                          2. the path must end with a back slash       ==*/
/*==                          3. the default is "/u/"                      ==*/
/*==                                                                       ==*/
/*== AUDIT_ERROR_ATTR: Standard C file open attributes for the Audit and   ==*/
/*==                   Error logs                                          ==*/
/*==                   Notes: 1. the default is "a" (text append)          ==*/
/*==                                                                       ==*/
/*== BUFFER_SIZE:      This is only used if raw device is used for logging.==*/
/*==                   It defines the size of the buffer used to read the  ==*/
/*==                   device and write to the target file.  It is in      ==*/
/*==                   unit of 4K pages.                                   ==*/
/*==                                                                       ==*/


#define ARCHIVE_PATH      "/var/db2_logarchive/" /* path must end with a slash */
#define RETRIEVE_PATH     "/var/db2_logarchive/" /* path must end with a slash */
#define AUDIT_ACTIVE          1           /* enable audit trail logging      */
#define ERROR_ACTIVE          1           /* enable error trail logging      */
#define AUDIT_ERROR_PATH  "/var/homes/db2instl/logs/uexits/" /* path must end with a slash      */
#define AUDIT_ERROR_ATTR    "a"           /* append to text file             */
#define BUFFER_SIZE          32           /* # of 4K pages for output buffer */

/*===========================================================================*/
/*==                                                                       ==*/
/*== ------------------------ EXAMPLE SCENARIOS -------------------------- ==*/
/*==                  ( FOR ILLUSTRATIVE PURPOSES ONLY )                   ==*/
/*==                                                                       ==*/
/*===========================================================================*/
/*==                                                                       ==*/
/*== 1) Given that we are "userid1", are working with a database           ==*/
/*==    named "SAMPLE1" on node NODE0001 and have modified the             ==*/
/*==    installation defined variables to the following values:            ==*/
/*==                                                                       ==*/
/*==        ARCHIVE_PATH      "/u/userid1/LogDirectory/"                   ==*/
/*==        RETRIEVE_PATH     "/u/userid1/LogDirectory/"                   ==*/
/*==        AUDIT_ACTIVE      1                                            ==*/
/*==        ERROR_ACTIVE      1                                            ==*/
/*==        AUDIT_ERROR_PATH  "/u/userid1/UserExits/"                      ==*/
/*==        AUDIT_ERROR_ATTR  "a"                                          ==*/
/*==                                                                       ==*/
/*==    If a request is received to archive log S0000000.LOG:              ==*/
/*==                                                                       ==*/
/*==    . the audit log would be opened in append text mode and be named:  ==*/
/*==      ==> "/u/userid1/UserExits/ARCHIVE.LOG"                           ==*/
/*==          ( AUDIT_ERROR_PATH + archive audit log file name )           ==*/
/*==                                                                       ==*/
/*==    . the log file "S0000000.LOG" in the log path directory for        ==*/
/*==      node NODE0000 of database "SAMPLE1" would be archived to:        ==*/
/*==      ==> "/u/userid1/LogDirectory/SAMPLE1/NODE0001/S0000000.LOG"      ==*/
/*==        ( ARCHIVE_PATH + database name + node number + log file name ) ==*/
/*==                                                                       ==*/
/*==    . the error log would be opened in append text mode and be named:  ==*/
/*==      ==> "/u/userid1/UserExits/USEREXIT.ERR"                          ==*/
/*==          ( AUDIT_ERROR_PATH + error log file name ( defined below ) ) ==*/
/*==                                                                       ==*/
/*==                                                                       ==*/
/*== 2) Given that we are "userid2", are working with a database           ==*/
/*==    named "SAMPLE2" on node NODE0002 and have modified the             ==*/
/*==    installation defined variables to the following values:            ==*/
/*==                                                                       ==*/
/*==        ARCHIVE_PATH      "/u/userid2/LogDirectory1/New/"              ==*/
/*==        RETRIEVE_PATH     "/u/userid2/LogDirectory2/Old/"              ==*/
/*==        AUDIT_ACTIVE      1                                            ==*/
/*==        ERROR_ACTIVE      0                                            ==*/
/*==        AUDIT_ERROR_PATH  "/u/userid2/AuditLogs/"                      ==*/
/*==        AUDIT_ERROR_ATTR  "w"                                          ==*/
/*==                                                                       ==*/
/*==    If a request is received to retrieve log S0000001.LOG:             ==*/
/*==                                                                       ==*/
/*==    . the audit log would be opened in write text mode and be named:   ==*/
/*==      ==> "/u/userid2/AuditLogs/RETRIEVE.LOG"                          ==*/
/*==          ( AUDIT_ERROR_PATH + retrieve audit log file name )          ==*/
/*==                                                                       ==*/
/*==    . the log file would be retrieved from:                            ==*/
/*==      ==> "/u/userid2/LogDirectory2/Old/SAMPLE2/NODE0002/S0000001.LOG" ==*/
/*==       ( RETRIEVE_PATH + database name + node number + log file name ) ==*/
/*==      and copied into the log path directory for database "SAMPLE2"    ==*/
/*==      as:                                                              ==*/
/*==      ==> "S0000001.LOG"                                               ==*/
/*==          ( log file name )                                            ==*/
/*==                                                                       ==*/
/*==    . error logging is disabled                                        ==*/
/*==                                                                       ==*/
/*===========================================================================*/


/* ----------------------------------------------------------------- */
/* User Exit Supported Return Codes                                  */
/*    NOTE: DB2 will reinvoke the user exit for the same request     */
/*          after 5 minutes if return code is 4 or 8.                */
/*                                                                   */
/*          For other non-zero return codes, DB2 will not invoke     */
/*          user exit for the database for at least 5 minutes.       */
/*          If this request is to archive a log file, DB2 will not   */
/*          make another archive request for this file, or other     */
/*          log files produced during the 5 minute time period.      */
/*          These log files will only be archived when all           */
/*          applications disconnect from and the database, and the   */
/*          database is reopenned.                                   */
/* ----------------------------------------------------------------- */
#define RC_OK                0    /* ok                              */
#define RC_RES               4    /* resource allocation error       */
#define RC_OPATTN            8    /* operator/user attention required*/
#define RC_HARDWARE         12    /* hardware error                  */
#define RC_DEFECT           16    /* software error                  */
#define RC_PARM             20    /* invalid parameters              */
#define RC_NOTFOUND         24    /* db2uext2() / file not found     */
#define RC_UNKNOWN          28    /* unknown error                   */
#define RC_OPCAN            32    /* operator/user terminated        */


/* ----------------------------------------------------------------- */
/* User Exit Constants                                               */
/* ----------------------------------------------------------------- */
#define NUM_VALID_PARMS      5    /* number of valid parameters      */
#define SLASH              "/"    /* default slash character         */
#define NEW_LINE          "\n"    /* new line character              */
#define NULL_TERM         "\0"    /* null terminator                 */
#define FILE_EXT        ".LOG"    /* audit log file extension/type   */
#define COPY              "cp"    /* disk copy command               */
#define REMOVE         "rm -f"    /* disk remove command             */
#define MEDIA_TYPE      "disk"    /* media type used                 */
#define AUDIT_IO_ERROR      99    /* audit log I/O error             */
#define SYSTEM_CALL_LEN    550    /* system call string length       */
#define OUTPUT_LINE_LEN    550    /* output line length              */
#define FILE_NAME_LEN      255    /* file name length                */
#define HELP_STRING_LEN     80    /* error help string length        */
#define DELIMITER_LEN       80    /* delimiter length                */
#define ERROR_FILE_NAME "USEREXIT.ERR"
                                  /* error log file name             */


/* There is no O_RSHARE on SUN */
#ifndef O_RSHARE
#define O_RSHARE 0
#endif

/* ----------------------------------------------------------------- */
/* Define TRUE and FALSE if required                                 */
/* ----------------------------------------------------------------- */
#ifndef TRUE
#  define  TRUE   1
#endif

#ifndef FALSE
#  define  FALSE  0
#endif

/* ----------------------------------------------------------------- */
/* Define structure for input parameters                             */
/* ----------------------------------------------------------------- */
typedef struct input_parms
{
   int   argc;
   char* adsmPasswd;
   char* dbName;
   char* logFile;
   char* label;
   char* logFilePath;
   char* logSize;
   char* mode;
   char* nodeNumber;
   char* operatingSys;
   char* redFile;
   char* responseFile;
   char* release;
   char* request;
   char* startingPage;
} INPUT_PARMS;

/* ----------------------------------------------------------------- */
/* Print error to Error Log macro                                    */
/* ----------------------------------------------------------------- */
#define PrintErr  { if ( ERROR_ACTIVE )                               \
                    {                                                 \
                      ErrorLog( inputParms, auditFileName,            \
                               systemCallParms, userExitRc,           \
                               errorHelpString) ;                     \
                    }                                                 \
                  }


/* ----------------------------------------------------------------- */
/* User Exit Function Prototypes for Archive and Retrieve            */
/* ----------------------------------------------------------------- */
unsigned int
    ArchiveFile( INPUT_PARMS *,      /* input parameter structure    */
                 char *,             /* system call parameter string */
                 char * ) ;          /* error isolation string       */

unsigned int
    RetrieveFile( INPUT_PARMS *,     /* input parameter structure    */
                  char *,            /* system call parameter string */
                  char * ) ;         /* error isolation string       */


/* ----------------------------------------------------------------- */
/* User Exit Function Prototype for Signal Handler                   */
/* ----------------------------------------------------------------- */
void SignalEnd( int ) ;              /* signal type                  */


/* ----------------------------------------------------------------- */
/* User Exit Function Prototypes for Audit and Error Logs            */
/* ----------------------------------------------------------------- */
unsigned int
     AuditLogStart( INPUT_PARMS *,   /* input parameter structure    */
                    char *,          /* audit file name (with path)  */
                    char *);         /* error isolation string       */

unsigned int
     AuditLogEnd( char *,            /* audit file name (with path)  */
                  unsigned int,      /* user exit return code        */
                  char * );          /* error isolation string       */

void ErrorLog(  INPUT_PARMS *,       /* input parameter structure    */
                char *,              /* audit file name (with path)  */
                char *,              /* system call parameter string */
                unsigned int,        /* user exit return code        */
                char * ) ;           /* error isolation string       */

unsigned int
   ParseArguments( int ,             /* input parameter count        */
                   char * [] ,       /* input paramter list          */
                   INPUT_PARMS * ,   /* input parameter structure    */
                   char * ) ;        /* error help string            */

unsigned int
   PrintArguments( FILE* fp,         /* output file pointer          */
                   INPUT_PARMS * ) ; /* input parameter structure    */


/* ----------------------------------------------------------------- */
/* User Exit Global variables                                        */
/* ----------------------------------------------------------------- */
unsigned int archiveRequested ;           /* archive requested flag  */
unsigned int retrieveRequested ;          /* retrieve requested flag */


/*********************************************************************/
/* User Exit Mainline                                                */
/*********************************************************************/
int main( int argc, char *argv[] )
{
   unsigned int userExitRc ;              /* user exit return code   */
   unsigned int auditLogRc ;              /* return call from audit  */

   INPUT_PARMS  inputParmsStruct;
   INPUT_PARMS *inputParms = &inputParmsStruct;

   char         systemCallParms[ SYSTEM_CALL_LEN ] ;
                                          /* system call parm string */
   char         auditFileName[ FILE_NAME_LEN ] ;
                                          /* audit log file name     */
   char         errorHelpString[ HELP_STRING_LEN ] ;
                                          /* error help string       */

   /* -------------------------------------------------------------- */
   /* Initialize variables                                           */
   /* -------------------------------------------------------------- */
   archiveRequested  = FALSE ;
   retrieveRequested = FALSE ;
   userExitRc        = RC_OK ;
   auditLogRc        = RC_OK ;
   memset( inputParms, '\0', sizeof(INPUT_PARMS) );
   memset( systemCallParms, '\0', SYSTEM_CALL_LEN ) ;
   memset( auditFileName,   '\0', FILE_NAME_LEN   ) ;
   memset( errorHelpString, '\0', HELP_STRING_LEN ) ;

   /* -------------------------------------------------------------- */
   /* Install signal handlers for terminate and interrupt            */
   /* -------------------------------------------------------------- */
   if (( signal( SIGTERM, SignalEnd ) == SIG_ERR ) ||
       ( signal( SIGINT,  SignalEnd ) == SIG_ERR ))
   {
      userExitRc = RC_DEFECT ;              /* handler not installed */

      sprintf( errorHelpString, "%s%s",
               "Unable to install signal handler(s)", NEW_LINE ) ;

      PrintErr ;
   }

   /* -------------------------------------------------------------- */
   /* Set the local variables to the passed parameters and create    */
   /* the system call string depending on the indicated action       */
   /* -------------------------------------------------------------- */
   if ( userExitRc == RC_OK )
   {
      userExitRc = ParseArguments( argc, argv, inputParms,
                                   errorHelpString ) ;

      /* ----------------------------------------------------------- */
      /* Determine the user exit action                              */
      /* ----------------------------------------------------------- */
      if ( (inputParms->request != NULL) &&
           (strcmp( inputParms->request, "ARCHIVE" ) == 0 ) )
      {
         archiveRequested = TRUE ;          /* action is ARCHIVE     */
      }
      else
      {
         if ( (inputParms->request != NULL) &&
              (strcmp( inputParms->request, "RETRIEVE" ) == 0 ))
         {
            retrieveRequested = TRUE ;      /* action is RETRIEVE    */
         }
         else
         {
            userExitRc = RC_PARM ;          /* invalid action        */

            sprintf( errorHelpString, "%s %s %s%s", "Action",
                     inputParms->request, "is not valid", NEW_LINE ) ;

            PrintErr ;
         }
      }
   }
   if ( userExitRc == RC_OK )
   {
      /* ----------------------------------------------------------- */
      /* Trace the start of execution if the user has asked for an   */
      /* audit log                                                   */
      /* ----------------------------------------------------------- */
      if ( AUDIT_ACTIVE )
      {
         sprintf( auditFileName,            /* audit log file name   */
                  "%s%s%s",                 /* format of the name    */
                  AUDIT_ERROR_PATH,         /* audit log file path   */
                  inputParms->request,      /* ARCHIVE or RETRIEVE   */
                  FILE_EXT ) ;              /* file extension/type   */

         auditLogRc = AuditLogStart(
                          inputParms,
                          auditFileName,
                          errorHelpString );/* error isolation string*/

         if ( auditLogRc == AUDIT_IO_ERROR )/* IO error on audit log */
         {
            PrintErr ;
         }
      }

      /* ----------------------------------------------------------- */
      /* Archive or retrieve the specified log file                  */
      /* ----------------------------------------------------------- */
      if ( archiveRequested )
      {
         userExitRc = ArchiveFile(
                       inputParms,          /* input param structure */
                       systemCallParms,     /* system call string    */
                       errorHelpString ) ;  /* error isolation string*/
      }
      else
      {
         userExitRc = RetrieveFile(
                       inputParms,          /* input param structure */
                       systemCallParms,     /* system call string    */
                       errorHelpString ) ;  /* error isolation string*/
      }

      /* ----------------------------------------------------------- */
      /* Trace any errors                                            */
      /* ----------------------------------------------------------- */
      if ( userExitRc != RC_OK )
      {
         PrintErr ;
      }

      /* ----------------------------------------------------------- */
      /* Trace the end of execution if the user has asked for an     */
      /* audit log and no error was received from the audit log      */
      /* start                                                       */
      /* ----------------------------------------------------------- */
      if (( AUDIT_ACTIVE ) &&
          ( auditLogRc != AUDIT_IO_ERROR ))
      {
         auditLogRc = AuditLogEnd (
                       auditFileName,       /* audit log file name   */
                       userExitRc,          /* user exit return code */
                       errorHelpString ) ;  /* error isolation string*/

         if ( auditLogRc == AUDIT_IO_ERROR )/* IO error on audit log */
         {
            PrintErr ;
         }
      }
   }

   /* -------------------------------------------------------------- */
   /* Return the specified value to the caller                       */
   /* -------------------------------------------------------------- */
   exit(userExitRc);
}


/*********************************************************************/
/* ArchiveFile() - Archive a log file to disk                        */
/*********************************************************************/
unsigned int ArchiveFile(INPUT_PARMS *inputParms,
                         char *systemCallParms,
                         char *errorHelpString)
{
   /* -------------------------------------------------------------- */
   /* Declare and initialize variables                               */
   /* -------------------------------------------------------------- */
   unsigned int  rc     = RC_OK ;           /* function return code  */
   signed   int  systemCallRc = RC_OK ;     /* system call rc        */
   char     archiveTargetName[FILE_NAME_LEN];/* qualified target name */
   char    *bufptr = NULL;
   char    *alignedbufptr;
   struct stat   stStatBuf;              

   if (stat(inputParms->logFilePath, &stStatBuf) == -1)
   {
      sprintf(errorHelpString, "stat() call failed, errno = %d%s",
              errno, NEW_LINE);
      
      rc = RC_UNKNOWN;
      goto exit;
   }

   /* -------------------------------------------------------------- */
   /* Construct the archive name                                     */
   /* -------------------------------------------------------------- */
   sprintf(archiveTargetName,              /* qualified target name */
           "%s%s%s%s%s%s",                 /* format of parm string */
           ARCHIVE_PATH,                   /* user ARCHIVE path     */
           inputParms->dbName,             /* database name         */
           SLASH,                          /* slash character       */
           inputParms->nodeNumber,         /* node number           */
           SLASH,                          /* slash character       */
           inputParms->logFile);           /* log file name         */
      
   if (S_ISDIR(stStatBuf.st_mode))
   {
      FILE *tempFp = NULL;                 /* temporary file pointer*/
      char  fileToArchive[FILE_NAME_LEN];  /* file to be archived   */

      memset( fileToArchive, '\0', FILE_NAME_LEN ) ;

      /* -------------------------------------------------------------- */
      /* Construct the file to archive                                  */
      /* -------------------------------------------------------------- */
      sprintf( fileToArchive,                  /* file to be archived   */
               "%s%s",                         /* format of parm string */
               inputParms->logFilePath,        /* log file path         */
               inputParms->logFile ) ;         /* log file name         */
      
      /* -------------------------------------------------------------- */
      /* Construct the archive system call string                       */
      /* -------------------------------------------------------------- */
      sprintf( systemCallParms,                /* parameter string      */
               "%s %s %s",
               COPY,                           /* system copy command   */
               fileToArchive,                  /* file to archive       */
               archiveTargetName ) ;           /* qualified target name */
      
      systemCallRc = system( systemCallParms ) ;
      
      if ( systemCallRc != RC_OK )
      {
         /* ----------------------------------------------------------- */
         /* Check to see if the file exists                             */
         /* ----------------------------------------------------------- */
         if (( tempFp = fopen( fileToArchive, "rb" )) == NULL )
         {
            if (errno == ENOENT)
            {
                rc = RC_OK;                
                strcpy(errorHelpString, 
                       "File does not exist, assume it is already archived.");
            }
            else
            {
                rc = RC_UNKNOWN;   
                sprintf(errorHelpString, 
                        "Fail to open the log file %s, errno = %d%s",
                        fileToArchive, errno, NEW_LINE);
            }
         }
         else
         {
            ( void ) fclose( tempFp ) ;        /* close the file        */
            
            sprintf( errorHelpString, "%s %d %s%s",
                     "Error archiving file.  Return code", systemCallRc,
                     "received from the system call", NEW_LINE ) ;

            /* -------------------------------------------------------------- */
            /* Remove the archive target if archive fails. If not removed,    */
            /* it is possible that the partial archived target file that      */
            /* left over might cause future confusion.                        */
            /* -------------------------------------------------------------- */
            sprintf( systemCallParms,                /* parameter string      */
                     "%s %s",
                     REMOVE,                         /* system remove command */
                     archiveTargetName ) ;           /* qualified target name */
      
            systemCallRc = system( systemCallParms ) ;

            rc = RC_UNKNOWN;                   /* copy failed           */
         }
      }
      else
      {
         rc = RC_OK;                           /* successful archive    */
      }
   }
   else /* logFilePath is not a directory, assume it is a raw device */
   {
      int fhSource; 
      int fhTarget;
      int logSize;
      int startingPage;
      int numBytes;
      offset_t offsetll;
      mode_t targetCreateMode;

      if (inputParms->logSize == NULL ||
          inputParms->startingPage == NULL)
      {
         sprintf(errorHelpString,
                 "logSize and startingPage parameters must be specified "
                 "for raw device.%s",
                 NEW_LINE);
      
         rc = RC_UNKNOWN;
         goto exit;
      }
      
      fhSource = open(inputParms->logFilePath, O_RDONLY | O_RSHARE);
      if (fhSource == -1)
      {
         sprintf(errorHelpString, "open log file path failed, errno = %d%s",
                 errno, NEW_LINE);
      
         rc = RC_UNKNOWN;
         goto exit;
      }
      
      targetCreateMode = S_IRUSR + S_IWUSR + S_IRGRP + S_IROTH;
      fhTarget = open(archiveTargetName, O_WRONLY | O_CREAT | O_TRUNC,
                      targetCreateMode);
      if (fhTarget == -1)
      {
         if (errno == ENOENT)
         {
            char subDir[FILE_NAME_LEN];
            int len;

            /* In case that subdirectories are not created */
            sprintf(subDir, "%s%s", 
                    ARCHIVE_PATH, inputParms->dbName);
            len = strlen(subDir);
            mkdir(subDir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
            sprintf(&subDir[len], "%s%s",
                    SLASH,
                    inputParms->nodeNumber);
            mkdir(subDir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

            fhTarget = open(archiveTargetName, O_WRONLY | O_CREAT | O_TRUNC,
                            targetCreateMode);
         }

         if (fhTarget == -1)
         { 
            sprintf(errorHelpString, "open archive target failed, errno = %d%s",
                    errno, NEW_LINE);

            rc = RC_UNKNOWN;
            goto exit;
         }
      }

      /* -------------------------------------------------------------- */
      /* The following lines of code are used to sector-align the       */
      /* the memory buffer that the log file will be placed in          */
      /* before being flushed to disk.  The reason this alignment is    */
      /* necessary is that IO on a raw device MUST be aligned.          */
      /* -------------------------------------------------------------- */
      bufptr = (char *)malloc(BUFFER_SIZE * 4096 + 511);
      alignedbufptr = (char*)(((((long)bufptr + 511) << 9) >> 9) & ~((long)0x1ff));
      alignedbufptr = (char*)(bufptr - (char*)(((long)bufptr << 9)>>9) + alignedbufptr);

      logSize = atoi(inputParms->logSize);
      startingPage = atoi(inputParms->startingPage);

      offsetll = (offset_t)startingPage * 4096;
      if (llseek(fhSource, offsetll, SEEK_SET) == -1)
      {
         sprintf(errorHelpString, "llseek() failed, errno = %d%s",
                 errno, NEW_LINE);

         rc = RC_UNKNOWN;
         goto exit;
      }

      while (logSize > 0)
      {
         if (logSize > BUFFER_SIZE)
         {
            numBytes = BUFFER_SIZE * 4096;
            logSize -= BUFFER_SIZE;
         }
         else
         {
            numBytes = logSize * 4096;
            logSize = 0;
         }

         rc = read(fhSource, alignedbufptr, numBytes);
         if (rc != numBytes)
         {
            sprintf(errorHelpString,
                    "read %d bytes failed, rc = %d, errno = %d%s",
                    numBytes, rc, errno, NEW_LINE);
      
            rc = RC_UNKNOWN;
            goto exit;
         }

         rc = write(fhTarget, alignedbufptr, numBytes);
         if (rc != numBytes)
         {
            sprintf(errorHelpString,
                    "write %d bytes failed, rc = %d, errno = %d%s",
                    numBytes, rc, errno, NEW_LINE);
      
            rc = RC_UNKNOWN;
            goto exit;
         }
      }

      rc = RC_OK;
      close(fhTarget);
      close(fhSource);
   }

exit:
   if (bufptr != NULL)
      free(bufptr);
   
   return( rc );
}


/*********************************************************************/
/* RetrieveFile() - Retrieve a log file from disk                    */
/*********************************************************************/
unsigned int
   RetrieveFile( INPUT_PARMS *inputParms,
                 char *systemCallParms,
                 char *errorHelpString )
{
   /* -------------------------------------------------------------- */
   /* Declare and initialize variables                               */
   /* -------------------------------------------------------------- */
   FILE        *tempFp = NULL  ;            /* temporary file pointer*/
   unsigned int rc     = RC_OK ;            /* function return code  */
   signed   int systemCallRc = RC_OK ;      /* system call rc        */
   char         fileToRetrieve[ FILE_NAME_LEN ] ;
                                            /* file to be retrieved  */
   char               tempFile[FILE_NAME_LEN];

   memset( fileToRetrieve, '\0', FILE_NAME_LEN ) ;

   /* -------------------------------------------------------------- */
   /* Construct the file to retrieve                                 */
   /* -------------------------------------------------------------- */
   sprintf( fileToRetrieve,                 /* file to be retrieved  */
            "%s%s%s%s%s%s%s",               /* format of parm string */
            RETRIEVE_PATH,                  /* user RETRIEVE path    */
            inputParms->dbName,             /* database name         */
            SLASH,                          /* slash character       */
            inputParms->nodeNumber,         /* node number           */
            SLASH,                          /* slash character       */
            inputParms->logFile,            /* log file name         */
            NULL_TERM ) ;                   /* NULL terminator       */

   strcpy(tempFile, inputParms->logFilePath);
   strcat(tempFile, inputParms->logFile);
   strcpy(&tempFile[strlen(tempFile) - 3], "TMP");

   /* -------------------------------------------------------------- */
   /* Construct the retrieve system call string                      */
   /* -------------------------------------------------------------- */
   sprintf( systemCallParms,                /* parameter string      */
            "%s %s %s%s",                   /* format of parm string */
            COPY,                           /* system copy command   */
            fileToRetrieve,                 /* file to retrieve      */
            tempFile,                       /* target file name      */
            NULL_TERM ) ;                   /* NULL terminator       */

   systemCallRc = system( systemCallParms ) ;

   if ( systemCallRc != RC_OK )
   {
      /* ----------------------------------------------------------- */
      /* Check to see if the file exists                             */
      /* ----------------------------------------------------------- */
      if (( tempFp = fopen( fileToRetrieve, "rb" )) == NULL )
      {
         /* File not in RETRIEVE_PATH, return OK to DB2 */
         rc = RC_OK;
      }
      else
      {
         ( void ) fclose( tempFp ) ;        /* close the file        */

         sprintf( errorHelpString, "%s %d %s%s",
                  "Error retrieving file.  Return code", systemCallRc,
                  "received from the system call", NEW_LINE ) ;

         rc = RC_UNKNOWN ;                  /* copy failed           */
      }
   }
   else
   {
      /* rename the .TMP file to .LOG file */
      sprintf(systemCallParms, "mv %s %s%s", 
              tempFile, inputParms->logFilePath, 
              inputParms->logFile);             
      rc = system(systemCallParms);
      if (rc != RC_OK)
      {
         sprintf(errorHelpString,"Filed to rename temp file, rc = %d",
                 rc);
         rc = RC_UNKNOWN;
      }
   }

   return( rc ) ;
}


/*********************************************************************/
/* SignalEnd() - If a signal has been raised for which we have       */
/*               installed a handler, perform the following:         */
/*                 . trace the signal in the error log  (if enabled) */
/*                 . exit the user exit with a RC_OPCAN return code  */
/*********************************************************************/
void SignalEnd( int sigNum )
{
   unsigned int userExitRc ;              /* user exit return code   */
   char         errorHelpString[ HELP_STRING_LEN ] ;
                                          /* error help string       */

   /* -------------------------------------------------------------- */
   /* Set the user exit return code to operator cancelled            */
   /* -------------------------------------------------------------- */
   userExitRc = RC_OPCAN ;

   /* -------------------------------------------------------------- */
   /* Log the error if the error log has been requested              */
   /* -------------------------------------------------------------- */
   if ( ERROR_ACTIVE )
   {
      memset(  errorHelpString, '\0', HELP_STRING_LEN ) ;

      sprintf( errorHelpString, "%s %d %s %s%s", "Signal", sigNum,
               ( sigNum == SIGTERM ) ? "(SIGTERM)" : "(SIGINT)",
               "has been raised", NEW_LINE ) ;

      ErrorLog( NULL, NULL, NULL, userExitRc, errorHelpString ) ;
   }

   /* -------------------------------------------------------------- */
   /* Exit the user exit with the appropriate return code            */
   /* -------------------------------------------------------------- */
   exit( userExitRc ) ;
}


/*********************************************************************/
/* AuditLogStart() - Log the following at user exit entrance:        */
/*                     1. time system call was made                  */
/*                     2. parameters passed to the user exit         */
/*                     3. system action                              */
/*                     4. media type                                 */
/*********************************************************************/
unsigned int
   AuditLogStart( INPUT_PARMS *inputParms,
                  char *auditFileName,
                  char *errorHelpString )
{
   FILE         *auditLogFp ;          /* pointer to audit log file  */
   unsigned int  auditLogRc ;          /* AuditLogStart() return code*/
   time_t        actionTime ;          /* date and time of exit start*/
   char          outputLine[ OUTPUT_LINE_LEN ] ;
                                       /* line to be written to log  */

   /* -------------------------------------------------------------- */
   /* Initialize variables                                           */
   /* -------------------------------------------------------------- */
   auditLogFp = NULL  ;
   auditLogRc = RC_OK ;
   memset( &actionTime,   0, sizeof( actionTime )) ;
   memset( outputLine, '\0', OUTPUT_LINE_LEN     ) ;

   /* -------------------------------------------------------------- */
   /* Open the audit log file using the appropriate file name and    */
   /* user defined file attributes                                   */
   /* -------------------------------------------------------------- */
   auditLogFp = fopen( auditFileName, AUDIT_ERROR_ATTR ) ;

   /* -------------------------------------------------------------- */
   /* If the audit log file opened successfully, write the data to   */
   /* the file                                                       */
   /* -------------------------------------------------------------- */
   if ( auditLogFp != NULL )
   {
      memset( outputLine, '*', DELIMITER_LEN ) ;
      outputLine[ DELIMITER_LEN ] = '\n' ;

      if (( fprintf( auditLogFp, outputLine ) ) <= 0 )
         auditLogRc = AUDIT_IO_ERROR ;

      time( &actionTime ) ;            /* time user exit started     */
      sprintf( outputLine,
               "%s%s%s",
               "Time Started:      ",
               ctime( &actionTime ),
               NEW_LINE ) ;

      if (( fprintf( auditLogFp, outputLine ) ) <= 0 )
         auditLogRc = AUDIT_IO_ERROR ;

      if ( PrintArguments( auditLogFp, inputParms ) != RC_OK )
         auditLogRc = AUDIT_IO_ERROR ;

      sprintf( outputLine,             /* system action              */
               "%s %s %s %s file %s %s %s%s%s",
               "System Action:    ",
               inputParms->request,
               ( archiveRequested ) ? "from" : "to",
               inputParms->logFilePath,
               inputParms->logFile,
               ( archiveRequested ) ? "to" : "from",
               ( archiveRequested ) ? ARCHIVE_PATH : RETRIEVE_PATH,
               inputParms->dbName,
               NEW_LINE ) ;

      if (( fprintf( auditLogFp, outputLine ) ) <= 0 )
         auditLogRc = AUDIT_IO_ERROR ;

      sprintf( outputLine,             /* user defined media type    */
               "%s %s%s",
               "Media Type:       ",
               MEDIA_TYPE,
               NEW_LINE ) ;

      if (( fprintf( auditLogFp, outputLine ) ) <= 0 )
         auditLogRc = AUDIT_IO_ERROR ;

      /* ----------------------------------------------------------- */
      /* If an error was encountered during the audit log write      */
      /* ----------------------------------------------------------- */
      if ( auditLogRc == AUDIT_IO_ERROR )
      {
         sprintf( errorHelpString,"%s%s",
                  "Error writing to the Audit Log file", NEW_LINE ) ;

         ( void ) fclose( auditLogFp ) ;
      }
      else
      {
         if ( fclose( auditLogFp ) )
         {
            auditLogRc = AUDIT_IO_ERROR ;

            sprintf( errorHelpString, "%s%s",
                     "Error closing Audit Log file", NEW_LINE ) ;
         }
      }
   }
   else                                     /* error opening file    */
   {
      auditLogRc = AUDIT_IO_ERROR ;

      sprintf( errorHelpString,"%s%s",
               "Error opening Audit Log file",NEW_LINE ) ;
   }

   return( auditLogRc ) ;
}


/*********************************************************************/
/* AuditLogEnd() - Log the following at user exit end:               */
/*                   1. time system call returned                    */
/*                   2. user exit return code                        */
/*********************************************************************/
unsigned int
   AuditLogEnd( char         *auditFileName,
                unsigned int  userExitRc,
                char         *errorHelpString )
{
   FILE         *auditLogFp ;          /* pointer to audit log file  */
   unsigned int  auditLogRc ;          /* AuditLogEnd() return code  */
   time_t        actionTime ;          /* date and time of exit end  */
   char          outputLine[OUTPUT_LINE_LEN];
                                       /* line to be written to log  */

   /* -------------------------------------------------------------- */
   /* Initialize variables                                           */
   /* -------------------------------------------------------------- */
   auditLogFp = NULL  ;
   auditLogRc = RC_OK ;
   memset( &actionTime,   0, sizeof( actionTime )) ;
   memset( outputLine, '\0', OUTPUT_LINE_LEN     ) ;

   /* -------------------------------------------------------------- */
   /* Open the audit log file using the appropriate file name and    */
   /* user defined file attributes                                   */
   /* -------------------------------------------------------------- */
   auditLogFp = fopen( auditFileName, AUDIT_ERROR_ATTR ) ;

   /* -------------------------------------------------------------- */
   /* If the audit log file opened successfully, write the data to   */
   /* the file                                                       */
   /* -------------------------------------------------------------- */
   if ( auditLogFp != NULL )
   {
      sprintf( outputLine,             /* user exit return code      */
               "%s %d            %s%s",
               "User Exit RC:     ",
               userExitRc,
               ( userExitRc ) ? "|||> ERROR <|||" : errorHelpString,
               NEW_LINE ) ;

      if (( fprintf( auditLogFp, outputLine ) ) <= 0 )
         auditLogRc = AUDIT_IO_ERROR ;

      time( &actionTime ) ;            /* time user exit completed   */
      sprintf( outputLine,
               "%s %s%s",
               "Time Completed:   ",
               ctime( &actionTime ),
               NEW_LINE ) ;

      if (( fprintf( auditLogFp, outputLine ) ) <= 0 )
         auditLogRc = AUDIT_IO_ERROR ;

      /* ----------------------------------------------------------- */
      /* If an error was encountered during the audit log write      */
      /* ----------------------------------------------------------- */
      if ( auditLogRc == AUDIT_IO_ERROR )
      {
         sprintf( errorHelpString,"%s%s",
                  "Error writing to the Audit Log file", NEW_LINE ) ;

         ( void ) fclose( auditLogFp ) ;
      }
      else
      {
         if ( fclose( auditLogFp ) )
         {
            auditLogRc = AUDIT_IO_ERROR ;

            sprintf( errorHelpString,"%s%s",
                     "Error closing Audit Log file", NEW_LINE ) ;
         }
      }
   }
   else                                     /* error opening file    */
   {
      auditLogRc = AUDIT_IO_ERROR ;

      sprintf( errorHelpString,  "%s%s",
               "Error opening Audit Log file", NEW_LINE ) ;
   }

   return( auditLogRc ) ;
}


/*********************************************************************/
/* ErrorLog() - Log the following if an error has occurred:          */
/*                . time the error occurred                          */
/*                . values of  parameters passed to the user exit    */
/*                . media type                                       */
/*                . audit log file name                              */
/*                . system call string                               */
/*                . user exit return code                            */
/*                . error isolation help string                      */
/*********************************************************************/
void ErrorLog( INPUT_PARMS *inputParms,
               char        *auditFileName,
               char        *systemCallParms,
               unsigned int userExitRc,
               char        *errorHelpString )
{
   FILE   *errorLogFp ;                /* pointer to error log file  */
   time_t  actionTime ;                /* date and time of error     */
   char    outputLine[ OUTPUT_LINE_LEN ] ;
                                       /* line to be written to log  */
   char    errorFileName[ FILE_NAME_LEN ] ;
                                       /* error log file name        */

   /* -------------------------------------------------------------- */
   /* Initialize variables                                           */
   /* -------------------------------------------------------------- */
   errorLogFp = NULL ;
   memset( &actionTime,      0, sizeof( actionTime )) ;
   memset( outputLine,    '\0', OUTPUT_LINE_LEN ) ;
   memset( errorFileName, '\0', FILE_NAME_LEN   ) ;

   /* -------------------------------------------------------------- */
   /* Open the error log file using the user defined name and file   */
   /* attributes                                                     */
   /* -------------------------------------------------------------- */
   sprintf( errorFileName,
            "%s%s",
            AUDIT_ERROR_PATH,          /* error log path             */
            ERROR_FILE_NAME ) ;        /* error log file name        */

   errorLogFp = fopen( errorFileName, AUDIT_ERROR_ATTR ) ;

   /* -------------------------------------------------------------- */
   /* If the error log file opened successfully, write the available */
   /* data to the file                                               */
   /* -------------------------------------------------------------- */
   if ( errorLogFp != NULL )
   {
      memset( outputLine, '*', DELIMITER_LEN ) ;
      outputLine[ DELIMITER_LEN ] = '\n' ;
      fprintf( errorLogFp, outputLine ) ;

      time( &actionTime ) ;            /* time error occurred        */
      sprintf( outputLine,
               "%s %s%s",
               "Time of Error:    ",
               ctime( &actionTime ),
               NEW_LINE ) ;
      fprintf( errorLogFp, outputLine ) ;


      if ( inputParms != NULL )        /* parmeters passed to user   */
      {                                /* exit                       */
         (void) PrintArguments( errorLogFp, inputParms ) ;
      }

      sprintf( outputLine,             /* audit log file name        */
               "%s %s%s",
               "Audit Log File:   ",
               auditFileName,
               NEW_LINE ) ;
      fprintf( errorLogFp, outputLine ) ;

      sprintf( outputLine,             /* system call string         */
               "%s %s%s",
               "System Call Parms:",
               systemCallParms,
               NEW_LINE ) ;
      fprintf( errorLogFp, outputLine ) ;

      sprintf( outputLine,             /* user defined media type    */
               "%s %s%s",
               "Media Type:       ",
               MEDIA_TYPE,
               NEW_LINE ) ;
      fprintf( errorLogFp, outputLine ) ;

      sprintf( outputLine,             /* user exit return code      */
               "%s %d %s%s",
               "User Exit RC:     ",
               userExitRc,
               NEW_LINE,
               NEW_LINE ) ;
      fprintf( errorLogFp, outputLine ) ;

      sprintf( outputLine,             /* error isolation string     */
               "%s %s%s%s",
               "> Error isolation:",
               errorHelpString,
               NEW_LINE,
               NEW_LINE ) ;
      fprintf( errorLogFp, outputLine ) ;

      /* ----------------------------------------------------------- */
      /* Close the error log file                                    */
      /* ----------------------------------------------------------- */
      fclose( errorLogFp ) ;
   }

   return ;
}


unsigned int
   ParseArguments( int argc ,
                   char *argv[] ,
                   INPUT_PARMS *inputParms ,
                   char *errorHelpString )
{
   int parseRc;                      /* ParseArguments() return code */
   int count;                        /* index for for loop           */
   char *argument;                   /* pointer to argument          */
   int  parmLen;                     /* length of parameter          */
   int  parmIden;                    /* parameter identifier         */
   char *parmValue;                  /* parameter value              */


   /* -------------------------------------------------------------- */
   /* Initialize variables                                           */
   /* -------------------------------------------------------------- */
   parseRc = RC_OK ;
   count   = 1 ;

   /* -------------------------------------------------------------- */
   /* Copy values into inputParms structure.                         */
   /* -------------------------------------------------------------- */
   inputParms->argc = argc;

   while ( ( count < argc ) && ( parseRc == RC_OK) )
   {
      argument = argv[count];

      parmLen = strlen(argument);
      if (parmLen < 4)
      {
        parseRc = RC_PARM;
        break;
      }

      parmIden = ((argument[2]) | (argument[1] << 8) | (argument[0] << 16));
      parmValue = &argument[3];

#define INPUT_PARM_AP 0x2d4150    /* -AP */
#define INPUT_PARM_DB 0x2d4442    /* -DB */
#define INPUT_PARM_LN 0x2d4c4e    /* -LN */
#define INPUT_PARM_LB 0x2d4c42    /* -LB */
#define INPUT_PARM_LP 0x2d4c50    /* -LP */
#define INPUT_PARM_LS 0x2d4c53    /* -LS */
#define INPUT_PARM_MD 0x2d4d44    /* -MD */
#define INPUT_PARM_NN 0x2d4e4e    /* -NN */
#define INPUT_PARM_OS 0x2d4f53    /* -OS */
#define INPUT_PARM_RD 0x2d5244    /* -RD */
#define INPUT_PARM_RF 0x2d5246    /* -RF */
#define INPUT_PARM_RL 0x2d524c    /* -RL */
#define INPUT_PARM_RQ 0x2d5251    /* -RQ */
#define INPUT_PARM_SP 0x2d5350    /* -SP */

      switch(parmIden)
      {
        case INPUT_PARM_AP:            /* ADSM password              */
           inputParms->adsmPasswd = parmValue;
           break;
        case INPUT_PARM_DB:            /* database name              */
           inputParms->dbName = parmValue;
           break;
        case INPUT_PARM_LN:            /* log file name              */
           inputParms->logFile = parmValue;
           break;
        case INPUT_PARM_LB:            /* label                      */
           inputParms->label = parmValue;
           break;
        case INPUT_PARM_LP:            /* log file path              */
           inputParms->logFilePath = parmValue;
           break;
        case INPUT_PARM_LS:            /* log file size              */
           inputParms->logSize = parmValue;
           break;
        case INPUT_PARM_MD:            /* mode                       */
           inputParms->mode = parmValue;
           break;
        case INPUT_PARM_NN:            /* node number                */
           inputParms->nodeNumber = parmValue;
           break;
        case INPUT_PARM_OS:            /* operating system           */
           inputParms->operatingSys = parmValue;
           break;
        case INPUT_PARM_RD:            /* redirection file           */
           inputParms->redFile = parmValue;
           break;
        case INPUT_PARM_RF:            /* response file              */
           inputParms->responseFile = parmValue;
           break;
        case INPUT_PARM_RL:            /* DB2 release                */
           inputParms->release = parmValue;
           break;
        case INPUT_PARM_RQ:            /* user exit request          */
           inputParms->request = parmValue;
           break;
        case INPUT_PARM_SP:            /* starting page offset       */
           inputParms->startingPage = parmValue;
           break;
        default:                       /* log unrecognized parameter */
           if ( ERROR_ACTIVE )
           {
              memset(  errorHelpString, '\0', HELP_STRING_LEN ) ;

              sprintf( errorHelpString,
                       "%s %s%s%s%s",
                       "Unrecognized parameter :",
                       argument,
                       NEW_LINE,
                       "Parameter has been ignored.",
                       NEW_LINE ) ;

              ErrorLog( NULL, NULL, NULL, RC_OK, errorHelpString ) ;
           }
           break;
      }

      count ++ ;                       /* increment count            */
   }

  return(parseRc);
}



unsigned int
   PrintArguments(FILE *fp, INPUT_PARMS *inputParms)
{
   char          outputLine[ OUTPUT_LINE_LEN ] ;
   int           printRc = RC_OK ;

   sprintf( outputLine,
            "%s %d%s",
            "Parameter Count:     ",
            inputParms->argc,
            NEW_LINE ) ;

   if (( fprintf( fp, outputLine ) ) <= 0 )
         printRc = AUDIT_IO_ERROR ;

   if ( printRc == RC_OK )
   {
      sprintf( outputLine,
               "%s %s",
               "Parameters Passed:",
               NEW_LINE ) ;
      if (( fprintf( fp, outputLine ) ) <= 0 )
         printRc = AUDIT_IO_ERROR;
   }

   if ( ( printRc == RC_OK ) &&
        ( inputParms->adsmPasswd != NULL ) )
   {
      sprintf( outputLine,
               "%s %s%s",
               "ADSM password:    ",
               inputParms->adsmPasswd,
               NEW_LINE ) ;
      if (( fprintf( fp, outputLine ) ) <= 0 )
         printRc = AUDIT_IO_ERROR;
   }

   if ( ( printRc == RC_OK ) &&
        ( inputParms->dbName != NULL ) )
   {
      sprintf( outputLine,
               "%s %s%s",
               "Database name:    ",
               inputParms->dbName,
               NEW_LINE ) ;
      if (( fprintf( fp, outputLine ) ) <= 0 )
         printRc = AUDIT_IO_ERROR;
   }

   if ( ( printRc == RC_OK ) &&
        ( inputParms->logFile != NULL ) )
   {
      sprintf( outputLine,
               "%s %s%s",
               "Logfile name:     ",
               inputParms->logFile,
               NEW_LINE ) ;
      if (( fprintf( fp, outputLine ) ) <= 0 )
         printRc = AUDIT_IO_ERROR;
   }

   if ( ( printRc == RC_OK ) &&
        ( inputParms->label != NULL ) )
   {
      sprintf( outputLine,
               "%s %s%s",
               "Label:            ",
               inputParms->label,
               NEW_LINE ) ;
      if (( fprintf( fp, outputLine ) ) <= 0 )
         printRc = AUDIT_IO_ERROR;
   }

   if ( ( printRc == RC_OK ) &&
        ( inputParms->logFilePath != NULL ) )
   {
      sprintf( outputLine,
               "%s %s%s",
               "Logfile path:     ",
               inputParms->logFilePath,
               NEW_LINE ) ;
      if (( fprintf( fp, outputLine ) ) <= 0 )
         printRc = AUDIT_IO_ERROR;
   }

   if ( ( printRc == RC_OK ) &&
        ( inputParms->logSize != NULL ) )
   {
      sprintf(outputLine,
              "%s%s 4K pages%s",
              "Logfile size:      ",
              inputParms->logSize,
              NEW_LINE);
      if (( fprintf( fp, outputLine ) ) <= 0 )
         printRc = AUDIT_IO_ERROR;
   }

   if ( ( printRc == RC_OK ) &&
        ( inputParms->mode != NULL ) )
   {
      sprintf( outputLine,
               "%s %s%s",
               "Mode:             ",
               inputParms->mode,
               NEW_LINE ) ;
      if (( fprintf( fp, outputLine ) ) <= 0 )
         printRc = AUDIT_IO_ERROR;
   }

   if ( ( printRc == RC_OK ) &&
        ( inputParms->nodeNumber != NULL ) )
   {
      sprintf( outputLine,
               "%s %s%s",
               "Node number:      ",
               inputParms->nodeNumber,
               NEW_LINE ) ;
      if (( fprintf( fp, outputLine ) ) <= 0 )
         printRc = AUDIT_IO_ERROR;
   }

   if ( ( printRc == RC_OK ) &&
        ( inputParms->operatingSys != NULL ) )
   {
      sprintf( outputLine,
               "%s %s%s",
               "Operating system: ",
               inputParms->operatingSys,
               NEW_LINE ) ;
      if (( fprintf( fp, outputLine ) ) <= 0 )
         printRc = AUDIT_IO_ERROR;
   }

   if ( ( printRc == RC_OK ) &&
        ( inputParms->redFile != NULL ) )
   {
      sprintf( outputLine,
               "%s %s%s",
               "Red file:         ",
               inputParms->redFile,
               NEW_LINE ) ;
      if (( fprintf( fp, outputLine ) ) <= 0 )
         printRc = AUDIT_IO_ERROR;
   }

   if ( ( printRc == RC_OK ) &&
        ( inputParms->responseFile != NULL ) )
   {
      sprintf( outputLine,
               "%s %s%s",
               "Response file:    ",
               inputParms->responseFile,
               NEW_LINE ) ;
      if (( fprintf( fp, outputLine ) ) <= 0 )
         printRc = AUDIT_IO_ERROR;
   }

   if ( ( printRc == RC_OK ) &&
        ( inputParms->release != NULL ) )
   {
      sprintf( outputLine,
               "%s %s%s",
               "Release:          ",
               inputParms->release,
               NEW_LINE ) ;
      if (( fprintf( fp, outputLine ) ) <= 0 )
         printRc = AUDIT_IO_ERROR;
   }

   if ( ( printRc == RC_OK ) &&
        ( inputParms->request != NULL ) )
   {
      sprintf( outputLine,
               "%s %s%s",
               "Request:          ",
               inputParms->request,
               NEW_LINE ) ;
      if (( fprintf( fp, outputLine ) ) <= 0 )
         printRc = AUDIT_IO_ERROR;
   }

   if ( ( printRc == RC_OK ) &&
        ( inputParms->startingPage != NULL ) )
   {
      sprintf(outputLine,
              "%s %s%s",
              "Starting page offset: ",
              inputParms->startingPage,
              NEW_LINE);
      if (( fprintf( fp, outputLine ) ) <= 0 )
         printRc = AUDIT_IO_ERROR;
   }

   return(printRc);
}
