# Design and Implementation of a Configurable Logging and Debugging System Using C Preprocessor Directives
/*
   1. Header file inclusion
   2. Comment removal
   3. Macro replacement without arguments

   Name : Sindageri Uday Kumar
   ID   : V25BE10S6
*/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>

typedef struct macro{
	char macro_name[20];
	char macro_body[50];
}macro;

void userdefined(char *,FILE*);
void predefined(char *);
void single_line_comment(char *);
void multi_line_comment(char *);
void macro_replace(int );
void file(char *,char *,char *);

char pdupli[20][30],udupli[20][30],file_i[20];
int dh=0,dh1=0;
macro mro[20];

int main(int argc,char **argv){
	if(argc!=2){
		printf("USAGE:./a.out File_name\n");
		return 0;
	}

	int i,j,k,c,n,l1,m=0;
	char *p1,*q,*q1;

	strcpy(file_i,argv[1]);
	c=strlen(file_i);
	file_i[c-1]='i';

	FILE *fp=fopen(argv[1],"r");

	if(fp==0){
		printf("%s file not found\n",argv[1]);
		return 0;
	}
	fseek(fp,0,2);
	c=ftell(fp);
	rewind(fp);

	char s[c+1],ch,fi[20],s1[20]="/usr/include/",s2[20];

	n=0;
	while(fgets(s,c,fp))
		n++;
	rewind(fp);

	FILE *fp1=fopen("temp.c","w");
	FILE *fp2=fopen(file_i,"w");             
	//fclose(fp2);

	//-------------------header file inclusion-----------------------------
	char **p=malloc(sizeof(char *)*n);
	for(i=0;i<n;i++)
		p[i]=malloc(c+1);

	rewind(fp);
	for(i=0;i<n;i++)
		fgets(p[i],c,fp);

	for(i=0,m=0;i<n;i++){
		strcpy(s2,s1);
		if(p1=strstr(p[i],"#include")){
			if((q=strchr(p1+1,'<'))&&(q1=strchr(q+1,'>'))){
				file(q+1,q1-1,fi);
				strcat(s2,fi);
				//printf("%s\n",s2);
				predefined(s2);	
				strcpy(pdupli[dh],s2);
				dh++;
			}	

			else if((q=strchr(p1+1,'"'))&&(q1=strchr(q+1,'"'))){	
				file(q+1,q1-1,fi);
				userdefined(fi,fp1);	
				strcpy(udupli[dh],fi);			
				dh1++;
			}	
		}

		else if(p1=strstr(p[i],"#define")){
			if((q=strchr(p1+1,' '))&&(q1=strchr(q+1,' '))){
				file(q+1,q1-1,mro[m].macro_name);
				strcpy(mro[m].macro_body,q1+1);
				l1=strlen(mro[m].macro_body);	
				mro[m].macro_body[l1-1]=mro[m].macro_body[l1];	
				m++;
			}
		}
		else
			fputs(p[i],fp1);	
	}
	//--------------Comment remove  --------------------------

	rewind(fp1);
	i=0;
	while((ch=fgetc(fp1))!=EOF){
		s[i]=ch;
		i++;
	}
	s[i]='\0';

	multi_line_comment(s);
	single_line_comment(s);	
	fputs(s,fp1);
	rewind(fp1);

	//-----------macro replace---------------------
	macro_replace(m);

	fclose(fp1);
	fclose(fp2);

	//-----------------create .i file------------------

	FILE *f1=fopen("temp.c","r");
	FILE *f2=fopen(file_i,"a+");

	fseek(f1,0,2);
	c=ftell(f1);

	rewind(f1);
	char f[c+1],ch2;

	i=0;
	while((ch2=fgetc(f1))!=EOF){
		f[i]=ch2;
		i++;
	}
	f[i]='\0';	
	fputs(f,f2);

	fclose(f1);
	fclose(f2);
	printf("---:Your %s file created:---\n",file_i);
	return 0;
}

void predefined(char *s1){

	int c,i,j;
	char ch;

	for(j=0;j<dh;j++){
		if((strcmp(pdupli[j],s1))==0)
			break;
	}
	if(j==dh){
		FILE *fp=fopen(s1,"r");
		fseek(fp,0,SEEK_END);
		c=ftell(fp);

		char s[c+1];
		rewind(fp);

		i=0;
		while((ch=fgetc(fp))!=EOF){
			s[i]=ch;
			i++;
		}
		s[i]='\0';
		FILE *fp1=fopen(file_i,"a");
		fputs(s,fp1);
		fclose(fp1);
	}

}

void userdefined(char *s3,FILE *fp1){
	int c,i,j,n;
	char ch,*p1,*q,*q1;

	for(j=0;j<dh1;j++){
		if((strcmp(udupli[j],s3))==0)
			break;
	}
	if(j==dh1){

		FILE *fp=fopen(s3,"r+");

		fseek(fp,0,SEEK_END);
		c=ftell(fp);
		rewind(fp);

		char s[c+1],fi[20],s1[20]="/usr/include/",s2[20];

		n=0;
		while(fgets(s,c,fp))
			n++;	

		char **p=malloc(sizeof(char *)*n);
		for(i=0;i<n;i++)
			p[i]=malloc(c+1);

		rewind(fp);
		for(i=0;i<n;i++)
			fgets(p[i],c,fp);

		for(i=0;i<n;i++){
			strcpy(s2,s1);
			if(p1=strstr(p[i],"#include")){
				if((q=strchr(p1+1,'<'))&&(q1=strchr(q+1,'>'))){
					file(q+1,q1-1,fi);
					strcat(s2,fi);
					//printf("%s\n",s2);
					predefined(s2);	
					strcpy(pdupli[dh],s2);
					dh++;
				}	
			}

			else
				fputs(p[i],fp1);	

		}
	}
}


void file(char *q,char *q1,char *f){

	while(q<=q1){
		*f=*q;
		f++,q++;
	}
	*f='\0';
}

void multi_line_comment(char *p){
	char *q=0,*q1=0;

	while((q=strstr(p,"/*")) && (q1=strstr(q+1,"*/"))){
		while(q<=q1+1){ 
			if((*q)=='\n'&& q++)
				continue ;
			*q=' ';
			q++;
		}
		p=q1+1;
	}	
}


void single_line_comment(char *p){
	char *q=0,*q1=0;
	int i=0;

	while((q=strstr(p,"//")) && (q1=strchr(q+1,'\n'))){
		while(q<q1){
			*q=' ';
			q++;
		}
	}
	p=q1+1;
}

void macro_replace(int j){

	int c,i,r,n,k,l,l1,l2;
	char ch,*q,*q1,*q2;

	FILE *fp1=fopen("temp.c","r");

	fseek(fp1,0,SEEK_END);
	c=ftell(fp1);
	rewind(fp1);

	char s[c+1];
	n=0;
	while(fgets(s,c,fp1))
		n++;	

	char **p=malloc(sizeof(char *)*n);
	for(i=0;i<n;i++)
		p[i]=malloc(c+1);

	rewind(fp1);
	for(i=0;i<n;i++)
		fgets(p[i],c,fp1);

	for(i=0;i<n;i++){
		q=p[i];
		for(k=0;k<j;k++){
			while(q1=strstr(q,mro[k].macro_name)){
				l=strlen(q1);
				l1=strlen(mro[k].macro_body);
				l2=strlen(mro[k].macro_name);
				strcpy(q1,q1+l2);	

				r=l;			
				while(r>=0){
					q1[r+l1]=q1[r];
					r--;
				}
				for(r=0; mro[k].macro_body[r]!='\0' ;r++)
					q1[r]=mro[k].macro_body[r];

				q=q1+l1;
			}			
		}
	}

	fp1=fopen("temp.c","w");
	for(i=0;i<n;i++)
		fputs(p[i],fp1);

	fclose(fp1);
}
