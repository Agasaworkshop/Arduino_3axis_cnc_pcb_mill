#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void convert_line(char[],FILE*);
int cerca_stringa(char, char[]);
void copia_float(char[],FILE*);

int main(int argc,char*argv[]) {
	char path[30];
	FILE*fin;
	FILE*fout;
	char line[200];
	printf("Percorso file: \n");
	scanf("%s", path);
	fin = fopen(path, "r");
	fout = fopen("./route.cns", "w"); 
	if (!fin) {		//controllo apertura file ingresso
		printf("Problema con file di ingresso");
		return 1;
	}
	if (!fout) {	//controllo apertura file uscita
		printf("Problema con file di uscita");
		return 1;
	}
	fgets(line, 200, fin);
	while(!feof(fin)) {
		convert_line(line, fout);
		fgets(line, 200, fin); 
	}
	fclose(fin);
	fclose(fout);
	return 0; 
}

void convert_line(char line[], FILE*fout) {
	int command_n;
	int pos_x;
	int pos_y;
	int pos_z;
	command_n = 0;
	if (strlen(line) >= 3) {
		if ((line[0] != 'G' && line[0] != 'M'))
			return;
		if (line[1] < '0' || line[1] > '9' || line[2] < '0' || line[2] > '9')
			return;
	if (line[0] == 'G' && line[1] == '0' && (line[2] == '1' || line[2] == '0')) {	//G1 e G0 spostamento
		pos_x = cerca_stringa('X', line);
		if(pos_x > 0)
			command_n = command_n +1;
		pos_y = cerca_stringa('Y', line);
		if(pos_y> 0)
			command_n = command_n +10;
		pos_z = cerca_stringa('Z', line);
		if(pos_z> 0)
			command_n = command_n +100;
		if((command_n != 0) && (command_n != 100)) {
			fprintf(fout, "%c ", 'A');
			fprintf(fout, "%d ", command_n);
			if (pos_x >= 0) 
				copia_float(&line[pos_x], fout);
			if (pos_y >= 0)
				copia_float(&line[pos_y], fout);
			if (pos_z >= 0)
				copia_float(&line[pos_z], fout);
			}else if (command_n == 100){
				if (cerca_stringa('-', line) >= 0) {
/*					fputs("C 3 \n",fout); */
					fprintf(fout,"%s", "C 3 \n");
					fprintf(fout,"%s", "C 100 \n");
					fprintf(fout,"%s", "C 4 \n");
					fprintf(fout,"%s", "C 2 \n");
					fprintf(fout, "%c ", 'A');
					fprintf(fout, "%d ", command_n);
					copia_float(&line[pos_z], fout);
					}else {
					fprintf(fout, "%c ", 'A');
					fprintf(fout, "%d ", command_n);
					copia_float(&line[pos_z], fout);
					}	
			}
			if (command_n != 0)
			fprintf(fout, "%c", '\n');
	}
	if (line[0] == 'G' && line[1] == '2' && line[2] == '8') {	//G28 (auto-home)
		fprintf(fout, "%c ", 'C');
		if (cerca_stringa('X', line) >= 0)
			command_n= command_n+1;
		if (cerca_stringa('Y',line) >= 0)
			command_n = command_n + 10;
		if (cerca_stringa('Z',line) >= 0)
			command_n = command_n + 100;
		if (command_n == 0)
			command_n = 111;
		fprintf(fout,"%d ", command_n);
		fprintf(fout, "%c", 'n');
	}
	if (line[0] == 'M' && line[1] == '0' && line[2] == '3') {	//M3 (accendi motore)
		fprintf(fout, "%s", "C 2 \n");
	}
	if (line[0] == 'M' && line[1] == '0' && line[2] == '5') {	//M5 (spegni motore)
		fprintf(fout, "%s", "C 3 \n");
	}
	}
	return;
}

int cerca_stringa(char car, char line[]) {
	int i; 
	i = 0;
	while(line[i] != '\0') {
		if (line[i] ==  car)
			return i;
		i++;
	}
	return -1;
	}
		
void copia_float(char line[], FILE*fout) {
	int i;
	i = 1;
	while((line[i] <= '9' && line[i] >='0') || (line[i] == '.') || (line[i] == '-')) {
		fprintf(fout, "%c", line[i]);
		i++;
	}
	fprintf(fout, "%c",' ');
	return;
}