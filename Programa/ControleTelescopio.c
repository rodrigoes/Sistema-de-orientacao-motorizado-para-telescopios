/**************************************************************
* Programa: SISTEMA DE ORIENTAÇÃO MOTORIZADO PARA TELESCÓPIOS *                             
* Autor: CAINÃ ANTUNES SILVA                                  *
**************************************************************/

#include <18f4550.h>
#define PASSOS_MOTOR 2048 // Quantidade de passos do motor

#fuses hs,nowdt,noprotect,put,nobrownout,nolvp,nocpd,PLL5,CPUDIV1 

#use delay(crystal=20Mhz,clock=48MHz)

#include <LCDKIT.c> // biblioteca do Kit 
#include <math.h>

#use fast_io(a)
#use fast_io(b)
#use fast_io(c)
#use fast_io(d)
#use fast_io(e)

//endereços dos ports do 18f4550
#byte porta=0xf80
#byte portb=0xf81
#byte portc=0xf82
#byte portd=0xf83
#byte porte=0xf84

//Entradas da Matriz
#bit linha1 = portb.4
#bit linha2 = portb.5
#bit linha3 = portb.6
#bit linha4 = portb.7

//Saidas da Matriz
#bit coluna1 = porta.1
#bit coluna2 = porta.2
#bit coluna3 = porta.3
//Saídas motores
#bit boba = porte.0
#bit bobb = porte.1
#bit bobc = porte.2
#bit bobd = portc.0


void telaInicial(void);			// Tela de apresentação.
void telaPolo(void);			// Permiti indicar com qual polo o telescopio esta alinhado.
void alinhamentoInicial(void);	// Força o usuário verificar o alinhamento inicial do telescópio antes de iniciar.
void menu(void);				// Permite escolher entre os modos de operação manual ou automático.
void manual(void);				// Permite posicionar o telescópio através das teclas de navegação da matriz de botões.
void automatico(void);			
void desabilitaMatriz(void);	// Bloqueia a leitura da matriz de botões.
int varreMatriz(void);			// Realiza varredura na matriz de botões retornando o valor da tecla pressionada.
int varreTecladoManual(void);
void motorAR_antih(int);
void motorAR_h(int);
int extraiDezena(int num);
int extraiUnidade(int num,int dezena);
void exibirAH(void);
void inverterAH(void);
void telaAcompanhamento(void);

//double vel_rotacao=

int leitura=0;		// Armazena o valor retornado pela leitura da matriz de botão.
int polo=0;			// Indica a qual polo celeste o telescopio esta alinhado: 1=Norte 2=Sul.
int vel=5;			// Velocidade de movimentação dos eixos.
char display='0';	// usado para converter inteiro em char para exibição no display.
double AH;			// Armazena o ângulo horário do telescópio.
double DEC;			// Armazena a declinação do telescópio.
double AH_obj;		// Armazena o ângulo horário do objeto. 
double DEC_obj;		// Armazena a declinação do objeto.	 

double passo_AH=0.16666667/PASSOS_MOTOR; 	// Define a variação do ângulo horário para cada passo do motor.
double passo_DEC=0.001953125;//4/PASSOS_MOTOR;		// Define a variação da declinação para cada passo do motor.

const double diaSideral=23+56/60+4/3600;				// Tempo  para uma estrela cruzar duas vezes consecultivas o meridiano local (em horas).
int velRotacao=292;//(diaSideral/(PASSOS_MOTOR*144))*3600000;	// Velocidade do movimento de compesação da rotação da terra.
int DEC_sentido = 1;


void main()
{
	port_b_pullups(TRUE); // pullup interno portb habilitado
    
	//Define Pinos de entrada e saída
	set_tris_a(0b11110001);
    set_tris_b(0b11111111);
    set_tris_c(0b11111110);         
    set_tris_d(0b11111111);
    set_tris_e(0b00000000);

	// zera todos os ports
    porta=0; 
    portb=0;
    portc=0;
    portd=0;
    porte=0;
    
    lcd_init();
	desabilitaMatriz();
	telaInicial();
	telaPolo();
	alinhamentoInicial();
	
	menu:
	menu();
	telaAcompanhamento();	
   	while(true)
   	{
		motorAR_h(velRotacao);
		leitura=varreMatriz();
		if(leitura==10)
			goto menu;
   	}
}


void telaInicial(void)
{
	int t=100;
	printf(lcd_putc,"\f");
    lcd_gotoxy (1,1);
    printf(lcd_putc,"TelescopeControl");
    delay_ms(5);
    lcd_gotoxy (4,2);
    printf(lcd_putc,"Aguarde");
	delay_ms(t);
	printf(lcd_putc,".");
	delay_ms(t);
	printf(lcd_putc,".");
	delay_ms(t);
	printf(lcd_putc,".");
	delay_ms(t);
}

void telaPolo(void)
{
	inicioPolo:

	printf(lcd_putc,"\f");
    lcd_gotoxy (1,1);
    printf(lcd_putc,"Escolha um polo:");
    delay_ms(5);
    lcd_gotoxy (1,2);
    printf(lcd_putc,"Norte(1)  Sul(2)");
	delay_ms(5);	
	
	while(leitura!=1 && leitura!=2)
		leitura = 2;//varreMatriz();
	
	printf(lcd_putc,"\f");
	lcd_gotoxy (1,1);
    printf(lcd_putc," Confirmar polo ");
    delay_ms(5);
	
	if(leitura==1)
	{
		leitura=0;
	    lcd_gotoxy (1,2);
	    printf(lcd_putc,"     Norte?     ");
		delay_ms(5);
		polo=1;
		AH=18;
		DEC=90;
	}else{
		leitura=0;
	    lcd_gotoxy (1,2);
	    printf(lcd_putc,"      Sul?      ");
		delay_ms(5);
		polo=2;
		AH=6;
		DEC=-90;
	}

	while(leitura!=10 && leitura!=11)
		leitura = varreMatriz();
	if(leitura == 10)
		goto inicioPolo;

	leitura = 0;
}

void alinhamentoInicial(void)
{
	printf(lcd_putc,"\f");
	lcd_gotoxy (1,1);
    printf(lcd_putc,"Alinhamento  OK?");
    delay_ms(5);
	if (polo==1)
	{
		lcd_gotoxy (1,2);
	    printf(lcd_putc,"AH=18h DEC=90deg");
		delay_ms(5);
	}
	else if (polo==2)
	{
		lcd_gotoxy (1,2);
	    printf(lcd_putc,"AH=6h DEC=-90deg");
		delay_ms(5);
	}
	while(leitura!=11)
		leitura=varreMatriz();

	leitura=0;
}

void menu(void)
{
	printf(lcd_putc,"\f");
    lcd_gotoxy (1,1);
    printf(lcd_putc,"Escolha um modo:");
    delay_ms(5);
    lcd_gotoxy (1,2);
    printf(lcd_putc,"Manual(1)Auto(2)");
	delay_ms(5);

	while(leitura!=1 && leitura!=2)
		leitura = varreMatriz();

	if(leitura==1)
	{
		leitura=0;
		manual();
	}
	else if (leitura==2)
	{
		leitura=0;
		automatico();
	}
}

void manual(void)
{
	int cont=0;
	exibirAH();
	while(leitura!=10 && leitura!=11)
	{
		leitura=varreTecladoManual();
		if(leitura==4)
		{
			if(AH<12)
				motorAR_antih(vel);
			else
				motorAR_h(vel);
			AH=AH-(passo_AH*4);
			if(AH<0)
				AH=24;
			exibirAH();
		}
		if(leitura==6)
		{
			
			if(AH<12)
				motorAR_h(vel);
			else
				motorAR_antih(vel);
			AH=AH+(passo_AH*4);
			if(AH>24)
				AH=0;
			exibirAH(); 
		}
		if(leitura==2)
		{
			if(AH>12 && DEC>-90){
				DEC-=(passo_DEC*4);
				motorAR_h(vel); // trocar por motorDEC_h
			}else if (AH<=12 && DEC<90){
				DEC+=(passo_DEC*4);
				motorAR_h(vel); // trocar por motorDEC_h
			}
			exibirAH();
		}
		if(leitura==8)
		{
			if(AH>12 && DEC<90){
				DEC+=(passo_DEC*4);
				motorAR_antih(vel); // trocar por motorDEC_antih
			}else if (AH<=12 && DEC>-90){
				DEC-=(passo_DEC*4);
				motorAR_antih(vel); // trocar por motorDEC_antih
			}
			exibirAH();
		}
	}
}

void telaAcompanhamento(void)
{
	printf(lcd_putc,"\f");
    lcd_gotoxy (1,1);
    printf(lcd_putc," Acompanhamento ");
    delay_ms(5);
    lcd_gotoxy (1,2);
    printf(lcd_putc,"     ligado     ");
    delay_ms(5);
}

void inverterAH(void)
{
	if(AH<12)
		AH+=12;
	else
		AH-=12;
}

void exibirAH(void)
{
	int h,m,s,dh,uh,dm,um,ds,us,t;
	int dd,ud,decd,cd,md;
	double temp,temp2;

	t=1;

	h=AH;
	temp=(AH-h)*60; 
	m=temp;
	s=(temp-m)*60;

	dh=extraiDezena(h);
	uh=extraiUnidade(h,dh);
	dm=extraiDezena(m);
	um=extraiUnidade(m,dm);
	ds=extraiDezena(s);
	us=extraiUnidade(s,ds);
	
	printf(lcd_putc,"\f");
    lcd_gotoxy (1,1);
    printf(lcd_putc,"AH=");
    delay_ms(t);
	lcd_gotoxy (4,1);	
	display=dh+'0';
    printf(lcd_putc,&display);
	delay_ms(t);
	lcd_gotoxy (5,1);
	display=uh+'0';
    printf(lcd_putc,&display);
	delay_ms(t);
	lcd_gotoxy (6,1);
	printf(lcd_putc,"h");
	delay_ms(t);
	lcd_gotoxy (7,1);
	display=dm+'0';
    printf(lcd_putc,&display);
	delay_ms(t);
	lcd_gotoxy (8,1);
	display=um+'0';
    printf(lcd_putc,&display);
	delay_ms(t);
	lcd_gotoxy (9,1);
	printf(lcd_putc,"m");
	delay_ms(t);
	lcd_gotoxy (10,1);
	display=ds+'0';
    printf(lcd_putc,&display);
	delay_ms(t);
	lcd_gotoxy (11,1);
	display=us+'0';
    printf(lcd_putc,&display);
	delay_ms(t);
	lcd_gotoxy (12,1);
	printf(lcd_putc,"s    ");
	delay_ms(t);

	if(DEC<0)temp2=DEC*-1;
	else temp2=DEC;

	dd=extraiDezena(temp2);
	ud=extraiUnidade(temp2,dd);
	temp=temp2-(dd*10+ud);
	decd=temp*10;
	cd=temp*100-decd*10;
	md=temp*1000-(decd*100+cd*10);

	lcd_gotoxy (1,2);
	if(DEC>=0)
    	printf(lcd_putc,"DEC=+");
	else
    	printf(lcd_putc,"DEC=-");
    delay_ms(t);
	lcd_gotoxy (6,2);	
	display=dd+'0';
    printf(lcd_putc,&display);
	delay_ms(t);
	lcd_gotoxy (7,2);	
	display=ud+'0';
    printf(lcd_putc,&display);
	delay_ms(t);
	lcd_gotoxy (8,2);	
    printf(lcd_putc,".");
	delay_ms(t);
	lcd_gotoxy (9,2);	
	display=decd+'0';
    printf(lcd_putc,&display);
	delay_ms(t);
	lcd_gotoxy (10,2);	
	display=cd+'0';
    printf(lcd_putc,&display);
	delay_ms(t);
	lcd_gotoxy (11,2);	
	display=md+'0';
    printf(lcd_putc,&display);
	delay_ms(t);
	lcd_gotoxy (12,2);	
    printf(lcd_putc,"deg ");
	delay_ms(t);	 
}

int extraiDezena(int num)
{
	return num*0.1;
}

int extraiUnidade(int num,int dezena)
{
	return num -(dezena*10);
}

int receberNum(void)
{
	leitura=12;
	while(leitura==12)
		leitura=varreMatriz();
	return leitura;
}

void automatico(void)
{
    
	inicioAuto:

	int n[6];
	int sinal;
	int cursor;
	
	for(int j=0;j<2;j++)
	{
		if(j==0)
		{
			printf(lcd_putc,"\f");
    		lcd_gotoxy (1,1);
    		printf(lcd_putc,"AH=");
    		delay_ms(5);
			cursor=4;
		}

		if(j==1)
		{
			lcd_gotoxy (1,2);
    		printf(lcd_putc,"DEC=");
    		delay_ms(5);
			do{
				sinal=receberNum();
			}while(sinal!=1 && sinal!=4 && sinal!=8 && sinal!=2 && sinal!=3 && sinal!=6);
			if(sinal==1 || sinal==4 || sinal==8){
				lcd_gotoxy (5,2);
    			printf(lcd_putc,"-");
				delay_ms(5);
				sinal=-1;
			}
			if(sinal==2 || sinal==3 || sinal==6){
				lcd_gotoxy (5,2);
    			printf(lcd_putc,"+");
				delay_ms(5);
				sinal=1;
			}
			cursor=6;
		}

		for(int i=0;i<6;i++)
		{
			n[i]=receberNum();
			display=n[i]+'0';
			lcd_gotoxy (cursor,j+1);
			
			if(cursor==6&&j==0){
		    	printf(lcd_putc,"h");
				delay_ms(5);
				cursor++;
			}
			if(cursor==8&&j==1){
				printf(lcd_putc,"deg");
				delay_ms(5);
				cursor+=3;
			}
			if((cursor==9&&j==0)||(cursor==13&&j==1)){
				printf(lcd_putc,"m");
				delay_ms(5);
				cursor++;
			}
			lcd_gotoxy (cursor,j+1);
			printf(lcd_putc,&display);
		    delay_ms(5);
			cursor++;
			if(n[i]==10)
				goto inicioAuto;
		}
		lcd_gotoxy (cursor,j+1);
		printf(lcd_putc,"s");
		delay_ms(5);

		if(j==0)
		{
			AH_obj=n[0]*10;
			AH_obj+=n[1];
			AH_obj+=(n[2]*10)/60;
			AH_obj+=n[3]/60;
			AH_obj+=(n[4]*10)/3600;
			AH_obj+=n[5]/3600;
		
			if(AH_obj>24||AH_obj<0)
			{	
		    	lcd_gotoxy (1,2);
		    	printf(lcd_putc," Valor Invalido ");
		    	delay_ms(1000);
				goto inicioAuto;
			}
		}
		if(j==1)
		{
			DEC_obj=n[0]*10;
			DEC_obj+=n[1];
			DEC_obj+=((n[2]*10)/60)*15;
			DEC_obj+=(n[3]/60)*15;
			DEC_obj+=((n[4]*10)/3600)*15;
			DEC_obj+=(n[5]/3600)*15;
			DEC_obj=DEC_obj*sinal;
			
			/*if(DEC_obj>90 || DEC_obj<-90)
			{
				lcd_gotoxy (1,2);
		    	printf(lcd_putc," Valor Invalido ");
		    	delay_ms(1000);
				goto inicioAuto;
			}*/
		}
	}
	do{
		leitura=varreMatriz();
	}while(leitura!=10&&leitura!=11);
	if(leitura==10)
		goto inicioAuto;

	if(AH<AH_obj)
	{
		while(AH<AH_obj)
		{
			if(AH<12)
				motorAR_h(2);
			else
				motorAR_antih(2);
			AH=AH+(passo_AH*4);
			if(AH>24)
				AH=0;
			exibirAH();
		} 
	}else{
		while(AH>AH_obj)
		{
			if(AH<12)
				motorAR_antih(vel);
			else
				motorAR_h(vel);
			AH=AH-(passo_AH*4);
			if(AH<0)
				AH=24;
			exibirAH();
		}
	}
	delay_ms(500);
	if(DEC<DEC_obj)
	{
		while(DEC<DEC_obj)
		{
			/*if(AH>12 && DEC>-90){
				DEC-=(passo_DEC*4);
				motorAR_h(vel); // trocar por motorDEC_h
			}else if (AH<=12 && DEC<90){*/
				DEC+=(passo_DEC*4);
				motorAR_h(vel); // trocar por motorDEC_h
			//}
			exibirAH();
		}
	}else{
		while(DEC>DEC_obj)
		{
			/*if(AH>12 && DEC<90){
				DEC+=(passo_DEC*4);
				motorAR_antih(vel); // trocar por motorDEC_antih
			}else if (AH<=12 && DEC>-90){*/
				DEC-=(passo_DEC*4);
				motorAR_antih(vel); // trocar por motorDEC_antih
			//}
			exibirAH();
		}
	}		
}

void desabilitaMatriz()
{
	coluna1 = 1;
	coluna2 = 1;
	coluna3 = 1;	
}


int varreMatriz()
{
	coluna1 = 0;
	if(!linha1){
		while(linha1==0);
		desabilitaMatriz();
		return 1;
	}
	if(!linha2){
		while(linha2==0);
		desabilitaMatriz();
		return 4;
	}
	if(!linha3){
		while(linha3==0);
		desabilitaMatriz();
		return 7;
	}
	if(!linha4){
		while(linha4==0);
		desabilitaMatriz();
		return 10;
	}
	desabilitaMatriz();

	coluna2=0;
	if(!linha1){
		while(linha1==0);
		desabilitaMatriz();
		return 2;
	}
	if(!linha2){
		while(linha2==0);
		desabilitaMatriz();
		return 5;
	}
	if(!linha3){
		while(linha3==0);
		desabilitaMatriz();
		return 8;
	}
	if(!linha4){
		while(linha4==0);
		desabilitaMatriz();
		return 0;
	}
	desabilitaMatriz();
	
	coluna3=0;
	if(!linha1){
		while(linha1==0);
		desabilitaMatriz();
		return 3;
	}
	if(!linha2){
		while(linha2==0);
		desabilitaMatriz();
		return 6;
	}
	if(!linha3){
		while(linha3==0);
		desabilitaMatriz();
		return 9;
	}
	if(!linha4){
		while(linha4==0);
		desabilitaMatriz();
		return 11;
	}
	desabilitaMatriz();
	return 12;
}

int varreTecladoManual(void)
{
	coluna1 = 0;
	if(!linha1){
		while(linha1==0);
		if (vel<10)
			vel++;
		desabilitaMatriz();
	}
	if(!linha2){
		desabilitaMatriz();
		return 4;
	}
	if(!linha4){
		desabilitaMatriz();
		return 10;
	}
	desabilitaMatriz();
	coluna2 = 0;
	if(!linha1){
		desabilitaMatriz();
		return 2;
	}
	if(!linha3){
		desabilitaMatriz();
		return 8;
	}
	if(!linha2){
		while(!linha2);
		inverterAH();
		exibirAH();
	}
	desabilitaMatriz();
	coluna3 = 0;
	if(!linha1){
		while(linha1==0);
		if (vel>3)
			vel--;
		desabilitaMatriz();
	}
	if(!linha2){
		desabilitaMatriz();
		return 6;
	}
	if(!linha4){
		desabilitaMatriz();
		return 11;
	}
	desabilitaMatriz();
	return 0;
}

void motorAR_antih(int v)
{   
	if(polo==2){   
		bobd=1; 
	    delay_ms(v);
		bobd=0;  
	
		bobc=1; 
	    delay_ms(v); 
		bobc=0;
	
		bobb=1; 
	    delay_ms(v); 
		bobb=0;
	
		boba=1;  
	    delay_ms(v);
		boba=0;
	}else if (polo==1){
		boba=1;  
	    delay_ms(v);
		boba=0;
	
		bobb=1; 
	    delay_ms(v); 
		bobb=0;
		
		bobc=1; 
	    delay_ms(v); 
		bobc=0;
	
		bobd=1; 
	    delay_ms(v);
		bobd=0; 
	}
}

void motorAR_h(int v)
{ 
	if(polo==2){
		boba=1;  
	    delay_ms(v);
		boba=0;
	
		bobb=1; 
	    delay_ms(v); 
		bobb=0;
		
		bobc=1; 
	    delay_ms(v); 
		bobc=0;
	
		bobd=1; 
	    delay_ms(v);
		bobd=0;  	 
	}else if (polo==1){
		bobd=1; 
	    delay_ms(v);
		bobd=0;  
	
		bobc=1; 
	    delay_ms(v); 
		bobc=0;
	
		bobb=1; 
	    delay_ms(v); 
		bobb=0;
	
		boba=1;  
	    delay_ms(v);
		boba=0;
	}
}