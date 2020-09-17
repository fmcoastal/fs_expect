// fstringval.h: int64_terface for the StringVal class.
//
//////////////////////////////////////////////////////////////////////


#define SV_MAX_CHARS_IN_STRING  80
#define SV_MAX_PAIRS_IN_TABLE   50

#ifndef NO_ERROR
#define NO_ERROR 0
#endif

// ERRORS
#define SV_MATCH           0
#define SV_ERROR_BASE      0
#define SV_NO_MATCH        SV_ERROR_BASE-1
#define SV_ENTRIES_FULL    SV_ERROR_BASE-2
#define SV_STRING_TO_LONG  SV_ERROR_BASE-3


#define SV_ERROR_NO_MATCH  0x80000000


typedef struct SVTableStruct{
    int64_t  value;
    char string[SV_MAX_CHARS_IN_STRING];
}SVTable;


typedef struct SVHandleStruct{
    int64_t  TableEntries;
    SVTable * Table;
}SVHandle;




int64_t  svInit(SVHandle ** SVH, SVTable * Table;);
#if 0
int64_t  svDistroy(SVHandle ** SVH);
int64_t  svAddStringValuePairOne( SVHandle * SVH , char * string, int64_t value);
int64_t  svAddStringValuePair(SVHandle * SVH, SVTable *ArrayPairs,int64_t Entries);
int64_t  svAddTable(SVHandle * SVH , SVTable *ArrayPairs);
#endif

int64_t svGetValueByStringM(SVHandle * SVH, char * String,int64_t *GoodMatch);
int64_t svGetValueByString (SVHandle * SVH, char * String);
char * svGetStringByValueM(SVHandle * SVH, int64_t Value,int64_t *GoodMatch);
char * svGetStringByValue(SVHandle * SVH, int64_t Value);
void svPrintStrings(SVHandle * SVH ,  char *HdrString);
// EVOL no check to make sure index is okay
int64_t svGetValueByIndex(SVHandle * SVH,int64_t index,char ** String,int64_t * Value);
int64_t svGetNumberOfElements(SVHandle * SVH);

//    int64_t GetNubOfEntries(void){return(m_TableEntries);};


