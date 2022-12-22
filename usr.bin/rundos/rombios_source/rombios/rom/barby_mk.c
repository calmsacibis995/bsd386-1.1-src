//char date_buffer[9];
//_strdate(date_buffer);
//MODEL: cl /AC /Zp
/*page 90,132
;public  EXE_H,TablOff                                                                            
k_data 	segment	public para 'CODE'
assume  cs:k_data

;               Standard EXE-format files begin with this header.             
;		Size        Offset  Contents                                                  
EXE_H	db	'MZ'	    ;0   .EXE file signature ('MZ')                              
PartPag	dw	0   ;2   length of partial page at end (generally ignored)       
PageCnt	dw	0   ;4   length of image in 512-byte pages, including the header 
ReloCnt	dw	0   ;6   number of items in relocation table                     
HdrSize	dw	0   ;8   size of header in 16-byte paragraphs                    
MinMem 	dw	0   ;0a  minimum memory needed above end of program (paragraphs) 
MaxMem 	dw	0   ;0c  maximum memory needed above end of program (paragraphs) 
ReloSS 	dw	0   ;0e  segment offset of stack segment (for setting SS)        
ExeSP  	dw	0   ;10  value for SP register (stack pointer) when started      
ChkSum 	dw	0   ;12  file checksum (negative sum of all words in file)       
ExeIP  	dw	0   ;14  value for IP register (instruction pointer) when started
ReloCS 	dw	0   ;16  segment offset of code segment (for setting CS)         
TablOff	dw	0   ;18  file-offset of first relocation item (often 001cH)      
Overlay	dw	0   ;1a  overlay number (0 for base module)                      
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	but we have RLD_TABLE at 001eh !!!!!
RLD_TABLE	label dword	

;         Formatted portion of EXE header -  Relocation table.
;	 Starts at file offset [EXE+18H]. 
;	 Has [EXE+6] DWORD entries.
;            ÚÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÂ Ä Ä ÂÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄ¿
;+ ?     4*? ³offset  segment³  ³offset  segment³
;            ÀÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁ Ä Ä ÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÙ
;+ ?     ?   filler to a paragraph boundary                                    
;+ ?     ?   start of program image                                            
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



begrld db       2024 dup(0)	 ;RLD elements

k_data	ends
end
*/
/* This program is exe2bin for long EXE-files
	The 1st parameter must be a source file
	name. Output file will have .BIN type.
	The 2nd parameter is the file's loading address.
*/

#include <stdio.h>
#include <io.h>
#include <dos.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>

typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned int RELBASE;
uint	firstl,lastl,lenl;

void ffwrite ();
void errprint ();
char huge * spt;
_segment segh;	//const
_segment segim; //const
_segment segr;	 //from 0 ????
uint cur_off_rel,cur_seg_rel;
RELBASE  _based(segr) * pbase;
RELBASE _based(segim) * pimage;
typedef struct	{uint off_rld,seg_rld;}RLD_elm;
RLD_elm cur_rld;
typedef struct	{	      //Offset  Contents                                                
	unsigned exe;		//0   .EXE file signature ('MZ')                              
	unsigned PartPag;	//2   length of partial page at end (generally ignored)       
	unsigned PageCnt;	//4   length of image in 512-byte pages, including the header &last page
	unsigned ReloCnt;	//6   number of items in relocation table                     
	unsigned HdrSize;	//8   size of header in 16-byte paragraphs                    
	unsigned min_plus16;	//0a  minimum memory needed above end of program (paragraphs) 
	unsigned max_plus16;	//0c  maximum memory needed above end of program (paragraphs) 
	unsigned place_ss;	//0e  segment offset of stack segment (for setting SS)        
	unsigned of_sp;		//10  value for SP register (stack pointer) when started      
	unsigned check;		//12  file checksum (negative sum of all words in file)       
	unsigned ip;		//14  value for IP register (instruction pointer) when started
	unsigned place_cod;	//16  segment offset of code segment (for setting CS)         
//unsigned relstart;		//18  file-offset of first relocation item (often 001cH)      
	RLD_elm  (_based(segh) * rlds)[]; 
	unsigned overl;		//1a  overlay number (0 for base module)                      

		} HEADER;


char huge * to_image;
HEADER far *hedd;
HEADER  _based(segh)*phed;
uint n=0xf000,d,n2;
HEADER sthdr;
HEADER *psthdr=&sthdr;			
int ls;

main (argc, argv)
int argc;
char *argv[];
{
	FILE * fds, * fdd, * fdt;
	char fname[64], * dotp, * tp;
	char fname0[64];
	ulong flen;		//len of source from disk 
	ulong flen_hdr;		//len of source from EXE-header
	uint i,len_file_para,xvost_byte;
//	printf("\nargc=%d\n",argc);
	if ( argc < 2 )
	   {printf("\nbarby_mk file-name rom-offs [rom-seg]\n\n");exit(0);}
	if ( argc < 3 )
		errprint ("Bad parameters\n");
//	printf("\nargv[2]=%s\n",argv[2]);
	if ( sscanf (argv[2], "%x", &d) < 1 )
		errprint ("Bad number\n");
	if (argc == 4)	
		if ( sscanf (argv[3], "%x", &n) < 1 )
						errprint ("Bad number\n");
	else
		n = 0xf000;

	strcpy (fname0, argv[1]);
	dotp = strchr (fname0, '.');
	if ( dotp == NULL ) {
		dotp = & fname0 [strlen(fname0)];
		*dotp = '.';
		*(dotp+1) = '\0';
	strcpy (dotp+1, "exe");
	}
	if ( (fds = fopen ( fname0, "rb")) == NULL )
		errprint ("No source file\n");
	strcpy (fname, argv[1]);
	dotp = strchr (fname, '.');
	if ( dotp == NULL ) {
		dotp = & fname [strlen(fname)];
		*dotp = '.';
		*(dotp+1) = '\0';
	}
	strcpy (dotp+1, "bil");
	if ( (fdd = fopen (fname, "wb")) == NULL )
		errprint ("Can`t open .bil\n");
//===================================================
	if ( fread (psthdr,1, sizeof (HEADER), fds) < sizeof (HEADER) )
		errprint ("Bad source\n");


	if ( (flen = filelength ( fileno (fds))) == -1L)
		errprint ("Bad handle\n");

	if (sthdr.PartPag==0 )
		flen_hdr=512L*(ulong)(sthdr.PageCnt);
	else
		flen_hdr=512L*(ulong)(sthdr.PageCnt-1)+(ulong)sthdr.PartPag;

	if ( flen_hdr>flen)
		errprint ("Source shorter than it's size in header\n");
/*
	if ( flen_hdr==flen)
		printf("\nEXE-file has not CV-infomation\n");
	else
		printf("\nEXE-file has CV-infomation %ld bytes\n",flen-flen_hdr);
*/
	len_file_para=(flen_hdr)/16; //all file
	xvost_byte=(flen_hdr)%16; //all file
	if ( (spt = halloc (flen_hdr/16+1 , 16)) == NULL )
		errprint ("Not enough memory\n");
	hedd = (HEADER *) spt;

	ls=fseek(fds, 0L,SEEK_SET);


	if(len_file_para!=0)
		{int resread;
		 resread=fread ((char huge *)spt, 16,len_file_para, fds);
		 if (resread< len_file_para)
			errprint ("Bad Source (1)\n");
		spt=spt+16*(ulong)len_file_para;
		}
	else  errprint ("Bad Source (3)\n");
	if(xvost_byte!=0)
		{int resread;
		resread=fread ((char huge *)spt,1,xvost_byte, fds);
		if (resread< xvost_byte)
			errprint ("Bad Source(2)\n");
		}

	segh=FP_SEG(hedd)+FP_OFF(hedd)/16;
	FP_OFF(phed)=0;

	to_image =((char *)phed+ phed->HdrSize * 16);

	segim=FP_SEG(to_image)+FP_OFF(to_image)/16;
	FP_OFF(pimage)=0;
	{
	uint	first,last,len;
	lastl =	*(uint *)(to_image+4);
	firstl =	*(uint *)(to_image+6);
	lenl	=  lastl - firstl;
	last =	*(uint *)to_image;
	first =	*(uint *)(to_image+2);
	len	=  (last<<4) - first;

	printf("\nLength all segments (excl. _TEXT) %.4x bytes",len);
	printf("\nReserved Room       %.4x - %.4x  (%.4x bytes)",
					   firstl,lastl,lenl);
	if(len>=lenl)
	   errprint("\nError:No enough room for segments.\nChange orgs in nbarz.asm\n");
	else
	   {
	   printf("\nSegments  start at %.4x:%.4x\n",n,firstl);
	   printf("\nROM-image  starts at %.4x:%.4x\n",n,d);
	   }
	}
//pbase is pointer to reloc base in body  (pimage)
//spt is pointer to begin of file

if(phed->ReloCnt!=0)
	{
	for (i=0; i<phed->ReloCnt; i++)
		{
		cur_rld=(* phed->rlds)[i];
//typedef struct	{uint off_rld,seg_rld;}RLD_elm;
		segr=cur_rld.seg_rld + segim;
		pbase=(RELBASE  _based(segr) * )cur_rld.off_rld;
		if (*pbase==0)
				*pbase += n;
		else
//				*pbase += n2 - 0x1000;
				*pbase += n - 0x1000;
		   		
		}
	}
else	printf("\nWARNING: no RLD-information!\n");
_strdate((char huge *)(to_image+0xfff5));
printf("\nROM vers: %s\n\n is created\n",(char huge *)(to_image+0xfff5));
_fmemmove((char huge *)(to_image+firstl),
				(char huge *)(to_image+0x10000+firstl),lenl);
	ffwrite ((char huge *)(to_image+d), 16,0x1000-(d>>4),fdd);

	exit (0);

}

void ffwrite (buf, size, count, stream)
char huge * buf;
int size, count;
FILE * stream;
{
	int n;
	
	if ( (n = fwrite (buf, size, count, stream))	!= count )
		errprint ("Write error\n");

}

void errprint (msg)
char * msg;
{
	printf ("%s", msg);
	exit (1);
}


