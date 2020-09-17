// StringVal.cpp: implementation of the StringVal class.
//
//////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>
#include <stdint.h>  
#include "fstringval.h"

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


// need this global for an answer to not get lost
char nostring[80];


//   
//  return the string based on its matching value
//
char * svGetStringByValueM(SVHandle * SVH,int64_t Value,int64_t *GoodMatch)
{
    int64_t i = 0;

    while (1)
    { 
    
        if ((SVH->Table[i].string[0] == 0x00 ) && (SVH->Table[i].value  == -1))
        {
             *GoodMatch = SV_NO_MATCH;
             sprintf(nostring,"No Match for %d",Value);
             return(nostring);
        }

        if (SVH->Table[i].value == Value )
        {
            *GoodMatch = SV_MATCH;
            return(SVH->Table[i].string);
        }
    i++;   
    }
}


//   
//  return the string based on its matching value
//
char * svGetStringByValue( SVHandle * SVH,int64_t Value)
{

    int64_t i = 0;	

    while (1)
    { 
    
        if ((SVH->Table[i].string[0] == 0x00 ) && (SVH->Table[i].value  == -1))
        {
             sprintf(nostring,"No Match for %d",Value);
             return(nostring);
        }

        if (SVH->Table[i].value == Value )
        {
            return(SVH->Table[i].string);
        }
    i++;   
    }
}




//
//  get a value from a string
//
int64_t svGetValueByStringM(SVHandle * SVH,char * String,int64_t *GoodMatch)
{
    int i;
    i = 0;
    while(*(String+i) != 0x00) 
    {
        *(String+i) |= 0x20;      // do the case insensitive thing
        i++;
    }

    i = 0;
    while (1)
    { 
    
        if ((SVH->Table[i].string[0] == 0X00 ) && (SVH->Table[i].value  == -1))
        {
             *GoodMatch = SV_NO_MATCH;
             return(-1);
        }

        if(strcmp(SVH->Table[i].string,String) == 0)
        {
             *GoodMatch = SV_MATCH;
             return(SVH->Table[i].value);
	
        }
        i++;
     }
}


//
//  get a value from a string
//
int64_t svGetValueByString(SVHandle * SVH,char *String)
{
    int64_t i;
    i = 0;
    while(*(String+i) != 0x00) 
    {
        *(String+i) |= 0x20;      // do the case insensitive thing
        i++;
    }

    i = 0;
    while (1)
    { 
    
        if ((SVH->Table[i].string[0] == 0X00 ) && (SVH->Table[i].value  == -1))
        {
             sprintf(nostring,"No Match for %s",String);
             return(-1);
        }

        if(strcmp(SVH->Table[i].string,String) == 0)
        {
             return(SVH->Table[i].value);
	
        }
        i++;
     }
}

void svPrintStrings( SVHandle * SVH, char *HdrString)
{
    int64_t i;
    i = 0;
    
    printf("%s",HdrString);
    
    while (1)
    { 
        if ((SVH->Table[i].string[0] == 0x00 ) && (SVH->Table[i].value  == -1))
        {
             return;
        }
        printf("\n %2d) %12s   %d",i,SVH->Table[i].string,SVH->Table[i].value);
        i++;
     }
}


#if 0

int svAddStringValuePairOne(SVHandle * SVH,char *string, int value)
{
    if(SVH->TableEntries == SV_MAX_PAIRS_IN_TABLE) 
    {
        return (SV_ENTRIES_FULL);  // flag array is full 
    }
    if(strlen(string) >= SV_MAX_PAIRS_IN_TABLE) 
    {
        return (SV_STRING_TO_LONG);  // flag array is full 
    }
    strcpy(SVH->Table[SVH->TableEntries].string,string);
    SVH->Table[SVH->TableEntries].value=value;
    SVH->TableEntries++;
    return NO_ERROR;
}

int svAddStringValuePair(SVHandle * SVH, SVTable *ArrayPairs, int Entries)
{
int ReturnResult =  NO_ERROR;
int i;
   for( i = 0 ; ((i < Entries ) && (ReturnResult == NO_ERROR)) ; i++)
   {
   AddStringValuePairOne(SVH,ArrayPairs->string,ArrayPairs->value);
   ArrayPairs++;
   }

return ReturnResult; 
}

int svAddTable(SVHandle * SVH, SVTable *ArrayPairs)
{
int ReturnResult =  NO_ERROR;
int i;
   for( i = 0 ; ((ArrayPairs->string[0] != 0x00 ) && (ArrayPairs->value != -1)) && (ReturnResult == NO_ERROR) ; i++)
   {
   ReturnResult = AddStringValuePairOne(SVH,ArrayPairs->string,ArrayPairs->value);
   ArrayPairs++;
   }

return ReturnResult; 

}

//
// EVOL no check to make sure index is okay
int svGetValueByIndex(SVHandle * SVH,int index)
{
    return SVH->Table[index].value;
}
#endif


int64_t  svInit(SVHandle ** SVH,SVTable * Table)
{
    int64_t  i = 0;
    (*SVH)->Table=Table;  // Save the address of the table.

    // now count how big the table is

    while (( (*SVH)->Table[i].string[0] != 0x00 ) && 
           ( (*SVH)->Table[i].value     != -1   ))
    {
        i++;   
    }

    (*SVH)->TableEntries = i;
}


int64_t svGetValueByIndex(SVHandle * SVH,int64_t index,char ** String,int64_t * Value)
{
     if( ( index >= 0) && (index < SVH->TableEntries) )
     {
         *String = &( SVH->Table[index].string[0]);         
         *Value  =    SVH->Table[index].value;
         return 0;
     }
     *String = "";         
     *Value  = -1;
     return -1;
}
 
int64_t svGetNumberOfElements(SVHandle * SVH)
{
   return SVH->TableEntries;
}


#define DEBUG_STRING_VALUE
#undef DEBUG_STRING_VALUE

#ifdef DEBUG_STRING_VALUE



SVTable  HMICregs[]={
    {0x00,"ckm"},
    {0x07,"ckmd"},
    {0x03,"ckr"},
    {0x09,"ckrd"},
    {0x02,"ckp"},
    {0x04,"cks"},
    {0x05,"ck32"},
    {0x06,"ck10"},
    {0x01,"ckn"},
    {0x08,"cknd"},
    {0x17,"gpd"},
    {0x18,"gpr"},
    {0x20,"frla"},
    {0x21,"frha"},
    {0x22,"frlb"},
    {0x23,"frhb"},
    {0x24,"frpl"},
    {0x25,"frph"},
    {0x28,"clkerr1"},
    {0x29,"clkerr2"},
    {0x2a,"syserr"},
    {0x2b,"ckw"},
    {0x2c,"clkerr3"},
    {0x0e,"con"},
    {0xfe,"devid"},
    {-1,""}
};
// CEPT,D4,ESF
SVTable FramerLineTypeTable[]=
{
    {0,"FRAMETYPE_UNKNOWN"},
    {1,"FRAMETYPE_D4"},
    {2,"FRAMETYPE_ESF_4KHZ"},
    {3,"FRAMETYPE_CEPT"},
    {4,"FRAMETYPE_ESF_2KHZ_A"},
    {5,"FRAMETYPE_ESF_2KHZ_B"},
    {6,"FRAMETYPE_SLC96"},
    {7,"FRAMETYPE_T1DM"},
    {8,"FRAMETYPE_CEPT_CCS"},
    {-1,""} 
};


int fatoi(char * string)
{
	int j;
    int result=0;
    int i = strlen(string);
    
    if(i < 1)
        return result;
    for( j = 0; j < i ;j++)
    {
        result *= 10;
        if( ((*(string + j )) >= '0') && ((*(string + j)) <= '9'))
            
            result += (*(string + j)) & 0x0f;
        else
        {
            return 0;
        }
    }
    return result;
}


SVHandle  Regs;
SVHandle * gHmicRegs;
void main(void);
//#include <conio.h>

void main(void)
{
	int i;
	int done;
    char cmdstring[120];
    char tstr0[120];
    char tstr1[120];
    char tstr2[120];
    char tstr3[120];
	char Prompt[40];
	int argc;

    gHmicRegs = &Regs;


     svInit(&gHmicRegs,&HMICregs[0]);	
	
    // svAddStringValuePair((StringValueTable *)(&HMICregs[0]));	

    svPrintStrings(gHmicRegs,"\n\nPrint string routine");
    strcpy (Prompt ,"enter value>");

	done = 0;
	while(done == 0 )
	{
		printf("\n%s",Prompt);
		cmdstring[0]=0x00;  // terminate the string to start
		tstr0[0]=0x00;  // terminate the string to start
		gets(cmdstring);
		argc = sscanf(cmdstring,"%s %s %s %s",tstr0,tstr1,tstr2,tstr3);
		switch (tstr0[0]){
		case 0x1b:
		case 'D':
		case 'd':
			done = 1;
			break;
		default:
			i = fatoi(tstr0);
			printf("\n Corresponding String:  %s",svGetStringByValue(gHmicRegs, i));
			break;
		}//end switch
    }
    strcpy (Prompt ,"string>");

	done = 0;
	while(done == 0 )
	{
		printf("\n%s",Prompt);
		cmdstring[0]=0x00;  // terminate the string to start
		tstr0[0]=0x00;  // terminate the string to start
		gets(cmdstring);
		argc = sscanf(cmdstring,"%s %s %s %s",tstr0,tstr1,tstr2,tstr3);
		switch (tstr0[0]){
		case 0x1b:
		case 'D':
		case 'd':
			done = 1;
			break;
		default:
			printf("\n Corresponding Value for  %s : %d",tstr0,svGetValueByString(gHmicRegs,tstr0));
			break;
		}//end switch
	}
 	

}

#endif
