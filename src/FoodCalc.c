/*

  FoodCalc

  This is the FoodCalc program written by Jesper Lauritsen (the writing of the
  initial version was in part funded by the Diet, Cancer and Health project at the
  Danish Cancer Society).
  The program is in the public domain. You can use it and change it in any way
  you like. However, you should always give due credit to Jesper Lauritsen. Also,
  neither Jesper Lauritsen nor the Danish Cancer Society can in no way be responsibly
  for any damages done by this program. Also, neither Jesper Lauritsen nor the Danish
  Cancer Society gives any guaranties what so ever regarding the functionality or
  correctness of this program. Basically you can use it if you want, but if it does not
  follow your expectations, you have no one to blame but your self.

  Enough of that. You may want to look for a newer version of this program at
  http://www.ibt.ku.dk/jesper/FoodCalc. If you have any questions, suggestions, bug
  reports or any thing else you may want to send an e-mail to Jesper.Laurtisen@acm.org
  -- you may get an answer :-).

  You should look at this source code with an editor with tabs set to 4.
  The program is written in ANSI C. An old fashioned UNIX C compiler will not work,
  but you do not need newish stuff like a C++ compiler.
  And of course, first of all you should read the documentation in FoodCalc.html!

  The program has evolved over quite a few versions (since pre version 1.0), whith
  the result that the code now is quite complex. The code will probably be easiest
  to follow if you know a little about classical compiler writing. You can se this
  program as doing a parse, a quite extensive optimation, a generation of (higly
  specialized) intermediary code, and interpretation of the intermediary. The save:
  command saves the intermediary without executing it, and the -s option executes
  a previously saved intermediary.

  The program usually follows these steps of execution:

  STEP 1: Parse the commands with readCommands().
  STEP 2: Lay out memory positions etc. for fields with readFields().
  STEP 3: Read any groups files with readGroups().
  STEP 4: Read any foods files with readFoods().
  STEP 5: Expand all groups with expandGroups().
  STEP 5.5: Calculate cook fractions for all weight cook commands with weightCookChange().
  STEP 6: Read any recipe files with readRecipes().
  STEP 7: Read the input file and write the output file with readInput().
  
  You can find these steps in main and in the partitioning of this source file.

  If the "save:" command is used, step 7 will not be done fully - instead the
  program state will be saved with save().
  If FoodCalc is called with -s, then stemp 1-6 will not be done - instead the
  program state will be read with get().


  Program history (make sure to change programMinor and/or programMajor below):

  v. 1.0  19980616  Jesper Lauritsen
                    Initial version!
  v. 1.1  19980703  Jesper Lauritsen
                    Treat \r as blank space.
					Fix bug when separator is space and there are blanks at the
					end of a line.
					Fix bug when the last lines are comment lines.
					Fix bug when a recipe is used as ingredins in the recipe just
					following.
  v. 1.2  19980717  Jesper Lauritsen
                    Allow comments at end of lines.
					Changable comment char in data files. New comment: command.
					Continuation lines (with =).
					Text fields in data files. New text fields: command.
					Star notation in data files. New input *fields: command.
					Recipe handling totally rewitten.
					Recipe fields with same name as noCalc fields in the food table
					will now be copied to the food table when ingredients: sum is
					used.
					New recipe weight reduce field: command.
					New recipe reduce field: command is an alias for reduce field:.
					New food weight: command is an alias for recipe sum:.
					New weight cook: and weight reduce field: commands.
					New non-edible field: command.
					Verbosity level. New -v option and new verbosity: command.
					New options -l, -i, -o. The -s option now takes only one argument.
  v. 1.3  19981217  Jesper Lauritsen
					Added set:, recipe set: and group set: commands. Changed calculate:
					and	food weight: (/recipe sum:) to use the new set: calculations.
					Added where: command and changed if: and if not: to use the new
					where: test.
					Added transpose: command.
					Allow more than one id field in groups files.
					Allow grouping on any number of input and food table fields.
					New keepx argument to ingredients: command.

*/


#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>


/* the name and current version of the program. You should increase programMinor
   or programMajor with any new release of the program. */
char* program = "FoodCalc";
int programMajor = 1;
int programMinor = 3;



/* The Num type is used for all numeric values (nutrient values, etc.) You could
   probably change it to double for greater precision. This would use more memory
   and it might or might not increase the running time. */
typedef float Num;


#define forward


/****************************************************************************/
/*** Syntax and syntax tree (read and written by step 1, readCommands() */

/* A Cmd is used to hold a series of parsed commands. */
typedef struct CmdStruct {
	struct CmdStruct* next;
	char** args;
} Cmd;

/* A CmdDef defines the syntax of a command */
typedef enum {strArg, chArg, listArg, numArg, setArg, whereArg} ArgType;
typedef struct {
	char* name;			/* the keyword of the command */
	enum {required, optional} type;
	enum {single, multiple} occur;
	int numArgs;		/* max number of arguments */
	int numReqArgs;		/* min number of arguments */
	Cmd** cmd;			/* address of pointer to the Cmd which will hold the parsed syntax tree */
	ArgType* args; 		/* The types of the arguments. listArg must only be last! */
} CmdDef;


/* Below is a CmdDef for each command, together they describe the syntax of the "language" */
/* Also below is a Cmd for each command. together the descibe the syntax tree of the "program" */

Cmd* logCmd = NULL;
ArgType	logArgs[] = {strArg/*filename*/};
CmdDef logDef = {"log",optional,single,1,1,&logCmd,logArgs};

Cmd* decimalPointCmd = NULL;
ArgType decimalPointArgs[] = {chArg/*decPoint char*/};
CmdDef decimalPointDef = {"decimal point",optional,single,1,1,&decimalPointCmd,decimalPointArgs};

Cmd* foodsCmd = NULL;
ArgType foodsArgs[] = {strArg/*filename*/,strArg/*id field*/,chArg/*sep char*/,chArg/*decPoint char*/};
CmdDef foodsDef = {"foods",required,multiple,4,1,&foodsCmd,foodsArgs};

Cmd* commandsCmd = NULL;
ArgType commandsArgs[] = {strArg/*filename*/};
CmdDef commandsDef = {"commands",optional,multiple,1,1,&commandsCmd,commandsArgs};

Cmd* separatorCmd = NULL;
ArgType separatorArgs[] = {chArg/*sep char*/};
CmdDef separatorDef = {"separator",optional,single,1,1,&separatorCmd,separatorArgs};

Cmd* groupsCmd = NULL;
ArgType groupsArgs[] = {strArg/*filename*/,listArg/*id fields*/,
						chArg/*sep char*/,chArg/*decPoint char*/,chArg/*comment char*/};
CmdDef groupsDef = {"groups",optional,multiple,5,1,&groupsCmd,groupsArgs};

Cmd* cookCmd = NULL;
ArgType cookArgs[] = {strArg/*type*/,strArg/*field*/,listArg/*field list*/};
CmdDef cookDef = {"cook",optional,multiple,3,3,&cookCmd,cookArgs};

Cmd* weightCookCmd = NULL;
ArgType weightCookArgs[] = {strArg/*type*/,strArg/*field*/,listArg/*field list*/};
CmdDef weightCookDef = {"weight cook",optional,multiple,3,3,
                        &weightCookCmd,weightCookArgs};

Cmd* inputCmd = NULL;
ArgType inputArgs[] = {strArg/*filename*/,strArg/*id field*/,strArg/*amount field*/,
					   chArg/*sep char*/,chArg/*decPoint char*/,chArg/*comment char*/};
CmdDef inputDef = {"input",required,single,6,1,&inputCmd,inputArgs};

Cmd* inputFieldsCmd = NULL;
ArgType inputFieldsArgs[] = {listArg/*field list*/};
CmdDef inputFieldsDef = {"input fields",optional,single,1,1,&inputFieldsCmd,inputFieldsArgs};

Cmd* inputScaleCmd = NULL;
ArgType inputScaleArgs[] = {numArg/*scale*/};
CmdDef inputScaleDef = {"input scale",optional,single,2,1,&inputScaleCmd,inputScaleArgs};

Cmd* cookFieldCmd = NULL;
ArgType cookFieldArgs[] = {strArg/*field*/,listArg/*types*/};
CmdDef cookFieldDef = {"cook field",optional,single,2,2,&cookFieldCmd,cookFieldArgs};

Cmd* weightReducFieldCmd = NULL;
ArgType weightReduceFieldArgs[] = {strArg/*field*/,listArg/*fields*/};
CmdDef weightReducFieldDef = {"weight reduce field",optional,multiple,2,2,
							  &weightReducFieldCmd,weightReduceFieldArgs};

Cmd* nonEdibleFieldCmd = NULL;
ArgType nonEdibleFieldArgs[] = {strArg/*field*/,strArg/*field*/};
CmdDef nonEdibleFieldDef = {"non-edible field",optional,single,2,1,
							&nonEdibleFieldCmd,nonEdibleFieldArgs};

Cmd* reducFieldCmd = NULL;
ArgType reduceFieldArgs[] = {strArg/*field*/,listArg/*fields*/};
CmdDef reducFieldDef = {"reduce field",optional,multiple,2,2,&reducFieldCmd,reduceFieldArgs};
/* recipe reduce field: is an alias for reduce field: */
CmdDef recipeReducFieldDef = {"recipe reduce field",optional,multiple,2,2,&reducFieldCmd,reduceFieldArgs};

Cmd* recipeWeightReducFieldCmd = NULL;
ArgType recipeWeightReduceFieldArgs[] = {strArg/*field*/,listArg/*fields*/};
CmdDef recipeWeightReducFieldDef = {"recipe weight reduce field",optional,multiple,2,2,
									&recipeWeightReducFieldCmd,recipeWeightReduceFieldArgs};

Cmd* outputCmd = NULL;
ArgType outputArgs[] = {strArg/*filename*/,chArg/*sep char*/,chArg/*decPoint char*/};
CmdDef outputDef = {"output",required,single,3,1,&outputCmd,outputArgs};

Cmd* outputFieldsCmd = NULL;
ArgType outputFieldsArgs[] = {listArg/*field list*/};
CmdDef outputFieldsDef = {"output fields",optional,single,1,1,&outputFieldsCmd,outputFieldsArgs};

Cmd* groupByCmd = NULL;
ArgType groupByArgs[] = {listArg/*field list*/};
CmdDef groupByDef = {"group by",optional,single,1,1,&groupByCmd,groupByArgs};

Cmd* noCalcFieldsCmd = NULL;
ArgType noCalcFieldsArgs[] = {listArg/*field list*/};
CmdDef noCalcFieldsDef = {"no-calc fields",optional,multiple,1,1,&noCalcFieldsCmd,noCalcFieldsArgs};

Cmd* calculateCmd = NULL;
ArgType calculateArgs[] = {strArg/*field name*/,listArg/*additives*/};
CmdDef calculateDef = {"calculate",optional,multiple,3,3,&calculateCmd,calculateArgs};

Cmd* setCmd = NULL;
ArgType setArgs[] = {setArg/*field = exp*/};
CmdDef setDef = {"set",optional,multiple,1,1,&setCmd,setArgs};

Cmd* recipeSetCmd = NULL;
ArgType recipeSetArgs[] = {setArg/*field = exp*/};
CmdDef recipeSetDef = {"recipe set",optional,multiple,1,1,&recipeSetCmd,recipeSetArgs};

Cmd* groupSetCmd = NULL;
ArgType groupSetArgs[] = {setArg/*field = exp*/};
CmdDef groupSetDef = {"group set",optional,multiple,1,1,&groupSetCmd,groupSetArgs};

Cmd* recipesCmd = NULL;
ArgType recipesArgs[] = {strArg/*file name*/,strArg/* recipe id*/,strArg/*food id*/,strArg/*amount*/,
						 chArg/*sep char*/,chArg/*decPoint char*/,chArg/*comment char*/};
CmdDef recipesDef = {"recipes",optional,multiple,7,1,&recipesCmd,recipesArgs};

/* the prefered is now to use the food weight: command, but it is actually just an
   alias for the obsolete recipe sum: command */
Cmd* recipeSumCmd = NULL;
ArgType recipeSumArgs[] = {numArg/*sum*/,listArg/*field list*/};
CmdDef recipeSumDef = {"recipe sum",optional,single,2,1,&recipeSumCmd,recipeSumArgs};
CmdDef foodWeightDef = {"food weight",optional,single,2,1,&recipeSumCmd,recipeSumArgs};

Cmd* ingredientsCmd = NULL;
ArgType ingredientsArgs[] = {strArg/*keep|sum*/};
CmdDef ingredientsDef = {"ingredients",optional,single,1,1,&ingredientsCmd,ingredientsArgs};

Cmd* inputFormatCmd = NULL;
ArgType inputFormatArgs[] = {strArg/*text|bin-native*/};
CmdDef inputFormatDef = {"input format",optional,single,1,1,&inputFormatCmd,inputFormatArgs};

Cmd* outputFormatCmd = NULL;
ArgType outputFormatArgs[] = {strArg/*text|text-no-head|bin-native*/};
CmdDef outputFormatDef = {"output format",optional,single,1,1,&outputFormatCmd,outputFormatArgs};

Cmd* saveCmd = NULL;
ArgType saveArgs[] = {strArg/*file name*/};
CmdDef saveDef = {"save",optional,single,1,1,&saveCmd,saveArgs};

Cmd* blipCmd = NULL;
ArgType blipArgs[] = {numArg/*num obs*/};
CmdDef blipDef = {"blip",optional,single,1,1,&blipCmd,blipArgs};

Cmd* ifCmd = NULL;
ArgType ifArgs[] = {strArg/*field name*/,listArg/*values*/};
CmdDef idDef = {"if",optional,multiple,2,2,&ifCmd,ifArgs};

Cmd* ifNotCmd = NULL;
ArgType ifNotArgs[] = {strArg/*field name*/,listArg/*values*/};
CmdDef ifNotDef = {"if not",optional,multiple,2,2,&ifNotCmd,ifNotArgs};

Cmd* whereCmd = NULL;
ArgType whereArgs[] = {whereArg/*bool exp*/};
CmdDef whereDef = {"where",optional,single,1,1,&whereCmd,whereArgs};

Cmd* inputStarFieldsCmd = NULL;
ArgType inputStarFieldsArgs[] = {listArg/*field list*/};
CmdDef inputStarFieldsDef = {"input *fields",optional,single,1,1,&inputStarFieldsCmd,inputStarFieldsArgs};

Cmd* textFieldsCmd = NULL;
ArgType textFieldsArgs[] = {listArg/*field list*/};
CmdDef textFieldsDef = {"text fields",optional,multiple,1,1,&textFieldsCmd,textFieldsArgs};

Cmd* commentCmd = NULL;
ArgType commentArgs[] = {chArg/*comment char*/};
CmdDef commentDef = {"comment",optional,single,1,1,&commentCmd,commentArgs};

Cmd* verbosityCmd = NULL;
ArgType verbosityArgs[] = {numArg/*level*/};
CmdDef verbosityDef = {"verbosity",optional,single,1,1,&verbosityCmd,verbosityArgs};

Cmd* transposeCmd = NULL;
ArgType transposeArgs[] = {strArg/*field name*/,numArg/*no*/,listArg/*field list*/};
CmdDef transposeDef = {"transpose",optional,multiple,3,3,&transposeCmd,transposeArgs};


/* a list of all command definitions: */
CmdDef* cmdDefs[] = {&logDef,&decimalPointDef,&foodsDef,&commandsDef,&separatorDef,
	&groupsDef,&cookDef,&inputDef,&inputFieldsDef,&inputScaleDef,
	&cookFieldDef,&reducFieldDef,&outputDef,&outputFieldsDef,&groupByDef,
	&noCalcFieldsDef,&calculateDef,&recipesDef,&recipeSumDef,&ingredientsDef,
	&inputFormatDef,&outputFormatDef,&saveDef,&blipDef,&idDef,&ifNotDef,
	&inputStarFieldsDef,&textFieldsDef,&commentDef,&recipeWeightReducFieldDef,
	&foodWeightDef,&recipeReducFieldDef,&weightReducFieldDef,&weightCookDef,
	&nonEdibleFieldDef,&verbosityDef,&transposeDef,&setDef,&recipeSetDef,&whereDef,
	&groupSetDef,NULL};



/****************************************************************************/
/*** Utility functions for log, error, string and memory handling */


char* mainCommandsName;		/* name of the main commands file - set by main() */
char* logFileName = NULL;	/* name of the log file - set by openLog() or get() */
char logName[300];			
FILE* logFile = NULL;		/* the log file */
int errors = 0;				/* number of errors so far - incrementet by error() */
int verbosity = 0;			/* current verbosity level */



/* opens the log file - should only be called by the other log and error utililty
   functions. */
void openLog() {

	/* set verbosity level */
	if (!verbosity) {
		/* it has not been set by someone else */
		if (verbosityCmd) {
			verbosity = atoi(*(verbosityCmd->args));
			if (verbosity <= 0) verbosity = 50;
		} else {
			verbosity = 50;
		}
	}
	
	/* find the name of the log file: */
	if (!logFileName) {
		/* logFileName has not been set by someone else */
		if (logCmd) {
			/* we have read a log: command, so we use that */
			logFileName = *(logCmd->args);
		}else {
			/* the user has not specified a name, so we make a name based on the
			   name of the main commands file */
			strncpy(logName,mainCommandsName,300);
			if (strcmp(logName,"-") != 0) {
				if (strlen(logName) > 3 && strcmp(logName+strlen(logName)-3,".fc") == 0) 
					logName[strlen(logName)-3] = '\0';
				else if (strlen(logName) > 4 && strcmp(logName+strlen(logName)-4,".txt") == 0) 
					logName[strlen(logName)-4] = '\0';
				strncat(logName,".log",300-strlen(logName));
			}
			logFileName = logName;
		}
	}

	/* open the log file, and tell the the user the name of the log file */
	if (verbosity <= 1) {
		logFile = (FILE*)1;
		return;
	}
	if (strcmp(logFileName,"-") != 0) {
		if (logFile = fopen(logFileName,"w")) {
			if (verbosity > 30) fprintf(stderr,"FoodCalc: Logging to %s\n",logFileName);
		} else {
			logFile = stderr;
			if (verbosity > 30) 
				fprintf(stderr,"FoodCalc: Could not open log file %s - logging to stderr\n",
					logFileName);
		}
	} else {
		logFile = stderr;
		if (verbosity > 30) fprintf(stderr,"FoodCalc: Logging to stderr\n");
	}

	if (verbosity > 20) {	/* write a greeting in the log */
		struct tm* newtime;
		time_t aclock;
		time(&aclock); newtime = localtime(&aclock);
		fprintf(logFile,"%s v. %d.%d\n%s\n",
			program,programMajor,programMinor,asctime(newtime));
	}
}


/* write a message to the log and then abort */
void abortAndExit(char* str, ...) {
	va_list va;
	if (!logFile) openLog();
	if (verbosity > 1) {
		va_start(va,str);
		fprintf(logFile,"** ");
		vfprintf(logFile,str,va);
		fprintf(logFile,"ABORTED!\n");
	}
	exit(1);
}


/* write a message to the log and increace the error count. Abort if too many errors */
void error(char* str, ...) {
	va_list va;
	if (!logFile) openLog();
	if (verbosity > 10) {
		va_start(va,str);
		fprintf(logFile,"** ");
		vfprintf(logFile,str,va);
	}
	if (++errors > 20) abortAndExit("Too many errors!\n");
}


/* write a warning to the log */
void warning(char* str, ...) {
	va_list va;
	if (!logFile) openLog();
	if (verbosity > 40) {
		va_start(va,str);
		fprintf(logFile,"Warning: ");
		vfprintf(logFile,str,va);
	}
}


/* write a message to the log */
void logmsg(char* str, ...) {
	va_list va;
	if (!logFile) openLog();
	if (verbosity > 20) {
		va_start(va,str);
		vfprintf(logFile,str,va);
	}
}


/* change upper case to lower case */
void strToLowercase(char* str) {
	char* p = str;
	while (*p) {if (isupper(*p)) *p = tolower(*p); p++;}
}


/* alloc a number of bytes of memory (with no initialization) */
void* alloc(size_t size) {
	void* p;
	if (!(p = malloc(size))) 
		abortAndExit("Could not allocate enough memory!\n");
	return(p);
}


/* allocate memory for a structure (with no initialization) */
#define allocStruct(Type) alloc(sizeof(Type))


/* allocate an array of element (initialized with zeros */
void* allocarray(size_t num, size_t size) {
	void* p;
	if (!(p = calloc(num,size))) abortAndExit("Could not allocate enough memory!\n");
	return(p);
}


/* alocate memory for a string and then copy the string to the new memory */
void* allocStr(char* str, int len) {
	void* p;
	p = alloc(len+1);
	memcpy(p,str,len+1);
	return(p);
}



/****************************************************************************/
/*** Utility types and functions to handle integer based hashes */


typedef struct _HashIntEntry {
	void* value; 
	struct _HashIntEntry* next;
	int key[1]; 
} HashIntEntry;

typedef struct {
	int size;				/* no entries in the table */
	int noKeys;				/* no keys */
	int entrySize;			/* size of HashIntEntry with noKeys keys */
	HashIntEntry** table;	/* the table proper */
} HashInt;


/* allocate and initialize a new hash */
HashInt* newHashInt(int size/* should be prime!*/) {
	HashInt* hash = allocStruct(HashInt);
	hash->size = size;
	hash->noKeys = 1;
	hash->entrySize = sizeof(HashIntEntry);
	hash->table = allocarray(size,sizeof(HashIntEntry*));
	return(hash);
}
HashInt* newHashIntN(int size/* should be prime!*/, int noKeys) {
	HashInt* hash = allocStruct(HashInt);
	hash->size = size;
	hash->noKeys = noKeys;
	hash->entrySize = sizeof(HashIntEntry)+(noKeys-1)*sizeof(int);
	hash->table = allocarray(size,sizeof(HashIntEntry*));
	return(hash);
}


#define hashIntKey(hkey,key) { \
	int i = 0; \
	hkey = 0; \
	while (i < hash->noKeys) hkey = (hkey << 10) + key[i++]; \
	hkey %= hash->size; \
}

/* look up an entry by key and return a pointer to the value or NULL if not found */
void* lookInt(HashInt* hash, int key) {
	HashIntEntry* entry = hash->table[key % hash->size];
	while (entry) {
		if (key == entry->key[0]) return(entry->value);
		entry = entry->next;
	}
	return(NULL);
}
void* lookIntN(HashInt* hash, int* key) {
	int hkey;
	HashIntEntry* entry;
	hashIntKey(hkey,key);
	entry = hash->table[hkey];
	while (entry) {
		int i;
		int eq = 1;
		for (i = 0; i < hash->noKeys; i++) if (key[i] != entry->key[i]) eq = 0;
		if (eq) return(entry->value);
		entry = entry->next;
	}
	return(NULL);
}

/* insert an entry (key and value) into the hash */
void insertInt(HashInt* hash, int key, void* value) {
	int hkey = key % hash->size;
	HashIntEntry* entry = allocStruct(HashIntEntry);
	entry->key[0] = key;
	entry->value = value;
	entry->next = hash->table[hkey];
	hash->table[hkey] = entry;
}
void insertIntN(HashInt* hash, int* key, void* value) {
	int hkey;
	int i;
	HashIntEntry* entry;
	hashIntKey(hkey,key);
	entry = alloc(hash->entrySize);
	for (i = 0; i < hash->noKeys; i++) entry->key[i] = key[i];
	entry->value = value;
	entry->next = hash->table[hkey];
	hash->table[hkey] = entry;
}


/* first look up an entry by key. If found the value of the entry is return, otherwise
   a new entry (key and value) is inseted into the hash */
void* lookInsertInt(HashInt* hash, int key, void* value) {
	int hkey = key % hash->size;
	HashIntEntry* entry;
	if (entry = hash->table[hkey]) {
		while(1) {
			if (entry->key[0] == key) return(entry->value);
			if (!entry->next) {
				HashIntEntry* entry2 = allocStruct(HashIntEntry);
				entry2->key[0] = key;
				entry2->value = value;
				entry2->next = NULL;
				entry->next = entry2;
				return(NULL);
			}
			entry = entry->next;
		};
	} else {
		entry = allocStruct(HashIntEntry);
		entry->key[0] = key;
		entry->value = value;
		entry->next = NULL;
		hash->table[hkey] = entry;
		return(NULL);
	}
}
void* lookInsertIntN(HashInt* hash, int* key, void* value) {
	void* value2;
	if (value2 = lookIntN(hash,key)) return(value2);
	insertIntN(hash,key,value); return(NULL);
}



/****************************************************************************/
/*** Utility types and functions to handle string based hashes */


typedef struct _HashStrEntry {
	char* key; 
	void* value; 
	struct _HashStrEntry* next;
} HashStrEntry;

typedef struct {int size; HashStrEntry** table;} HashStr;


/* allocate and initialize a new hash */
HashStr* newHashStr(int size/* should be prime!*/) {
	HashStr* hash = allocStruct(HashStr);
	hash->size = size;
	hash->table = allocarray(size,sizeof(HashStrEntry*));
	return(hash);
}


/* calculate hash index from key (a string) */
/* this is not a good algorithm. should be changed! */
#define hashStrKey(hkey,key) { \
	char* k = key; \
	int i = 0; \
	hkey = 0; \
	while (*k) { \
		hkey += *k++ << (i++ << 3); \
		if (i >= sizeof(int)) i = 0; \
	} \
	hkey = abs(hkey); \
	hkey %= hash->size; \
}


/* look up an entry by key and return a pointer to the value or NULL if not found */
void* lookStr(HashStr* hash, char* key) {
	int hkey;
	HashStrEntry* entry;
	hashStrKey(hkey,key);
	entry = hash->table[hkey];
	while (entry) {
		if (strcmp(key,entry->key) == 0) return(entry->value);
		entry = entry->next;
	}
	return(NULL);
}


/* insert an entry (key and value) into the hash */
void insertStr(HashStr* hash, char* key, void* value) {
	int hkey;
	HashStrEntry* entry = allocStruct(HashStrEntry);
	entry->key = key;
	entry->value = value;
	entry->next = NULL;
	hashStrKey(hkey,key);
	if (!hash->table[hkey]) hash->table[hkey] = entry;
	else {
		HashStrEntry* e = hash->table[hkey];
		while (e->next) e = e->next;
		e->next = entry;
	}
}



/****************************************************************************/
/*** Types, variables and functions for handling of input files */

/* Each open File keeps some state information. A specific File can be the current
   File. Most functions work on the current file. */


/* The different types of input files. Is kept in the File state but is not used
   by these utility functions */
typedef enum {commandFileT,foodsFileT,groupsFileT,recipeFileT,inputFileT} FileType;

/* This structure i a File. It keeps info and state for an input file */
typedef struct {
	FILE* file;			/* the stream file */
	char* name;			/* the name of the file */
	FileType type;		/* the type of the file */
	char separator;		/* the separator character used in the file */
	char decimalPoint;	/* the decimal point used in the file */
	char comment;		/* the comment char used in the file */
	char lineNo;		/* the number of the current file */
	int ch;				/* the last character read - the utility functions uses one read ahead */
} File;

/* the current file: */
FILE* currentFile;
char* currentFileName;
FileType currentType;
char separator;
char decimalPoint;
char comment;
int lineNo;
int ch;


/* set a file as current */
void setCurrent(File* file) {
	currentFile = file->file;
	currentFileName = file->name;
	currentType = file->type;
	separator = file->separator;
	decimalPoint = file->decimalPoint;
	comment = file->comment;
	lineNo = file->lineNo;
	ch = file->ch;
}


/* get the state from the current file */
void getCurrent(File* file) {
	file->file = currentFile;
	file->name = currentFileName;
	file->type = currentType;
	file->separator = separator;
	file->decimalPoint = decimalPoint;
	file->comment = comment;
	file->lineNo = lineNo;
	file->ch = ch;
}


/* open and initialize an input file */
int initFile(File* file, char* name, FileType type, char* mode,
			 char separator, char decimalPoint, char comment) {
	if (strcmp(name,"-") == 0) {
		file->file = stdin;
	} else {
		if (!(file->file = fopen(name,mode))) return(0);
	}
	file->name = name;
	file->type = type;
	file->separator = separator;
	file->decimalPoint = decimalPoint;
	file->comment = comment;
	file->lineNo = 1;
	if ((file->ch = getc(file->file)) == EOF) file->ch = '\n';
	return(1);
}


/* close the current file */
void closeCurrent() {fclose(currentFile);}


/* write an error regarding the current line of the current file */
void fileError(char* str) {
	error("%s in line %d of file %s.\n",str,lineNo,currentFileName);
}


#define maxStrLen 2048
char str[maxStrLen];		/* temporary storage for strings read */
char strLen;				/* length of string in str */
Num num;					/* temporary storage for number read */

/* save the last read string in new memory */
#define saveStr() allocStr(str,strLen)

/* get next char. return \n if eof */
#define getch() ( ((ch = getc(currentFile)) == EOF) ? (ch = '\n') : (ch) )

/* unget last char */
#define ungetch() ungetc(ch,currentFile)

/* eof? */
#define eof() feof(currentFile)

/* is last read char a blank (space, tab or cr)? */
#define space() (ch == ' ' || ch == '\t' || ch == '\r')

/* eol? */
#define eol() (ch == '\n')


/* skip any blanks - will also skip end of line comments and continuation lines */
#define xskipSpace() { \
	while (1) { \
		if (space()) getch(); \
		else if (ch = comment) while (!eol()) getch(); \
		else if (eol()) { \
			if (getch() == '=') {getch(); lineNo++;} \
			else {ungetch(); ch = '\n'; break;} \
		} else break; \
	} \
}
void skipSpace() { 
	while (1) {
		if (eof()) break;
		else if (space()) getch(); 
		else if (ch == comment) while (!eol()) getch(); 
		else if (eol()) { 
			if (getch() == '=') {getch(); lineNo++;} 
			else if (eof()) break;
			else {ungetch(); ch = '\n'; break;} 
		} else break; 
	} 
}


/* skip eol */
#define skipEol() {if (ch == '\n') getch(); lineNo++;}


/* skip any comment lines */
void skipComment() {
	while (1) {
		skipSpace();
		if (ch == comment) while (!eol()) getch();
	  if (eof() || !eol()) return;
		skipEol();
	}
}


/* skip untill start of next line */
void skipLine() {
	while (1) {
		while (!eol()) getch();
		skipEol();
		if (ch != '=') break;
	}
}


/* the the keyword of next command into str */
int getCmd() { /* call skipComment before getCmd! */
	char* p = str;
	strLen = 0;
	while (ch != ':' && !isspace(ch)) {
		while (ch != ':' && !isspace(ch)) {
			if (strLen++ >= maxStrLen-1) return(0);
			*p++ = ch;
			getch();
		}
		if (space()) {
			if (strLen++ >= maxStrLen-1) return(0);
			*p++ = ' ';
			skipSpace();
		}
	}
	if (ch != ':') return(0);
	getch();
	*p = '\0';
	strToLowercase(str);
	return(1);
}


/* get the next string into str. a string not in double quotes ends at the
   first separator, blank, comment char or the string "--". the string will
   be changed to lower case */
int getStr() { /* call skipSpace before getStr! */
	char *p = str;
	strLen = 0;

	if (ch == '"') {
		getch();
		while (1) {
			if (ch == '"') {
				getch();
				if (ch != '"') {
					*p = '\0'; 
					strToLowercase(str);
					return(1);
				}
			}
			if (eol()) return(0);
			if (strLen++ >= maxStrLen-1) return(0);
			*p++ = ch;
			getch();
		}
	} else {
		while (ch != separator && !isspace(ch) && ch != comment) {
			if (ch == '-') {
				getch();
				if (ch == '-') {ungetch(); break;}
				if (strLen++ >= maxStrLen-1) return(0);
				*p++ = '-';
				if (ch == separator || isspace(ch)) break;
			}
			if (strLen++ >= maxStrLen-1) return(0);
			*p++ = ch;
			getch();
		}
		*p = '\0';
		strToLowercase(str);
		return(1);
	}
}


/* get the next string into str. a string not in double quotes ends at the
   first separator or comment char or eol */
int getStr2() { /* call skipSpace before getStr2! */
	char *p = str;
	strLen = 0;

	if (ch == '"') {
		getch();
		while (1) {
			if (ch == '"') {
				getch();
				if (ch != '"') {
					*p = '\0'; 
					return(1);
				}
			}
			if (eol()) return(0);
			if (strLen++ >= maxStrLen-1) return(0);
			*p++ = ch;
			getch();
		}
	} else {
		if (separator == ' ') {
			while (!isspace(ch) && ch != comment) {
				if (strLen++ >= maxStrLen-1) return(0);
				*p++ = ch;
				getch();
			}
		} else {
			while (ch != separator && ch != comment && !eol()) {
				if (strLen++ >= maxStrLen-1) return(0);
				*p++ = ch;
				getch();
			}
			while (p > str && isspace(*(p-1))) p--;
		}
		*p = '\0';
		return(1);
	}
}


/* skip the next string. a string not in double quotes ends at the
   first separator or comment char or eol */
int skipStr2() { /* call skipSpace before skipStr2! */
	if (ch == '"') {
		getch();
		while (1) {
			if (ch == '"') {
				getch();
				if (ch != '"') {
					return(1);
				}
			}
			if (eol()) return(0);
			getch();
		}
	} else {
		if (separator == ' ') {
			while (!isspace(ch) && ch != comment) {
				getch();
			}
		} else {
			while (ch != separator && ch != comment && !eol()) {
				getch();
			}
		}
		return(1);
	}
}


/* get the next number into num */
int getNum() { /* call skipSpace before getNum! */
	Num sign = 1;
	num = 0;
	if (ch == '+') {sign = 1; getch();}
	else if (ch == '-') {sign = -1; getch();}
	while (isdigit(ch)) {
		num = num*10 + (ch-'0');
		getch();
	}
	if (ch == decimalPoint) {
		Num scale = 10;
		getch();
		while (isdigit(ch)) {
			num += (ch-'0')/scale;
			scale *= 10;
			getch();
		}
	}
	num *= sign;
	return(1);
}


/* get next string in a list into str */
int getFieldInList() { /* call skipSpace before getFieldInList! */
	if (!getStr()) {fileError("Field name too long\n"); return(0);}
	if (ch == separator) {
		getch(); skipSpace();
	} else if (separator == ' ' && (space() || ch == comment || eol())) {
		skipSpace();
	} else {
		skipSpace();
		if (ch == separator) {getch(); skipSpace();}
		else if (!eol()) {fileError("Error in list of field names"); return(0);}
	}
	return(1);
}


/****************************************************************************/
/* Additional utility functions for reading symbols from input files. These functions
   are for reading commands with non-trivial syntax. */

typedef enum {
	noSymX = 0,	dquoteSymX,
	eolSym, eqSym,    addSym, subSym, mulSym, divSym, lparSym, rparSym, gtSym,
	ltSym,  commaSym, andSym, orSym, notSym, inSym, numSym, fieldSym, otherSym,
	spaceSymX
} Symbol;

Symbol symCh[256];

#define symChInitLen 32
Symbol symChInit1[symChInitLen] = {
	eqSym, addSym,subSym,mulSym,divSym,lparSym,rparSym,gtSym, ltSym, commaSym,
	numSym,numSym,numSym,numSym,numSym,numSym, numSym, numSym,numSym,numSym,
	dquoteSymX,otherSym,otherSym,otherSym,otherSym,otherSym,otherSym,otherSym,
	spaceSymX,spaceSymX,spaceSymX,eolSym
};
char symChInit2[symChInitLen] = {
	'=',   '+',   '-',   '*',   '/',   '(',    ')',    '>',   '<',   ',',
	'0',   '1',   '2',   '3',   '4',   '5',    '6',    '7',   '8',   '9',
	'"',   '!',   '%',   '&',   '?',   '.',    ';',    ':',
	' ',   '\t',  '\r',  '\n'
};
void initSym() { /* call before first call of getSym */
	int i;
	for (i = 0; i < 256; i++) symCh[i] = noSymX;
	for (i = 0; i < symChInitLen; i++) symCh[symChInit2[i]] = symChInit1[i];
}


Symbol sym;	/* current Symbol */
char chSym;
int symError; /* should be set to 1 by parse function if they meet a syntax error */

void errorSym(char* symStr) {
	if (!symError) {
		symError = 1;
		error("Found a ");
		if (sym == fieldSym || sym == andSym || sym == orSym || sym == notSym ||
			sym == inSym) logmsg(str);
		else if (sym == numSym) logmsg("%f",(double)num);
		else logmsg("%c",chSym);
		logmsg(" when expecting a %s at line %d of file %s.\n",symStr,lineNo,currentFileName);
	}
}

void getSym() { /* get next symbol from current line */

	skipSpace();
	if (eol()) {sym = eolSym; return;}

	switch (symCh[ch]) {
	
	case noSymX: {
		char* p = str;
		strLen = 0;
		do {
			*p++ = ch; getch();
			sym = symCh[ch];
		} while (strLen++ < maxStrLen-1 && (sym == noSymX || sym == numSym));
		*p = '\0';
		strToLowercase(str);
		if (strcmp(str,"and") == 0) sym = andSym;
		else if (strcmp(str,"or") == 0) sym = orSym;
		else if (strcmp(str,"not") == 0) sym = notSym;
		else if (strcmp(str,"in") == 0) sym = inSym;
		else sym = fieldSym;
		return;
	}

	case dquoteSymX:
		getStr();
		sym = fieldSym;
		return;

	case numSym:
		getNum();
		sym = numSym;
		return;

	default:
		sym = symCh[ch];
		chSym = ch;
		getch();
		return;
	}
}


/****************************************************************************/
/*** Ulility "functions" to handle types that are chains of structure. */


/* define a chain from an type. This type must be a struct with an element
   with name next, which are a pointer to such a struct */
#define Chain(Type,Name) \
	typedef struct {Type* first; Type* last; int no;} Name
#define Chain2(Type,Name,SName) \
	typedef struct SName {Type* first; Type* last; int no;} Name

/* initialize a chain */
#define nolink(chain) { \
	(chain).first = (chain).last = NULL; \
	(chain).no = 0; \
}

/* add an element to end of chain - you must call endlink before doing anything
   else than adding more elements */
#define link(chain,newe) {\
	if (!(chain).first) (chain).first = (chain).last = (newe);  \
	else (chain).last = (chain).last->next = (newe); \
	(chain).no++; \
}

/* add an element to beginning of chain - you must call endlink before doing anything
   else than adding more elements */
#define link2(chain,newe) {\
	if (!(chain).first) {(chain).first = (chain).last = (newe);}  \
	else {(newe)->next = (chain).first; (chain).first = (newe);} \
	(chain).no++; \
}

/* no more elements */
#define endlink(chain) { \
	if ((chain).last) (chain).last->next = NULL; \
}



/****************************************************************************/
/* functions for reading and optimizing expressions with the following EBNF 
   LL(1) syntax:
     Exp  ::= Exp1 { ("+"|"-") Exp1 }
	 Exp1 ::= Exp2 { ("*"|"/") Exp2 }
	 Exp2 ::= [-] ( Num | Field | "(" Exp ")" )
   We use recursive descent to read the syntax. */

forward struct Exp_;

typedef enum {numVal, fieldVal, expVal} ExpValType;
typedef struct ExpVal_ { /* used to hold an "element" of an Exp1 or Exp2 */
	int neg;		/* if 1 the value should be negated */
	int recip;		/* if 1 the reciproc should be taken of the value */
	ExpValType type; /* type of the u union */
	union {
		Num num;			/* numVal */
		char* name;			/* fieldVal */
		struct Exp_* exp;	/* expVal */
	} u;
	struct ExpVal_* next;
} ExpVal;
Chain2(ExpVal,Exp,Exp_);	/* Exp is an Exp or an Exp1 */

	/* The pExp() function will parse an Exp and return a transformed version
	   of the parsed syntax tree in a Exp-type.
	   The transformed syntax tree can be seen as describing the following
	   (semantically equivilant) syntax:
			Exp      ::= ExpVal { + ExpValN}
			ExpValN  ::= [-] ExpVal
			ExpVal   ::= Num | Field | Exp1
			Exp1     ::= ExpVal1 { * ExpVal1R}
			ExpVal1R ::= [1/] ExpVal1
			ExpVal1  ::= Num | Field | Exp
		Also any constant expressions are folded to a single constant and iso-
		morphic operations are removed (*1 and +0).
		Among other it does the following transformations:
		a+(b+c)		-> a+b+c         Flattening
		-a+b		-> b-a			 Reordering to avoid negations
		a+(b*c)		-> (b*c)+a		 Reordering to avoid temporaries
		-a*-b*c		-> a*b*c		 Simplifications to avoid negations
		a*-b		-> a*b*-1		 Negation removal
		-a-b		-> 0-a-b		 Negation removal
	*/

#define xor(a,b) ((a && !b) || (!a && b))
#define setxor(a,b) (a = xor(a,b))

forward Exp* pExp1();
forward ExpVal* pExp2();

void linkExp(Exp* e, Num* c, ExpVal* v) {
	if (v->type == numVal) {
		if (v->neg) *c -= v->u.num;
		else *c += v->u.num;
	} else if (!v->neg && (v->type == expVal || (e->no && e->first->neg))) {
		link2(*e,v);
	} else {
		link(*e,v);
	}
}

void optExp(Exp* e, Num* c, int neg) {
	Exp* e1 = pExp1();
	if (e1->no > 1) {
		ExpVal* v = allocStruct(ExpVal);
		v->neg = neg;
		v->recip = 0;
		v->type = expVal;
		v->u.exp = e1;
		linkExp(e,c,v);
	} else {
		ExpVal* v1 = e1->first;
		if (v1->type == expVal) {
			ExpVal* v = v1->u.exp->first;
			while (v) {
				ExpVal* next = v->next;
				setxor(v->neg,xor(v1->neg,neg));
				linkExp(e,c,v);
				v = next;
			}
		} else {
			setxor(v1->neg,neg);
			linkExp(e,c,v1);
		}
	}
}

Exp* pExp() {	/* Exp  ::= Exp1 { ("+"|"-") Exp1 } */
	Exp* e = allocStruct(Exp);
	Num c = 0.0;
	nolink(*e);
	optExp(e,&c,0);
	while (sym == addSym || sym == subSym) {
		int neg = (sym == subSym);
		getSym();
		optExp(e,&c,neg);
	}
	if (c != 0.0 || !e->no || e->first->neg) {
		ExpVal* cv = allocStruct(ExpVal);
		cv->neg = 0; cv->recip = 0; cv->type = numVal; cv->u.num = c;
		if (e->no && e->first->neg) {
			link2(*e,cv);
		} else {
			link(*e,cv);
		}
	}
	endlink(*e);
	return e;
}

void linkExp1(Exp* e1, Num* c, ExpVal* v1) {
	if (v1->neg) {
		*c *= -1.0;
		v1->neg = 0;
	}
	if (v1->type == numVal) {
		if (v1->recip) *c /= v1->u.num;
		else *c *= v1->u.num;
	} else if ((v1->type == expVal && !v1->recip) || (e1->no && e1->first->recip)) {
		link2(*e1,v1);
	} else {
		link(*e1,v1);
	}
}

void optExp1(Exp* e1, Num* c, int recip) {
	ExpVal* v1 = pExp2();
	v1->recip = recip;
	if (v1->type == expVal && v1->u.exp->no == 1) {
		ExpVal* v = v1->u.exp->first;
		if (v->type == expVal) {
			ExpVal* v1x = v->u.exp->first;
			while (v1x) {
				ExpVal* next = v1x->next;
				setxor(v1x->recip,v1->recip);
				linkExp1(e1,c,v1x);
				v1x = next;
			}
			if (xor(v->next,v1->neg)) *c *= -1.0;
			return;
		}
		setxor(v->neg,v1->neg);
		v1 = v;
	}
	linkExp1(e1,c,v1);
}

Exp* pExp1() { /* Exp1 ::= Exp2 { ("*"|"/") Exp2 } */
	Exp* e1 = allocStruct(Exp);
	Num c = 1.0;
	nolink(*e1);
	optExp1(e1,&c,0);
	while (sym == mulSym || sym == divSym) {
		int recip = (sym == divSym);
		getSym();
		optExp1(e1,&c,recip);
	}
	if (c == -1.0 && e1->no == 1) {
		e1->first->neg = 1;
	} else if (c != 1.0 || !e1->no || e1->first->recip) {
		ExpVal* cv1 = allocStruct(ExpVal);
		cv1->neg = 0; cv1->recip = 0; cv1->type = numVal; cv1->u.num = c;
		if (e1->no && e1->first->recip) {
			link2(*e1,cv1);
		} else {
			link(*e1,cv1);
		}
	}
	endlink(*e1);
	return e1;
}

ExpVal* pExp2() { /* Exp2 ::= [-] ( Num | Field | "(" Exp ")" ) */
	ExpVal* v1 = allocStruct(ExpVal);
	v1->recip = v1->neg = 0;
	if (sym == subSym) {
		v1->neg = 1;
		getSym();
	}
	switch (sym) {
	case numSym:
		v1->type = numVal;
		v1->u.num = num;
		getSym();
		break;
	case fieldSym:
		v1->type = fieldVal;
		v1->u.name = saveStr();
		getSym();
		break;
	case lparSym:
		getSym();
		v1->type = expVal;
		v1->u.exp = pExp();
		if (sym == rparSym) getSym();
		else errorSym(")");
		break;
	default:
		errorSym("number or field name");
	}
	return v1;
}


forward void logExp1(Exp* e);

void logExp(Exp* e) {
	ExpVal* v = e->first;
	while (v) {
		if (v->neg) logmsg(" - ");
		else if (v != e->first) logmsg(" + ");
		switch (v->type) {
		case numVal: logmsg("%f",(double)v->u.num); break;
		case fieldVal: logmsg(v->u.name); break;
		case expVal:
			logmsg("( ");
			logExp1(v->u.exp);
			logmsg(" )");
			break;
		}
		v = v->next;
	}
}

void logExp1(Exp* e1) {
	ExpVal* v1 = e1->first;
	while (v1) {
		if (v1->recip) logmsg(" / ");
		else if (v1 != e1->first) logmsg(" * ");
		if (v1->neg) logmsg("- ");
		switch (v1->type) {
		case numVal: logmsg("%f",(double)v1->u.num); break;
		case fieldVal: logmsg(v1->u.name); break;
		case expVal:
			logmsg("( ");
			logExp(v1->u.exp);
			logmsg(" )");
			break;
		}
		v1 = v1->next;
	}
}


/****************************************************************************/
/* functions for reading and optimizing logical expressions with the following
   EBNF LL(1) syntax:
     Lexp  ::= Lexp1 { "or" Lexp1}
	 Lexp1 ::= Lexp2 { "and" Lexp2}
	 Lexp2 ::= ["not"] ( Val (=|<>) Val 
	                   | Val (<|<=) Val [ (<|<=) Val ]
					   | Val (>|>=) Val [ (>|>=) Val ]
					   | Val ["not"] "in" "(" Val {"," Val} ")"
					   | "(" Lexp ")"
     Val   ::= ( [-] Num ) | Field;
   We use recursive descent to read the syntax. */

forward struct Lexp_;

typedef enum {eqOp, neOp, gtOp, geOp, ltOp, leOp} LexpOp;
typedef struct LVal_ { /* a number or a field */
	ExpValType type;
	union {
		Num num;
		char* name;
	} u;
} LVal;
typedef enum {relVal, lexpVal} LexpValType;
typedef struct LexpVal_ { /* use to hold a relation or a Lexp or Lexp2 */
	LexpValType type;
	union {
		struct {
			LexpOp op;
			LVal* val1;
			LVal* val2;
		} r;					/* relVal */
		struct Lexp_* lexp;		/* lexpVal */
	} u;
	struct LexpVal_* next;
} LexpVal;
Chain2(LexpVal,Lexp,Lexp_);

	/* The pLexp() function will parse an Lexp and return a transformed version
	   of the parsed syntax tree in a Lexp-type.
	   The transformed syntax tree can be seen as describing the following
	   (semantically equivilant) syntax:
			Lexp      ::= LexpVal { or LexpVal}
			LexpVal   ::= LVal Op Lval | Lexp1
			Lexp1     ::= LexpVal1 { and LexpVal1}
			LexpVal1  ::= LVal Op Lval | Lexp
			LVal      ::= Num | Field
			Op        ::= = | <> | > | >= | < | <=
		Also any constant expressions are folded to a single constant.
		Among other it does the following transformations:
		a>b>c			-> a>b and b>c			Fewer operators
		a in (b,c)		-> a=b or a=c			Fewer operators
		not(l1 and l2)	-> not l1 or not l2		Not removal
		not(l1 or l2)	-> not l1 and not l2	Not removal
		not a>b			-> a <= b				Not removal
		l1 or (l2 or l3) -> l1 or l2 or l3		Flattening
	*/

forward Lexp* pLexp1(int not);
forward LexpVal* pLexp2(int not);
forward LVal* pLVal();

void linkLexp(Lexp* l, LexpVal* v) {
	link(*l,v);
}

void optLexp(Lexp* l, int not) {
	Lexp* l1 = pLexp1(not);
	if (l1->no > 1) {
		LexpVal* v = allocStruct(ExpVal);
		v->type = lexpVal;
		v->u.lexp = l1;
		linkLexp(l,v);
	} else {
		LexpVal* v1 = l1->first;
		if (v1->type == lexpVal) {
			LexpVal* v = v1->u.lexp->first;
			while (v) {
				LexpVal* next = v->next;
				linkLexp(l,v);
				v = next;
			}
		} else {
			linkLexp(l,v1);
		}
	}
}

Lexp* pLexp(int not) { /* Lexp  ::= Lexp1 { "or" Lexp1} */
	Lexp* l = allocStruct(Lexp);
	nolink(*l);
	optLexp(l,not);
	while (sym == orSym) {
		getSym();
		optLexp(l,not);
	}
	endlink(*l);
	return(l);
}

void linkLexp1(Lexp* l1, LexpVal* v1) {
	link(*l1,v1);
}

void optLexp1(Lexp* l1, int not) {
	LexpVal* v1 = pLexp2(not);
	if (v1->type == lexpVal && v1->u.lexp->no == 1) {
		LexpVal* v = v1->u.lexp->first;
		if (v->type == lexpVal) {
			LexpVal* v1x = v->u.lexp->first;
			while (v1x) {
				LexpVal* next = v1x->next;
				linkLexp1(l1,v1x);
				v1x = next;
			}
			return;
		}
		v1 = v;
	}
	linkLexp1(l1,v1);
}

Lexp* pLexp1(int not) { /* Lexp1 ::= Lexp2 { "and" Lexp2} */
	Lexp* l1 = allocStruct(Lexp);
	nolink(*l1);
	optLexp1(l1,not);
	while (sym == andSym) {
		getSym();
		optLexp1(l1,not);
	}
	endlink(*l1);
	return(l1);
}

LexpOp notOp(int not, LexpOp op) {
	if (!not) return(op);
	switch(op) {
	case eqOp: return(neOp);
	case neOp: return(eqOp);
	case gtOp: return(leOp);
	case leOp: return(gtOp);
	case ltOp: return(geOp);
	case geOp: return(ltOp);
	}
	/* we never get here - just to make a warning go away */ return(eqOp);
}

LexpVal* and2or(LexpVal* v1) {
	if (v1->type == relVal) {
		return(v1);
	} else {
		Lexp* l = v1->u.lexp;
		if (l->no == 1) {
			return(l->first);
		} else {
			Lexp* l1 = allocStruct(Lexp);
			LexpVal* v = allocStruct(LexpVal);
			nolink(*l1);
			link(*l1,v1);
			endlink(*l1);
			v->type = lexpVal;
			v->u.lexp = l1;
			return(v);
		}
	}
}

forward void pLexp2eq(LexpVal* v1, int not, LexpOp op, LVal* e);
forward void pLexp2lt(LexpVal* v1, int not, LexpOp op, LVal* e);
forward void pLexp2gt(LexpVal* v1, int not, LexpOp op, LVal* e);
forward void pLexp2in(LexpVal* v1, int not, LVal* e);

LexpVal* pLexp2(int not) {
	LexpVal* v1 = allocStruct(ExpVal);
	int lnot = 0;
	if (sym == notSym) {
		lnot = 1;
		setxor(not,lnot);
		getSym();
	}
	if (sym == lparSym) {
		Lexp* l;
		getSym();
		l = pLexp(not);
		if (sym == rparSym) getSym();
		else errorSym(")");
		v1->type = lexpVal;
		v1->u.lexp = l;
	} else {
		LVal* e = pLVal();
		if (sym == eqSym) {getSym(); pLexp2eq(v1,not,eqOp,e);}
		else if (sym == ltSym) {getSym();
			if (sym == gtSym) {getSym(); pLexp2eq(v1,not,neOp,e);}
			else if (sym == eqSym) {getSym(); pLexp2lt(v1,not,leOp,e);}
			else {pLexp2lt(v1,not,ltOp,e);} }
		else if (sym == gtSym) {getSym();
			if (sym == eqSym) {getSym(); pLexp2gt(v1,not,geOp,e);}
			else {pLexp2gt(v1,not,gtOp,e);} }
		else if (sym == notSym) {getSym(); 
			setxor(lnot,1); setxor(not,1); pLexp2in(v1,not,e); }
		else {pLexp2in(v1,not,e);}
	}
	if (lnot) return(and2or(v1));
	else return(v1);
}

void setRelVal(LexpVal* v1, LexpOp op, LVal* e1, LVal* e2) {
	v1->type = relVal;
	v1->u.r.op = op;
	v1->u.r.val1 = e1;
	v1->u.r.val2 = e2;
}

void linkRelVal(Lexp* l1, LexpOp op, LVal* e1, LVal* e2) {
	LexpVal* v1 = allocStruct(LexpVal);
	setRelVal(v1,op,e1,e2);
	link(*l1,v1);
}

void pLexp2eq(LexpVal* v1, int not, LexpOp op, LVal* e)  {
	setRelVal(v1,notOp(not,op),e,pLVal());
}

void setAnd(LexpVal* v1, Lexp* l) {
	LexpVal* vx = allocStruct(LexpVal);
	Lexp* lx = allocStruct(Lexp);
	vx->type = lexpVal;
	vx->u.lexp = l;
	nolink(*lx);
	link(*lx,vx);
	endlink(*lx);
	v1->type = lexpVal;
	v1->u.lexp = lx;
}

void pLexp2lt(LexpVal* v1, int not, LexpOp op, LVal* e) {
	LVal* e2 = pLVal();
	if (sym != ltSym) {
		setRelVal(v1,notOp(not,op),e,e2);
	} else {
		Lexp* l1 = allocStruct(Lexp);
		LexpOp op2 = ltOp;
		getSym(); if (sym == eqSym) {getSym(); op2 = leOp;}
		nolink(*l1);
		linkRelVal(l1,notOp(not,op),e,e2);
		linkRelVal(l1,notOp(not,op2),e2,pLVal());
		endlink(*l1);
		setAnd(v1,l1);
	}
}

void pLexp2gt(LexpVal* v1, int not, LexpOp op, LVal* e) {
	LVal* e2 = pLVal();
	if (sym != gtSym) {
		setRelVal(v1,notOp(not,op),e,e2);
	} else {
		Lexp* l1 = allocStruct(Lexp);
		LexpOp op2 = gtOp;
		getSym(); if (sym == eqSym) {getSym(); op2 = geOp;}
		nolink(*l1);
		linkRelVal(l1,notOp(not,op),e,e2);
		linkRelVal(l1,notOp(not,op2),e2,pLVal());
		endlink(*l1);
		setAnd(v1,l1);
	}
}

void pLexp2in(LexpVal* v1, int not, LVal* e) {
	Lexp* l = allocStruct(Lexp);
	if (sym != inSym) errorSym("in");
	getSym(); if (sym != lparSym) errorSym("(");
	nolink(*l);
	do {
		LVal* e2;
		getSym();
		e2 = pLVal();
		linkRelVal(l,notOp(not,eqOp),e,e2);
	} while (sym == commaSym);
	endlink(*l);
	if (sym != rparSym) errorSym(")");
	getSym();
	if (l->no == 1) {
		LexpVal* v = l->first;
		setRelVal(v1,v->u.r.op,v->u.r.val1,v->u.r.val2);
	} else {
		v1->type = lexpVal;
		v1->u.lexp = l;
	}
}

LVal* pLVal() {
	LVal* e = allocStruct(LVal);
	if (sym == subSym) {
		getSym();
		if (sym != numSym) {
			errorSym("number");
		} else {
			e->type = numVal;
			e->u.num = -num;
			getSym();
		}
	} else if (sym == numSym) {
		e->type = numVal;
		e->u.num = num;
		getSym();
	} else if (sym == fieldSym) {
		e->type = fieldVal;
		e->u.name = saveStr();
		getSym();
	} else {
		errorSym("number or field name");
	}
	return(e);
}

forward void logLexp1(Lexp* l1);
forward void logLVal(LVal* e);

void logRel(LexpVal* v) {
	logLVal(v->u.r.val1);
	switch(v->u.r.op) {
	case eqOp: logmsg(" = "); break;
	case neOp: logmsg(" <> "); break;
	case gtOp: logmsg(" > "); break;
	case geOp: logmsg(" >= "); break;
	case ltOp: logmsg(" < "); break;
	case leOp: logmsg(" <= "); break;
	}
	logLVal(v->u.r.val2);
}

void logLexp(Lexp* l) {
	LexpVal* v = l->first;
	while (v) {
		if (v != l->first) logmsg(" or ");
		if (v->type == relVal) {
			logRel(v);
		} else {
			logmsg("( ");
			logLexp1(v->u.lexp);
			logmsg(" )");
		}
		v = v->next;
	}
}

void logLexp1(Lexp* l1) {
	LexpVal* v1 = l1->first;
	while (v1) {
		if (v1 != l1->first) logmsg(" and ");
		if (v1->type == relVal) {
			logRel(v1);
		} else {
			logmsg("( ");
			logLexp(v1->u.lexp);
			logmsg(" )");
		}
		v1 = v1->next;
	}
}

void logLVal(LVal* e) {
	if (e->type == numVal) logmsg("%f",(double)e->u.num);
	else logmsg(e->u.name);
}



/****************************************************************************/
/*** STEP 1: parse commands */

/* You should first call commandsHashInit(), then readCommands() for each commands
   file, and last checkCommands() */


/* this will be used in a Cmd.args to identify the following to arguments to be
   first and last field in a range of fields. Note that a--b in a commands file
   will be --,a,b in Cmd.args. */
char* fieldRange = "--";


/* write about error in a command */
void cmdError(char* str,char* cmd) {
	error("%s in command '%s' at line %d of file %s.\n",str,cmd,lineNo,currentFileName);
}


/* this will be a hash with all command CmdDefs where the command keyword is key. This
   is only used by the functions in this STEP */
HashStr* commandsHash;


/* initializes the hash. Should be called before readCommands() is called! */
void commandsHashInit() {
	CmdDef** p = cmdDefs;
	commandsHash = newHashStr(89);
	while (*p) {
		insertStr(commandsHash,(*p)->name,*p);
		p++;
	}
	initSym();
}


/* read all commands from a commands file. It will read commands acording to the
   syntax defined in the CmdDef's defined ealier. The result will be syntax trees
   in the Cmd's also defined ealier. */
void readCommands(File* file) {
	CmdDef* cmdDef;
	Cmd* cmd;
	char** cmdArgs;
	char* args[1002];
	int argno;
	int noCommands = 0;

	setCurrent(file);
	while (1) {
		skipComment();
		if (eof()) break;
		if (!getCmd()) {fileError("Unknown command"); goto cmderr;}

		if (!(cmdDef = lookStr(commandsHash,str)))
			{cmdError("Unknown command",str); goto cmderr;}

		if (cmdDef->occur == single && *(cmdDef->cmd))
			{cmdError("Command used more than once",cmdDef->name); goto cmderr;}

		skipSpace();
		argno = 0;
		while (!eol()) {
			if (argno >= cmdDef->numArgs) 
				{cmdError("Too many arguments",cmdDef->name); goto cmderr;}

			switch (cmdDef->args[argno]) {

			case  strArg:
				if (!getStr())
					{cmdError("Argument too long",cmdDef->name); goto cmderr;}
				args[argno++] = saveStr();
				break;

			case chArg:
				if (!getStr() || strLen != 1)
					{cmdError("Argument too long",cmdDef->name); goto cmderr;}
				args[argno++] = saveStr();
				break;

			case numArg:
				if (!getNum() || (ch != separator && !isspace(ch)))
					{cmdError("Error in numeric argument",cmdDef->name); goto cmderr;}
				sprintf(str,"%g",(double)num); strLen = strlen(str);
				args[argno++] = saveStr();
				break;

			case listArg:
				while(1) {
					if (!getStr())
						{cmdError("Argument too long",cmdDef->name); goto cmderr;}
					if (strcmp(str,",") == 0 || strcmp(str,fieldRange) == 0)
						{cmdError("Error in list",cmdDef->name); goto cmderr;}
					args[argno++] = saveStr();
					skipSpace();
					if (ch == ',') {
						getch();
					} else if (ch == '-' && getch() == '-') {
						getch();
						args[argno] = args[argno-1];
						args[argno-1] = fieldRange;
						argno++;
					} else {
						args[argno++] = NULL;
						break; /* end of list reached */
					}
					skipSpace();
					if (argno >= 1000)
						{cmdError("Too many arguments",cmdDef->name); goto cmderr;}
				}
				break;

			case setArg: {
				Exp* e;
				symError = 0;
				getSym(); if (sym != fieldSym) errorSym("field name");
				args[argno++] = saveStr();
				getSym(); if (sym != eqSym) errorSym("=");
				getSym(); e = pExp();
				if (sym != eolSym) {errorSym("end of line"); goto cmderr;}
				if (!symError && verbosity >= 80) {
					logmsg("***set: %s = ",args[argno-1]); logExp(e); logmsg("\n");
				}
				args[argno++] = (char*)e;
				break;}

			case whereArg: {
				Lexp* l;
				symError = 0;
				getSym(); l = pLexp(0);
				if (sym != eolSym) {errorSym("end of line"); goto cmderr;}
				if (!symError && verbosity >= 80) {
					logmsg("***where: "); logLexp(l); logmsg("\n");
				}
				args[argno++] = (char*)l;
				break;}

			}

			skipSpace();
			if (argno >= 1000)
				{cmdError("Too many arguments",cmdDef->name); goto cmderr;}
		}

		if (argno < cmdDef->numReqArgs)
			{cmdError("Too few arguments",cmdDef->name); goto cmderr;}
		do {args[argno++] = NULL;} while (argno <= cmdDef->numArgs);
		
		cmd = allocStruct(Cmd);
		cmd->next = NULL;
		cmd->args = cmdArgs = alloc(argno*sizeof(char*));
		do {argno--; cmdArgs[argno] = args[argno];} while (argno);
		if (!*(cmdDef->cmd)) {
			*(cmdDef->cmd) = cmd;
		} else {
			Cmd* p = *(cmdDef->cmd);
			while (p->next) p = p->next;
			p->next = cmd;
		}

		noCommands++;
cmderr:	skipLine();

	}
	closeCurrent();
	logmsg("Read %d commands from file %s.\n",noCommands,currentFileName);
}


/* checks that all required commands has been given. Should be called when
   all commands files has been read with readCommands() */
void checkCommands() {
	CmdDef** cmdDefp = cmdDefs;
	while (*cmdDefp) {
		if ((*cmdDefp)->type == required && *((*cmdDefp)->cmd) == NULL)
			error("Required command '%s' not specified\n",(*cmdDefp)->name);
		cmdDefp++;
	}
}



/****************************************************************************/
/*** Types and variables which you can see as a transformation and anotation of the
     syntax tree in the CmdDef's. We call this the semantic tree */


/* Each field from all fiels are described by a Field structure. All fields in the food
   table are linked together and fields from each input and recipe file are linked
   together */
typedef struct Field_ {
	char* name;		/* name of the field */
	int toPos;		/* 0 if not to be output, otherwise position in output obs (first=1) */
	int noCalc;		/* 0 if a calculate field, 1 otherwise */
	int groupBy;	/* 0 if not a group by field, otherwise order in group by sequence (first=1) */
	int fromPos;	/* 0 if not to be in food table, otherwise position in food table (first=1) */
	int onlyRecipe;	/* 1 if only to be output if calulating recipes, 0 otherwise */
	int text;		/* 1 if text field (= comment field), 0 otherwise */
	struct Field_* next; /* the next field in the file */
} Field;
Chain(Field,FieldChain);

/* allocate a new field */
Field* allocField(char* name) {
	Field* field = allocStruct(Field);
	field->name = name;
	field->toPos = 0;
	field->noCalc = 0;
	field->groupBy = 0;
	field->fromPos = 0;
	field->onlyRecipe = 0;
	field->text = 0;
	return(field);
}
int noTempFields = 0;
Field* allocTempField() {
	char* name = alloc(8);
	sprintf(name,"temp%d",++noTempFields);
	return(allocField(name));
}

/* The FieldP and FieldPChain are used for lists of fields */
typedef struct FieldP_ {
	Field* field;
	struct FieldP_* next;
} FieldP;
Chain(FieldP,FieldPChain);

/* allocate a new FieldP */
FieldP* allocFieldP(Field* field) {
	FieldP* fieldP = allocStruct(FieldP);
	fieldP->field = field;
	return(fieldP);
}

/* the types of files we handle */
typedef enum {formatText,formatTextNoHead,formatBinNative} FileFormat;

/** bin: command */
int saveBin;			/* one if save: used */
char* saveFileName = NULL;/* name of save file if saveBin is one */

/** input:, input fields:, input format:, input scale: commands */
char* inputFileName = NULL;/* the name of the input file */
char inputSep;			/* seperator for input file */
char inputDecPoint;		/* decimal point for input file */
char inputComment;		/* comment char for input file */
FileFormat inputFormat;	/* format of input file */
File* inputFile;		/* the input file */
FieldChain inputFields;	/* the fields in the input file */
int inputStarFields;	/* no of fields which are star fields */
HashStr* inputFieldsHash; /* a hash with the fields in the input file */
Field* inputFoodField;	/* the food field */
Field* inputAmountField;/* the amount field */
Num inputAmountScale;	/* the input scale */

/** non-edible field: command */
Field* nonEdibleField;	/* the field containing non-edible fraction */
char* nonEdibleFlagName;/* name of field with flag for non-edible reduction - may be
						   NULL */

/** group by: command */
FieldPChain groupByFields; /* list of group by fields in input file (could be empty) */
FieldPChain groupByFoodFields; /* list of group by fields in food table or in input file (could be empty) */

/** set: and calculate: commands */
FieldChain calculateFields; /* new set/calculate fields */
HashStr* calculateFieldsHash; /* hash with new set/calculate fields */
typedef struct Constant_ {
	Num num;
	Field* field;
	struct Constant_* next;
} Constant;
Chain(Constant,ConstantChain);
ConstantChain constants;
Field* allocConstant(Num num) {
	Constant* constant = allocStruct(Constant);
	Field* field;
	char* cname = alloc(11);
	sprintf(cname,"%10.2f",num);
	constant->num = num;
	constant->field = field = allocField(cname);
	field->noCalc = 9; /* special marker for constants */
	field->next = (Field*)&(constant->num); /* pointer to the constant value */
	link(constants,constant);
	return(constant->field);
}
void initConstants() { /* must be called before any calls of allocConstant */
	nolink(constants);
}
typedef enum {cpyOp,addOp,subOp,mulOp,divOp,cpyOpC,addOpC,subOpC,mulOpC,divOpC} SetOp;
typedef struct SetOper_ {
	SetOp op;
	Field* field;
	struct SetOper_* next;
} SetOper;
SetOper* allocOper(SetOp op, Field* field) {
	SetOper* oper = allocStruct(SetOper);
	oper->op = op;
	oper->field = field;
	return(oper);
}
Chain(SetOper,SetOperChain);
typedef struct Set_ {
	Field* field;
	SetOperChain opers;
	struct Set_* next;
} Set;
Set* allocSet(Field* field) {
	Set* set = allocStruct(Set);
	set->field = field;
	nolink(set->opers);
	return(set);
}
Chain(Set,SetChain);
SetChain sets;

/* recipe set: command */
FieldChain setRecipeFields; /* fields from recipe files in recipe set: commands */
HashStr* setRecipeFieldsHash;/* hash with fields from recipe files in recipe set: commands */
SetChain recipeSets;

/* group set: command */
FieldChain groupSetFields;		/* all group set fields */
HashStr* groupSetFieldsHash; /* hash with new group set fields */
SetChain groupSets;

typedef enum {			/* types of reductions: */
	weightReduc,			/* relative to total weight */
	nutriReduc				/* relative to nutrient */
} ReducType;

/** cook:, cook field: commands */
HashStr* cookTypesHash; /* a hash with all cook types (= cook methods) */
typedef struct Cook_ {
	Field* field;			/* the reduce field */
	ReducType type;			/* the type of reduction */
	Field* calcField;		/* used if type=weightReduc with a calc field */
	FieldPChain fields;		/* the fields to reduce */
	int used; /* this field is only used locally in setFoodCalcPos() */
	struct Cook_* next;
} Cook; /* definition of a cook reduce field */
Chain(Cook, CookChain); 
typedef struct CookType_ {
	char* type;			/* the name of the cook type (=method) */
	CookChain* cooks;
	struct CookType_* next;
} CookType; /* defenition of a cook type (=cook method) */
Chain(CookType, CookTypeChain);
char* cookFieldName; /* the name of the cook field */
CookTypeChain cookTypes; /* a list with the cook types to be used */

/** reduce field: command */
typedef struct Reduct_ {
	char* fieldName;
	Field* field; /* only used locally in setCookReductPos() */
	ReducType type;			/* the type of reduction */
	Field* calcField;		/* used if type=weightReduc with a calc field */
	FieldPChain fields;
	int used; /* only used locally in setCookReductPos() */
	struct Reduct_* next;
} Reduct;
Chain(Reduct, ReductChain);
ReductChain reducts; /* a list with all the reduce fields */

/** recipe reduce field: command */
typedef struct RecipeReduct_ {
	char* fieldName;
	Field* field; /* only used locally in ?? */
	Field* calcField; /* may be NULL */
	FieldPChain fields;
	int used; /* only used locally in ?? */
	struct RecipeReduct_* next;
} RecipeReduct;
Chain(RecipeReduct, RecipeReductChain);
RecipeReductChain recipeReducts; /* a list with all the recipe reduce fields */

/** where:, if: and if not: commands */
typedef struct Test_ {	/* a single test */
	LexpOp op;				/* operator for the test */
	Field* field1;			/* first operand */
	Field* field2;			/* second operand */
	int action;				/* no of Test in tests to go to if true */
	struct Test_* next;
} Test;
Test* allocTest(LexpOp op, Field* field1, Field* field2) {
	Test* test = allocStruct(Test);
	test->op = op;
	test->field1 = field1;
	test->field2 = field2;
	return(test);
}
Chain(Test,TestChain);
typedef struct TestP_ { /* pointer to a Test */
	Test* test;
	struct TestP_* next;
} TestP;
TestP* allocTestP(Test* test) {
	TestP* testP = allocStruct(TestP);
	testP->test = test;
	return(testP);
}
Chain(TestP,TestPChain);
TestChain tests;
int simpleTest;	/* 1 if all tests are on the form no-calc filed Op constant */

/** transpose: commands */
typedef struct Transpose_ {
	Field* groupField;	/* field to transpose on */
	int noGroups;		/* no of groups */
	FieldPChain fields;	/* fields to transpose */
	FieldPChain transFields; /* fields to transpose to */
	struct Transpose_* next;
} Transpose;
Chain(Transpose,TransposeChain);
TransposeChain transposes;

/** output:, output format:, output fields: commands */
char* outputFileName = NULL;/* the name of the output file */
char outputSep;			/* the separator for the output file */
char outputDecPoint;	/* the decimal point for the output file */
FileFormat outputFormat;/* the format of the output file */
FILE* output;			/* the output file */
FieldPChain outputFields;/* the fields to output */
int noRealOutputFields;	/* the number of "real" output fields. The first fields in
outputFields are the real output fields; i.e. those given (possibly by default) in a
output fields: command. The calculate, recipe sum, if and if not commands may add
extra fields to the list. There should be room for those in the output buffer, and
the are otherwise treated like output fields, but they are not to be finally output.*/

/** foods: command */
typedef struct FoodsFile_ {
	File* file;			/* the foods file */
	FieldPChain* fieldPs; /* the fields in the foods file */
	int starFields;		/* no of star fields */
	int noFromFields;	/* number of fields to be put in the food table */
	struct FoodsFile_* next;
} FoodsFile;
Chain(FoodsFile, FoodsFileChain);
FoodsFileChain foodsFiles; /* a list with all foods files */

HashStr* foodFieldsHash; /* a hash with all fields which could be in the food table */
FieldChain foodFields;	/* a list with all fields which could be in the food table */
Field* foodId;			/* the food id field */
FieldPChain foodTableFields; /* a list with the fields actually in the food table */

/** the food table */
HashInt* foodTable;		/* this is the food table! */
int totFoods = 0;		/* totally number of foods and recipes in the table */
typedef enum {			/* types of entries in the food table: */
	simpleFood,				/* a simple, basic food. The value of the entry is an array of Num's */
	simpleRecipe,			/* a recipe aggregated to be just like a simpleFood */
	expandedRecipe			/* a recipe. The value is a list of RecipeEntries, which each is an array of Num's */
} FoodType;
typedef struct RecipeEntry_ {Num* obs; struct RecipeEntry_* next;} RecipeEntry;
typedef struct FoodEntry_ {
	FoodType foodType;
	union {
		Num* obs; /* when foodType is simpleFood or simpleRecipe */
		RecipeEntry* recipe; /* when foodType is expandedRecipe */
	} u;
} FoodEntry;
FoodEntry* allocFoodEntry(FoodType foodType, void* obs) { /* allocate a food entry */
	FoodEntry* foodEntry = allocStruct(FoodEntry);
	foodEntry->foodType = foodType;
	if (foodType == expandedRecipe) foodEntry->u.recipe = obs;
	else foodEntry->u.obs = obs;
	return(foodEntry);
}

/** groups: command */
typedef struct GroupsFile_ {
	File* file;			/* the groups file */
	FieldPChain* fieldPs; /* the fields in the foods file */
	int starFields;		/* no of star fields */
	FieldPChain* groupIds;/* the group id fields */
	int noFromFields;	/* number of fields to be put in the food table */
	HashInt* hash;		/* hash with an entry for each group in the file, the value of an entry is an array of Num's */
	struct GroupsFile_* next;
} GroupsFile;
Chain(GroupsFile, GroupsFileChain);
GroupsFileChain groupsFiles; /* a list with all groups files */

/** recipes:, recipe sum: and ingredients: commands */
typedef struct RecipesFile_ {
	File* file;			/* the recipes file */
	FieldChain* fields;	/* the fields in the recipes file */
	int starFields;		/* no of star fields */
	HashStr* fieldsHash;/* hash with the fields in the recipes file */
	Field* recipeId;	/* the recipe id field in the recipes file */
	Field* foodId;		/* the food id field in the recipes file */
	Field* amountField;	/* the amount field in the recipes file */
	struct RecipesFile_* next;
} RecipesFile;
Chain(RecipesFile, RecipesFileChain);
RecipesFileChain recipesFiles; /* a list with all recipes files */
Num recipeSum;			/* the value form the recipes sum: command */
Field* recipeSumField;	/* the field which will be the recipe sum */
int keepIngredients;	/* is one if ingredints: keep or keepx is used */
int copyRecipeFields;	/* is one if ingredints: sum or keepx is used */


/**********************************************************************************/
/*** STEP 2: readFields() */

/* This step basically takes takes the syntax tree from the Cmd structures and 
   transform it into the set of variables above, which we call the semantic tree.
   As part of this the step will read all field names from all data files. */
/* The readFields() function is at the end of this section of the source file. Before
   that is a large number of functions which all are only called from readFields(), and
   they are called in the same order as they are defined.
   These function can be roughly devided into four sub-steps:
   sub-step A: Check that all data files are readable and read all field names.
   sub-step B: Expand and check lists of fields. Also set noCalc and groupBy for
               all fields.
   sub-step C: Set toPos and onlyRecipe for all fields.
   sub-step D: Set fromPos for all food table fields. Also check for type of fields. */


/*----------------------------------------------------------------------------------*/
/*--- STEP 2, sub-step A -*/

/* Check that all data files are readable and read all field names. */


/* utility function to set the separator, decimalPoint and comment arguments according
   to the sep, decPoint and comm arguments or to the separator:, decimal point: and
   comment: commands or to the global default */
void setSepDecPoint(char* separator, char* decimalPoint, char* comment,
					char* sep, char* decPoint, char* comm) {
	if (separator) *sep = *separator;
	else if (separatorCmd) *sep = *(separatorCmd->args[0]);
	else *sep = ',';
	if (separator && decimalPoint) *decPoint = *decimalPoint;
	else if (decimalPointCmd) *decPoint = *(decimalPointCmd->args[0]);
	else *decPoint = '.';
	if (separator && decimalPoint && comment) *comm = *comment;
	else if (commentCmd) *comm = *(commentCmd->args[0]);
	else *comm = ';';
}


/* open an input file. returns NULL if any error */
File* openInFile(char* name, FileType type,
				 char* separator, char* decimalPoint, char* comment) {
	File* file = allocStruct(File);
	char sep;
	char decPoint;
	char comm;
	
	setSepDecPoint(separator,decimalPoint,comment,&sep,&decPoint,&comm);
	if (!initFile(file,name,type,"r",sep,decPoint,comm)) 
		{error("Could not open file %s.\n",name); return(NULL);}
	return(file);
}


/* reads the header (the field names line) from a foods or groups file. Takes the args:
   file		The foods or groups file.
   fieldsPs Will put all fields in the file in this list. All fields will also be
            added to the foodFields list and the foodFieldsHash (contains all fields which can be in the
			food table) if they are not allready there.
   idNames  If this is not NULL it should be the names of the id fields, otherwise the
            first field will be the sole id field.
   idFields Will be set to point to the id fields.
   star     Will be set to number of fields which are star fields.
   Returns 1 if there was no errors.
*/
int readFoodsHeader(File* file, FieldPChain* fieldPs, char** idNames,
					FieldPChain* idFields, int* star) {
	int noId;			/* number of id fields */
	int idFound = 0;	/* will become noId when all id fields are found */
	int status = 1;		/* will become 0 if any errors occour */
	int inStar = 0;		/* will be 1 if we are reading a star line */

	/* count no of id fields */
	if (!idNames) {
		noId = 1;
	} else {
		char** idName = idNames;
		noId = 0;
		while (*idName++) noId++;
	}

	nolink(*fieldPs);
	*star = 0;
	setCurrent(file);
	
	skipComment(); /* skip any comment lines */
	if (ch == '*') {inStar = 1; getch(); skipSpace();}
	while (1) { /* read fields one by one*/
		char* name; /* the name of the current field */
		int foundId = 0; /* will be 1 if this field is an id field */

		if (eol()) {
			if (!inStar) break; /* no more fields */
			skipEol(); skipComment();
			inStar = 0;
		}
		
		if (!getFieldInList()) return(0);	/* get the field next field name ... */
		name = saveStr();					/* ... and save it */

		if (idFound < noId) { /* if we have not yet found all id fields, we see if we got it now */
			if (!idNames) {
				foundId = 1;
			} else {
				char** idName = idNames;
				while (*idName) if (strcmp(*idName++,name) == 0) foundId = 1;
			}
			if (foundId) {
				idFound++;
				if (idFound == 1 && idFields->no) {
					name = idFields->first->field->name; /* rename of field */
					foundId = 0;
				} else if (file->type == groupsFileT && !lookStr(foodFieldsHash,name)) {
					error("Id field '%s' for groups file %s not found in any foods file.\n",
						name,file->name);
					status = 0;
				}
			}

		}

		{ /* add the field to the lists */
			Field* field;
			/* the field may already be in the foodFieldsHash because it is also in
			   a previous foods or groups file (or previously in this file). If it
			   is not, we allocate a new Field and insert it in the hash and in the list */
			if (!(field = lookStr(foodFieldsHash,name))) {
				link(foodFields,(field = allocField(name)));
				insertStr(foodFieldsHash,name,field);
			}
			if (foundId) {link(*idFields,allocFieldP(field)); field->noCalc = 1;} /* id fields are noCalc */
			link(*fieldPs,allocFieldP(field));
			if (inStar) (*star)++;
		}
	}
	skipEol();

	if (idFound != noId)
		{error("Id field(s) not found in file %s.\n",file->name); status = 0;}

	endlink(*fieldPs);
	endlink(foodFields);
	endlink(*idFields);
	getCurrent(file);
	return(status);
}


/* handle the save: command */
void setSave() {
	if (saveCmd) {
		saveBin = 1;
		saveFileName = saveCmd->args[0];
		if (!inputFieldsCmd)
			error("When saving the 'input fields' command must be used.\n");
	} else {
		saveBin = 0;
	}
}


/* handle all foods: commands */
void setFoods() {
	Cmd* cmd = foodsCmd;

	FieldPChain idFields;
	nolink(idFields);
	nolink(foodsFiles);
	while (cmd) { /* handle the foods: commands one by one */
		File* file;
		FieldPChain* fieldPs = allocStruct(FieldPChain);
		int star;
		char* idnames[2];
		idnames[0] = cmd->args[1]; idnames[1] = NULL;
		if ((file = openInFile(cmd->args[0],foodsFileT,cmd->args[2],cmd->args[3],cmd->args[4])) &&
			readFoodsHeader(file,fieldPs,&(idnames[0]),&idFields,&star)) {
			FoodsFile* foodsFile = allocStruct(FoodsFile);
			foodsFile->file = file;
			foodsFile->fieldPs = fieldPs;
			foodsFile->starFields = star;
			foodsFile->noFromFields = 0;
			link(foodsFiles,foodsFile);
		}
		cmd = cmd->next;
	}
	endlink(foodsFiles);
	foodId = idFields.first->field;
}

/* handle all groups: commands */
void setGroups() {
	Cmd* cmd = groupsCmd;

	nolink(groupsFiles);
	while (cmd) { /* handle the groups: commands one by one */
		File* file;
		FieldPChain* fieldPs = allocStruct(FieldPChain);
		int star;
		FieldPChain* idFields = allocStruct(FieldPChain);
		char** args = cmd->args+2;
		nolink(*idFields);
		while (*args++);
		if ((file = openInFile(cmd->args[0],groupsFileT,args[0],args[1],args[2])) &&
			readFoodsHeader(file,fieldPs,&(cmd->args[1]),idFields,&star)) {
			GroupsFile* groupsFile = allocStruct(GroupsFile);
			groupsFile->file = file;
			groupsFile->fieldPs = fieldPs;
			groupsFile->starFields = star;
			groupsFile->groupIds = idFields;
			groupsFile->noFromFields = 0;
			link(groupsFiles,groupsFile);
		}
		cmd = cmd->next;
	}
	endlink(groupsFiles);
}


/* handle the recipe set: command */
void setRecipeSet1() {
	Cmd* cmd = recipeSetCmd;
	if (cmd && !recipesCmd) error("The recipe set: command can only be used with recipe files.\n");
	while (cmd) {
		char* name = cmd->args[0];
		if (!lookStr(foodFieldsHash,name)) {
			Field* field = allocField(name);
			link(foodFields,field);
			insertStr(foodFieldsHash,name,field);
		}
		cmd = cmd->next;
	}
	endlink(foodFields);
}


/* handle the input:, input format:, input fields:, input *fields: and
   input scale: commands */
void setInput() {

	if (!inputFileName) inputFileName = inputCmd->args[0];
	setSepDecPoint(inputCmd->args[3],inputCmd->args[4],inputCmd->args[5],
				   &inputSep,&inputDecPoint,&inputComment);

	/* decide the format of the input file */
	if (inputFormatCmd) {
		if (strcmp(inputFormatCmd->args[0],"text") == 0) {
			inputFormat = formatText;
			if (inputFieldsCmd)
				error("Input format text can not be used with the 'input fields' command.\n");
			if (inputStarFieldsCmd)
				error("Input format text can not be used with the 'input *fields' command.\n");
		} else if (strcmp(inputFormatCmd->args[0],"text-no-head") == 0) {
			inputFormat = formatTextNoHead;
			if (!inputFieldsCmd) {
				error("Input format text-no-head can not be used without the 'input fields' command.\n");
				return;
			}
		} else if (strcmp(inputFormatCmd->args[0],"bin-native") == 0) {
			inputFormat = formatBinNative;
			if (!inputFieldsCmd) {
				error("Input format bin-native can not be used without the 'input fields' command.\n");
				return;
			}
			if (inputStarFieldsCmd) {
				error("Input format bin-native can not be used with the 'input *fields' command.\n");
				return;
			}
			if (strcmp(inputFileName,"-") == 0) {
				error("Input format bin-native can not be used with standard input.\n");
				return;
			}
		} else {
			error("Unknown input format '%s'.\n",inputFormatCmd->args[0]);
		}
	} else {
		if (inputFieldsCmd) inputFormat = formatTextNoHead;
		else {
			inputFormat = formatText;
			if (inputStarFieldsCmd)
				error("The 'input *fields' command can not be used without the 'input fields' command.\n");
		}
	}

	nolink(inputFields);
	inputFieldsHash = newHashStr(89);
	inputStarFields = 0;

	if (inputFormat != formatText) {
		/* read field names from the input *fields: and input fields: commands */
		if (inputStarFieldsCmd) {
			char** arg = inputStarFieldsCmd->args;
			while (*arg) {
				char* name = *arg;
				Field* field;
				link(inputFields,(field = allocField(name)));
				field->fromPos = inputFields.no;

				if (lookStr(inputFieldsHash,name)) {
					error("Dublicate field '%s' in input file.\n",name); 
				} else {
					field->noCalc = 1;
					insertStr(inputFieldsHash,field->name,field);
				}
				arg++;
				inputStarFields++;
			}
		}
		{
			char** arg = inputFieldsCmd->args;
			while (*arg) {
				char* name = *arg;
				Field* field;
				link(inputFields,(field = allocField(name)));
				field->fromPos = inputFields.no;

				if (lookStr(inputFieldsHash,name)) {
					error("Dublicate field '%s' in input file.\n",name); 
				} else {
					field->noCalc = 1;
					insertStr(inputFieldsHash,field->name,field);
				}
				arg++;
			}
		}

	} else {
		/* read field names from the input file */
		int inStar = 0;
		if (!(inputFile = openInFile(inputFileName,inputFileT,&inputSep,&inputDecPoint,&inputComment)))
			return;
		setCurrent(inputFile);
		skipComment();
		if (ch == '*') {inStar = 1; getch(); skipSpace();}
		while (1) {
			char* name;
			Field* field;
			if (eol()) {
				if (!inStar) break;
				skipEol(); skipComment();
				inStar = 0;
			}
			if (!getFieldInList()) return;
			name = saveStr();
			link(inputFields,(field = allocField(name)));
			field->fromPos = inputFields.no;
			if (inStar) inputStarFields++;

			if (lookStr(inputFieldsHash,name)) {
				error("Dublicate field '%s' in input file %s.\n",name,inputFile->name); 
			} else {
				field->noCalc = 1;
				insertStr(inputFieldsHash,field->name,field);
			}
		}
		skipEol();
		getCurrent(inputFile);
	}

	endlink(inputFields);

	/* set/check the food id field and the amount field */
	if (inputFields.no < 2) {
		error("Food field and amount field not found in input file.\n");
	} else {
		if (inputCmd->args[1]) {
			if (!(inputFoodField = lookStr(inputFieldsHash,inputCmd->args[1])))
				{error("Food field '%s' not found in input file.\n",inputCmd->args[1]);}
		} else {
			inputFoodField = inputFields.first;
		}
		if (inputCmd->args[2]) {
			if (!(inputAmountField = lookStr(inputFieldsHash,inputCmd->args[2])))
				{error("Amount field '%s' not found in input file.\n",inputCmd->args[2]);}
		} else {
			inputAmountField = inputFields.first->next;
		}
	}

	/* handle the input scale command */
	if (inputScaleCmd) {
		if (!(inputAmountScale = (Num)atof(inputScaleCmd->args[0])))
			error("Bad input scale value '%s'.\n",inputScaleCmd->args[0]);
	} else {
		inputAmountScale = 1.0;
	}
}


/* handle the recipes:, recipe sum: and ingredients: commands */
void setRecipes() {
	Cmd* cmd = recipesCmd;

	nolink(recipesFiles);
	while (cmd) { /* handle the recipes: commands one by one */
		File* file;
		if ((file = openInFile(cmd->args[0],groupsFileT,cmd->args[4],cmd->args[5],cmd->args[6]))) {
			int status = 0;
			int inStar = 0;
			RecipesFile* recipesFile = allocStruct(RecipesFile);

			recipesFile->file = file;
			recipesFile->fields = allocStruct(FieldChain);
			recipesFile->fieldsHash = newHashStr(13);
			recipesFile->starFields = 0;

			/* get field names from the file */
			nolink(*(recipesFile->fields));
			setCurrent(file);
			skipComment();
			if (ch == '*') {inStar = 1; getch(); skipSpace();}
			while (1) {
				char* name;
				Field* field;
				if (eol()) {
					if (!inStar) break;
					skipEol(); skipComment();
					inStar = 0;
				}
				if (!getFieldInList()) {status = 1; break;}
				name = saveStr();
				link(*(recipesFile->fields),(field = allocField(name)));
				field->fromPos = recipesFile->fields->no;
				if (inStar) recipesFile->starFields++;

				if (lookStr(recipesFile->fieldsHash,name)) {
					status = 1;
					error("Dublicate field '%s' in recipes file %s.\n",name,currentFileName); 
				} else {
					field->noCalc = 1;
					insertStr(recipesFile->fieldsHash,name,field);
				}
			}
			skipEol();
			getCurrent(file);
			endlink(*(recipesFile->fields));

			/* set/check the recipe id, food id, and amount fields */
			if (!status) {
				if (recipesFile->fields->no < 3) {
					error("Recipe, Food and amount field not found in recipe file %s.\n",
						currentFileName);
				} else {
					if (cmd->args[1]) {
						if (!(recipesFile->recipeId = lookStr(recipesFile->fieldsHash,cmd->args[1])))
							error("Recipe field '%s' not found in recipe file %s.\n",
								cmd->args[1],currentFileName);
					} else {
						recipesFile->recipeId = recipesFile->fields->first;
					}
					if (cmd->args[2]) {
						if (!(recipesFile->foodId = lookStr(recipesFile->fieldsHash,cmd->args[2])))
							error("Food field '%s' not found in recipe file %s.\n",
								cmd->args[2],currentFileName);
					} else {
						recipesFile->foodId = recipesFile->fields->first->next;
					}
					if (cmd->args[3]) {
						if (!(recipesFile->amountField = lookStr(recipesFile->fieldsHash,cmd->args[3])))
							error("Amount field '%s' not found in recipe file %s.\n",
								cmd->args[3],currentFileName);
					} else {
						recipesFile->amountField = recipesFile->fields->first->next->next;
					}
				}
			}

			link(recipesFiles,recipesFile);
		}
		cmd = cmd->next;
	}
	endlink(recipesFiles);

	/* handle the recipe sum: and the ingredients: commands */
	if (recipesCmd) {
		if (!recipeSumCmd)
			error("When recipes files are used the 'food weight' command must also be used.\n");
		if (ingredientsCmd) {
			if (strcmp(ingredientsCmd->args[0],"keep") == 0) {
				keepIngredients = 1;
				copyRecipeFields = 0;
			} else if (strcmp(ingredientsCmd->args[0],"keepx") == 0) {
				keepIngredients = 1;
				copyRecipeFields = 1;
			} else if (strcmp(ingredientsCmd->args[0],"sum") == 0) {
				keepIngredients = 0;
				copyRecipeFields = 1;
			} else {
				error("Unknown value '%s' for the ingredients command.\n",
					ingredientsCmd->args[0]);
			}
		} else {
			keepIngredients = 0;
			copyRecipeFields = 1;
		}
	} else {
		if (ingredientsCmd)
			error("The 'ingredients' command can not be used without recipes files.\n");
		keepIngredients = 0;
	}
}



/*---------------------------------------------------------------------------------*/
/*--- STEP 2, sub-step B -*

/* Expand and check lists of fields. Also set text, noCalc and groupBy for all fields. */


/* this utility function takes a field-list (in args) from a Cmd and check and
   expand the fields into a FielP list (in fields). All the fields must be in the
   foodFieldsHash (which contains all fields wich can be in the food table (from
   foods files or groups files (*not* from calculate: commands)). Note, that nolink() and
   endlink() are *not* called for the fields list */
void setFieldList(FieldPChain* fields, char** args, char* msg) {
	while (*args) {
		Field* field;
		if (*args == fieldRange) {
			char* from;
			char* to;
			if (!(from = *++args) || !(to = *++args)) {
				error("Error in format of range of fields in %s\n",msg);
			} else if (!(field = lookStr(foodFieldsHash,from))) {
				error("Field '%s' not found in the food table in %s.\n",from,msg);
			} else {
				while (1) {
					link(*fields,allocFieldP(field));
					if (strcmp(field->name,to) == 0) break;
					if (!(field = field->next)) {
						error("Field '%s' not found in the food table in %s.\n",to,msg);
						break;
					}
				}
			}
		} else if (!(field = lookStr(foodFieldsHash,*args))) {
			error("Field '%s' not found in the food table in %s.\n",*args,msg);
		} else {
			link(*fields,allocFieldP(field));
		}
		args++;
	}
}

/* handle the no-calc fields: command */
void checkNoCalc(Field* field, char* type) {
	if (field->noCalc) error("The %s field '%s' must not be a no-clac field.\n",type,field->name);
}
void setNoCalc() {
	Cmd* cmd = noCalcFieldsCmd;
	FieldPChain fields; /* will contain all fields in all no-calc fields: commands */

	nolink(fields);
	while (cmd) { /* expand the arguments from the no-calc commands one by one */
		setFieldList(&fields,cmd->args,"no-calc list");
		cmd = cmd->next;
	}
	endlink(fields);

	{ /* no make all the fields noCalc fields */
		FieldP* fieldP = fields.first;
		while (fieldP) {
			fieldP->field->noCalc = 1;
			fieldP = fieldP->next;
		}
	}
}


/* handle the text fields: command */
void checkText(Field* field, char* type) {
	if (field->text) error("The %s field '%s' must not be a text field.\n",type,field->name);
}
void setText() {
	Cmd* cmd = textFieldsCmd;
	while (cmd) {
		char** args = cmd->args;
		while (*args) {
			Field* field;
			if (*args == fieldRange) {
				char* from;
				char* to;
				if (!(from = *++args) || !(to = *++args)) {
					error("Error in format of range of fields in list of text fields.\n");
					return;
				}
				if (field = lookStr(foodFieldsHash,from)) {
					while (1) {
						field->text = field->noCalc = 1;
						if (strcmp(field->name,to) == 0) break;
						if (!(field = field->next)) {
							error("Ending field in range '%s'--'%s' not found.\n",from,to);
							break;
						}
					}
				} else {
					error("Text field '%s' not found.\n",from);
				}
			} else {
				int found = 0;
				if (field = lookStr(inputFieldsHash,*args)) {
					field->text = 1;
					found = 1;
				}
				if (field = lookStr(foodFieldsHash,*args)) {
					field->text = field->noCalc = 1;
					found = 1;
				}
				{
					RecipesFile* recipesFile = recipesFiles.first;
					while (recipesFile) {
						if (field = lookStr(recipesFile->fieldsHash,*args)) {
							field->text = 1;
							found = 1;
						}
						recipesFile = recipesFile->next;
					}
				}

			    if (!found)	error("Text field '%s' not found.\n",*args);
			}
			args++;
		}
		cmd = cmd->next;
	}
	checkText(inputFoodField,"input food");
	checkText(inputAmountField,"input amount");
	checkText(foodId,"food id");
	{
		GroupsFile* groupsFile = groupsFiles.first;
		while (groupsFile) {
			FieldP* fieldP = groupsFile->groupIds->first;
			while (fieldP) {
				checkText(fieldP->field,"group id");
				fieldP = fieldP->next;
			}
			groupsFile = groupsFile->next;
		}
	}
	{
		RecipesFile* recipesFile = recipesFiles.first;
		while (recipesFile) {
			checkText(recipesFile->recipeId,"recipe id");
			checkText(recipesFile->foodId,"recipe food id");
			checkText(recipesFile->amountField,"recipe amount");
			recipesFile = recipesFile->next;
		}
	}
}


/* handle the non-edible field: command */
void setNonEdible() {
	nonEdibleField = NULL;
	nonEdibleFlagName = NULL;
	if (nonEdibleFieldCmd) {
		char **args = nonEdibleFieldCmd->args;
		if (!(nonEdibleField = lookStr(foodFieldsHash,*args))) {
			error("Non-edible field '%s' not found in food table.\n",*args);
		} else {
			checkText(nonEdibleField,"non-edible");
			nonEdibleField->noCalc = 1;
			nonEdibleFlagName = *++args;
		}
	}
}


/* handle the group by: command */
void setGroupBy() {

	nolink(groupByFields);
	nolink(groupByFoodFields);
	if (groupByCmd) {
		char **args = groupByCmd->args;
		FieldPChain* fields = &groupByFields;
		while (*args) { /* handle the fields in the group by: command one by one */
			Field* field = NULL;
			if (!(field = lookStr(inputFieldsHash,*args))) {
				if (field = lookStr(foodFieldsHash,*args)) {
					fields = &groupByFoodFields;
				} else {
					error("Group By field '%s' not found in input file nor in food table.\n",*args); 
				}
			}
			if (field) {
				link(*fields,allocFieldP(field));
				field->groupBy = 1;
				field->noCalc = 1; /* group by fields are noCalc */
				checkText(field,"group by");
			}
			args++;
		}
		endlink(groupByFields);
		endlink(groupByFoodFields);
	}
}

/* handle the set: commands */
forward void setSetH(Field* field, Exp* e, int add);
forward void setSetExp(SetOperChain* opers, Exp* e);
forward void setSetExp1(SetOperChain* opers, Exp* e1);
forward void linkOper(SetOperChain* opers, SetOp op, ExpVal* v, int add);
SetChain* currentSets;
void setSet() {
	Cmd* cmd = setCmd;

	calculateFieldsHash = newHashStr(89);
	nolink(calculateFields);
	nolink(sets);
	while (cmd) { /* handle the set: commands one by one */
		char* name = cmd->args[0];
		Exp* e = (Exp*)cmd->args[1];

		if (lookStr(foodFieldsHash,name)) {
			error("Set field '%s' found in food table.\n",name);
		} else if (lookStr(inputFieldsHash,name)) {
			error("Set field '%s' found in input file.\n",name);
		} else if (lookStr(calculateFieldsHash,name)) {
			error("Set field '%s' set twice.\n",name);
		} else {

			Field* field = allocField(name);
			link(calculateFields,field);
			insertStr(calculateFieldsHash,name,field);
			currentSets = &sets;
			setSetH(field,e,1);
		}
		cmd = cmd->next;
	}
	endlink(sets);
	endlink(constants);
	endlink(calculateFields);
}
void setSetH(Field* field, Exp* e, int add) {
	Set* set = allocSet(field);
	if (currentSets == &sets) link(calculateFields,field);
	if (add) setSetExp(&(set->opers),e);
	else setSetExp1(&(set->opers),e);
	endlink(set->opers);
	link(*currentSets,set);
}
void setSetExp(SetOperChain* opers, Exp* e) {
	ExpVal* v = e->first;
	if (v->type == expVal) setSetExp1(opers,v->u.exp);
	else linkOper(opers,cpyOp,v,1);
	v = v->next;
	while (v) {
		linkOper(opers,((v->neg)?subOp:addOp),v,1);
		v = v->next;
	}
}
void setSetExp1(SetOperChain* opers, Exp* e1) {
	ExpVal* v1 = e1->first;
	if (v1->type == expVal) setSetExp(opers,v1->u.exp);
	else linkOper(opers,cpyOp,v1,0);
	v1 = v1->next;
	while (v1) {
		linkOper(opers,((v1->recip)?divOp:mulOp),v1,0);
		v1 = v1->next;
	}
}
void linkOper(SetOperChain* opers, SetOp op, ExpVal* v, int add) {
	Field* field;
	if (v->type == numVal) {
		field = allocConstant(v->u.num);
	} else if (v->type == fieldVal) {
		if (!(field = lookStr(foodFieldsHash,v->u.name)) &&
			!(field = lookStr(calculateFieldsHash,v->u.name))) {
			if (currentSets == &sets) {
				error("Field '%s' in set expression not found.\n",v->u.name);
				return;
			} else if (currentSets == &recipeSets) {
				if (!(field = lookStr(setRecipeFieldsHash,v->u.name))) {
					link(setRecipeFields,field = allocField(v->u.name));
					insertStr(setRecipeFieldsHash,v->u.name,field);
				}
			} else /*(currentSets == &groupSets) */ { 
				if (!(field = lookStr(groupSetFieldsHash,v->u.name))) {
					error("Field '%s' in group set expression not found.\n",v->u.name);
					return;
				}
			}
		}
		checkText(field,"set");
	} else { /* expVal */
		field = allocTempField();
		setSetH(field,v->u.exp,!add);
	}
	link(*opers,allocOper(op,field));
}


/* handle the calculate: commands */
void setCalculate() {
	Cmd* cmd = calculateCmd;

	while (cmd) { /* handle the calculate: commands one by one */
		char** arg = cmd->args;
		char* name = *arg++;

		if (lookStr(foodFieldsHash,name)) {
			error("Calculate field '%s' found in food table.\n",name);
		} else if (lookStr(inputFieldsHash,name)) {
			error("Calculate field '%s' found in input file.\n",name);
		} else if (lookStr(calculateFieldsHash,name)) {
			error("Calculate field '%s' already calculated.\n",name);
		} else {

			int first = 1;
			Field* field = allocField(name);
			Set* set = allocSet(field);
			link(calculateFields,field);
			insertStr(calculateFieldsHash,name,field);
			while (*arg) { /* handle the elements in the list one by one */
				if (isdigit(**arg)) {
					Num num;
					Field* field;
					/* there is an multiplier (weight) */
					if (!(num = (Num)atof(*arg++))) {
						error("Error in calculate '%s', bad multiplier %s.\n",name,*(arg-1));
					} else if (!*arg) {
						error("Error in calculate '%s', field name missing after multiplier.\n",name);
					} else {
						if (!(field = lookStr(foodFieldsHash,*arg)) &&
							!(field = lookStr(calculateFieldsHash,*arg))) {
							error("Error in calculate '%s', field '%s' not found.\n",
								name,*arg);
						} else {
							Field* tfield = allocTempField();
							Set* tset = allocSet(tfield);	
							link(calculateFields,tfield);
							link(tset->opers,allocOper(cpyOp,allocConstant(num)));
							link(tset->opers,allocOper(mulOp,field));
							endlink(tset->opers);
							link(sets,tset);
							link(set->opers,allocOper((first?cpyOp:addOp),tfield));
						}
						arg++;
					}
				} else {
					/* an additor can be a field from a foods file, a groups file, or
					   from a calculate command */
					if (!(field = lookStr(foodFieldsHash,*arg)) &&
						!(field = lookStr(calculateFieldsHash,*arg))) {
						error("Error in calculate '%s', field '%s' not found.\n",
							name,*arg);
					} else {
						link(set->opers,allocOper((first?cpyOp:addOp),field));
					}
					arg++;
				}
				first = 0;
			}
			endlink(set->opers);
			link(sets,set);
		}
		cmd = cmd->next;
	}
	endlink(sets);
	endlink(constants);
	endlink(calculateFields);
}


/* handle the recipe sum: command */
void setRecipeSum() {
	if (recipeSumCmd) {
		char** arg = recipeSumCmd->args;
		if ((recipeSum = (Num)atof(*arg++)) <= 0)
			error("Bad food weight value '%s'.\n",*(arg-1));
		if (!*(arg+1)) {
			/* only one field in the list */
			if (!(recipeSumField = lookStr(foodFieldsHash,*arg)) &&
				!(recipeSumField = lookStr(calculateFieldsHash,*arg))) {
				error("Recipe sum field '%s' not found.\n",*arg);
			}
		} else {
			/* more than one field in the list, so make an extra calculate field */
			Set* set = allocSet(recipeSumField = allocField("food weight"));
			int first = 1;
			link(calculateFields,recipeSumField);
			while (*arg) {
				Field* field;
				if (!(field = lookStr(foodFieldsHash,*arg)) &&
					!(field = lookStr(calculateFieldsHash,*arg))) {
					error("Recipe sum field '%s' not found.\n",*arg);
				} else {
					link(set->opers,allocOper((first?cpyOp:addOp),field));
				}
				arg++;
				first = 0;
			}
			endlink(set->opers);
			link(sets,set); endlink(sets);
		}
	} else {
		recipeSumField = NULL;
	}
}


/* handle the recipe set: command */
void setRecipeSet2() {
	Cmd* cmd = recipeSetCmd;
	
	nolink(recipeSets);
	nolink(setRecipeFields);
	setRecipeFieldsHash = newHashStr(89);
	while (cmd) {
		char* name = cmd->args[0];
		Exp* e = (Exp*)cmd->args[1];
		Field* field = lookStr(foodFieldsHash,name); /* we know it is there bacause of setRecipeSet1() */
		Field* tfield = allocTempField();
		Set* tset = allocSet(field);
		currentSets = &recipeSets;
		setSetH(tfield,e,1); /* we calulate to a temporary */
		link(tset->opers,allocOper(cpyOp,tfield)); /* and then copy it to the resulting field */
		endlink(tset->opers);
		link(recipeSets,tset);
		cmd = cmd->next;
	}
	endlink(setRecipeFields);
	endlink(recipeSets);
	endlink(constants);
}


/* handle the cook:, cook field: and weight cook: commands */
void setCookX(Cmd* cmd, ReducType reducType) {
	while (cmd) { /* handle the cook: commands one by one */
		char** args = cmd->args;
		char* type = *args++;
		char* name = *args++;
		Field* field; /* the reduce field */

		if (!(field = lookStr(foodFieldsHash,name))) {
			error("Cook reduct field '%s' not found in food table.\n",name);
		} else {
			CookChain* cooks;
			Cook* cook = allocStruct(Cook);
			/* se if we alrady has this cook type, if not we insert it in the hash */
			if (!(cooks = lookStr(cookTypesHash,type))) {
				cooks = allocStruct(CookChain);
				nolink(*cooks);
				insertStr(cookTypesHash,type,cooks);
			}
			link(*cooks,cook); /* add this cook reduction to the cook type */
			field->noCalc = 1; /* reduce fields are noCalc */
			checkText(field,"cook reduct");
			cook->field = field;
			cook->type = reducType;
			cook->next = NULL;

			if (reducType == weightReduc) {
				/* is the first field a calculate field? */
				Field* field;
				if (field = lookStr(calculateFieldsHash,*args)) {
					cook->calcField = field;
					args++;
				} else {
					cook->calcField = NULL;
				}
			}

			/* expand the list of fields to reduce */
			nolink(cook->fields);
			setFieldList(&(cook->fields),args,"cook definition");
			endlink(cook->fields);
		}
		cmd = cmd->next;
	}
}
void setCook() {

	cookTypesHash = newHashStr(7);

	if (weightCookCmd && !recipeSumCmd)
		error("You must use the food weight: command when you use the weight cook command.\n");
	setCookX(cookCmd,nutriReduc);
	setCookX(weightCookCmd,weightReduc);

	/* handle the cook field: command. Note that we here only save the name of the
	   field as it is leagal to use the field in the input file, in any recipe file,
	   or not at all.
	   We use the cook types in the cook field: command to build an ordered list
	   of the cook types actually used. The order in the list also gives the number
	   used to identify that cooking type in an input or recipe file. */
	cookFieldName = NULL;
	nolink(cookTypes);
	if (cookFieldCmd) {
		char** args = cookFieldCmd->args;
		cookFieldName = *args++;
		while (*args) {
			CookChain* cooks;
			if (!(cooks = lookStr(cookTypesHash,*args))) {
				error("Cook type '%s' not defined.\n",*args);
			} else {
				CookType* cookType = allocStruct(CookType);
				cookType->type = *args;
				cookType->cooks = cooks;
				link(cookTypes,cookType);
			}
			args++;
		}
	}
	endlink(cookTypes);
}


/* handle reduce field: commands */
void setReductX(Cmd* cmd, ReducType type) {
	while (cmd) { /* handle the reduce field: commands one by one */
		char** args = cmd->args;
		char* name = *args++;

		Reduct* reduct = allocStruct(Reduct);
		link(reducts,reduct);
		reduct->fieldName = name; /* we only save the name of the reduce field for now */
		reduct->type = type;
		reduct->next = NULL;

		if (type == weightReduc) {
			/* is the first field a calculate field? */
			Field* field;
			if (field = lookStr(calculateFieldsHash,*args)) {
				reduct->calcField = field;
				args++;
			} else {
				reduct->calcField = NULL;
			}
		}
			
		/* expand the list of fields to reduce */
		nolink(reduct->fields);
		setFieldList(&(reduct->fields),args,"reduce definition");
		endlink(reduct->fields);
		cmd = cmd->next;
	}
}
void setReduct() {
	Cmd* cmd = reducFieldCmd;

	nolink(reducts);
	setReductX(reducFieldCmd,nutriReduc);
	setReductX(weightReducFieldCmd,weightReduc);
	endlink(reducts);
}


/* handle recipe reduce field: commands */
void setRecipeReduct() {
	Cmd* cmd = recipeWeightReducFieldCmd;

	nolink(recipeReducts);
	while (cmd) { /* handle the recipe reduce field: commands one by one */
		char** args = cmd->args;
		char* name = *args++;

		RecipeReduct* recipeReduct = allocStruct(RecipeReduct);
		link(recipeReducts,recipeReduct);
		recipeReduct->fieldName = name; /* we only save the name of the reduce field for now */
		recipeReduct->next = NULL;

		{ /* is the first field a calculate field? */
			Field* field;
			if (field = lookStr(calculateFieldsHash,*args)) {
				recipeReduct->calcField = field;
				args++;
			} else {
				recipeReduct->calcField = NULL;
			}
		}

		/* expand the list of fields to reduce */
		nolink(recipeReduct->fields);
		setFieldList(&(recipeReduct->fields),args,"recipe reduce definition");
		endlink(recipeReduct->fields);
		cmd = cmd->next;
	}
	endlink(recipeReducts);
}


/* hande the transpose: commands */
void setTranspose() {
	Cmd* cmd = transposeCmd;

	if (cmd && !groupByCmd)
		error("The transpose command can only be used if the group by command is also used.\n");

	nolink(transposes);
	while (cmd) {
		char** args = cmd->args;
		Transpose* transpose = allocStruct(Transpose);
		if (!(transpose->groupField = lookStr(foodFieldsHash,*args))) {
			error("Transpose field '%s' not found in food table.\n",*args);
		} else if ((transpose->noGroups = atoi(*++args)) < 1) {
			error("Bad number of groups '%s' in transpose command.\n",*args);
		} else {
			checkText(transpose->groupField,"transpose");
			transpose->groupField->noCalc = 1;
			nolink(transpose->fields);
			while (*++args) {
				Field* field;
				if (!(field = lookStr(foodFieldsHash,*args)) &&
					!(field = lookStr(calculateFieldsHash,*args))) {
					error("Transpose field '%s' not found.\n",*args);
				} else {
					checkNoCalc(field,"transpose");
					link(transpose->fields,allocFieldP(field));
				}
			}
			endlink(transpose->fields);
			link(transposes,transpose);
		}
		cmd = cmd->next;
	}
	endlink(transposes);
}


/* handle the where: command */
forward void setWhereLexp(Lexp* l, int last, TestPChain* backT, TestPChain* backF);
forward void setWhereLexp1(Lexp* l1, int last, TestPChain* back1T, TestPChain* back1F);
forward Test* setWhereTest(LexpVal* v, int not, TestPChain* back);
typedef enum {constLVal,noCalcLVal,calcLVal} LValType;
forward Field* setWhereField(LVal* val, LValType* type);
forward void setWhereBackPatch(TestPChain* back, int action);
forward void setIf(TestPChain* backF);
forward void setIfNot(TestPChain* backF);
void setWhere() {
	TestPChain backF;
	nolink(backF);
	nolink(tests);
	simpleTest = 1;
	if (whereCmd) {
		TestPChain backT;
		nolink(backT);
		setWhereLexp((Lexp*)whereCmd->args[0],0,&backT,&backF);
		setWhereBackPatch(&backT,tests.no);
	}
	setIf(&backF);
	setIfNot(&backF);
	setWhereBackPatch(&backF,tests.no+1);
	endlink(constants);
	endlink(tests);
}
void setWhereLexp(Lexp* l, int last1, TestPChain* backT, TestPChain* backF) {
	LexpVal* v = l->first;
	while (v) {
		int last = (v == l->last);
		if (v->type == relVal) {
			if (last && !last1) setWhereTest(v,1,backF);
			else setWhereTest(v,0,backT);
		} else {
			TestPChain back1F;
			nolink(back1F);
			setWhereLexp1(v->u.lexp,(last && !last1),backT,(last?backF:&back1F));
			if (!last) setWhereBackPatch(&back1F,tests.no);
		}
		v = v->next;
	}
}
void setWhereLexp1(Lexp* l1, int last, TestPChain* back1T, TestPChain* back1F) {
	LexpVal* v1 = l1->first;
	while (v1) {
		int last1 = (v1 == l1->last);
		if (v1->type == relVal) {
			if (last1 && !last) setWhereTest(v1,0,back1T);
			else setWhereTest(v1,1,back1F);
		} else {
			TestPChain backT;
			nolink(backT);
			setWhereLexp(v1->u.lexp,(last1 && !last),(last1?back1T:&backT),back1F);
			if (!last1) setWhereBackPatch(&backT,tests.no);
		}
		v1 = v1->next;
	}
}
Test* setWhereTest(LexpVal* v, int not, TestPChain* back) {
	LValType type1,type2;
	Test* test = allocTest(notOp(not,v->u.r.op),
		setWhereField(v->u.r.val1,&type1),setWhereField(v->u.r.val2,&type2));
	if (type1 == constLVal && type2 == noCalcLVal) { /* swap */
		Field* field = test->field1;
		test->field1 = test->field2;
		test->field2 = field;
	}
	if (type1 != noCalcLVal || type2 != constLVal) simpleTest = 0;
	link(tests,test);
	link(*back,allocTestP(test));
	return(test);
}
void setWhereBackPatch(TestPChain* back, int action) {
	TestP* testP = back->first;
	endlink(*back);
	while (testP) {
		testP->test->action = action;
		testP = testP->next;
	}
}
Field* setWhereField(LVal* val, LValType* type) {
	Field* field;
	if (val->type == numVal) {
		field = allocConstant(val->u.num);
		*type = constLVal;
	} else if ((field = lookStr(foodFieldsHash,val->u.name)) ||
			   (field = lookStr(calculateFieldsHash,val->u.name))) {
		checkText(field,"where");
		*type = (field->noCalc?noCalcLVal:calcLVal);
	} else {
		error("Where field '%s' not found.\n",val->u.name);
		field = allocTempField();
		*type = calcLVal;
	}
	return(field);
}
void setIf(TestPChain* backF) {
	Cmd* cmd = ifCmd;
	TestPChain backT;
	nolink(backT);
	while (cmd) {
		char** arg = cmd->args;
		Field* field;
		if (!(field = lookStr(foodFieldsHash,*arg)))  {
			error("If field '%s' not found in food table.\n",*arg);
		} else {
			field->noCalc = 1; /* fields in if: are noCalc */
			checkText(field,"if");
			while (*++arg) {
				int last = (!cmd->next && !*(arg+1));
				Test* test = allocTest((last?neOp:eqOp),
					field,allocConstant((Num)atof(*arg)));
				if (last) {link(*backF,allocTestP(test));}
				else {link(backT,allocTestP(test));}
				link(tests,test);
			}
		}
		cmd = cmd->next;
	}
	setWhereBackPatch(&backT,tests.no);
}
void setIfNot(TestPChain* backF) {
	Cmd* cmd = ifNotCmd;
	while (cmd) {
		char** arg = cmd->args;
		Field* field;
		if (!(field = lookStr(foodFieldsHash,*arg)))  {
			error("If not field '%s' not found in food table.\n",*arg);
		} else {
			field->noCalc = 1; /* fields in if not: are noCalc */
			checkText(field,"if not");
			while (*++arg) {
				Test* test = allocTest(eqOp,
					field,allocConstant((Num)atof(*arg)));
				link(*backF,allocTestP(test));
				link(tests,test);
			}
		}
		cmd = cmd->next;
	}
}


/* handle the group set: command */
void setGroupSet() {
	Cmd* cmd = groupSetCmd;
	
	nolink(groupSets);
	nolink(groupSetFields);
	groupSetFieldsHash = newHashStr(89);
	while (cmd) {
		char* name = cmd->args[0];
		Exp* e = (Exp*)cmd->args[1];
		if (lookStr(foodFieldsHash,name)) {
			error("Group set field '%s' found in food table.\n",name);
		} else if (lookStr(inputFieldsHash,name)) {
			error("Group set field '%s' found in input file.\n",name);
		} else if (lookStr(calculateFieldsHash,name)) {
			error("Group set field '%s' is also set.\n",name);
		} else if (lookStr(groupSetFieldsHash,name)) {
			error("Group set field '%s' set twice.\n",name);
		} else {
			Field* field = allocField(name);
			link(groupSetFields,field);
			insertStr(groupSetFieldsHash,name,field);
			currentSets = &groupSets;
			setSetH(field,e,1);
		}
		cmd = cmd->next;
	}
	endlink(groupSets);
	endlink(groupSetFields);
	endlink(sets);
	endlink(constants);
}



/*-------------------------------------------------------------------------------*/
/*--- STEP 2, sub-step C -*/

/* Set toPos and onlyRecipe for all fields. */


/* utilty function which marks a field for output, thereby also giving it a position
   in the output file. Also sets onlyRecipe as specified in the argument */
void setFieldToPosNoCheck(Field* field, int onlyRecipe) {
	link(outputFields,allocFieldP(field));
	field->toPos = outputFields.no;
	field->onlyRecipe = onlyRecipe;
}
void setFieldToPos(Field* field, int onlyRecipe) {
	if (field->toPos) {
		error("Output field '%s' listed more than once.\n",field->name);
	} else if (field->text) {
		error("Output field '%s' must not be a text field.\n",field->name);
	} else {
		if ((groupByFields.no || groupByFoodFields.no) && field->noCalc && !field->groupBy) {
			error("Can not output field '%s' because it is a no-calc field and not a group by field.\n",field->name);
		} else {
			setFieldToPosNoCheck(field,onlyRecipe);
		}
	}
}
void setFieldToPosX(Field* field, int onlyRecipe) {
	if (!field->toPos) setFieldToPosNoCheck(field,onlyRecipe);
	else if (!onlyRecipe) field->onlyRecipe = 0;
}



/* handle the output format: and output fields: commands or uses defaults */
void setToPos() {
	char dummy;

	if (!outputFileName) outputFileName = outputCmd->args[0];
	setSepDecPoint(outputCmd->args[1],outputCmd->args[2],&dummy,
				   &outputSep,&outputDecPoint,&dummy);

	/* decide the format of the format of the output file */
	if (outputFormatCmd) {
		if (strcmp(outputFormatCmd->args[0],"text") == 0) {
			outputFormat = formatText;
		} else if (strcmp(outputFormatCmd->args[0],"text-no-head") == 0) {
			outputFormat = formatTextNoHead;
		} else if (strcmp(outputFormatCmd->args[0],"bin-native") == 0) {
			outputFormat = formatBinNative;
			if (strcmp(outputFileName,"-") == 0)
				error("Output format bin-native can not be used with standard output.\n");
		} else {
			error("Unknown output format '%s'.\n",outputFormatCmd->args[0]);
		}
	} else {
		outputFormat = formatText;
	}

	if (outputFieldsCmd) {
		/* handle the output fields: command. The fields may be from foods fiels,
		   groups files or calculate: commands */
		char** args = outputFieldsCmd->args;
		while (*args) {
			Field* field;
			if (*args == fieldRange) {
				char* from;
				char* to;
				if (!(from = *++args) || !(to = *++args)) {
					error("Error in format of range of fields in list of output fields.\n");
					return;
				}
				if (field = lookStr(foodFieldsHash,from)) {
					while (1) {
						setFieldToPos(field,0);
						if (strcmp(field->name,to) == 0) break;
						if (!(field = field->next)) {
							error("Ending field in range '%s'--'%s' not found.\n",from,to);
							break;
						}
					}
				} else {
					error("Output field '%s' not found.\n",from);
				}
			} else if (field = lookStr(inputFieldsHash,*args)) {
				setFieldToPos(field,0);
			} else if (field = lookStr(foodFieldsHash,*args)) {
				setFieldToPos(field,0);
			} else if (field = lookStr(calculateFieldsHash,*args)) {
				setFieldToPos(field,0);
			} else if (field = lookStr(groupSetFieldsHash,*args)) {
				setFieldToPos(field,0);
			} else {
				error("Output field '%s' not found.\n",*args);
			}
			args++;
		}

	} else {
		/* no output fields: command used, so we output all meaningsfull fields (from 
		   input file, from the food table and from calculate: commands */
		Field* field = inputFields.first;
		while (field) {
			if (!field->text &&
				(!(groupByFields.no || groupByFoodFields.no) 
				 || !field->noCalc || field->groupBy))
				setFieldToPos(field,0);
			field = field->next;
		}
		field = foodFields.first;
		while (field) {
			if (!field->text &&
				(!(groupByFields.no || groupByFoodFields.no) 
				 || !field->noCalc || field->groupBy))
				setFieldToPos(field,0);
			field = field->next;
		}
		field = calculateFields.first;
		while (field) {
			setFieldToPos(field,0);
			field = field->next;
		}
		field = groupSetFields.first;
		while (field) {
			setFieldToPos(field,0);
			field = field->next;
		}
	}

}


/* hande transpose: command */
void setToPosTranspose() {

	Transpose* transpose = transposes.first;
	while (transpose) {
		FieldP* fieldP = transpose->fields.first;
		nolink(transpose->transFields);
		while (fieldP) {
			int digits = ((transpose->noGroups <= 9)? 1 : 2);
			char* format = ((digits == 1)? "%s%1d" : "%s%02d");
			char* tname = fieldP->field->name;
			int nameLen = strlen(tname) + digits;
			int i = 0;
			while (++i <= transpose->noGroups) {
				char* name = alloc(nameLen+1);
				Field* field = allocField(name);
				sprintf(name,format,tname,i);
				setFieldToPosNoCheck(field,0);
				if (i == 1) link(transpose->transFields,allocFieldP(field));
			}
			fieldP = fieldP->next;
		}
		endlink(transpose->transFields);
		transpose = transpose->next;
	}

	/* the output fields we have now is the "real" output fields, which are to be written
	   to the output file. However we may later add extra fields to the outputFields list
	   bacause we need to otherwise threed these fields as output fields (we need to
	   reduce them or move them to a recipe ingredient) */
	noRealOutputFields = outputFields.no;

	transpose = transposes.first;
	while (transpose) {
		FieldP* fieldP = transpose->fields.first;
		while (fieldP) {
			Field* field = fieldP->field;
			checkNoCalc(field,"transpose");
			setFieldToPosX(field,0);
			fieldP = fieldP->next;
		}
		setFieldToPosX(transpose->groupField,1);
		transpose = transpose->next;
	}
}


/* handle group by: command */
void setToPosGroupBy() {
	/* all group by fields need to be output fields because of the way we aggregate
	   groups */
	FieldP* fieldP = groupByFields.first;
	while (fieldP) {
		Field* field = fieldP->field;
		setFieldToPosX(field,0);
		fieldP = fieldP->next;
	}
	fieldP = groupByFoodFields.first;
	while (fieldP) {
		Field* field = fieldP->field;
		setFieldToPosX(field,0);
		fieldP = fieldP->next;
	}
}


/* handle recipe sum: command */	
void setToPosRecipeSum() {
	if (recipeSumField) {
		checkNoCalc(recipeSumField,"recipe sum");
		setFieldToPosX(recipeSumField,1);
	}
}


/* handle weight cook: command. */
void setToPosCook() {
	CookType* cookType = cookTypes.first;
	int onlyRecipe = 1;
	if (cookFieldName && lookStr(inputFieldsHash,cookFieldName)) onlyRecipe = 0;

	while (cookType) {
		Cook* cook = cookType->cooks->first;
		while (cook) {
			if (cook->type == weightReduc) {
				int used = 0;
				int onlyRecipe2 = 1;

				FieldP* fieldP = cook->fields.first;
				while (fieldP) {
					if (fieldP->field->toPos) {
						used = 1;
						if (!fieldP->field->onlyRecipe && !onlyRecipe) onlyRecipe2 = 0;
					}
					fieldP = fieldP->next;
				}
				
				if (used) {
					Field* field = cook->calcField;
					if (field) {
						setFieldToPosX(field,onlyRecipe2);
					} else {
						field = cook->fields.first->field;
						setFieldToPosX(field,onlyRecipe2);
					}
				}
			}

			cook = cook->next;
		}
		cookType = cookType->next;
	}
}


/* handle weight reduc: command. */
void setToPosReduct() {
	Reduct* reduct = reducts.first;
	while (reduct) {
		if (reduct->type == weightReduc) {
			int used = 0;
			int onlyRecipe = 1;

			FieldP* fieldP = reduct->fields.first;
			while (fieldP) {
				if (fieldP->field->toPos) {
					used = 1;
					if (!fieldP->field->onlyRecipe) onlyRecipe = 0;
				}
				fieldP = fieldP->next;
			}
			if (!lookStr(inputFieldsHash,reduct->fieldName)) onlyRecipe = 1;
			
			if (used) {
				Field* field = reduct->calcField;
				if (field) {
					setFieldToPosX(field,onlyRecipe);
				} else {
					field = reduct->fields.first->field;
					setFieldToPosX(field,onlyRecipe);
				}
			}
		}

		reduct = reduct->next;
	}
}


/* handle recipe reduce field: command */
void setToPosRecipeReduct() {
	RecipeReduct* recipeReduct = recipeReducts.first;
	while (recipeReduct) {
		Field* field = recipeReduct->calcField;
		if (field) {
			setFieldToPosX(field,1);
		} else {
			field = recipeReduct->fields.first->field;
			setFieldToPosX(field,1);
		}
		recipeReduct = recipeReduct->next;
	}
}


/* handle where: */
void setToPosWhere() {
	Test* test = tests.first;
	while (test) {
		setFieldToPosX(test->field1,simpleTest);
		setFieldToPosX(test->field2,simpleTest);
		test = test->next;
	}
}


/* handle set: and recipe set: commands. We only do calculations if the new set field
   is an output field. All fields used in a calculation must be output, so they are
   reduced if they are food table fields or calculated if they them self are calculate
   fields - for this last reason we hande the set commands backwards (if a
   set field has to be calculated, this may cause other set fields to
   be also calculated) */
void setToPosSetRecurs(Set* set, int onlyRecipe) {
	if (set) {
		Field* setField = set->field;
		int only = (onlyRecipe || setField->onlyRecipe);
		SetOper* oper = set->opers.first;
		setToPosSetRecurs(set->next,onlyRecipe);
		if (setField->toPos) while (oper) {
			Field* field = oper->field;
			setFieldToPosX(field,only);
			oper = oper->next;
		}
	}
}
void setToPosSet() {
	setToPosSetRecurs(groupSets.first,0);
	setToPosSetRecurs(recipeSets.first,1);
	setToPosSetRecurs(sets.first,0);
}


/* handle recipes: and recipe reduce field: commands. Fields from recipe files
   must be output so we have access to them when flushing recipes. */
void setToPosRecipes() {
	RecipesFile* recipesFile = recipesFiles.first;
	while (recipesFile) {
		setFieldToPosNoCheck(recipesFile->recipeId,1);
		setFieldToPosNoCheck(recipesFile->amountField,1);
		{ /* output all fields used in recipe set: commands */
			Field* field = setRecipeFields.first;
			while (field) {
				Field* field2;
				if (field2 = lookStr(recipesFile->fieldsHash,field->name)) {
					checkText(field2,"recipe set");
					field2->toPos = field->toPos;
				} else {
					error("Field '%s' used in recipe set: command is not found in %s.\n",
						field->name,recipesFile->file->name);
				}
				field = field->next;
			}
		}
		{ /* output all recipe reduce fields */
			RecipeReduct* recipeReduct = recipeReducts.first;
			while (recipeReduct) {
				Field* field;
				if (field = lookStr(recipesFile->fieldsHash,recipeReduct->fieldName)) {
					checkText(field,"recipe reduce");
					setFieldToPosX(field,1);
				}
				recipeReduct = recipeReduct->next;
			}
		}
		if (copyRecipeFields) { /* output fields wich are found with same name in
								   the food table and are noCalc */
			Field* field = recipesFile->fields->first;
			while (field) {
				if (field != recipesFile->foodId && !field->toPos) {
					Field* field2;
					if ((field2 = lookStr(foodFieldsHash,field->name)) && 
						field2->toPos && field2->noCalc) {
						setFieldToPosNoCheck(field,1);
						field->groupBy = 1; /* special marker! */
					}
				}
				field = field->next;
			}
		}
		recipesFile = recipesFile->next;
	}
}


void logToPosField(Field* field) {
	logmsg("%s(%d%s)",field->name,field->toPos,(field->onlyRecipe?"R":""));
}

void logToPosSetH(Set* set, char* setType) {
	while (set) {
		SetOper* oper = set->opers.first;
		logmsg("+++%s: ",setType);
		logToPosField(set->field);
		logmsg(" = ");
		while (oper) {
			switch (oper->op) {
			case addOp: logmsg("+"); break;
			case subOp: logmsg("-"); break;
			case mulOp: logmsg("*"); break;
			case divOp: logmsg("/"); break;
			}
			logToPosField(oper->field);
			oper = oper->next;
		}
		logmsg("\n");
		set = set->next;
	}
}
void logToPosSet() {
	logToPosSetH(sets.first,"Set");
	logToPosSetH(recipeSets.first,"Recipe set");
	logToPosSetH(groupSets.first,"Group set");
}

void logToPosWhere() {
	Test* test = tests.first;
	int no = 0;
	if (test) {
		logmsg("+++where:\n");
		while (test) {
			logmsg("          %d: ",no++);
			logToPosField(test->field1);
			switch(test->op) {
			case eqOp: logmsg(" = "); break;
			case neOp: logmsg(" <> "); break;
			case gtOp: logmsg(" > "); break;
			case geOp: logmsg(" >= "); break;
			case ltOp: logmsg(" < "); break;
			case leOp: logmsg(" <= "); break;
			}
			logToPosField(test->field2);
			logmsg("  -> %d\n",test->action);
			test = test->next;
		}
		logmsg("          %d: Use\n",no++);
		logmsg("          %d: Skip\n",no++);
	}
}


/*------------------------------------------------------------------------------*/
/*--- PART 2, sub-part D -*

/* set fromPos for all food table fields. Also check for type of fields */


/* utility function to mark a field for inclusion in the food table. This will also
   set the position in the food table */
void setFromPosField(Field* field) {
	link(foodTableFields,allocFieldP(field));
	field->fromPos = foodTableFields.no;
}


/* all fields from foods and groups files that we are outputing should be in the
   food table */
void setFromPosOutput() {	
	Field* field = foodFields.first;
	while (field) {
		if (field == foodId || field->toPos) setFromPosField(field);
		field = field->next;
	}
}


/* handle the non-edible field: command */
void setFromPosNonEdible() {
	if (nonEdibleField && !nonEdibleField->fromPos) setFromPosField(nonEdibleField);
}


/* handle the cook types */
void setFromPosCook() {
	CookType* cookType = cookTypes.first;

	while (cookType) {
		Cook* cook = cookType->cooks->first;

		while (cook) {
			/* we need the reduce field in the food table if any of the fields it
			   reduces are in the food table */
			FieldP* fieldP = cook->fields.first;
			int use = 0;

			while (fieldP) {
				Field* field = fieldP->field;
				if (field->toPos) {
					checkNoCalc(field,"cook");
					use = 1;
				}
				fieldP = fieldP->next;
			}
			if (use && !cook->field->fromPos) setFromPosField(cook->field);
			cook = cook->next;
		}
		cookType = cookType->next;
	}
}


/* handle the reduct fields */
void setFromPosReduct() {
	Reduct* reduct = reducts.first;

	while (reduct) {
		FieldP* fieldP = reduct->fields.first;

		while (fieldP) {
			Field* field = fieldP->field;
			if (field->toPos) {
				checkNoCalc(field,"reduce field");
			}
			fieldP = fieldP->next;
		}
		reduct = reduct->next;
	}
}


/* handle the recipe reduct fields */
void setFromPosRecipeReduct() {
	RecipeReduct* recipeReduct = recipeReducts.first;

	while (recipeReduct) {
		FieldP* fieldP = recipeReduct->fields.first;

		while (fieldP) {
			Field* field = fieldP->field;
			if (field->toPos) {
				checkNoCalc(field,"recipe reduce");
			}
			fieldP = fieldP->next;
		}
		recipeReduct = recipeReduct->next;
	}
}


/* handle groups files, note that we handle the files in reverse order */
void setFromPosGroups(GroupsFile* groupsFile) {

	if (groupsFile) setFromPosGroups(groupsFile->next); else return;

	{ /* first count how many fields in the groups file are in the food table */
		FieldP* fieldP = groupsFile->fieldPs->first;
		while (fieldP) {
			if (fieldP->field->fromPos) groupsFile->noFromFields++;
			fieldP = fieldP->next;
		}
	}

	if (groupsFile->noFromFields) {
		/* at least one field are in the food table */
		FieldP* fieldP = groupsFile->groupIds->first;
		while (fieldP) {
			Field* field = fieldP->field;
			if (!field->fromPos) {
				/* the group id field must also be in the food table */
				setFromPosField(field);
				groupsFile->noFromFields++;
			}
			fieldP = fieldP->next;
		}
		if (groupsFile->noFromFields == groupsFile->groupIds->no) {
			/* if the group id fields is the only fields in the food table, we do
			   not need to use this groups file */
			groupsFile->noFromFields = 0;
		}
	}
}


/* handle foods fields */
void setFromPosFoods() {

	FoodsFile* foodsFile = foodsFiles.first;
	while (foodsFile) {

		/* count how many fields in the foods file are in the food table */
		FieldP* fieldP = foodsFile->fieldPs->first;
		while (fieldP) {
			if (fieldP->field->fromPos) foodsFile->noFromFields++;
			fieldP = fieldP->next;
		}

		/* if it is not the first foods file, and the only field in the food table
		   is the food id, then we do not need to use this foods file */
		if (foodsFile != foodsFiles.first && foodId->fromPos 
			&& foodsFile->noFromFields == 1) 
			foodsFile->noFromFields = 0;
		
		foodsFile = foodsFile->next;
	}
}



/*-------------------------------------------------------------------------------*/
/*--- STEP 2 -*/


/* this is the readFields() function that does STEP 2. it does not do much more
   than call the functions in the sub-steps in the same order as they are defined
   above */
void readFields() {

	nolink(foodFields);
	foodFieldsHash = newHashStr(547);
	initConstants();

	/* sub-step A: check files and get list of fields from the files: */
	setSave();
	setFoods();
	setGroups();
	setRecipeSet1();
	setInput();
	setRecipes();
	if (errors) return;

	/* sub-step B: expand and check list of fields, etc. also set noCalc and groupBy
	   for all fields: */
	setNoCalc();
	setText();
	setNonEdible();
	setGroupBy();
	setSet();
	setCalculate();
	setRecipeSum();
	setRecipeSet2();
	setCook();
	setReduct();
	setRecipeReduct();
	setTranspose();
	setWhere();
	setGroupSet();

	/* sub-step C: set toPos and onlyRecipe for all fields: */
	nolink(outputFields);
	setToPos();
	setToPosTranspose();
	setToPosGroupBy();
	setToPosCook();
	setToPosReduct();
	setToPosRecipeSum();
	setToPosRecipeReduct();
	setToPosWhere();
	setToPosSet();
	setToPosRecipes();
	endlink(outputFields);
	if (verbosity >= 95) {logToPosSet(); logToPosWhere();}
	if (errors) return;

	/* sub-step D: set fromPos for all food table fields. also check for type of fields: */
	nolink(foodTableFields);
	setFromPosOutput();
	setFromPosNonEdible();
	setFromPosCook();
	setFromPosReduct();
	setFromPosRecipeReduct();
	setFromPosGroups(groupsFiles.first);
	setFromPosFoods();
	endlink(foodTableFields);

}




/******************************************************************************/
/*** Utility functions to read lines from the current input file. These functions
     has type int fun(Num*,int,int*,int) */


/* read a line from a data file. no should be number of values in lines. On return
   the line array will contain the values read. Returns 1 if no errors, 0 otherwise */
int readNumLine(Num* line, int no, int* text, int star) {
	while (1) { /* loop until we get a line without errors or reach eof */
		Num* p = line;
		int n = no;
		int* t = text;
		int inStar = 0;
		skipComment(); /* skip any comment lines */
		if (eof()) return(0); /* we reached the end of the file */
		if (star) {
			if (ch == '*') { /* this is a star line */
				getch(); skipSpace(); n = star;
				inStar = 1;
			} else { /* this is not a star line */
				p += star; n -= star; t += star;
			}
		}
		while (1) {
			if (!n--) { /* too many values */
				fileError("Error in list of values");
				skipLine();
				break;
			}
			if (*t++) {
				if (!skipStr2()) {
					fileError("Error in list of values");
					skipLine();
					break;
				}
				*p++ = (Num)0.0;
			} else {
				getNum();
				*p++ = num;
			}
			if (separator == ' ') {
				skipSpace();
				if (eol()) { /* end of line reached */
					if (n) { /* if not all values read we have an error */
						fileError("Error in list of values");
						skipLine();
						break;
					}
					skipEol();
					if (inStar) break; /* we read a star line with no errors */
					else return(1); /* we read a non-star line with no errors */
				}
			} else if (ch == separator) {
				getch(); skipSpace();
			} else {
				skipSpace();
				if (eol()) { /* end of line reached */
					if (n) { /* if not all values read we have an error */
						fileError("Error in list of values");
						skipLine();
						break;
					}
					skipEol();
					if (inStar) break; /* we read a star line with no errors */
					else return(1); /* we read a non-star line with no errors */
				}
				if (ch = separator) {getch(); skipSpace();}
				else {
					fileError("Error in list of values");
					skipLine();
					break;
				}
			}
		}
	}
}


/* read a line from a binary file. Use only this function if sizeof(Num) != sizeof(double).
   Used just like radNumLine(). Before it is called the first time you must allocate an
   array of doubles with length equal to no and assign it to read4buf. */
double* read4buf;
int readNumBinNative4(Num* line, int no, int* dummy1, int dummy2) {
	int read;
	double* b = read4buf;
	if (eof()) return(0);
	read = fread(read4buf,sizeof(double),no,currentFile);
	if (!read) return(0);
	if (read != no) {
		fileError("File ends too soon");
		return(0);
	}
	lineNo++;
	while (no--) *line++ = (Num)*b++;
	return(1);
}


/* read a line from a binary file. Use only this function if sizeof(Num) == sizeof(double).
   Used just like radNumLine(). */
int readNumBinNative(Num* line, int no, int* dummy1, int dummy2) {
	int read;
	if (eof()) return(0);
	read = fread(line,sizeof(double),no,currentFile);
	if (!read) return(0);
	if (read != no) {
		fileError("File ends too soon");
		return(0);
	}
	lineNo++;
	return(1);
}



/*********************************************************************************/
/*** STEP 3 */


/* this functions is STEP 3. It reads the groups files which contributes fields to
   the food table. The groups read are inserted in the hash'es in the GroupsFile
   structures. */
void readGroups() {
	GroupsFile* groupsFile = groupsFiles.first;
	while (groupsFile) { /* the groups files one by one */
		int noFrom = groupsFile->noFromFields;
		if (noFrom) { /* only if it has fields in the food table */
			int noFields = groupsFile->fieldPs->no;
			int noIds = groupsFile->groupIds->no;
			HashInt* hash = newHashIntN(241,noIds);
			Num* line = alloc(noFields*sizeof(Num));/* input buffer */
			int* text = alloc(noFields*sizeof(int));/* text type array */
			Num** ids = alloc(noIds*sizeof(Num*));	/* pointers to id fields in line */
			int* key = alloc(noIds*sizeof(int));	/* key values */
			int star = groupsFile->starFields;		/* no of star fields */
			Num** move = alloc(noFrom*sizeof(Num*));/* pointers to fields in line to move to obs */
			Num* obs = alloc(noFrom*sizeof(Num));	/* obs to insert in hash */
			int groups = 0;							/* no of groups read */

			{	/* build the move and text arrays and find the id fields */
				FieldP* fieldP = groupsFile->fieldPs->first;
				Num* linep = line;
				Num** movep = move;
				int* textp = text;
				while (fieldP) {
					Field* field = fieldP->field;
					if (field->fromPos) {
						int i = 0;
						FieldP* fieldP = groupsFile->groupIds->first;
						while (i < noIds) {
							if (fieldP->field == field) ids[i] = linep;
							i++; fieldP = fieldP->next;
						}
						*movep++ = linep;
					}
					*textp++ = field->text;
					linep++;
					fieldP = fieldP->next;
				}
			}

			/* read the file */
			setCurrent(groupsFile->file);
			while (readNumLine(line,noFields,text,star)) {
				int i = 0;
				int n = noFrom;
				Num* obsp = obs;
				Num** movep = move;

				while (i < noIds) {
					key[i] = (int)*ids[i];
					if (key[i] <= 0)
						error("Bad group id value %d at line %d of file %s.\n",
							key[i],lineNo-1,currentFileName);
					i++;
				}

				while (n--) *obsp++ = **movep++; /* move from line to obs */

				if (lookInsertIntN(hash,key,obs)) {
					error("Dublicate group id at line %d of file %s.\n",
						lineNo-1,currentFileName);
				} else {
					obs = alloc(noFrom*sizeof(Num)); /* new obs to insert in hash */
					groups++;
				}
			}
			closeCurrent();
			groupsFile->hash = hash;

			{	/* log what we did */
				int lineLen = 0;
				FieldP* fieldP = groupsFile->fieldPs->first;
				logmsg("Read groups file %s. Groups: %d. Fields:\n",currentFileName,groups);
				while (fieldP) {
					if (fieldP->field->fromPos) {
						lineLen += strlen(fieldP->field->name)+1;
						if (lineLen > 78) {logmsg("\n"); lineLen = strlen(fieldP->field->name)+1;}
						logmsg("%s ",fieldP->field->name);
					}
					fieldP = fieldP->next;
				}
				logmsg("\n\n");
			}

		} else {
			logmsg("Did not have to read groups file %s.\n\n",groupsFile->file->name);
		}

		groupsFile = groupsFile->next;
	}
}



/*********************************************************************************/
/*** STEP 4 */


/* this function is STEP 4. It reads the foods files which contributes fields to
   the food table. The foods read are inserted in the foodTable hash */
void readFoods() {
	FoodsFile* foodsFile = foodsFiles.first;
	int noTableFields = foodTableFields.no;

	foodTable = newHashInt(3571);

	while (foodsFile) { /* the foods files are read one by one */
		int noFrom = foodsFile->noFromFields;
		if (noFrom) { /* only if it has fields in the food table */
			int noFields = foodsFile->fieldPs->no;
			Num* line = alloc(noFields*sizeof(Num));	/* input buffer */
			int* text = alloc(noFields*sizeof(int));	/* text type array */
			int star = foodsFile->starFields;			/* no of star fields */
			Num* id;									/* ponter to food id in line */
			Num** move = alloc(noTableFields*sizeof(Num*)); /* pointers to fields to move from line to obs */
			Num* obs = alloc(noTableFields*sizeof(Num)); /* obs to insert in foodEntry */
			FoodEntry* foodEntry = allocFoodEntry(simpleFood,obs); /* food entry to insert in foodTable hash */
			int foods = 0;								/* no of foods read */

			{	/* build the move and text arraya and find the id field */
				FieldP* fieldP = foodsFile->fieldPs->first;
				Num** zerop = move;
				int n = noTableFields;
				Num* linep = line;
				int* textp = text;
				while (n--) *zerop++ = NULL;
				while (fieldP) {
					Field* field = fieldP->field;
					if (field->fromPos) {
						move[field->fromPos-1] = linep;
						if (foodId == field) id = linep;
					}
					*textp++ = field->text;
					linep++;
					fieldP = fieldP->next;
				}
			}

			setCurrent(foodsFile->file);
			/* we read the whole file... */
			while (readNumLine(line,noFields,text,star)) {
				/* line read into an array. we try to insert the array into the food table */
				int key = (int)*id;
				int n = noTableFields;
				FoodEntry* foodEntry2;
				Num** movep = move;

				if (key <= 0) {
					error("Bad food id value %d at line %d of file %s.\n",
						key,lineNo,currentFileName);
				} else if (foodEntry2 = lookInsertInt(foodTable,key,foodEntry)) {
					/* found, so not inserted. copying new values to old array */
					Num* obsp = foodEntry2->u.obs;
					while (n--) {
						if (*movep) *obsp = **movep; 
						obsp++; movep++;
					}
					foods++;
				} else {
					/* inserted */
					Num* obsp = obs;
					while (n--) {
						if (*movep) *obsp = **movep; else *obsp = 0.0;
						obsp++; movep++;
					}
					foods++;
					totFoods++;
					obs = alloc(noTableFields*sizeof(Num));
					foodEntry = allocFoodEntry(simpleFood,obs);
				}
			}
			closeCurrent();

			{	/* log what we did */
				int lineLen = 0;
				FieldP* fieldP = foodsFile->fieldPs->first;
				logmsg("Read foods file %s. Foods: %d. Fields:\n",currentFileName,foods);
				while (fieldP) {
					if (fieldP->field->fromPos) {
						lineLen += strlen(fieldP->field->name)+1;
						if (lineLen > 78) {logmsg("\n"); lineLen = strlen(fieldP->field->name)+1;}
						logmsg("%s ",fieldP->field->name);
					}
					fieldP = fieldP->next;
				}
				logmsg("\n\n");
			}

		} else {
			logmsg("Did not have to read foods file %s.\n\n",foodsFile->file->name);
		}

		foodsFile = foodsFile->next;
	}

}



/*********************************************************************************/
/*** STEP 5 */


/* this function is STEP 5. It will expand all groups into the food table */
void expandGroups() {
	GroupsFile* groupsFile = groupsFiles.first;   
	while (groupsFile) { /* the groups file are handled one by one */
		int noFrom = groupsFile->noFromFields;
		if (noFrom) { /* only if it adds fields to the food table */
			HashInt* hash = groupsFile->hash;
			int noIds = groupsFile->groupIds->no;	/* no of id fields */
			int* idPos = alloc(noIds*sizeof(int));	/* position of the group ids in the food table */
			int* idGroupPos = alloc(noIds*sizeof(int));/* position of the group ids in the groups file */
			int* movePos = alloc(noFrom*sizeof(int)); /* array with positions in the food table */
			int* key = alloc(noIds*sizeof(int));

			{	/* set idPos */
				int i = 0;
				FieldP* fieldP = groupsFile->groupIds->first;
				while (i < noIds) {
					idPos[i++] = fieldP->field->fromPos - 1;
					fieldP = fieldP->next;
				}
			}

			{	/* build the move array and set idGroupPos */
				FieldP* fieldP = groupsFile->fieldPs->first;
				int* movePosp = movePos;
				while (fieldP) {
					Field* field = fieldP->field;
					if (field->fromPos) {
						int i = 0;
						FieldP* fieldP = groupsFile->groupIds->first;
						while (i < noIds) {
							if (fieldP->field == field) idGroupPos[i] = movePosp-movePos;
							i++; fieldP = fieldP->next;
						}
						*movePosp++ = field->fromPos - 1;
					}
					fieldP = fieldP->next;
				}
			}

			{ /* we run through the food table and expand the groups for each entry */
				HashIntEntry** p1 = foodTable->table;
				HashIntEntry* p2;
				int n = foodTable->size;
				while (n--) {
					p2 = *p1++;
					while (p2) {
						FoodEntry* foodEntry = p2->value;
						Num* obs = foodEntry->u.obs;
						int okKey = 1;
						int i = 0;
						while (i < noIds) {
							key[i] = (int)obs[idPos[i]];
							if (key[i] <= 0) {
								okKey = 0;
								if (key[i] < 0)
									error("Invalid group id %d used by food %d.\n",key[i],p2->key);
							}
							i++;
						}
						if (okKey) {
							Num* groupObs;
							if (groupObs = lookIntN(hash,key)) {
								int n = noFrom;
								int* movePosp = movePos;
								while (n--) 
									obs[*movePosp++] = *groupObs++;
							} else {
								Num* groupObs = allocarray(noFrom,sizeof(Num));
								int i = 0;
								while (i < noIds) groupObs[idGroupPos[i]] = obs[idPos[i++]];
								insertIntN(hash,key,groupObs);
								warning("Group id used by food %d is not found in groups file %s.\n",
									p2->key[0],groupsFile->file->name);
							}
						}
						p2 = p2->next;
					}
				}
			}

		}
		groupsFile = groupsFile->next;
	}
}



/****************************************************************************/
/*** Utility functions to write lines to file output. These functions has type
     void fun(Num*,int) */


/* write a line to a data file. no should be number of values in lines and the obs
   array should contain the values to write. */
void outputLine(Num* obs, int no) {

	while (no--) {
		Num num = *obs++;
		
		if (num < 0.0001 && num > -0.0001) {
			/* so near zero that we declare it zero */
			putc('0',output);

		} else {
			int e = 0;
			long numl;
			long numl2;
			char buf[9];
			char* p = buf;

			/* output sign */
			if (num < 0.0) {putc('-',output); num = -num;}

			/* we don't print more than max 4 decimals, so we round it */
			num += (Num)0.00005;

			/* if larger than 10**10-1 we print it with exponental notation */
			while (num > 999999999.0) {e++; num /= 10;}

			/* print the integer part */
			numl2 = numl = (long)num;
			if (numl) {
				do {
					*p++ = '0' + (char)(numl%10l);
					numl /= 10l;
				} while (numl);
				while (--p >= buf) putc(*p,output);
			} else {
				putc('0',output);
			}

			/* print the decimals */
			numl = (long)((num-numl2)*10000l);
			if (numl) {
				putc(outputDecPoint,output);
				if (numl > 9999l) numl = 9999l;
				putc('0' + numl/1000l, output); numl %= 1000l;
				if (numl) {
					putc('0' + numl/100l, output); numl %= 100l;
					if (numl) {
						putc('0' + numl/10l, output); numl %= 10l;
						if (numl) {
							putc('0' + numl, output);
				}}}
			}

			/* print exponents */
			if (e) {
				putc('e',output);
				p = buf;
				do {
					*p++ = '0' + e%10;
					e /= 10;
				} while (e);
				while (--p >= buf) putc(*p,output);
			}

		}
		if (no) putc(outputSep,output);
	}
	putc('\n',output);
	if (ferror(output)) {abortAndExit("Error writing to output file\n");}
}


/* write a line to a binary file. Use only this function if sizeof(Num) != sizeof(double).
   Used just like outputLine(). Before it is called the first time you must allocate an
   array of doubles with length equal to no and assign it to output4buf. */
double* output4buf;
void outputBinNative4(Num* obs, int no) {
	double* b = output4buf;
	int n = no;
	while (n--) *b++ = (double)*obs++;
	fwrite(output4buf,sizeof(double),no,output);
	if (ferror(output)) {abortAndExit("Error writing to output file\n");}
}


/* write a line to a binary file. Use only this function if sizeof(Num) == sizeof(double).
   Used just like outputLine(). */
void outputBinNative(Num* obs, int no) {
	fwrite(obs,sizeof(double),no,output);
	if (ferror(output)) {abortAndExit("Error writing to output file\n");}
}



/*********************************************************************************/
/*** FoodCalc() function. This function does the actual food calculations. Before
     it is called you should set all the variables listed below. On return it will
	 have set the variables listed futher down. Note that foodCalc() is used both to
     calculate the output from the input, and to calculate recipes from recipes
     files! */


/*=== all the following variables should be set before foodCalc is called: */
/* you can se these variables as a compiled version of the semantic tree! */

void (*XoutputFun)(Num*,int);/* function to output an obs */
int XnoOutput;			/* no of fields in output array Xobs*/
Num* Xobs;				/* array[XnoOutput] of output values */
int XnoRealOutput;		/* no of fields to acutally output */

int (*XinputFun)(Num*,int,int*,int);/* function to input a line */
void (*Xflush)();		/* hack, see comments in foodCalc() */
int XnoInput;			/* no of fields to input */
int* Xtext;				/* array[XnoInput], is 1 if text field */
int Xstar;				/* no of star fields */
Num* Xline;				/* array[XnoInput] of input values */
int XnoInputMove;		/* no of fields to move unchanged from Xline to Xobs */
Num** XinputMove;		/* array[XnoInputMove] of pointers to fields in Xline to move from */
Num** XoutputInput;		/* array[XnoInputMove] of pointers to fields in Xobs to move to */
int XnoInputGroupBy;	/* no of fields from input to group by */
Num** XinputGroupBy;	/* array[XnoInputGroupBy] of pointers to fields in Xline to group by */
int* XgroupInputPos;	/* array[XnoInputGroupBy] of positions of fields in groupObs to group by */
Num* XinputFood;		/* pointer to the food num field in Xline */
Num* XinputAmount;		/* pointer to the amount field in Xline */
Num XinputAmountScale;	/* scale of amount value */

int XnoInputCook;		/* 1 if cooking should be done */
Num* XinputCook;		/* pointer to the cook field in Xline */

int XnoCookTypes;		/* no of cook types */
typedef struct {		/* Cook reduction: */
	int foodPos;			/* position of reduct field in food table */
	int noOutput;			/* no of fields to reduce */
	Num** output;			/* array[noOutput] of pointers to fields to reduce in Xobs */
} XCook;
typedef struct {		/* Cook type: */
	int no;					/* no of cook reductions */
	XCook* cook;			/* array[no] of cook reductions */
} XCookType;
XCookType* XcookType;	/* array[XnoCookTypes] of cook types, indexed by cook type no */

int XnoNonEdible;		/* 1 if reduction with non-edible par, else 0 */
int XnonEdible;			/* position of field with non-edible fraction in food table */
int XnoNonEdibleFlag;	/* 1 if input field with flag for non-edible reduction */
Num* XnonEdibleFlag;	/* pointer to flag in Xline */

int XnoFoodGroupBy;		/* no of fields from food table or input to group by */
Num** XoutputGroupBy;	/* array[XnoFoodGroupBy] of pointers to fields to group by in food table */
int* XgroupFoodPos;		/* array[XnoFoodGroupBy] of position of fields to group by in groupObs */

int XnoFoodMove;		/* no of fields to move unchanged from food table to Xobs */
int* XfoodMovePos;		/* array[XnoFoodMove] of positions of fields to move from in food table */
Num** XoutputFood;		/* array[XnoFoodMove] of pointers to fields in Xobs to move to */
int XnoFoodNutri;		/* no of fields in food table to calculate from */
int* XfoodNutriPos;		/* array[XnoFoodNutri] of positions of fields to calculate from in food table */
Num** XoutputNutri;		/* array[XnoFoodNutri] of pointers to fields in Xobs to calculate to */

int XnoReduct;			/* no of reductions */
typedef struct {		/* reduction: */
	Num* input;				/* pointer to reduct field in Xline */
	int noOutput;			/* no of fields to reduce */
	Num** output;			/* array[noOutput] of pointers to fields to reduce in Xobs */
} XReduct;
XReduct* Xreduct;		/* array[XnoReduct] of reductions */

int XnoWeightReduct;	/* no of reductions wich are weightReduc */
int XnoCalcWeightReduct;/* 1 if any wightReduc uses the calcField */
typedef struct {		/* weight reduction: */
	Num* input;				/* pointer to reduct field in Xline */
	Num* output;			/* pointer to fraction field in Xobs */
} XWeightReduct;
XWeightReduct* XweightReduct;

int XnoSet;				/* no of calculations */
int XnoSet2;			/* no of calculations to do a second time */
typedef struct {		/* operation */
	Num* output;		/* pointer to field in Xobs to operate on */
	SetOp op;				/* operator */
	union {
		Num* operan;		/* pointer to field in Xobs to use as operands */
		Num num;			/* number to use as operand */
	} u;
} XSet;
XSet* Xset;

int XnoGroupSet;		/* no of group set calculations */
typedef struct {		/* operation */
	int pos;				/* position of field in Xobs to operate on */
	SetOp op;				/* operator */
	union {
		int operan;			/* position of field in Xobs to use as operands */
		Num num;			/* number to use as operand */
	} u;
} XGroupSet;
XGroupSet* XgroupSet;

int XnoSimpleTest;		/* no of simple tests */
typedef struct XSimpleTest_ {
	LexpOp op;				/* operator */
	int pos;				/* position of 1. operand in food table */
	Num num;				/* value of 2. operand */
	struct XSimpleTest_* action; /* continue with this, if the test was true */
} XSimpleTest;
XSimpleTest* XsimpleTest;/* array[XnoSimpleTest] of test */
int XnoTest;				/* no of tests */
typedef struct XTest_ {
	LexpOp op;				/* operator */
	Num* output1;			/* pointer to 1. operand in Xobs */
	Num* output2;			/* pointer to 2. operand in Xobs */
	struct XTest_* action; /* continue with this, if the test was true */
} XTest;
XTest* Xtest;/* array[XnoSimpleTest] of test */

int XnoTranspose;		/* no of transpose on fields */
typedef struct {
	int pos;				/* position of field in food table to transpose on */
	int noGroups;			/* no of groups */
	int groupPos;			/* position of first field in groupObs to transpose to */
	int noOutput;			/* no of fields to transpose */
	Num** output;			/* array[noOutput] of pointers to fields to transpose */
} XTranspose;
XTranspose* Xtranspose;	/* array[XnoTranspose] of transposes */

int XnoBlip;			/* blip value */

/*=== you must set all the above vars before calling foodCalc, */
 
/*=== the vars below will be set by foodCalc */
int noInputLines;		/* no of lines input */
int noOutputObs;		/* no of obs output */


/* the following vars are used by FoodCalc and its utility function */

int groupBy;			/* no of fields to group by */
Num* groupObs;			/* array[XnoOutput] of aggregated fields */
HashInt* groupHash;		/* groups when group by food table field */
HashIntEntry* groupHashFree;/* free hash entries for groupHash */
int noGroupAdd;			/* no of fields to aggregate in group */
int* groupPos;			/* array[noGroupAdd] of positions of fields to aggregate in groupObs */
Num** outputGroup;		/* array[noGroupAdd] of pointers to fields in Xobs to aggregate */
int blip;				/* next blip when this number of lines input */
XSimpleTest* simpleUse;	/* if xsimpleTest->action == simpleUse then use this obs */
XTest* use;				/* if xsimpleTest->action == use then use this obs */


void foodCalcGroupOutput(Num* groupObs) {

	if (XnoGroupSet) {
		int n = XnoGroupSet;
		XGroupSet* set = XgroupSet;
		while (n--) {
			switch (set->op) {
			case cpyOp: groupObs[set->pos] = groupObs[set->u.operan]; break;
			case addOp: groupObs[set->pos] += groupObs[set->u.operan]; break;
			case subOp: groupObs[set->pos] -= groupObs[set->u.operan]; break;
			case mulOp: groupObs[set->pos] *= groupObs[set->u.operan]; break;
			case divOp: if (groupObs[set->u.operan]) groupObs[set->pos] /= groupObs[set->u.operan]; 
						else groupObs[set->pos] = (Num)0;
						break;
			case cpyOpC: groupObs[set->pos] = set->u.num; break;
			case addOpC: groupObs[set->pos] += set->u.num; break;
			case subOpC: groupObs[set->pos] -= set->u.num; break;
			case mulOpC: groupObs[set->pos] *= set->u.num; break;
			case divOpC: groupObs[set->pos] /= set->u.num; break;
			}
			set++;
		}
	}

	XoutputFun(groupObs,XnoRealOutput);
	noOutputObs++;
}

/* this utility function is called by foodCalcFood() and foodCalc() when group by: is
   used and a group is finished and should be output */
void foodCalcGroupFlush() {
	if (noInputLines > 1) {
		/* only if we read something should we output anything */
		if (XnoFoodGroupBy) { 
			/* we group by a food table field, so we have to output all groupObs in
			   the groupHash */
			int n = groupHash->size;
			HashIntEntry** p1 = groupHash->table;
			while (n--) {
				if (*p1) {
					HashIntEntry* p2 = *p1;
					while (p2) {
						HashIntEntry* e = p2;
						Num* groupObs = e->value;
						foodCalcGroupOutput(groupObs);
						p2 = p2->next;
						e->next = groupHashFree;
						groupHashFree = e;
					}
					*p1 = NULL;
				}
				p1++;
			}
		} else {
			/* we only group by input fields, so we just output the groupObs */
			foodCalcGroupOutput(groupObs);
		}
	}
	if (XnoInputGroupBy) {
		if (!XnoFoodGroupBy) {
			/* initialize all groupObs fields to zero */
			int n = XnoOutput;
			Num* pgroupObs = groupObs;
			while (n--) *pgroupObs++ = 0.0;
		}
		{ /* initialize the groupObs with the input group by fields */
			int n = XnoInputGroupBy;
			Num** pinput = XinputGroupBy;
			int* pgroupPos = XgroupInputPos;
			while (n--) groupObs[*pgroupPos++] = **pinput++;
		}
	}
}


/* this utility function is called by foodCalc() to calculate an ingredients or a
   simple food */
void foodCalcFood(Num* foodObs, FoodType foodType) {

	Num amount = *XinputAmount;

	if (XnoSimpleTest) {
		XSimpleTest* test = XsimpleTest;
		while (test < simpleUse) {
			switch (test->op) {
			case eqOp: if (foodObs[test->pos] == test->num) test = test->action; else test++; break;
			case neOp: if (foodObs[test->pos] != test->num) test = test->action; else test++; break;
			case gtOp: if (foodObs[test->pos] > test->num) test = test->action; else test++; break;
			case geOp: if (foodObs[test->pos] >= test->num) test = test->action; else test++; break;
			case ltOp: if (foodObs[test->pos] < test->num) test = test->action; else test++; break;
			case leOp: if (foodObs[test->pos] <= test->num) test = test->action; else test++; break;
			}
		}
		if (test > simpleUse) return; /* skip! */
	}

	if (XnoInputMove) {
		/* move all fields from line to obs */
		int n = XnoInputMove;
		Num** pinput = XinputMove;
		Num** poutput = XoutputInput;
		while (n--) **poutput++ = **pinput++;
	}

	if (XnoFoodMove) {
		/* move all noCalc fields from foodObs to obs */
		int n = XnoFoodMove;
		int* pfoodPos = XfoodMovePos;
		Num** poutput = XoutputFood;
		while (n--) **poutput++ = foodObs[*pfoodPos++];
	}

	if (XnoNonEdible) {
		if (foodType == simpleFood && (!XnoNonEdibleFlag || *XnonEdibleFlag))
			amount *= (Num)1.0 - foodObs[XnonEdible];
	}

	if (XnoFoodNutri) {
		/* calculate all nutrient fields from foodObs to obs */
		int n = XnoFoodNutri;
		int* pfoodPos = XfoodNutriPos;
		Num** poutput = XoutputNutri;
		while (n--) 
			**poutput++ = amount*XinputAmountScale*foodObs[*pfoodPos++];
	}

	if (XnoWeightReduct) {
		if (XnoCalcWeightReduct) {
			/* calculate new fields - will be recalculated after reductions */
			int n = XnoSet;
			XSet* set = Xset;
			while (n--) {
				switch (set->op) {
				case cpyOp: *(set->output) = *(set->u.operan); break;
				case addOp: *(set->output) += *(set->u.operan); break;
				case subOp: *(set->output) -= *(set->u.operan); break;
				case mulOp: *(set->output) *= *(set->u.operan); break;
				case divOp: if (*(set->u.operan)) *(set->output) /= *(set->u.operan); 
							else *(set->output) = (Num)0;
							break;
				case cpyOpC: *(set->output) = set->u.num; break;
				case addOpC: *(set->output) += set->u.num; break;
				case subOpC: *(set->output) -= set->u.num; break;
				case mulOpC: *(set->output) *= set->u.num; break;
				case divOpC: *(set->output) /= set->u.num; break;
				}
				set++;
			}
		}
		{ /* change fractions */
			int n = XnoWeightReduct;
			XWeightReduct* weightReduct = XweightReduct;
			while (n--) {
				if (*(weightReduct->output) == (Num)0.0) 
					*(weightReduct->input) = (Num)0.0;
				else
					*(weightReduct->input) = 
						amount * *(weightReduct->input) / *(weightReduct->output);
				weightReduct++;
			}
		}
	}

	if (XnoInputCook) {
		/* do reductions by cooking */
		int cookId = (int)*XinputCook;
		if (cookId) {
			if (foodType != simpleFood) {
				error("You can not cook a recipe at line %d in %s.\n",
					lineNo,currentFileName);
			} else if (cookId < 0 || cookId > XnoCookTypes) {
				error("Cook id %d not defined at line %d in %s.\n",
					cookId,lineNo,currentFileName);
			} else {
				XCookType* cookType = XcookType+cookId-1;
				int n = cookType->no;
				XCook* cook = cookType->cook;
				while (n--) {
					if (foodObs[cook->foodPos] != (Num)0.0) {
						Num factor = (Num)1.0-foodObs[cook->foodPos];
						int n = cook->noOutput;
						Num** poutput = cook->output;
						while (n--) **poutput++ *= factor;
					}
					cook++;
				}
			}
		}
	}

	if (XnoReduct) {
		/* do reductions by input fields */
		int n = XnoReduct;
		XReduct* reduct = Xreduct;
		while (n--) {
			if (*(reduct->input) != (Num)0.0) {
				Num factor = (Num)1.0-*(reduct->input);
				int n = reduct->noOutput;
				Num** poutput = reduct->output;
				while (n--) **poutput++ *= factor;
			}
			reduct++;
		}
	}

	if (XnoSet) {
		/* calculate new fields */
		int n = ((XnoSet2 && foodType != simpleFood)? XnoSet2 : XnoSet);
			/* calculations after XnoSet2 are recipe set: calculations; we only
			   do these for simple foods! */
		XSet* set = Xset;
		while (n--) {
			switch (set->op) {
			case cpyOp: *(set->output) = *(set->u.operan); break;
			case addOp: *(set->output) += *(set->u.operan); break;
			case subOp: *(set->output) -= *(set->u.operan); break;
			case mulOp: *(set->output) *= *(set->u.operan); break;
			case divOp: if (*(set->u.operan)) *(set->output) /= *(set->u.operan); 
						else *(set->output) = (Num)0;
						break;
			case cpyOpC: *(set->output) = set->u.num; break;
			case addOpC: *(set->output) += set->u.num; break;
			case subOpC: *(set->output) -= set->u.num; break;
			case mulOpC: *(set->output) *= set->u.num; break;
			case divOpC: *(set->output) /= set->u.num; break;
			}
			set++;
		}
		if (XnoSet2) {
			int n = XnoSet2;
			XSet* set = Xset;
			while (n--) {
				switch (set->op) {
				case cpyOp: *(set->output) = *(set->u.operan); break;
				case addOp: *(set->output) += *(set->u.operan); break;
				case subOp: *(set->output) -= *(set->u.operan); break;
				case mulOp: *(set->output) *= *(set->u.operan); break;
				case divOp: if (*(set->u.operan)) *(set->output) /= *(set->u.operan); 
							else *(set->output) = (Num)0;
							break;
				case cpyOpC: *(set->output) = set->u.num; break;
				case addOpC: *(set->output) += set->u.num; break;
				case subOpC: *(set->output) -= set->u.num; break;
				case mulOpC: *(set->output) *= set->u.num; break;
				case divOpC: *(set->output) /= set->u.num; break;
				}
				set++;
			}
		}
	}

	if (XnoTest) {
		XTest* test = Xtest;
		while (test < use) {
			switch (test->op) {
			case eqOp: if (*(test->output1) == *(test->output2)) test = test->action; else test++; break;
			case neOp: if (*(test->output1) != *(test->output2)) test = test->action; else test++; break;
			case gtOp: if (*(test->output1) > *(test->output2)) test = test->action; else test++; break;
			case geOp: if (*(test->output1) >= *(test->output2)) test = test->action; else test++; break;
			case ltOp: if (*(test->output1) < *(test->output2)) test = test->action; else test++; break;
			case leOp: if (*(test->output1) <= *(test->output2)) test = test->action; else test++; break;
			}
		}
		if (test > use) return; /* skip! */
	}

	if (!groupBy) {
		/* output the obs */
		XoutputFun(Xobs,XnoRealOutput);
		noOutputObs++;
	} else {

		if (XnoFoodGroupBy) {
			/* we group by a food table fields. if this field has a value wich is not
			   already in the groupHash we make a new initialized enty in the hash. We
			   set groupObs to the found/new group from the groupHash */
			if (XnoFoodGroupBy == 1) {

				int key = (int)**XoutputGroupBy;
				int hkey = key % groupHash->size;
				HashIntEntry* entry;
				HashIntEntry* newEntry = NULL;
				if (entry = groupHash->table[hkey]) {
					while (1) {
						if (entry->key[0] == key) {
							/* found entry */
							groupObs = entry->value;
							break;
						}
						if (!entry->next) {
							/* entry not found */
							if (groupHashFree) {
								entry->next = newEntry = groupHashFree;
								groupHashFree = groupHashFree->next;
							} else {
								entry->next = newEntry = allocStruct(HashIntEntry);
								newEntry->value = alloc(XnoOutput*sizeof(Num));
							}
							break;
						}
						entry = entry->next;
					}
				} else {
					/* entry not found */
					if (groupHashFree) {
						groupHash->table[hkey] = newEntry = groupHashFree;
						groupHashFree = groupHashFree->next;
					} else {
						groupHash->table[hkey] = newEntry = allocStruct(HashIntEntry);
						newEntry->value = alloc(XnoOutput*sizeof(Num));
					}
				}
				if (newEntry) {
					/* we did not find the entry, so we make a new one */
					newEntry->key[0] = key;
					newEntry->next = NULL;
					groupObs = newEntry->value;
					{ /* initialize all groupObs fields to zero */
						int n = XnoOutput;
						Num* pgroupObs = groupObs;
						while (n--) *pgroupObs++ = 0.0;
					}
					groupObs[*XgroupFoodPos] = **XoutputGroupBy;
					if (XnoInputGroupBy) {
						/* initialize the groupObs with the input group by fields */
						int n = XnoInputGroupBy;
						Num** pinput = XinputGroupBy;
						int* pgroupPos = XgroupInputPos;
						while (n--) groupObs[*pgroupPos++] = **pinput++;
					}
				}

			} else { /*(XnoFoodGroupBy > 1)*/

				int key[20];
				int hkey = 0;
				HashIntEntry* entry;
				HashIntEntry* newEntry = NULL;

				{ /* set key og hkey */
					int n = XnoFoodGroupBy;
					int* keyp = &(key[0]);
					Num** output = XoutputGroupBy;
					while (n--) hkey = (hkey << 10) + (*keyp++ = (int)**output++);
					hkey %= groupHash->size;
				}

				if (entry = groupHash->table[hkey]) {
					while (1) {
						int eq = 1;
						int n = XnoFoodGroupBy;
						int* keyp = &(key[0]);
						int* ekeyp = &(entry->key[0]);
						while (n--) if (*keyp++ != *ekeyp++) {eq = 0; break;}
						if (eq) {
							/* found entry */
							groupObs = entry->value;
							break;
						}
						if (!entry->next) {
							/* entry not found */
							if (groupHashFree) {
								entry->next = newEntry = groupHashFree;
								groupHashFree = groupHashFree->next;
							} else {
								entry->next = newEntry = alloc(groupHash->entrySize);
								newEntry->value = alloc(XnoOutput*sizeof(Num));
							}
							break;
						}
						entry = entry->next;
					}
				} else {
					/* entry not found */
					if (groupHashFree) {
						groupHash->table[hkey] = newEntry = groupHashFree;
						groupHashFree = groupHashFree->next;
					} else {
						groupHash->table[hkey] = newEntry = alloc(groupHash->entrySize);
						newEntry->value = alloc(XnoOutput*sizeof(Num));
					}
				}
				if (newEntry) {
					/* we did not find the entry, so we make a new one */
					{ /* set key */
						int n = XnoFoodGroupBy;
						int* keyp = &(key[0]);
						int* ekeyp = &(newEntry->key[0]);
						while (n--) {
							*ekeyp++ = *keyp++;
						}
					}
					newEntry->next = NULL;
					groupObs = newEntry->value;
					{ /* initialize all groupObs fields to zero */
						int n = XnoOutput;
						Num* pgroupObs = groupObs;
						while (n--) *pgroupObs++ = 0.0;
					}
					{ /* initialize the groupObs with the food group by fields */
						int n = XnoFoodGroupBy;
						Num** output = XoutputGroupBy;
						int* pos = XgroupFoodPos;
						while (n--) groupObs[*pos++] = **output++;
					}
					if (XnoInputGroupBy) {
						/* initialize the groupObs with the input group by fields */
						int n = XnoInputGroupBy;
						Num** pinput = XinputGroupBy;
						int* pgroupPos = XgroupInputPos;
						while (n--) groupObs[*pgroupPos++] = **pinput++;
					}
				}
			}
		}

		{ /* add the obs to the groupObs */
			int n = noGroupAdd;
			int* pgroupPos = groupPos;
			Num** poutput = outputGroup;
			while (n--) 
				groupObs[*pgroupPos++] += **poutput++;
		}
		if (XnoTranspose) {
			int n = XnoTranspose;
			XTranspose* transpose = Xtranspose;
			while (n--) {
				int key = (int)foodObs[transpose->pos];
				if (key > 0 && key <= transpose->noGroups) {
					int groupPos = transpose->groupPos - 1;
					int n = transpose->noOutput;
					Num** poutput = transpose->output;
					while (n--) {
						groupObs[groupPos+key] += **poutput++;
						groupPos += transpose->noGroups;
					}
				}
				transpose++;
			}
		}
	}
}


/* this is the foodCalc() function - see comments above! */
void foodCalc() {

	/* initialize variables */
	noInputLines = 0;
	noOutputObs = 0;
	groupBy = XnoInputGroupBy+XnoFoodGroupBy;
	blip = XnoBlip;
	simpleUse = XsimpleTest+XnoSimpleTest;
	use = Xtest+XnoTest;

	if (groupBy) {
		/* initialize group by work variables */
		int n = XnoOutput;
		Num* pgroupObs = groupObs = alloc(XnoOutput*sizeof(Num));
		while (n--) *pgroupObs++ = 0.0;
		if (XnoFoodGroupBy) {
			if (XnoFoodGroupBy == 1) groupHash = newHashInt(241);
			else groupHash = newHashIntN(241,XnoFoodGroupBy);
			groupHashFree = NULL;
		}
		{	/* count group add positions */
			XSet* set = Xset;
			int n = XnoSet;
			noGroupAdd = XnoFoodNutri;
			while (n--) {
				if (set->op == cpyOp || set->op == cpyOpC) noGroupAdd++;
				set++;
			}
		}
		{	/* set group add positions */
			int* pos = groupPos = alloc(noGroupAdd*sizeof(int));
			Num** output = outputGroup = alloc(noGroupAdd*sizeof(Num*));
			Num** xoutputNutri = XoutputNutri;
			XSet* set = Xset;
			int n = XnoFoodNutri;
			while (n--) {
				*pos++ = *xoutputNutri - Xobs;
				*output++ = *xoutputNutri++;
			}
			n = XnoSet;
			while (n--) {
				if (set->op == cpyOp || set->op == cpyOpC) {
					*pos++ = set->output - Xobs;
					*output++ = set->output;
				}
				set++;
			}
		}
	}

	while (XinputFun(Xline,XnoInput,Xtext,Xstar)) {
		FoodEntry* foodEntry;
		
		if (++noInputLines == blip) {
			fprintf(stderr," %d\r",blip);
			blip += XnoBlip;
		}

		if (XnoInputGroupBy) {
			/* check if we have reached a new group. if we have, we call 
			   foodCalcGroupFlush() to output the current group */
			int n = XnoInputGroupBy;
			Num** pinput = XinputGroupBy;
			int* pgroupPos = XgroupInputPos;
			while (n--) {
				if (**pinput != groupObs[*pgroupPos]) {
					if (**pinput < groupObs[*pgroupPos])
						abortAndExit("File %s not sorted on the group by fields.\n",currentFileName);
					foodCalcGroupFlush();
					break;
				}
				pinput++; pgroupPos++;
			}
		}

		/* find the food in the table */
		if (!(foodEntry = lookInt(foodTable,(int)*XinputFood))) {
			/* food not found */
			/* if Xflush is not NULL we call it and the we try to look for the
			   food again. This is used when we read a recipe file and
			   keepIngredients is 1. A realy ugly hack! */
			if (!Xflush || 
				!((*Xflush)(), (foodEntry = lookInt(foodTable,(int)*XinputFood)))) {
				error("Food id %d not found in food table at line %d in %s.\n",
					(int)*XinputFood,lineNo,currentFileName);
				continue;
			}
		}

		/* food found */
		if (foodEntry->foodType == expandedRecipe) {
			RecipeEntry* recipeEntry = foodEntry->u.recipe;
			while (recipeEntry) {
				foodCalcFood(recipeEntry->obs,foodEntry->foodType);
				recipeEntry = recipeEntry->next;
			}
		} else {
			foodCalcFood(foodEntry->u.obs,foodEntry->foodType);
		}
	}

	if (groupBy && noInputLines) foodCalcGroupFlush();
}



/************************************************************************************/
/*** these utility functions are used to set set the foodCalc() variables before
     calling foodCalc() */


/* utility function to count number of toPos fields in a list */
int countToPos(FieldP* fieldP, int recipe) {
	int count = 0;
	while (fieldP) {
		Field* field = fieldP->field;
		if (field->toPos && field->onlyRecipe <= recipe) count++;
		fieldP = fieldP->next;
	}
	return(count);
}


/* this functions should be call once before all recipes files and then once before
   the input file. it sets variables which are mostly the same for recipes files and
   the input file, and which do not change between recipes files. */
void setFoodCalcPos(int recipe) {

	XnoOutput = outputFields.no;
	XnoRealOutput = (recipe?XnoOutput:noRealOutputFields);
	Xobs = alloc(XnoOutput*sizeof(Num));

	{ /* food fields */
		{	/* count fields */
			FieldP* fieldP = foodTableFields.first;
			XnoFoodNutri = XnoFoodMove = 0;
			while (fieldP) {
				Field* field = fieldP->field;
				if (field->toPos && field->onlyRecipe <= recipe) {
					if (!field->noCalc) XnoFoodNutri++;
					else XnoFoodMove++;
				}
				fieldP = fieldP->next;
			}
		}
		{	/* set positions */
			FieldP* fieldP = foodTableFields.first;
			int* movePos = XfoodMovePos = alloc(XnoFoodMove*sizeof(int));
			Num** moveOutput = XoutputFood = alloc(XnoFoodMove*sizeof(Num*));
			int* nutriPos = XfoodNutriPos = alloc(XnoFoodNutri*sizeof(int));
			Num** nutriOutput = XoutputNutri = alloc(XnoFoodNutri*sizeof(Num*));
			while (fieldP) {
				Field* field = fieldP->field;
				if (field->toPos && field->onlyRecipe <= recipe) {
					if (!field->noCalc) {
						*nutriPos++ = field->fromPos - 1;
						*nutriOutput++ = Xobs + field->toPos - 1;
					} else {
						*movePos++ = field->fromPos - 1;
						*moveOutput++ = Xobs + field->toPos - 1;
					}
				}
				fieldP = fieldP->next;
			}
		}
	}

	{	/* cook types */
		CookType* cookType = cookTypes.first;
		XCookType* xcookType = XcookType = alloc(cookTypes.no*sizeof(XCookType));
		XnoCookTypes = cookTypes.no;
		while (cookType) {
			{ /* count used */
				Cook* cook = cookType->cooks->first;
				xcookType->no = 0;
				while (cook) {
					if (cook->used = countToPos(cook->fields.first,recipe))
						xcookType->no++;
					cook = cook->next;
				}
			}
			{ /* set positions */
				Cook* cook = cookType->cooks->first;
				XCook* xcook = xcookType->cook = alloc(xcookType->no*sizeof(XCook));
				while (cook) {
					if (cook->used) {
						FieldP* fieldP = cook->fields.first;
						Num** output = xcook->output = alloc(cook->used*sizeof(Num*));
						xcook->foodPos = cook->field->fromPos - 1;
						xcook->noOutput = cook->used;
						while (fieldP) {
							Field* field = fieldP->field;
							if (field->toPos && field->onlyRecipe <= recipe) 
								*output++ = Xobs + fieldP->field->toPos - 1;
							fieldP = fieldP->next;
						}
						xcook++;
					}
					cook = cook->next;
				}
			}
			xcookType++;
			cookType = cookType->next;
		}
	}

	{	/* calculates */
		{ /* count used */
			XnoSet = XnoSet2 = XnoGroupSet = 0;
			{ /* operations in set: commands */
				Set* set = sets.first;
				while (set) {
					Field* field = set->field;
					if (field->toPos && field->onlyRecipe <= recipe) XnoSet += set->opers.no;
					set = set->next;
				}
			}
			if (recipe) { /* operations in recipe set: commands */
				Set* set = recipeSets.first;
				XnoSet2 = XnoSet;
				while (set) {
					Field* field = set->field;
					if (field->toPos) XnoSet += set->opers.no;
					set = set->next;
				}
			} else { /* operations in group set: commands */
				Set* set = groupSets.first;
				int* no = (groupByCmd? &XnoGroupSet : &XnoSet);
				while (set) {
					Field* field = set->field;
					if (field->toPos) *no += set->opers.no;
					set = set->next;
				}
			}
		}
		{ /* set positions */
			XSet* xset = Xset = alloc(XnoSet*sizeof(XSet));
			{ /* operations in set: commands */
				Set* set = sets.first;
				while (set) {
					Field* setField = set->field;
					if (setField->toPos && setField->onlyRecipe <= recipe) {
						SetOper* oper = set->opers.first;
						while (oper) {
							xset->output = Xobs + setField->toPos - 1;
							if (oper->field->noCalc == 9/*constant*/) {
								xset->op = oper->op+5;
								xset->u.num = *(Num*)oper->field->next;
							} else {
								xset->op = oper->op;
								xset->u.operan = Xobs + oper->field->toPos - 1;
							}
							xset++;
							oper = oper->next;
						}
					}
					set = set->next;
				}
			}
			if (recipe) { /* operations in recipe set: commands */
				Set* set = recipeSets.first;
				while (set) {
					Field* setField = set->field;
					if (setField->toPos) {
						SetOper* oper = set->opers.first;
						while (oper) {
							xset->output = Xobs + setField->toPos - 1;
							if (oper->field->noCalc == 9/*constant*/) {
								xset->op = oper->op+5;
								xset->u.num = *(Num*)oper->field->next;
							} else {
								xset->op = oper->op;
								xset->u.operan = Xobs + oper->field->toPos - 1;
							}
							xset++;
							oper = oper->next;
						}
					}
					set = set->next;
				}
			} else { /* operations in group set: commands */
				if (!groupByCmd) {
					Set* set = groupSets.first;
					while (set) {
						Field* setField = set->field;
						if (setField->toPos) {
							SetOper* oper = set->opers.first;
							while (oper) {
								xset->output = Xobs + setField->toPos - 1;
								if (oper->field->noCalc == 9/*constant*/) {
									xset->op = oper->op+5;
									xset->u.num = *(Num*)oper->field->next;
								} else {
									xset->op = oper->op;
									xset->u.operan = Xobs + oper->field->toPos - 1;
								}
								xset++;
								oper = oper->next;
							}
						}
						set = set->next;
					}
				} else { /*(groupByCmd)*/
					Set* set = groupSets.first;
					XGroupSet* xset = XgroupSet = alloc(XnoGroupSet*sizeof(XGroupSet));
					while (set) {
						Field* setField = set->field;
						if (setField->toPos) {
							SetOper* oper = set->opers.first;
							while (oper) {
								xset->pos = setField->toPos - 1;
								if (oper->field->noCalc == 9/*constant*/) {
									xset->op = oper->op+5;
									xset->u.num = *(Num*)oper->field->next;
								} else {
									xset->op = oper->op;
									xset->u.operan = oper->field->toPos - 1;
								}
								xset++;
								oper = oper->next;
							}
						}
						set = set->next;
					}
				}
			}
		}
	}
}


/* call this function before each recipe file and before the input file. it sets
   variables which depents on the file, but are set the same way for recipes fiels
   and the input file */
void setFilePos(HashStr* fieldsHash, FieldChain* fields, char* fileName, int recipe) { 

	XnoInput = fields->no;
	Xline = alloc(XnoInput*sizeof(Num));
	Xtext = alloc(XnoInput*sizeof(int));

	{ /* non-edible field */
		if (nonEdibleField) {
			Field* field;
			if (nonEdibleFlagName && !(field = lookStr(fieldsHash,nonEdibleFlagName))) {
				warning("Non-edible field '%s' not found in file %s.\n",
					nonEdibleFlagName,fileName);
				XnoNonEdible = 0;
			} else {
				XnoNonEdible = 1;
				XnonEdible = nonEdibleField->fromPos - 1;
				if (nonEdibleFlagName) {
					XnoNonEdibleFlag = 1;
					XnonEdibleFlag = Xline + field->fromPos - 1;
				} else {
					XnoNonEdibleFlag = 0;
				}
			}
		} else {
			XnoNonEdible = 0;
		}
	}

	{ /* input positions */
		{ /* count fields and set the text array */
			Field* field = fields->first;
			int* text = Xtext;
			XnoInputMove = 0;
			while (field) {
				if (field->toPos) XnoInputMove++;
				*text++ = field->text;
				field = field->next;
			}
		}
		{ /* set positions */
			Field* field = fields->first;
			Num** input = XinputMove = alloc(XnoInputMove*sizeof(Num*));
			Num** output = XoutputInput = alloc(XnoInputMove*sizeof(Num*));
			while (field) {
				if (field->toPos) {
					*input++ = Xline + field->fromPos - 1;
					*output++ = Xobs + field->toPos - 1;
				}
				field = field->next;
			}
		}
	}

	{ /* cook position */
		if (cookTypes.no) {
			Field* field;
			if (!(field = lookStr(fieldsHash,cookFieldName))) {
				warning("Cook field '%s' not found in file %s.\n",cookFieldName,fileName);
				XnoInputCook = 0;
			} else if (field->text) {
				error("Cook field '%s' must not be a text field in file %s.\n",cookFieldName,fileName);
				XnoInputCook = 0;
			} else {
				XnoInputCook = 1;
				XinputCook = Xline + field->fromPos - 1;
			}
		} else {
			XnoInputCook = 0;
		}
	}

	{ /* reduct positions */
		Reduct* reduct = reducts.first;
		XReduct* xreduct;
		XWeightReduct* xweightReduct;
		XnoReduct = 0;
		XnoWeightReduct = 0;
		XnoCalcWeightReduct = 0;
		while (reduct) { /* first we only count the used reducts */
			if (reduct->used = countToPos(reduct->fields.first,recipe)) {
				if (!(reduct->field = lookStr(fieldsHash,reduct->fieldName))) {
					warning("Reduce field '%s' not found in file %s.\n",reduct->fieldName,fileName);
					reduct->used = 0;
				} else if (reduct->field->text) {
					error("Reduce field '%s' must not be a text field in file %s.\n",reduct->fieldName,fileName);
					reduct->used = 0;
				} else {
					XnoReduct++;
					if (reduct->type == weightReduc) {
						XnoWeightReduct++;
						if (reduct->calcField) XnoCalcWeightReduct++;
					}
				}
			}
			reduct = reduct->next;
		}
		xreduct = Xreduct = alloc(XnoReduct*sizeof(XReduct));
		xweightReduct = XweightReduct = alloc(XnoWeightReduct*sizeof(XWeightReduct));
		reduct = reducts.first;
		while (reduct) { /* then we set the positions */
			if (reduct->used) {
				FieldP* fieldP = reduct->fields.first;
				Num** output = xreduct->output = alloc(reduct->used*sizeof(Num*));
				xreduct->input = Xline + reduct->field->fromPos - 1;
				xreduct->noOutput = reduct->used;
				while (fieldP) {
					Field* field = fieldP->field;
					if (field->toPos && field->onlyRecipe <= recipe)
						*output++ =  Xobs + fieldP->field->toPos - 1;
					fieldP = fieldP->next;
				}
				if (reduct->type == weightReduc) {
					Field* field;
					if (reduct->calcField) field = reduct->calcField;
					else field = reduct->fields.first->field;
					xweightReduct->input = Xline + reduct->field->fromPos - 1;
					xweightReduct->output = Xobs + field->toPos - 1;
					xweightReduct++;
				}
				xreduct++;
			}
			reduct = reduct->next;
		}
	}
}


/* call this function before each recipes files. it sets variables specially for
   recipes files */
void setRecipesPos(RecipesFile* recipesFile) {

	setFilePos(recipesFile->fieldsHash,recipesFile->fields,recipesFile->file->name,1);
	XnoBlip = 0;
	Xstar = recipesFile->starFields;

	{ /* input positions */
		XinputFood = Xline + recipesFile->foodId->fromPos - 1;
		XinputAmount = Xline + recipesFile->amountField->fromPos - 1;
		XinputAmountScale = (Num)(1.0/recipeSum);
	}

	{ /* no group by or if or transpose */
		XnoInputGroupBy = 0;
		XnoFoodGroupBy = 0;
		XnoSimpleTest = 0;
		XnoTest = 0;
		XnoTranspose = 0;
	}
}


/* call this function before the input file is read. it sets variables specially
   for the input file */
void setInputPos() {

	setFilePos(inputFieldsHash,&inputFields,inputFileName,0);
	if (blipCmd) XnoBlip = atoi(*(blipCmd->args)); else XnoBlip = 0;
	Xstar = inputStarFields;

	{ /* input positions */
		XinputFood = Xline + inputFoodField->fromPos - 1;
		XinputAmount = Xline + inputAmountField->fromPos - 1;
		XinputAmountScale = inputAmountScale;
	}

	{ /* group by positions */
		Num** input = XinputGroupBy = alloc(groupByFields.no*sizeof(Num*));
		int* group = XgroupInputPos = alloc(groupByFields.no*sizeof(int));
		Num** output = XoutputGroupBy = alloc(groupByFoodFields.no*sizeof(Num*));
		int* fgroup = XgroupFoodPos = alloc(groupByFoodFields.no*sizeof(int));
		FieldP* fieldP = groupByFields.first;
		XnoInputGroupBy = groupByFields.no;
		XnoFoodGroupBy = groupByFoodFields.no;
		while (fieldP) {
			Field* field = fieldP->field;
			*input++ = Xline + field->fromPos - 1;
			*group++ = field->toPos - 1;
			fieldP = fieldP->next;
		}
		fieldP = groupByFoodFields.first;
		while (fieldP) {
			Field* field = fieldP->field;
			*output++ = Xobs + field->toPos - 1;
			*fgroup++ = field->toPos - 1;
			fieldP = fieldP->next;
		}
	}

	if (simpleTest) {
		XSimpleTest* xtest = XsimpleTest = alloc(tests.no*sizeof(XSimpleTest));
		Test* test = tests.first;
		XnoSimpleTest = tests.no;
		while (test) {
			xtest->op = test->op;
			xtest->pos = test->field1->fromPos - 1;
			xtest->num = *(Num*)(test->field2->next);
			xtest->action = XsimpleTest + test->action;
			test = test->next;
			xtest++;
		}
	} else {
		XTest* xtest = Xtest = alloc(tests.no*sizeof(XTest));
		Test* test = tests.first;
		XnoTest = tests.no;
		while (test) {
			xtest->op = test->op;
			xtest->output1 = Xobs + test->field1->toPos - 1;
			xtest->output2 = Xobs + test->field2->toPos - 1;
			xtest->action = Xtest + test->action;
			test = test->next;
			xtest++;
		}
	}

	{ /* transpose positions */
		XTranspose* xtranspose = Xtranspose = alloc(transposes.no*sizeof(XTranspose));
		Transpose* transpose = transposes.first;
		XnoTranspose = transposes.no;
		while (transpose) {
			Num** poutput = xtranspose->output = alloc(transpose->fields.no*sizeof(Num*));
			FieldP* fieldP = transpose->fields.first;
			xtranspose->noOutput = transpose->fields.no;
			xtranspose->pos = transpose->groupField->fromPos - 1;
			xtranspose->noGroups = transpose->noGroups;
			xtranspose->groupPos = transpose->transFields.first->field->toPos - 1;
			while (fieldP) {
				*poutput++ = Xobs + fieldP->field->toPos - 1;
				fieldP = fieldP->next;
			}
			xtranspose++;
			transpose = transpose->next;
		}
	}
}



/***********************************************************************************/
/*** STEP 5.5 */

/* Calculate cook fractions for all weight cook commands. after this change, all weight
   cook: commands are just like cook: commands */
void weightCookChange() {
	if (weightCookCmd) {

		int XnoWeight = 0; /* number of used weight cook: commands */
		typedef struct {
			int foodPos;	/* position of reduct field in food table */
			Num* output;	/* pointer to fraction field in Xobs */
		} XWeight;
		XWeight* Xweight;


		/* we use the foodCalc variables (the compiled semantic tree) */
		setFoodCalcPos(1);

		{ /* count weight */
			CookType* cookType = cookTypes.first;
			while (cookType) {
				Cook* cook = cookType->cooks->first;
				while (cook) {
					if (cook->type == weightReduc && cook->used) XnoWeight++;
					cook = cook->next;
				}
				cookType = cookType->next;
			}
		}
		{ /* make Xweight */
			CookType* cookType = cookTypes.first;
			XWeight* weight = Xweight = alloc(XnoWeight*sizeof(XWeight));
			while (cookType) {
				Cook* cook = cookType->cooks->first;
				while (cook) {
					if (cook->type == weightReduc && cook->used) {
						Field* field;
						if (cook->calcField) field = cook->calcField;
						else field = cook->fields.first->field;
						weight->foodPos = cook->field->fromPos - 1;
						weight->output = Xobs + field->toPos - 1;
					}
					cook = cook->next;
					weight++;
				}
				cookType = cookType->next;
			}
		}
  

		{ /* loop throug the food table and change all weight cook fractions to
		     nutri cook fractions. */
			HashIntEntry** p1 = foodTable->table;
			int n = foodTable->size;
			while (n--) {
				HashIntEntry* p2 = *p1++;
				while (p2) {
					FoodEntry* foodEntry = p2->value;
					Num* obs = foodEntry->u.obs;

					{ /* move all nutri fields from obs to Xobs */
						int n = XnoFoodNutri;
						int* pfoodPos = XfoodNutriPos;
						Num** poutput = XoutputNutri;
						while (n--) **poutput++ = obs[*pfoodPos++];
					}

					{ /* calculate new fields */
						int n = XnoSet;
						XSet* set = Xset;
						while (n--) {
							switch (set->op) {
							case cpyOp: *(set->output) = *(set->u.operan); break;
							case addOp: *(set->output) += *(set->u.operan); break;
							case subOp: *(set->output) -= *(set->u.operan); break;
							case mulOp: *(set->output) *= *(set->u.operan); break;
							case divOp: if (*(set->u.operan)) *(set->output) /= *(set->u.operan); 
										else *(set->output) = (Num)0;
										break;
							case cpyOpC: *(set->output) = set->u.num; break;
							case addOpC: *(set->output) += set->u.num; break;
							case subOpC: *(set->output) -= set->u.num; break;
							case mulOpC: *(set->output) *= set->u.num; break;
							case divOpC: *(set->output) /= set->u.num; break;
							}
							set++;
						}
					}

					{ /* change fractions */
						int n = XnoWeight;
						XWeight* weight = Xweight;
						while (n--) {
							if (*(weight->output) == (Num)0.0) 
								obs[weight->foodPos] = (Num)0.0;
							else
								obs[weight->foodPos] = 
									recipeSum * obs[weight->foodPos] / *(weight->output);
							weight++;
						}
					}

					p2 = p2->next;
				}
			}
		}
	}
}




/***********************************************************************************/
/*** STEP 6 */

/* The function readRecipes() is STEP 6, and it reads all recipes files and inserts
   the recipes into the food table.
   The calculations are done with the foodCalc() function, but with special input and
   output functions. These special functions and other utility functions are defined
   before the readRecipes() function */


/* the following vars are used by outputRecipe and flushRecipe. they should be set
   before FoodCalc() are called for any recipes file */
int XrecipeIdOutput;	/* position of recipe id in obs */
int XrecipeId;			/* current recipe id */
Num* Xrecipe;			/* array to hold sums and noCalc values */
int XrecipeNoSum;		/* no of fields to sum into Xrecipe */
int* XrecipeSumOutput;	/* array[XrecipeNoSum] of positions of fields in obs to sum */
int* XrecipeSumRecipe;	/* array[XrecipeNoSum] of positions of fields in Xrecipe to sum */
int XrecipeNoMove1;		/* no of fields to move to Xrecipe (only from first ingredint) */
int* XrecipeMove1Output;/* array[XrecipeNoMove1] of positions of fields in obs to move */
int* XrecipeMove1Recipe;/* array[XrecipeNoMove1] of positions of fields in Xrecipe to move to */
int XrecipeNoNutri;		/* no of fields to scale */
int* XrecipeNutriOutput;/* array[XrecipeNoNutri] of positions of nutri fields in obs */
int* XrecipeNutriTable;	/* array[XrecipeNoNutri] of positions of nutri fields in table */
int XrecipeNoMove2;		/* no of fields to move from obs to table */
int* XrecipeMove2Output;/* array[XrecipeNoMove2] of positions of fields in obs to move */
int* XrecipeMove2Table;	/* array[XrecipeNoMove2] of positions of fields in table to move to */
int XrecipeSum;			/* position of recipe sum in Xrecipe */
int XrecipeAmount;		/* position of amount in Xrecipe */
int XrecipeNoReduct;	/* no of recipe reducts */
typedef struct XRecipeReduct_ {
	int reduct;				/* position in Xrecipe of reduct field */
	int sum;				/* position in Xrecipe of sum */
	int noTable;			/* no of fields to reduce */
	int* table;				/* positions in table of fields to reduce */
} XRecipeReduct;
int XrecipeNoIngredients;/* number of ingredients in recipe so far */
XRecipeReduct* XrecipeReduct; /* array[XrecipeNoReduct] of recipe reductions */
RecipeEntry* XrecipeEntry;/* current list of ingredients */


/* this vars will be set after return of foodCalc(). You must initialize before calling
   foodCalc() */
int noOutputRecipes;


/* this should be called to flush the current recipe to the food table */
void flushRecipe() {
	if (XrecipeNoIngredients) { /* only if we got any ingredients at all */

		if (XrecipeId <= 0) {
			error("Bad recipe id value %d at line %d of recipe file %s.\n",
				XrecipeId,lineNo-2,currentFileName);
		} else if (Xrecipe[XrecipeSum] <= (Num)0.0) {
			error("Recipe sum %f not positive for recipe %d at line %d of recipe file %s.\n",
				(double)Xrecipe[XrecipeSum],XrecipeId,lineNo-2,currentFileName);
		} else {

			Num sum = Xrecipe[XrecipeSum];
			FoodEntry* foodEntry;

			{ /* recipe reduce */
				int n = XrecipeNoReduct;
				XRecipeReduct* p = XrecipeReduct;
				while (n--) {
					if (Xrecipe[p->sum] > (Num)0.0 && Xrecipe[p->reduct] != 0) {
						RecipeEntry* r = XrecipeEntry;
						Num factor;
						Num loss = Xrecipe[XrecipeAmount]*Xrecipe[p->reduct];
						if (loss > Xrecipe[p->sum]) loss = Xrecipe[p->sum];
						sum -= loss;
						factor = (Num)1.0 - loss/Xrecipe[p->sum];
						while (r) {
							int n = p->noTable;
							int* q = p->table;
							while (n--) r->obs[*q++] *= factor;
							r = r->next;
						}
					}
					p++;
				}
			}
			{ /* scale */
				Num scale = recipeSum/sum;
				RecipeEntry* r = XrecipeEntry;
				while (r) {
					Num* table = r->obs;
					int n = XrecipeNoNutri;
					int* p = XrecipeNutriTable;
					while (n--) table[*p++] *= scale;
					r = r->next;
				}
			}

			if (keepIngredients) foodEntry = allocFoodEntry(expandedRecipe,XrecipeEntry);
			else foodEntry = allocFoodEntry(simpleRecipe,XrecipeEntry->obs);

			/* finally insert in the food table */
			if (lookInsertInt(foodTable,XrecipeId,foodEntry)) {
				error("Recipe id %d allready in food table at line %d of recipe file %s.\n",
					XrecipeId,lineNo-1,currentFileName);
			} else {
				noOutputRecipes++;
				totFoods++;
			}
		}
		XrecipeNoIngredients = 0;
	}
}


/* this is for the foodCalc() XoutputFun */
void outputRecipe(Num* obs, int no) {
	int key = (int)obs[XrecipeIdOutput];
	Num* table;

	if (key != XrecipeId) {
		flushRecipe();
		XrecipeId = key;
		if (!keepIngredients) {
			XrecipeEntry = allocStruct(RecipeEntry);
			XrecipeEntry->next = NULL;
			XrecipeEntry->obs = allocarray(foodTableFields.no,sizeof(Num));
		} else {
			XrecipeEntry = NULL;
		}
		{ /* zero out Xrecipe */
			int n = XrecipeNoSum;
			int* p = XrecipeSumRecipe;
			while (n--) Xrecipe[*p++] = (Num)0.0;
		}
		{ /* move to Xrecipe */
			int n = XrecipeNoMove1;
			int* p = XrecipeMove1Output;
			int* q = XrecipeMove1Recipe;
			while (n--) Xrecipe[*q++] = obs[*p++];
		}
	}

	if (keepIngredients) {
		RecipeEntry* recipe = allocStruct(RecipeEntry);
		recipe->next = XrecipeEntry;
		XrecipeEntry = recipe;
		recipe->obs = table = allocarray(foodTableFields.no,sizeof(Num));
	} else {
		table = XrecipeEntry->obs;
	}

	{ /* add to Xrecipe */
		int n = XrecipeNoSum;
		int* p = XrecipeSumOutput;
		int* q = XrecipeSumRecipe;
		while (n--) Xrecipe[*q++] += obs[*p++];
	}
	{ /* add to table */
		int n = XrecipeNoNutri;
		int* p = XrecipeNutriOutput;
		int* q = XrecipeNutriTable;
		while (n--) table[*q++] += obs[*p++];
	}
	{ /* move to table */
		int n = XrecipeNoMove2;
		int* p = XrecipeMove2Output;
		int* q = XrecipeMove2Table;
		while (n--) table[*q++] = obs[*p++];
	}
	XrecipeNoIngredients++;
}


/* this is the function which does STEP 6 - see the comments above! */
void readRecipes() {

	RecipesFile* recipesFile = recipesFiles.first;

	setFoodCalcPos(1);
	
	while (recipesFile) {

		int pXrecipe = 0;
		int pSum = XrecipeNoSum = 0;
		int pMove1 = XrecipeNoMove1 = 0;
		int pNutri = XrecipeNoNutri = 0;
		int pMove2 = XrecipeNoMove2 = 0;
		int pReduct = XrecipeNoReduct = 0;

		setRecipesPos(recipesFile);

		XrecipeNoSum += 2; /* recipe sum and amount */

		{ /* count nutri and move2 of table fields */
			FieldP* fieldP = foodTableFields.first;
			while (fieldP) {
				Field* field = fieldP->field;
				if (field->toPos) {
					if (!field->noCalc) XrecipeNoNutri++;
					else if (keepIngredients) XrecipeNoMove2++;
				}
				fieldP = fieldP->next;
			}
		}
		if (!keepIngredients && foodId->toPos) XrecipeNoMove2++; /* recipe id */
		if (copyRecipeFields) { /* count move2 of recipes file fields */
			Field* field = recipesFile->fields->first;
			while (field) {
				if (field->toPos && field->groupBy) XrecipeNoMove2++;
				field = field->next;
			}
		}
		{ /* count recipe reduce */
			RecipeReduct* r = recipeReducts.first;
			while (r) {
				if (r->field = lookStr(recipesFile->fieldsHash,r->fieldName)) {
					FieldP* fieldP = r->fields.first;
					r->used = 0;
					while (fieldP) {
						if (fieldP->field->toPos) r->used++;
						fieldP = fieldP->next;
					}
					XrecipeNoSum++;
					XrecipeNoMove1++;
					XrecipeNoReduct++;
				}
				r = r->next;
			}
		}

		Xrecipe = alloc((XrecipeNoSum+XrecipeNoMove1)*sizeof(Num));
		XrecipeSumOutput = alloc(XrecipeNoSum*sizeof(int));
		XrecipeSumRecipe = alloc(XrecipeNoSum*sizeof(int));
		XrecipeMove1Output = alloc(XrecipeNoMove1*sizeof(int));
		XrecipeMove1Recipe = alloc(XrecipeNoMove1*sizeof(int));
		XrecipeNutriOutput = alloc(XrecipeNoNutri*sizeof(int));
		XrecipeNutriTable = alloc(XrecipeNoNutri*sizeof(int));
		XrecipeMove2Output = alloc(XrecipeNoMove1*sizeof(int));
		XrecipeMove2Table = alloc(XrecipeNoMove1*sizeof(int));
		XrecipeReduct = alloc(XrecipeNoReduct*sizeof(XRecipeReduct));

		{ /* set recipe id, recipe sum and amount positions */
			XrecipeIdOutput = recipesFile->recipeId->toPos - 1;
			XrecipeSumOutput[pSum] = recipeSumField->toPos - 1;
			XrecipeSumRecipe[pSum++] = pXrecipe;
			XrecipeSum = pXrecipe++;
			XrecipeSumOutput[pSum] = recipesFile->amountField->toPos - 1;
			XrecipeSumRecipe[pSum++] = pXrecipe;
			XrecipeAmount = pXrecipe++;
		}
		{ /* set nutri and move2 of table fields */
			FieldP* fieldP = foodTableFields.first;
			while (fieldP) {
				Field* field = fieldP->field;
				if (field->toPos) {
					if (!field->noCalc) {
						XrecipeNutriOutput[pNutri] = field->toPos - 1;
						XrecipeNutriTable[pNutri++] = field->fromPos - 1;
					} else if (keepIngredients) {
						XrecipeMove2Output[pMove2] = field->toPos - 1;
						XrecipeMove2Table[pMove2++] = field->fromPos - 1;
					}
				}
				fieldP = fieldP->next;
			}
		}
		if (!keepIngredients && foodId->toPos) { /* recipe id */
				XrecipeMove2Output[pMove2] = recipesFile->recipeId->toPos - 1;
				XrecipeMove2Table[pMove2++] = foodId->fromPos - 1;
		}
		if (copyRecipeFields) { /* set move2 of recipes file fields */
			Field* field = recipesFile->fields->first;
			while (field) {
				if (field->toPos && field->groupBy) {
					Field* field2 = lookStr(foodFieldsHash,field->name);
					XrecipeMove2Output[pMove2] = field->toPos - 1;
					XrecipeMove2Table[pMove2++] = field2->fromPos - 1;
				}
				field = field->next;
			}
		}
		{ /* set recipe reduce */
			RecipeReduct* r = recipeReducts.first;
			int i = 0;
			while (r) {
				if (r->field) {
					FieldP* fieldP = r->fields.first;
					XRecipeReduct* rr = XrecipeReduct+i++;
					{ /* reduct field */
						XrecipeMove1Output[pMove1] = r->field->toPos - 1;
						XrecipeMove1Recipe[pMove1++] = pXrecipe;
						rr->reduct = pXrecipe++;
					}
					{ /* sum field */
						if (r->calcField) XrecipeSumOutput[pSum] = r->calcField->toPos - 1;
						else XrecipeSumOutput[pSum] = fieldP->field->toPos - 1;
						XrecipeSumRecipe[pSum++] = pXrecipe;
						rr->sum = pXrecipe++;
					}
					{ /* fields to reduce */
						int* t = rr->table = alloc(r->used*sizeof(int));
						rr->noTable = r->used;
						while (fieldP) {
							if (fieldP->field->toPos) 
								*t++ = fieldP->field->fromPos - 1;
							fieldP = fieldP->next;
						}
					}
				}
				r = r->next;
			}
		}

		XinputFun = &readNumLine;
		XoutputFun = &outputRecipe;
		Xflush = &flushRecipe;
		noOutputRecipes = 0;
		XrecipeId = 0;
		setCurrent(recipesFile->file);
		foodCalc();
		flushRecipe();

		{	/* log what we did */
			int lineLen = 0;
			Field* field = recipesFile->fields->first;
			logmsg("Read recipes file %s. Recipes: %d. Ingredients: %d. Fields:\n",
				currentFileName,noOutputRecipes,noInputLines);
			while (field) {
				lineLen += strlen(field->name)+1;
				if (lineLen > 78) {logmsg("\n"); lineLen = strlen(field->name)+1;}
				logmsg("%s ",field->name);
				field = field->next;
			}
			logmsg("\n\n");
		}

		closeCurrent();
		recipesFile = recipesFile->next;
	}
}



/********************************************************************************/
/*** Utility function to save the state of FoodCalc (for the save: command) and to
     read a saved state back into FoodCalc (for the -s option to FoodCalc). */


FILE* saveFile;			/* the save file */
int saveFileIntBuf;		/* work buffer used by the macros an functions below */

#define getIntP(i) fread((i),sizeof(int),1,saveFile)
#define getI1(i1) getIntP(&(i1))
#define getI2(i1,i2) (getIntP(&(i1)),getIntP(&(i2)))
#define getI3(i1,i2,i3) (getIntP(&(i1)),getIntP(&(i2)),getIntP(&(i3)))
#define getIA(ia,n) fread((ia) = alloc(sizeof(int)*(n)),sizeof(int),(n),saveFile)
#define getInt() (getI1(saveFileIntBuf), saveFileIntBuf)
#define getNumP(n) fread((n),sizeof(Num),1,saveFile)
#define getN1(n1) getNumP(&(n1))
#define getN2(n1,n2) (getNumP(&(n1)),getNumP(&(n2)))
#define getNP(np,base) ((np) = (base) + getInt())
#define getNA(na,n) fread((na) = alloc(sizeof(Num)*(n)),sizeof(Num),(n),saveFile)
void getNPA_(Num** npa, int n, Num* base) {while (n--) *npa++ = base + getInt();}
#define getNPA(npa,n,base) getNPA_((npa) = alloc(sizeof(Num*)*(n)),(n),(base))
#define getStrP(s) (getInt(), fread((s) = alloc(saveFileIntBuf+1),saveFileIntBuf,1,saveFile), *((s)+saveFileIntBuf) = '\0')
#define getC(c) fread(&(c),sizeof(char),1,saveFile)

#define saveIntP(i) fwrite((i),sizeof(int),1,saveFile)
#define saveI1(i1) saveIntP(&(i1))
#define saveI2(i1,i2) (saveIntP(&(i1)),saveIntP(&(i2)))
#define saveI3(i1,i2,i3) (saveIntP(&(i1)),saveIntP(&(i2)),saveIntP(&(i3)))
#define saveIA(ia,n) fwrite((ia),sizeof(int),n,saveFile)
#define saveInt(i) (saveFileIntBuf = (i), saveI1(saveFileIntBuf))
#define saveNumP(n) fwrite((n),sizeof(Num),1,saveFile)
#define saveN1(n1) saveNumP(&(n1))
#define saveN2(n1,n2) (saveNumP(&(n1)),saveNumP(&(n2)))
#define saveNA(na,n) fwrite((na),sizeof(Num),(n),saveFile)
#define saveNP(np,base) saveInt((np) - (base))
void saveNPA(Num** npa, int n, Num* base) {while (n--) saveInt(*npa++ - base);}
#define saveStrP(s) (saveInt(strlen(s)), fwrite((s),saveFileIntBuf,1,saveFile))
#define saveC(c) fwrite(&(c),sizeof(char),1,saveFile)


/* saves the state of FoodCalc to the save file */
void save() {

	if (!(saveFile = fopen(saveFileName,"wb")))
		abortAndExit("Could not open save file %s.\n",saveFileName);

	saveStrP(program); saveI2(programMajor,programMinor);

	saveI1(verbosity);
	saveStrP(logFileName);
	saveI1(inputFormat); saveStrP(inputFileName); 
	saveC(inputSep); saveC(inputDecPoint);
	saveI1(outputFormat); saveStrP(outputFileName);
	saveC(outputSep); saveC(outputDecPoint);

	saveI2(XnoOutput,XnoRealOutput);

	saveI1(XnoInput);
	saveIA(Xtext,XnoInput);
	saveI2(Xstar,XnoInputMove);
	saveNPA(XinputMove,XnoInputMove,Xline);
	saveNPA(XoutputInput,XnoInputMove,Xobs);
	saveI1(XnoInputGroupBy);
	saveNPA(XinputGroupBy,XnoInputGroupBy,Xline);
	saveIA(XgroupInputPos,XnoInputGroupBy);
	saveNP(XinputFood,Xline);
	saveNP(XinputAmount,Xline);
	saveN1(XinputAmountScale);

	saveI3(XnoNonEdible,XnonEdible,XnoNonEdibleFlag);
	saveNP(XnonEdibleFlag,Xline);

	saveI1(XnoInputCook);
	saveNP(XinputCook,Xline);

	saveI1(XnoCookTypes);
	while (XnoCookTypes--) {
		XCook* cook = XcookType->cook;
		saveI1(XcookType->no);
		while ((XcookType->no)--) {
			saveI2(cook->foodPos,cook->noOutput);
			saveNPA(cook->output,cook->noOutput,Xobs);
			cook++;
		}
		XcookType++;
	}

	saveI1(XnoFoodGroupBy);
	saveNPA(XoutputGroupBy,XnoFoodGroupBy,Xobs);
	saveIA(XgroupFoodPos,XnoFoodGroupBy);

	saveI1(XnoFoodMove);
	saveIA(XfoodMovePos,XnoFoodMove);
	saveNPA(XoutputFood,XnoFoodMove,Xobs);
	saveI1(XnoFoodNutri);
	saveIA(XfoodNutriPos,XnoFoodNutri);
	saveNPA(XoutputNutri,XnoFoodNutri,Xobs);

	saveI1(XnoReduct);
	while (XnoReduct--) {
		saveNP(Xreduct->input,Xline);
		saveI1(Xreduct->noOutput);
		saveNPA(Xreduct->output,Xreduct->noOutput,Xobs);
		Xreduct++;
	}
	saveI2(XnoWeightReduct,XnoCalcWeightReduct);
	while (XnoWeightReduct--) {
		saveNP(XweightReduct->input,Xline);
		saveNP(XweightReduct->output,Xobs);
		XweightReduct++;
	}

	saveI2(XnoSet,XnoSet2);
	while (XnoSet--) {
		saveNP(Xset->output,Xobs);
		saveI1(Xset->op);
		if (Xset->op < cpyOpC) {saveNP(Xset->u.operan,Xobs);}
		else {saveN1(Xset->u.num);}
		Xset++;
	}
	saveI1(XnoGroupSet);
	while (XnoGroupSet--) {
		saveI2(XgroupSet->pos,XgroupSet->op);
		if (XgroupSet->op < cpyOpC) {saveI1(XgroupSet->u.operan);}
		else {saveN1(XgroupSet->u.num);}
		XgroupSet++;
	}

	saveI1(XnoSimpleTest);
	while (XnoSimpleTest--) {
		saveI2(XsimpleTest->op,XsimpleTest->pos);
		saveN1(XsimpleTest->num);
		saveNP(XsimpleTest->action,XsimpleTest);
		XsimpleTest++;
	}
	saveI1(XnoTest);
	while (XnoTest--) {
		saveI1(Xtest->op);
		saveNP(Xtest->output1,Xobs);
		saveNP(Xtest->output2,Xobs);
		saveNP(Xtest->action,Xtest);
		Xtest++;
	}

	saveI1(XnoTranspose);
	while (XnoTranspose--) {
		saveI3(Xtranspose->pos,Xtranspose->noGroups,Xtranspose->groupPos);
		saveI1(Xtranspose->noOutput);
		saveNPA(Xtranspose->output,Xtranspose->noOutput,Xobs);
		Xtranspose++;
	}

	saveI1(XnoBlip);

	saveI2(foodTableFields.no,totFoods);
	{
		HashIntEntry** p1 = foodTable->table;
		int n = foodTable->size;
		while (n--) {
			HashIntEntry* p2 = *p1++;
			while (p2) {
				FoodEntry* foodEntry = p2->value;
				saveI2(p2->key,foodEntry->foodType);
				if (foodEntry->foodType != expandedRecipe) {
					Num* obs = foodEntry->u.obs;
					saveNA(obs,foodTableFields.no);
				} else {
					RecipeEntry* recipeEntry = foodEntry->u.recipe;
					int n = 0;
					while (recipeEntry) {n++; recipeEntry = recipeEntry->next;}
					saveI1(n);
					recipeEntry = foodEntry->u.recipe;
					while (recipeEntry) {
						saveNA(recipeEntry->obs,foodTableFields.no);
						recipeEntry = recipeEntry->next;
					}
				}
				p2 = p2->next;
			}
		}
	}

	{
		Field* field = inputFields.first;
		saveI1(inputFields.no);
		while (field) {
			saveStrP(field->name);
			field = field->next;
		}
	}

	{
		FieldP* fieldP = outputFields.first;
		int n = noRealOutputFields;
		saveI1(n);
		while (n--) {
			saveStrP(fieldP->field->name);
			fieldP = fieldP->next;
		}
	}

	saveInt(12345); /* eof marker */

	if (ferror(saveFile)) abortAndExit("Error writing save file %s.\n",saveFileName);
	logmsg("Wrote the save file %s.\n\n",saveFileName);
	fclose(saveFile);
}


/* read a saved state from a save file into FoodCalc */
void get() {
	int saveProgramVer;
	char* dummy;
	int dummyi;

	if (!(saveFile = fopen(saveFileName,"rb")))
		abortAndExit("Could not open save file %s.\n",saveFileName);

	{
		char* saveProgram;
		int saveProgramMajor, saveProgramMinor;
		getStrP(saveProgram);
		if (strcmp(program,saveProgram) != 0)
			abortAndExit("File %s is not a save file.\n",saveFileName);
		getI2(saveProgramMajor,saveProgramMinor);
		if (programMajor != saveProgramMajor || programMinor < saveProgramMinor)
			abortAndExit("Save file %s is saved with a different version of FoodCalc.\n",
				saveFileName);
		saveProgramVer = saveProgramMajor*100+saveProgramMinor;
		if (saveProgramVer < 103)
			abortAndExit("Save file %s is saved with a different version of FoodCalc.\n",
				saveFileName);
	}

	if (verbosity) getI1(dummyi); else getI1(verbosity);
	if (logFileName) getStrP(dummy); else getStrP(logFileName);
	getI1(inputFormat); if (inputFileName) getStrP(dummy); else getStrP(inputFileName);
	getC(inputSep); getC(inputDecPoint);
	getI1(outputFormat); if (outputFileName) getStrP(dummy); else getStrP(outputFileName);
	getC(outputSep); getC(outputDecPoint);

	getI2(XnoOutput,XnoRealOutput);
	Xobs = alloc(XnoOutput*sizeof(Num));

	getI1(XnoInput);
	getIA(Xtext,XnoInput);
	Xline = alloc(XnoInput*sizeof(num));
	getI2(Xstar,XnoInputMove);
	getNPA(XinputMove,XnoInputMove,Xline);
	getNPA(XoutputInput,XnoInputMove,Xobs);
	getI1(XnoInputGroupBy);
	getNPA(XinputGroupBy,XnoInputGroupBy,Xline);
	getIA(XgroupInputPos,XnoInputGroupBy);
	getNP(XinputFood,Xline);
	getNP(XinputAmount,Xline);
	getN1(XinputAmountScale);
	getI3(XnoNonEdible,XnonEdible,XnoNonEdibleFlag);
	getNP(XnonEdibleFlag,Xline);

	getI1(XnoInputCook);
	getNP(XinputCook,Xline);

	{
		int n;
		XCookType* cookType;
		getI1(XnoCookTypes); n = XnoCookTypes;
		cookType = XcookType = alloc(sizeof(XCookType)*XnoCookTypes);
		while (n--) {
			int m;
			XCook* cook;
			getI1(cookType->no); m = cookType->no;
			cook = cookType->cook = alloc(sizeof(XCook)*cookType->no);
			while (m--) {
				getI2(cook->foodPos,cook->noOutput);
				getNPA(cook->output,cook->noOutput,Xobs);
				cook++;
			}
			cookType++;
		}
	}

	getI1(XnoFoodGroupBy);
	getNPA(XoutputGroupBy,XnoFoodGroupBy,Xobs);
	getIA(XgroupFoodPos,XnoFoodGroupBy);

	getI1(XnoFoodMove);
	getIA(XfoodMovePos,XnoFoodMove);
	getNPA(XoutputFood,XnoFoodMove,Xobs);
	getI1(XnoFoodNutri);
	getIA(XfoodNutriPos,XnoFoodNutri);
	getNPA(XoutputNutri,XnoFoodNutri,Xobs);

	{
		int n;
		XReduct* reduct;
		getI1(XnoReduct); n = XnoReduct;
		reduct = Xreduct = alloc(sizeof(XReduct)*XnoReduct);
		while (n--) {
			getNP(reduct->input,Xline);
			getI1(reduct->noOutput);
			getNPA(reduct->output,reduct->noOutput,Xobs);
			reduct++;
		}
	}
	{
		int n;
		XWeightReduct* weightReduct;
		getI2(XnoWeightReduct,XnoCalcWeightReduct); n = XnoWeightReduct;
		weightReduct = XweightReduct = alloc(sizeof(XWeightReduct)*XnoWeightReduct);
		while (n--) {
			getNP(XweightReduct->input,Xline);
			getNP(XweightReduct->output,Xobs);
			weightReduct++;
		}
	}

	{
		int n;
		XSet* set;
		getI2(XnoSet,XnoSet2); n = XnoSet;
		set = Xset = alloc(sizeof(XSet)*XnoSet);
		while (n--) {
			getNP(set->output,Xobs);
			getI1(set->op);
			if (set->op < cpyOpC) {getNP(set->u.operan,Xobs);}
			else {getN1(set->u.num);}
			set++;
		}
	}
	{
		int n;
		XGroupSet* groupSet;
		getI1(XnoGroupSet); n = XnoGroupSet;
		groupSet = XgroupSet = alloc(sizeof(XGroupSet)*XnoGroupSet);
		while (n--) {
			getI2(groupSet->pos,groupSet->op);
			if (groupSet->op < cpyOpC) {getI1(groupSet->u.operan);}
			else {getN1(groupSet->u.num);}
			groupSet++;
		}
	}

	{
		int n;
		XSimpleTest* simpleTest;
		getI1(XnoSimpleTest); n = XnoSimpleTest;
		simpleTest = XsimpleTest = alloc(sizeof(XSimpleTest)*XnoSimpleTest);
		while (n--) {
			getI2(simpleTest->op,simpleTest->pos);
			getN1(simpleTest->num);
			getNP(simpleTest->action,XsimpleTest);
			simpleTest++;
		}
	}
	{
		int n;
		XTest* test;
		saveI1(XnoTest); n = XnoTest;
		test = Xtest = alloc(sizeof(XTest)*XnoTest);
		while (n--) {
			getI1(test->op);
			getNP(test->output1,Xobs);
			getNP(test->output2,Xobs);
			getNP(test->action,Xtest);
			test++;
		}
	}

	{
		int n;
		XTranspose* transpose;
		getI1(XnoTranspose); n = XnoTranspose;
		transpose = Xtranspose = alloc(sizeof(XTranspose)*XnoTranspose);
		while (n--) {
			getI3(transpose->pos,transpose->noGroups,transpose->groupPos);
			getI1(transpose->noOutput);
			getNPA(transpose->output,transpose->noOutput,Xobs);
			transpose++;
		}
	}

	getI1(XnoBlip);

	{
		int noFields;
		int noFoods;

		foodTable = newHashInt(3571);
		getI2(noFields,noFoods);

		while (noFoods--) {
			FoodEntry* foodEntry = allocStruct(FoodEntry);
			int key;
			getI2(key,foodEntry->foodType);
			if (foodEntry->foodType != expandedRecipe) {
				Num** obs = &(foodEntry->u.obs);
				getNA(*obs,noFields);
			} else {
				int n;
				RecipeEntry* lastRecipeEntry = NULL;
				getI1(n);
				while (n--) {
					RecipeEntry* recipeEntry = allocStruct(RecipeEntry);
					getNA(recipeEntry->obs,noFields);
					recipeEntry->next = lastRecipeEntry;
					lastRecipeEntry = recipeEntry;
				}
				foodEntry->u.recipe = lastRecipeEntry;
			}
			insertInt(foodTable,key,foodEntry);
		}
	}

	{
		int n;
		getI1(n);
		nolink(inputFields);
		while (n--) {
			char* s;
			getStrP(s);
			link(inputFields,allocField(s))
		}
		endlink(inputFields);
	}
	{
		int n;
		getI1(n);
		nolink(outputFields);
		while (n--) {
			char* s;
			getStrP(s);
			link(outputFields,allocFieldP(allocField(s)))
		}
		endlink(outputFields);
	}

	if (feof(saveFile) || ferror(saveFile) || getInt() != 12345)
		abortAndExit("Error reading save file %s.\n",saveFileName);
	logmsg("Read the save file %s.\n\n",saveFileName);
	fclose(saveFile);

}



/*********************************************************************************/
/*** STEP 7 */

/* the readInput() functions is STEP 7. it reads the input, does all calculations etc.,
   and writes the output. The meat of the work is done by the doIt() function. If the
   save: command is used, the state is saved just before doIt() would have been called,
   and FoodCalc then exits. If the -s options is used the saved state is read and then
   doIt() is called. */


/* this is the meat of it all */
void doIt() {
	{ /* open input */
		char* mode;
		if (inputFormat == formatBinNative) {
			if (sizeof(Num) != sizeof(double)) {
				read4buf = alloc(sizeof(double)*inputFields.no);
				XinputFun = &readNumBinNative4;
			} else {
				XinputFun = &readNumBinNative;
			}
			inputFile = allocStruct(File);
			mode = "rb";
			if (!(inputFile->file = fopen(inputFileName,mode))) {
				error("Could not open file %s.\n",inputFileName);
				return;
			}
			inputFile->name = inputFileName;
			inputFile->type = inputFileT;
			inputFile->lineNo = 1;
		} else if (inputFormat != formatText) {
			XinputFun = &readNumLine;
			mode = "r";
			inputFile = allocStruct(File);
			if (!initFile(inputFile,inputFileName,inputFileT,mode,
				          inputSep,inputDecPoint,inputComment)) {
				error("Could not open file %s.\n",inputFileName);
				return;
			}
		} else {
			XinputFun = &readNumLine;
		}
		setCurrent(inputFile);
	}

	{ /* open output */
		char* mode;
		if (outputFormat == formatBinNative) {
			if (sizeof(Num) != sizeof(double)) {
				output4buf = alloc(sizeof(double)*outputFields.no);
				XoutputFun = outputBinNative4;
			} else {
				XoutputFun = outputBinNative;
			}
			mode = "wb";
		} else {
			XoutputFun = &outputLine;
			mode = "w";
		}
		if (strcmp(outputFileName,"-") == 0) {
			output = stdout;
		} else if (!(output = fopen(outputFileName,mode))) {
			error("Could not open file %s.\n",outputFileName);
			return;
		}
		if (outputFormat == formatText) {
			/* output header line */
			FieldP* fieldP = outputFields.first;
			int n = XnoRealOutput;
			while (n--) {
				fputs(fieldP->field->name,output);
				fieldP = fieldP->next;
				if (n) fputc(outputSep,output);
			}
			fputc('\n',output);
		}
	}

	Xflush = NULL;
	foodCalc();

	{	/* log what we read */
		int lineLen = 0;
		Field* field = inputFields.first;
		logmsg("Read input file %s. Lines: %d. Fields: %d\n",
			currentFileName,noInputLines,inputFields.no);
		while (field) {
			lineLen += strlen(field->name)+1;
			if (lineLen > 78) {logmsg("\n"); lineLen = strlen(field->name)+1;}
			logmsg("%s ",field->name);
			field = field->next;
		}
		logmsg("\n\n");
	}
	{	/* log what we wrote */
		int lineLen = 0;
		FieldP* fieldP = outputFields.first;
		int n = XnoRealOutput;
		logmsg("Wrote output file %s. Lines: %d. Fields: %d\n",
			outputFileName,noOutputObs,XnoRealOutput);
		while (n--) {
			lineLen += strlen(fieldP->field->name)+1;
			if (lineLen > 78) {logmsg("\n"); lineLen = strlen(fieldP->field->name)+1;}
			logmsg("%s ",fieldP->field->name);
			fieldP = fieldP->next;
		}
		logmsg("\n\n");
	}
	
	closeCurrent();
}


/* this function is STEP 7 - se comments above */
void readInput() {

	{	/* log about food table */
		int lineLen = 0;
		FieldP* fieldP = foodTableFields.first;
		logmsg("The food table contains %d foods and the fields: \n",totFoods);
		while (fieldP) {
			lineLen += strlen(fieldP->field->name)+1;
			if (lineLen > 78) {logmsg("\n"); lineLen = strlen(fieldP->field->name)+1;}
			logmsg("%s ",fieldP->field->name);
			fieldP = fieldP->next;
		}
		logmsg("\n\n");
	}

	setFoodCalcPos(0);
	setInputPos();

	if (saveBin) save();
	else doIt();

}
		
		

/**********************************************************************************/
/**********************************************************************************/


void main(int argc, char** argv) {
	
	File commands;
	Cmd* cmd;

	argv++;
	commandsHashInit();
	mainCommandsName = "-";

	while (*argv && *argv[0] == '-' && *argv[1]) {
		char option = *++*argv;
		char* optionArg = ++*argv;
		if (!*optionArg) {
			if (!*++argv) abortAndExit("Argument missing to -%c option\n",option);
			optionArg = *argv;
		}
		
		switch (option) {
		case 'v': verbosity = atoi(optionArg); break;
		case 'i': inputFileName = optionArg; break;
		case 'o': outputFileName = optionArg; break;
		case 'l': logFileName = optionArg; break;
		case 's': saveFileName = optionArg; break;
		default: abortAndExit("Unknown option -%c\n",option);
		}
		++argv;
	}

	/* check first if we are called with -s. If we are, we do not have to do all
	   the initial steps. */
	if (saveFileName) {
		if (*argv) abortAndExit("You can not specify commands files with -s.\n");
		get();
		doIt();
		if (errors) abortAndExit("");
		exit(0);
	}
	
	/* set file name of the main commands file */
	if (*argv) {
		mainCommandsName = *argv;
		argv++;
#if defined(_DEBUG)
	} else {
		/*mainCommandsName = "p:\\jesper\\work\\levtab\\test\\t.txt";*/
		/*mainCommandsName = "p:\\jesper\\work\\foodcalc\\debug\\tst.fc";*/
		mainCommandsName = "v:\\levtab\\connie.tst\\kostber.fc";
		/*mainCommandsName = "v:\\levtab\\connie.tst\\t.txt";*/
		verbosity = 99;
#endif
	}
	
	/** STEP 1 **/
	/* read the main commands file */
	if (!initFile(&commands,mainCommandsName,commandFileT,"r",',','.',';'))
		abortAndExit("Could not open commands file %s.\n",mainCommandsName);
	readCommands(&commands);
	/* read any other commands files specified as arguments to FoodCalc */
	while (*argv) {
		if (!initFile(&commands,*argv,commandFileT,"r",',','.',';'))
			error("Could not open commands file %s.\n",*argv);
		else
			readCommands(&commands);
		argv++;
	}
	/* read any commands files specified with the commands: command */
	cmd = commandsCmd;
	while (cmd && !errors) {
		if (!initFile(&commands,*(cmd->args),commandFileT,"r",',','.',';'))
			error("Could not open commands file %s.\n",*(cmd->args));
		else
			readCommands(&commands);
		cmd = cmd->next;
	}

	if (errors) abortAndExit(""); 
	logmsg("\n");
	checkCommands(); 
	if (errors) abortAndExit("");

	/* STEP 2 */
	readFields(); 
	if (errors) abortAndExit("");

	/* STEP 3 */
	readGroups();

	/* STEP 4 */
	readFoods();
	if (errors) abortAndExit("");

	/* STEP 5 */
	expandGroups();
	if (errors) abortAndExit("");

	/* STEP 5.5 */
	weightCookChange();

	/* STEP 6 */
	if (recipesFiles.first) readRecipes();
	if (errors) abortAndExit("");

	/* STEP 7 */
	readInput();
	if (errors) abortAndExit("");

	exit(0);
}
	
