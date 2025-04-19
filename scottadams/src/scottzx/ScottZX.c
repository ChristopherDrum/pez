/*
 *	ScottFree Revision 1.14
 *	Adapted by Stefano Bodrato to run in tiny environments.
 *	
 *
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *
 *	You must have an ANSI C compiler to build this program.

Examples:

	zcc +zxansi -D__Z88DK__ -lzxmdv -lmalloc -create-app -DDEBUG -zorg=28000 scottzx.c

	cl scottzx.c

	gcc -oscottzx scottzx.c

 */

//#define wprintw(a,b) printf(b)
//#define wprintw(a,b,c) printf(b,c)

//#define DEBUG 1

#include <fcntl.h>

#include <stdio.h>
#include <string.h>

#ifndef PYRAMID
#include <stdlib.h>
#endif

#include <ctype.h>
//#include <curses.h>
#include <stdarg.h>
//#include <signal.h>

#include "Scottzx.h"


#ifdef __Z88DK__
	#include <malloc.h>
	#define HPSIZE 19000
	//#define HPSIZE 17000
	//#define HPSIZE 16000
	//#define HPSIZE 12000
	HEAPSIZE(HPSIZE)
#else
	#define fputc_cons(a) printf("%c",a)
	#define fgetc_cons() fgetc(stdin)
	#define strcasecmp stricmp
	#define strncasecmp strnicmp
	#define heapinit /*Heap init*/
#endif


int myct;
int ldct;
int num, vc;
int ne;
char *tp;
void *tmem;

/*
#define strcasecmp stricmp
#define strncasecmp strnicmp
*/

#ifdef AMIGA
#define NOGETPID
#define NOSTRNCASECMP
#endif

#ifdef PYRAMID
#define NOSTRNCASECMP
#endif



/*
 *	Configuration Twiddles
 */
 
#ifdef NOSTRNCASECMP

static int strncasecmp(char *a,char *b, int n)
{
	while(*a&&n>0)
	{
		char c=*a;
		char d=*b;
		if(islower(c))
			c=toupper(c);
		if(islower(d))
			d=toupper(d);
		if(c<d)
			return(-1);
		if(c>d)
			return(1);
		a++;
		b++;
		n--;
		if(n==0)
			return(0);
	}
	if(*b)
		return(1);
	return(0);
}
#endif

#ifdef NOGETPID
#ifndef AMIGA
#define getpid()   6031769
#else
#define getpid()   ((int)FindTask(NULL))
#endif
#endif

Header GameHeader;
Tail GameTail;
Item *Items;
Room *Rooms;
char **Verbs;
char **Nouns;
char **Messages;
Action *Actions;
int LightRefill;
char NounText[16];
int Counters[16];	/* Range unknown */
int CurrentCounter;
int SavedRoom;
int RoomSaved[16];	/* Range unknown */
//int DisplayUp;		/* Curses up */
//WINDOW *Top,*Bottom;
int Redraw;		/* Update item window */
int Options = TRS80_STYLE|DEBUGGING;		/* Option flags set */

int Width;		/* Terminal width */
//int TopHeight;		/* Height of top window */
//int BottomHeight;	/* Height of bottom window */



//#define TRS80_LINE	"\n<------------------------------------------------------------>\n"
#define TRS80_LINE	"\n<------------------------------------------------->\n"

#define MyLoc	(GameHeader.PlayerRoom)

long BitFlags=0;	/* Might be >32 flags - I haven't seen >32 yet */

void ClearScreen ()
{
	//fputc(12,stdout);
	printf ("%c[%um%c[%um",27,47,27,30);
	printf ("%c[2J",27);
}

void Fatal(char *x)
{
	//if(DisplayUp)
		//endwin();
	fprintf(stderr,"%s.\n",x);
	exit(1);
}

void *MemAlloc(int size)
{
	//printf("\nReserving %u bytes\n",size);

	//void *t=(void *)malloc(size);
	tmem=(void *)malloc(size);
	if(tmem==NULL)
//	if(tmem==0)
		Fatal("Out of memory");
	return(tmem);
}

int RandomPercent(int n)
{
	unsigned int rv=rand()<<6;
	rv%=100;
	if(rv<n)
		return(1);
	return(0);
}

int CountCarried()
{
	int n=0;
	myct=0;
	while(myct<=GameHeader.NumItems)
	{
		if(Items[myct].Location==CARRIED)
			n++;
		myct++;
	}
	return(n);
}

char *MapSynonym(char *word)
{
	int n=1;
	//char *tp;
	static char lastword[16];	/* Last non synonym */
	while(n<=GameHeader.NumWords)
	{
		tp=Nouns[n];
		if(*tp=='*')
			tp++;
		else
			strcpy(lastword,tp);
		if(strncasecmp(word,tp,GameHeader.WordLength)==0)
			return(lastword);
		n++;
	}
	return(NULL);
}

int MatchUpItem(char *text, int loc)
{
	char *word=MapSynonym(text);
	myct=0;
	
	if(word==NULL)
		word=text;
	
	while(myct<=GameHeader.NumItems)
	{
		if(Items[myct].AutoGet && Items[myct].Location==loc &&
			strncasecmp(Items[myct].AutoGet,word,GameHeader.WordLength)==0)
			return(myct);
		myct++;
	}
	return(-1);
}

char *ReadString(FILE *f)
{
	char tmp[1024];
	char *t;
	int c,nc;
	myct=0;
oops:	do
	{
		c=fgetc(f);
	}
	while(c!=EOF && isspace(c));
	if(c!='"')
	{
		//printf("\n#%s#\n",tmp);
		Fatal("Initial quote expected");
	}
	do
	{
		c=fgetc(f);
		if(c==EOF)
			Fatal("EOF in string");
		if(c=='"')
		{
			nc=fgetc(f);
			if(nc!='"')
			{
				ungetc(nc,f);
				break;
			}
		}
		if(c==0x60) 
			c='"'; /* pdd */
		tmp[myct++]=c;
	}
	while(1);
	tmp[myct]=0;
	t=MemAlloc(myct+1);
	memcpy(t,tmp,myct+1);
	return(t);
}

// loud = debug
void LoadDatabase(FILE *f)
{
	int ni,na,nw,nr,mc,pr,tr,wl,lt,mn,trm;
	int n;
	short lo;
	Action *ap;
	Room *rp;
	Item *ip;
/* Load the header */
	
#ifdef DEBUG
  printf ("Loading\n");
#endif
	//if(fscanf(f,"%*d %d %d %d %d %d %d %d %d %d %d %d",
	if(fscanf(f,"%*u %u %u %u %u %u %u %u %u %u %u %u",
		&ni,&na,&nw,&nr,&mc,&pr,&tr,&wl,&lt,&mn,&trm,&ldct)<10)
		Fatal("Invalid database(bad header)");
	GameHeader.NumItems=ni;
#ifdef DEBUG
  printf ("Number of items: %u\n",ni);
#endif
	Items=(Item *)MemAlloc(sizeof(Item)*(ni+1));
	GameHeader.NumActions=na;
#ifdef DEBUG
  printf ("Number of actions: %u\n",na);
#endif
	Actions=(Action *)MemAlloc(sizeof(Action)*(na+1));
	GameHeader.NumWords=nw;
	GameHeader.WordLength=wl;
#ifdef DEBUG
  printf ("%u words with a length of: %u\n",nw,wl);
#endif
	Verbs=(char **)MemAlloc(sizeof(char *)*(nw+1));
	Nouns=(char **)MemAlloc(sizeof(char *)*(nw+1));
	GameHeader.NumRooms=nr;
	Rooms=(Room *)MemAlloc(sizeof(Room)*(nr+1));
	GameHeader.MaxCarry=mc;
	GameHeader.PlayerRoom=pr;
	GameHeader.Treasures=tr;
	GameHeader.LightTime=lt;
	LightRefill=lt;
	GameHeader.NumMessages=mn;
#ifdef DEBUG
  printf ("%u messages\n",na);
#endif
	Messages=(char **)MemAlloc(sizeof(char *)*(mn+1));
	GameHeader.TreasureRoom=trm;
	
/* Load the actions */

	ldct=0;
	ap=Actions;
#ifdef DEBUG
		printf("Reading %u actions.\n",na);
#endif
	while(ldct<na+1)
	{
		//if(fscanf(f,"%hd %hd %hd %hd %hd %hd %hd %hd",
		if(fscanf(f,"%u %u %u %u %u %u %u %u",
			&ap->Vocab,
			&ap->Condition[0],
			&ap->Condition[1],
			&ap->Condition[2],
			&ap->Condition[3],
			&ap->Condition[4],
			&ap->Action[0],
			&ap->Action[1])!=8)
		{
			printf("Bad action line (%u)\n",ldct);
			exit(1);
		}
		ap++;
		ldct++;
	}			
	ldct=0;
#ifdef DEBUG
		printf("Reading %u word pairs.\n",nw);
#endif
	while(ldct<nw+1)
	{
		Verbs[ldct]=ReadString(f);
		Nouns[ldct]=ReadString(f);
		ldct++;
	}
	ldct=0;
	rp=Rooms;
#ifdef DEBUG
		printf("Reading %u rooms.\n",nr);
#endif
	while(ldct<nr+1)
	{
		//fscanf(f,"%hd %hd %hd %hd %hd %hd",
		fscanf(f,"%u %u %u %u %u %u",
			&rp->Exits[0],&rp->Exits[1],&rp->Exits[2],
			&rp->Exits[3],&rp->Exits[4],&rp->Exits[5]);
		rp->Text=ReadString(f);
		ldct++;
		rp++;
	}
	ldct=0;
#ifdef DEBUG
		printf("Reading %u messages.\n",mn);
#endif
	while(ldct<mn+1)
	{
		Messages[ldct]=ReadString(f);
		ldct++;
	}
	ldct=0;
#ifdef DEBUG
		printf("Reading %u items.\n",ni);
#endif
	ip=Items;
	while(ldct<ni+1)
	{
		ip->Text=ReadString(f);
		ip->AutoGet=strchr(ip->Text,'/');
		/* Some games use // to mean no auto get/drop word! */
		if(ip->AutoGet && strcmp(ip->AutoGet,"//") && strcmp(ip->AutoGet,"/*"))
		{
			char *t;
			*ip->AutoGet++=0;
			t=strchr(ip->AutoGet,'/');
			if(t!=NULL)
				*t=0;
		}
		//fscanf(f,"%hd",&lo);
		fscanf(f,"%d",&lo);
		ip->Location=(unsigned char)lo;
		ip->InitialLoc=ip->Location;
		ip++;
		ldct++;
	}
	ldct=0;
	/* Discard Comment Strings */
	while(ldct<na+1)
	{
		free(ReadString(f));
		ldct++;
	}
	//fscanf(f,"%d",&ldct);
	fscanf(f,"%u",&ldct);
#ifdef DEBUG
		//printf("Version %d.%02d of Adventure ",	ldct/100,ldct%100);
		printf("Version %u.%02u of Adventure ",	ldct/100,ldct%100);
#endif
	//fscanf(f,"%d",&ldct);
	fscanf(f,"%u",&ldct);
#ifdef DEBUG
		printf("%u.\nLoad Complete.\n\n",ldct);
#endif
}


void Look()
{
	static char *ExitNames[6]=
	{
		"North","South","East","West","Up","Down"
	};
	Room *r;
	int ct,f;
	int pos;
	
	//fputc(12,stdout);
	printf(TRS80_LINE);

	//wmove(Top,0,0);	/* Needed by some curses variants */
	if((BitFlags&(1<<DARKBIT)) && Items[LIGHT_SOURCE].Location!= CARRIED
	            && Items[LIGHT_SOURCE].Location!= MyLoc)
	{
		//if(Options&YOUARE)
		#ifdef YOUARE
			printf("You can't see. It is too dark!\n");
		#else
			printf("I can't see. It is too dark!\n");
		#endif
		//if (Options & TRS80_STYLE)
			printf(TRS80_LINE);
		//wrefresh(Top);
		return;
	}
	r=&Rooms[MyLoc];
	if(*r->Text=='*')
		//printf("%s\n",r->Text+1);
		printf("%s\n",r->Text+1);
	else
	{
		//if(Options&YOUARE)
		#ifdef YOUARE
			//printf("You are %s\n",r->Text);
			printf("You are in a %s\n",r->Text);
		#else
			//printf("I'm in a %s\n",r->Text);
			printf("I'm in a %s\n",r->Text);
		#endif
	}
	ct=0;
	f=0;
	printf("\nObvious exits: ");
	while(ct<6)
	{
		if(r->Exits[ct]!=0)
		{
			if(f==0)
				f=1;
			else
				printf(", ");
			printf ("%c[%um",27,32);
			printf("%s ",ExitNames[ct]);
			printf ("%c[%um",27,30);
		}
		ct++;
	}
	if(f==0)
		printf("none");
	printf(".\n");
	ct=0;
	f=0;
	pos=0;
	while(ct<=GameHeader.NumItems)
	{
		if(Items[ct].Location==MyLoc)
		{
			if(f==0)
			{
				//if(Options&YOUARE)
				#ifdef YOUARE
					printf("\nYou can also see: ");
				#else
					printf("\nI can also see: ");
				#endif
				pos=16;
				f++;
			}
			/*
			else if (!(Options & TRS80_STYLE))
			{
				printf(" - ");
				pos+=3;
			}
			*/
			if(pos+strlen(Items[ct].Text)>(Width-10))
			{
				pos=0;
				printf("\n");
			}
			printf ("%c[%um",27,34);
			printf("%s ",Items[ct].Text);
			printf ("%c[%um",27,30);
			pos += strlen(Items[ct].Text);
			//if (Options & TRS80_STYLE)
			//{
				printf(". ");
				pos+=2;
			//}
		}
		ct++;
	}
	printf("\n");
	//if (Options & TRS80_STYLE)
		printf(TRS80_LINE);
	//wrefresh(Top);
}

int WhichWord(char *word, char **list)
{
	int n=1;
	//char *tp;
	ne=1;
	
	while(ne<=GameHeader.NumWords)
	{
		tp=list[ne];
		if(*tp=='*')
			tp++;
		else
			n=ne;
		if(strncasecmp(word,tp,GameHeader.WordLength)==0)
			return(n);
		ne++;
	}
	return(-1);
}



void LineInput(char *buf)
{
	int pos=0;
	int ch;
	while(1)
	{
		//wrefresh(Bottom);
		//ch=wgetch(Bottom);
		//ch=getch();
		ch=fgetc_cons();
		switch(ch)
		{
			case 10:;
			case 13:;
				buf[pos]=0;
				buf[pos+1]=0;
				//scroll(Bottom);
				//wmove(Bottom,BottomHeight,0);
				fputc(13,stdout);
				return;
			case 8:;

			case 127:;

			case 12:;
				if(pos>0)
				{
					/*
					int y,x;
					getyx(Bottom,y,x);
					x--;
					if(x==-1)
					{
						x=Width-1;
						y--;
					}
					*/
					fputc(8,stdout);
					fputc(' ',stdout);
					fputc(8,stdout);
					//mvwaddch(Bottom,y,x,' ');
					//wmove(Bottom,y,x);
					//wrefresh(Bottom);
					pos--;
				}
				break;

			default:
				if(ch>=' '&&ch<=126)
				{
					buf[pos++]=ch;
					//waddch(Bottom,(char)ch);
					//wrefresh(Bottom);
					fputc((char)ch,stdout);
				}
				break;
		}
	}
}

void GetInput(vb,no)
int *vb,*no;
{
	char buf[256];
	char verb[10],noun[10];
	int nc;
	do
	{
		do
		{
			printf ("\nTell me what to do ? ");
			//wrefresh(Bottom);
			LineInput(buf);
			//OutReset();
			num=sscanf(buf,"%9s %9s",verb,noun);
		}
		while(num==0||*buf=='\n');
		if(num==1)
			*noun=0;
		if(*noun==0 && strlen(verb)==1)
		{
			switch(isupper(*verb)?tolower(*verb):*verb)
			{
				case 'n':strcpy(verb,"NORTH");break;
				case 'e':strcpy(verb,"EAST");break;
				case 's':strcpy(verb,"SOUTH");break;
				case 'w':strcpy(verb,"WEST");break;
				case 'u':strcpy(verb,"UP");break;
				case 'd':strcpy(verb,"DOWN");break;
				/* Brian Howarth interpreter also supports this */
				case 'i':strcpy(verb,"INVENTORY");break;
			}
		}
		nc=WhichWord(verb,Nouns);
		/* The Scott Adams system has a hack to avoid typing 'go' */
		if(nc>=1 && nc <=6)
		{
			vc=1;
		}
		else
		{
			vc=WhichWord(verb,Verbs);
			nc=WhichWord(noun,Nouns);
		}
		*vb=vc;
		*no=nc;
		if(vc==-1)
		{
			printf ("You use word(s) I don't know! ");
		}
	}
	while(vc==-1);
	strcpy(NounText,noun);	/* Needed by GET/DROP hack */
}

/*
void SaveGame()
{
	printf ("Save not implemented.\n");
}
*/

/*
void LoadGame(char *name)
{
	printf ("Load not implemented.\n");
}
*/

int PerformLine(int ct)
{
	int continuation=0;
	int param[5],pptr=0;
	int act[4];
	int cc=0;
	
	while(cc<5)
	{
		int cv,dv;
		cv=Actions[ct].Condition[cc];
		dv=cv/20;
		cv%=20;
		switch(cv)
		{
			case 0:
				param[pptr++]=dv;
				break;
			case 1:
				if(Items[dv].Location!=CARRIED)
					return(0);
				break;
			case 2:
				if(Items[dv].Location!=MyLoc)
					return(0);
				break;
			case 3:
				if(Items[dv].Location!=CARRIED&&
					Items[dv].Location!=MyLoc)
					return(0);
				break;
			case 4:
				if(MyLoc!=dv)
					return(0);
				break;
			case 5:
				if(Items[dv].Location==MyLoc)
					return(0);
				break;
			case 6:
				if(Items[dv].Location==CARRIED)
					return(0);
				break;
			case 7:
				if(MyLoc==dv)
					return(0);
				break;
			case 8:
				if((BitFlags&(1<<dv))==0)
					return(0);
				break;
			case 9:
				if(BitFlags&(1<<dv))
					return(0);
				break;
			case 10:
				if(CountCarried()==0)
					return(0);
				break;
			case 11:
				if(CountCarried())
					return(0);
				break;
			case 12:
				if(Items[dv].Location==CARRIED||Items[dv].Location==MyLoc)
					return(0);
				break;
			case 13:
				if(Items[dv].Location==0)
					return(0);
				break;
			case 14:
				if(Items[dv].Location)
					return(0);
				break;
			case 15:
				if(CurrentCounter>dv)
					return(0);
				break;
			case 16:
				if(CurrentCounter<=dv)
					return(0);
				break;
			case 17:
				if(Items[dv].Location!=Items[dv].InitialLoc)
					return(0);
				break;
			case 18:
				if(Items[dv].Location==Items[dv].InitialLoc)
					return(0);
				break;
			case 19:/* Only seen in Brian Howarth games so far */
				if(CurrentCounter!=dv)
					return(0);
				break;
		}
		cc++;
	}
	/* Actions */
	act[0]=Actions[ct].Action[0];
	act[2]=Actions[ct].Action[1];
	act[1]=act[0]%150;
	act[3]=act[2]%150;
	act[0]/=150;
	act[2]/=150;
	cc=0;
	pptr=0;
	while(cc<4)
	{
		if(act[cc]>=1 && act[cc]<52)
		{
			printf ("%s\n",Messages[act[cc]]);
		}
		else if(act[cc]>101)
		{
			printf ("%s\n",Messages[act[cc]-50]);
		}
		else switch(act[cc])
		{
			case 0:/* NOP */
				break;
			case 52:
				if(CountCarried()==GameHeader.MaxCarry)
				{
					//if(Options&YOUARE)
					#ifdef YOUARE
						printf ("You are carrying too much. ");
					#else
						printf ("I've too much to carry! ");
					#endif
					break;
				}
				if(Items[param[pptr]].Location==MyLoc)
					Redraw=1;
				Items[param[pptr++]].Location= CARRIED;
				break;
			case 53:
				Redraw=1;
				Items[param[pptr++]].Location=MyLoc;
				break;
			case 54:
				Redraw=1;
				MyLoc=param[pptr++];
				break;
			case 55:
				if(Items[param[pptr]].Location==MyLoc)
					Redraw=1;
				Items[param[pptr++]].Location=0;
				break;
			case 56:
				BitFlags|=1<<DARKBIT;
				break;
			case 57:
				BitFlags&=~(1<<DARKBIT);
				break;
			case 58:
				BitFlags|=(1<<param[pptr++]);
				break;
			case 59:
				if(Items[param[pptr]].Location==MyLoc)
					Redraw=1;
				Items[param[pptr++]].Location=0;
				break;
			case 60:
				BitFlags&=~(1<<param[pptr++]);
				break;
			case 61:
				//if(Options&YOUARE)
				#ifdef YOUARE
					printf ("You are dead.\n");
				#else
					printf ("I am dead.\n");
				#endif
				BitFlags&=~(1<<DARKBIT);
				MyLoc=GameHeader.NumRooms;/* It seems to be what the code says! */
				Look();
				break;
			case 62:
			{
				/* Bug fix for some systems - before it could get parameters wrong */
				int i=param[pptr++];
				Items[i].Location=param[pptr++];
				Redraw=1;
				break;
			}
			case 63:
doneit:				printf ("The game is now over.\n");
				//wrefresh(Bottom);
				//sleep(5);
				//endwin();
				exit(0);
			case 64:
				Look();
				break;
			case 65:
			{
				int cnt=0;
				int n=0;
				while(cnt<=GameHeader.NumItems)
				{
					if(Items[cnt].Location==GameHeader.TreasureRoom &&
					  *Items[cnt].Text=='*')
					  	n++;
					cnt++;
				}
				//if(Options&YOUARE)
				#ifdef YOUARE
					printf ("You have stored ");
				#else
					printf ("I've stored ");
				#endif
				printf("%u treasures.  On a scale of 0 to 100, that rates %u.\n",n,(n*100)/GameHeader.Treasures);
				if(n==GameHeader.Treasures)
				{
					printf ("Well done.\n");
					goto doneit;
				}
				break;
			}
			case 66:
			{
				int cnt=0;
				int f=0;
				//if(Options&YOUARE)
				#ifdef YOUARE
					printf ("You are carrying:\n");
				#else
					printf ("I'm carrying:\n");
				#endif
				while(cnt<=GameHeader.NumItems)
				{
					if(Items[cnt].Location==CARRIED)
					{
						if(f==1)
						{
							//if (Options & TRS80_STYLE)
								printf (". ");
							//else
							//	printf (" - ");
						}
						f=1;
						//printf ("%s",Items[cnt].Text);
						printf ("%c[%um",27,34);
						printf("%s ",Items[cnt].Text);
						printf ("%c[%um",27,30);

					}
					cnt++;
				}
				if(f==0)
					printf ("Nothing");
				printf (".\n");
				break;
			}
			case 67:
				BitFlags|=(1<<0);
				break;
			case 68:
				BitFlags&=~(1<<0);
				break;
			case 69:
				GameHeader.LightTime=LightRefill;
				if(Items[LIGHT_SOURCE].Location==MyLoc)
					Redraw=1;
				Items[LIGHT_SOURCE].Location=CARRIED;
				BitFlags&=~(1<<LIGHTOUTBIT);
				break;
			case 70:
				//printf(TRS80_LINE);
				//fputc(12,stdout);

				ClearScreen(); /* pdd. */
				//OutReset();
				break;
			case 71:
				//SaveGame();
				printf("Save not implemented");
				break;
			case 72:
			{
				int i1=param[pptr++];
				int i2=param[pptr++];
				int t=Items[i1].Location;
				if(t==MyLoc || Items[i2].Location==MyLoc)
					Redraw=1;
				Items[i1].Location=Items[i2].Location;
				Items[i2].Location=t;
				break;
			}
			case 73:
				continuation=1;
				break;
			case 74:
				if(Items[param[pptr]].Location==MyLoc)
					Redraw=1;
				Items[param[pptr++]].Location= CARRIED;
				break;
			case 75:
			{
				int i1,i2;
				i1=param[pptr++];
				i2=param[pptr++];
				if(Items[i1].Location==MyLoc)
					Redraw=1;
				Items[i1].Location=Items[i2].Location;
				if(Items[i2].Location==MyLoc)
					Redraw=1;
				break;
			}
			case 76:	/* Looking at adventure .. */
				Look();
				break;
			case 77:
				if(CurrentCounter>=0)
					CurrentCounter--;
				break;
			case 78:
				printf("%u ",CurrentCounter);
				break;
			case 79:
				CurrentCounter=param[pptr++];
				break;
			case 80:
			{
				int t=MyLoc;
				MyLoc=SavedRoom;
				SavedRoom=t;
				Redraw=1;
				break;
			}
			case 81:
			{
				/* This is somewhat guessed. Claymorgue always
				   seems to do select counter n, thing, select counter n,
				   but uses one value that always seems to exist. Trying
				   a few options I found this gave sane results on ageing */
				int t=param[pptr++];
				int c1=CurrentCounter;
				CurrentCounter=Counters[t];
				Counters[t]=c1;
				break;
			}
			case 82:
				CurrentCounter+=param[pptr++];
				break;
			case 83:
				CurrentCounter-=param[pptr++];
				if(CurrentCounter< -1)
					CurrentCounter= -1;
				/* Note: This seems to be needed. I don't yet
				   know if there is a maximum value to limit too */
				break;
			case 84:
				//printf ("%c[%um",27,31);
				printf ("%s",NounText);
				//printf ("%c[%um",27,30);
				break;
			case 85:
				//printf ("%c[%um",27,31); // ??? why does this line crash the compiler ?
				printf ("%s\n",NounText);
				//printf ("%c[%um",27,30); // ??? and why this one does the same ?
				break;
			case 86:
				printf ("\n");
				break;
			case 87:
			{
				/* Changed this to swap location<->roomflag[x]
				   not roomflag 0 and x */
				int p=param[pptr++];
				int sr=MyLoc;
				MyLoc=RoomSaved[p];
				RoomSaved[p]=sr;
				Redraw=1;
				break;
			}
			case 88:
				//wrefresh(Top);
				//wrefresh(Bottom);
				//sleep(2);	/* DOC's say 2 seconds. Spectrum times at 1.5 */
				// *** need to add a delay here
				break;
			case 89:
				pptr++;
				/* SAGA draw picture n */
				/* Spectrum Seas of Blood - start combat ? */
				/* Poking this into older spectrum games causes a crash */
				break;
			default:
				fprintf(stderr,"Unknown action %u [Param begins %u %u]\n",
					act[cc],param[pptr],param[pptr+1]);
				break;
		}
		cc++;
	}
	return(1+continuation);		
}


int PerformActions(int vb,int no)
{
	static int disable_sysfunc=0;	/* Recursion lock */
	int d=BitFlags&(1<<DARKBIT);
	
	int ct=0;
	int fl;
	int doagain=0;
	if(vb==1 && no == -1 )
	{
		printf ("Give me a direction too.");
		return(0);
	}
	if(vb==1 && no>=1 && no<=6)
	{
		int nl;
		if(Items[LIGHT_SOURCE].Location==MyLoc ||
		   Items[LIGHT_SOURCE].Location==CARRIED)
		   	d=0;
		if(d)
			printf ("Dangerous to move in the dark! ");
		nl=Rooms[MyLoc].Exits[no-1];
		if(nl!=0)
		{
			MyLoc=nl;
			Look();
			return(0);
		}
		if(d)
		{
			//if(Options&YOUARE)
			#ifdef YOUARE
				printf ("You fell down and broke your neck. ");
			#else
				printf ("I fell down and broke my neck. ");
			#endif
			//wrefresh(Bottom);
			//sleep(5);
			//endwin();
			exit(0);
		}
		//if(Options&YOUARE)
		#ifdef YOUARE
			printf ("You can't go in that direction. ");
		#else
			printf ("I can't go in that direction. ");
		#endif
		return(0);
	}
	fl= -1;
	while(ct<=GameHeader.NumActions)
	{
		int vv,nv;
		vv=Actions[ct].Vocab;
		/* Think this is now right. If a line we run has an action73
		   run all following lines with vocab of 0,0 */
		if(vb!=0 && (doagain&&vv!=0))
			break;
		/* Oops.. added this minor cockup fix 1.11 */
		if(vb!=0 && !doagain && fl== 0)
			break;
		nv=vv%150;
		vv/=150;
		if((vv==vb)||(doagain&&Actions[ct].Vocab==0))
		{
			if((vv==0 && RandomPercent(nv))||doagain||
				(vv!=0 && (nv==no||nv==0)))
			{
				int f2;
				if(fl== -1)
					fl= -2;
				if((f2=PerformLine(ct))>0)
				{
					/* ahah finally figured it out ! */
					fl=0;
					if(f2==2)
						doagain=1;
					if(vb!=0 && doagain==0)
						return;
				}
			}
		}
		ct++;
		if(Actions[ct].Vocab!=0)
			doagain=0;
	}
	if(fl!=0 && disable_sysfunc==0)
	{
		int i;
		if(Items[LIGHT_SOURCE].Location==MyLoc ||
		   Items[LIGHT_SOURCE].Location==CARRIED)
		   	d=0;
		if(vb==10 || vb==18)
		{
			/* Yes they really _are_ hardcoded values */
			if(vb==10)
			{
				if(strcasecmp(NounText,"ALL")==0)
				{
					int cnt=0;
					int f=0;
					
					if(d)
					{
						printf ("It is dark.\n");
						return 0;
					}
					while(cnt<=GameHeader.NumItems)
					{
						if(Items[cnt].Location==MyLoc && Items[cnt].AutoGet!=NULL && Items[cnt].AutoGet[0]!='*')
						{
							no=WhichWord(Items[cnt].AutoGet,Nouns);
							disable_sysfunc=1;	/* Don't recurse into auto get ! */
							PerformActions(vb,no);	/* Recursively check each items table code */
							disable_sysfunc=0;
							if(CountCarried()==GameHeader.MaxCarry)
							{
								//if(Options&YOUARE)
								#ifdef YOUARE
									printf ("You are carrying too much. ");
								#else
									printf ("I've too much to carry. ");
								#endif
								return(0);
							}
						 	Items[cnt].Location= CARRIED;
						 	Redraw=1;
							printf ("%c[%um",27,35);
							printf("%s",Items[cnt].Text);
						 	printf (": O.K. \n");
							printf ("%c[%um",27,30);
						 	f=1;
						 }
						 cnt++;
					}
					if(f==0)
						printf ("Nothing taken.");
					return(0);
				}
				if(no==-1)
				{
					printf ("What ? ");
					return(0);
				}
				if(CountCarried()==GameHeader.MaxCarry)
				{
					//if(Options&YOUARE)
					#ifdef YOUARE
						printf ("You are carrying too much. ");
					#else
						printf ("I've too much to carry. ");
					#endif
					return(0);
				}
				i=MatchUpItem(NounText,MyLoc);
				if(i==-1)
				{
					//if(Options&YOUARE)
					#ifdef YOUARE
						printf ("It is beyond your power to do that. ");
					#else
						printf ("It's beyond my power to do that. ");
					#endif
					return(0);
				}
				Items[i].Location= CARRIED;
				printf ("%c[%um",27,35);
				printf ("O.K. ");
				printf ("%c[%um",27,30);
				Redraw=1;
				return(0);
			}
			if(vb==18)
			{
				if(strcasecmp(NounText,"ALL")==0)
				{
					int cnt=0;
					int f=0;
					while(cnt<=GameHeader.NumItems)
					{
						if(Items[cnt].Location==CARRIED && Items[cnt].AutoGet && Items[cnt].AutoGet[0]!='*')
						{
							no=WhichWord(Items[cnt].AutoGet,Nouns);
							disable_sysfunc=1;
							PerformActions(vb,no);
							disable_sysfunc=0;
							Items[cnt].Location=MyLoc;

							printf ("%c[%um",27,35);
							printf("%s",Items[cnt].Text);
						 	printf (": O.K. \n");
							printf ("%c[%um",27,30);
							Redraw=1;
							f=1;
						}
						cnt++;
					}
					if(f==0)
						printf ("Nothing dropped.\n");
					return(0);
				}
				if(no==-1)
				{
					printf ("What ? ");
					return(0);
				}
				i=MatchUpItem(NounText,CARRIED);
				if(i==-1)
				{
					//if(Options&YOUARE)
					#ifdef YOUARE
						printf ("It's beyond your power to do that.\n");
					#else
						printf ("It's beyond my power to do that.\n");
					#endif
					return(0);
				}
				Items[i].Location=MyLoc;
				printf ("%c[%um",27,35);
				printf ("O.K. ");
				printf ("%c[%um",27,30);
				Redraw=1;
				return(0);
			}
		}
	}
	return(fl);
}
	
void main(int argc, char *argv[])
{
	char buf[10];
	char numgame[10];
	char fname[20];
	FILE *f;
	int vb,no;

#ifdef __Z88DK__
	heapinit(HPSIZE);
#endif

	//printf ("%c[2J",27);
	ClearScreen();
	do
	{
		printf ("\nWhich adventure do you wish to play (00-17) ?  ");
		LineInput(buf);
		num=sscanf(buf,"%9s",numgame);
		//sprintf(fname, "adv%s.dat", numgame);
		sprintf(fname, "adv%s", numgame);
	//	printf ("\n%s\n",fname);
	//	f=fopen(argv[1],"r");
	//	f=fopen("adv01.dat","r");
		f=fopen(fname,"r");
	}
	while (f == NULL);
	
	//Width = 64;
	Width = 51;

/*
	if (Options & TRS80_STYLE)
	{
		Width = 64;
		TopHeight = 11;
		BottomHeight = 13;
	}
	else
	{
		Width = 80;
		TopHeight = 10;
		BottomHeight = 14;
	}
*/

/*
	DisplayUp=1;
	initscr();
	Top=newwin(TopHeight,Width,0,0);
	Bottom=newwin(BottomHeight,Width,TopHeight,0);
	scrollok(Bottom,TRUE);
	leaveok(Top,TRUE);
	leaveok(Bottom,FALSE);
	idlok(Bottom,TRUE);
	noecho();
	cbreak();
	wmove(Bottom,BottomHeight-1,0);
	//OutReset();
*/
	//fputc(12,stdout);
	printf ("%c[2J",27);
	printf ("\
Scott Free, A Scott Adams game driver in C.\n\
Release 1.14, (c) 1993,1994,1995 \n\
Swansea University Computer Society.\n\
Distributed under the GNU software license\n\n\
Z88DK port v1.0 by Stefano Bodrato\n\n");

	LoadDatabase(f);
	fclose(f);
/*
	if(argc==3)
		LoadGame(argv[2]);
*/
	//srand(time(NULL)^getpid()^getuid());
	//srand(time(NULL)^getpid());
	Look();
	while(1)
	{
		if(Redraw!=0)
		{
			Look();
			Redraw=0;
		}
		PerformActions(0,0);
		if(Redraw!=0)
		{
			Look();
			Redraw=0;
		}
		GetInput(&vb,&no);
		switch(PerformActions(vb,no))
		{
			case -1:printf ("I don't understand your command. ");
				break;
			case -2:printf ("I can't do that yet. ");
				break;
		}
		/* Brian Howarth games seem to use -1 for forever */
		if(Items[LIGHT_SOURCE].Location/*==-1*/!=DESTROYED && GameHeader.LightTime!= -1)
		{
			GameHeader.LightTime--;
			if(GameHeader.LightTime<1)
			{
				BitFlags|=(1<<LIGHTOUTBIT);
				if(Items[LIGHT_SOURCE].Location==CARRIED ||
					Items[LIGHT_SOURCE].Location==MyLoc)
				{
					if(Options&SCOTTLIGHT)
						printf ("Light has run out! ");
					else
						printf ("Your light has run out. ");
				}
				if(Options&PREHISTORIC_LAMP)
					Items[LIGHT_SOURCE].Location=DESTROYED;
			}
			else if(GameHeader.LightTime<25)
			{
				if(Items[LIGHT_SOURCE].Location==CARRIED ||
					Items[LIGHT_SOURCE].Location==MyLoc)
				{
			
					if(Options&SCOTTLIGHT)
					{
						printf ("Light runs out in ");
						printf ("%u ", GameHeader.LightTime);
						printf (" turns. ");
					}
					else
					{
						if(GameHeader.LightTime%5==0)
							printf ("Your light is growing dim. ");
					}
				}
			}
		}
	}
}
