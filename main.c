////Desmadre de librerias por declarar 
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "btstack_run_loop.h"
#include "btstack_config.h"
#include "Matrix/Max7219.h"
#include "Matrix/StringArray.h"
#include <stdio.h>   
#include "LED/LedRGB.h"
#include <string.h>
#include "hardware/timer.h"
//Variables de interrupciones y de Ledsitos 
#define Alarm1_us 10000 
#define Alarm2_us 20000                        
#define GPIO16 15
#define LED GPIO16
volatile bool runLEDSequence = false;
volatile unsigned char i_2,flag=0,j,contador_2=0,brillo=0;
volatile int r=0,g=0,b=0;
const char mensaje_1[]="    ENTREGANDO   ";
const char mensaje_2[]="    EN ESPERA DE CXFX   ";
const char mensaje_3[]="    DISTANCIA X.XX   ";
const char mensaje_4[]="    TRASLADO A CXFX   ";
const char mensaje_5[]="    NO SUBO GORDAS   ";
const char mensaje_6[]="    OBSTACULO DETECTADO   ";
const char mensaje_7[]="    RECOGIENDO   ";
/////Para Matriz Bluetooth
extern void btstack_main(void);
extern int Rx_BT_RFCOMM(char **Rx_buffer, char *Rx_size);
extern void Tx_BT_RFCOMM(char *Tx_Buffer, int Tx_size);
//Declaramos funciones del programa
void My_ISR_Alarm1_handler(void);
void ISR_Alarm0_handler(void);
void Derecha_Izquierda(char *Rows);
//////Variable de la matriz bt
char zise_BT;
char *PTR;
char Tx_Buffer[30];
int contador=0;

unsigned char Memoria[300]={};
static const unsigned char Mensaje[] = "   FUNCIONANDO";
//Variables del ultrasonico
#define TRIG_PIN 14
#define ECHO_PIN 1
#define buzzer 13
volatile float paro=0;


void ultrasonico();
//Inicia el main
int main() {
  set_sys_clock_khz(133000, true);  
  stdio_init_all();

  /////////////////////////////////////GPIO Configuration///////////////////////////
  gpio_init(TRIG_PIN);
  gpio_set_dir(TRIG_PIN, GPIO_OUT);
  gpio_init(ECHO_PIN);
  gpio_set_dir(ECHO_PIN, GPIO_IN);

  gpio_init(buzzer);
  gpio_set_dir(buzzer, GPIO_OUT);

  gpio_set_dir(LED, GPIO_OUT);
  gpio_put(LED, 0);
  gpio_set_function(LED, GPIO_FUNC_SIO);
  ////////////////////////////////////////CONFIGURACION DE INTERRUPCIONES////////////////////////////////

  hw_set_bits(&timer_hw->inte, 1<<1 | 1 << 0);               // Enable the interrupt alarm1 and alarm0
  irq_set_exclusive_handler(TIMER_IRQ_0,&ISR_Alarm0_handler);// Set irq handler for alarm0 irq
  irq_set_exclusive_handler(TIMER_IRQ_1,&My_ISR_Alarm1_handler);// Set irq handler for alarm1 irq
  irq_set_enabled(TIMER_IRQ_0,true);                         // Enable the alarm0 irq
  irq_set_enabled(TIMER_IRQ_1,true);                         // Enable the alarm1 irq

  timer_hw->alarm[0] = timer_hw->timerawl + Alarm1_us;       // Load initial time to alarm 0
  timer_hw->alarm[1] = timer_hw->timerawl + Alarm2_us;       // Load initial time to alarm 1

  //////////////////////////////////////// initialize CYW43 driver//////////////////////////////////
  if (cyw43_arch_init()) {
    printf("cyw43_arch_init() failed.\n");
    return -1;
  }
  btstack_main();            //Inicializa BL y espera a la conexión.
   
  Ini_Matrix();                // Initialize SPI and MAX7219
  Matrix(Memoria,Mensaje);     // Convert ASCII Char To ROWS

  while(1) {
  if(Rx_BT_RFCOMM(&PTR, &zise_BT)) {
   
    memset(Memoria, 0, sizeof(Memoria));  //Se limpia memoria 
    Matrix(Memoria,PTR); 
    printf("esperando\n");
    busy_wait_ms(30);
    printf("RCV: ");
    for (int i = 0; i < zise_BT; i++) {
      putchar(PTR[i]);
    }
    Derecha_Izquierda(Memoria);
    busy_wait_ms(20);
    }
    Derecha_Izquierda(Memoria);  //Ejecuta Bt matriz
  //////////////////////////////////////////Casos de entrega//////////////////
  if(PTR[0]=='1'){
    r= 0;
    g=250;
    b=0;
    Matrix(Memoria ,mensaje_1);
    gpio_put(TRIG_PIN,0);
    gpio_put(buzzer,0);
  }
  if(PTR[0]=='2'){
    r= 250;
    g=0;
    b=0;
    Matrix(Memoria ,mensaje_2);
    gpio_put(TRIG_PIN,0);
    gpio_put(buzzer,0);
  }
  if(PTR[0]=='3'){
    r= 0;
    g=0;
    b=250;
    Matrix(Memoria ,mensaje_3);
    gpio_put(TRIG_PIN,0);
    gpio_put(buzzer,0);
  }
  if(PTR[0]=='4'){
    r= 250;
    g=70;
    b=0;
    Matrix(Memoria ,mensaje_4);
    gpio_put(TRIG_PIN,0);
    gpio_put(buzzer,0);
  }
  if(PTR[0]=='5'){
    r= 250;
    g=0;
    b=120;
    Matrix(Memoria ,mensaje_5);
    gpio_put(TRIG_PIN,0);
    gpio_put(buzzer,0);
  }
   if(gpio_get(ECHO_PIN)==0){
    memset(Memoria, 0, sizeof(Memoria));  //Se limpia memoria 
    paro=1;
    r= 10;
    g=50;
    b=120;
    Matrix(Memoria ,mensaje_6);
    
  }
   if(PTR[0]=='7'){
   memset(Memoria, 0, sizeof(Memoria));  //Se limpia memoria 
    r= 60;
    g=10;
    b=10;
    Matrix(Memoria ,mensaje_7);
    gpio_put(TRIG_PIN,0);
    gpio_put(buzzer,0);
   }
  if(PTR[0]!='1'&PTR[0]!='2'&PTR[0]!='3'&PTR[0]!='4'&PTR[0]!='5'&gpio_get(TRIG_PIN)==1 &PTR[0]!='7' ){
    memset(Memoria, 0, sizeof(Memoria));  //Se limpia memoria 
    r=0;
    g=0;
    b=0;
    gpio_put(TRIG_PIN,0);
    gpio_put(buzzer,0);
    Matrix(Memoria ,mensaje_7);
  }
  if (paro==1){
    paro=0;
    gpio_put(TRIG_PIN,1);
    gpio_put(buzzer,1);
    r= 10;
    g=50;
    b=120;
  }


}//Termina While

  
}//Termina el main
///////////////////////////////////////////////////////Funcion de la Matriz LED/////////////
void Derecha_Izquierda(char *Rows)
{
   static unsigned char R=0, C=0, temp2;
   int temp, temp3;
   unsigned char Buffer[4095];
   unsigned char *Punt;
   for (char i = 0; i <= 47; i++)
   {
 
      temp=(int)(*(Rows+i+C+8));
      printf("contador: %i \r\n", temp-8);
      printf("i: %i \r\n", i);
      temp = temp<<R;
      Punt=(unsigned char*)&temp;
      temp2=*(Punt+1);
      Buffer[i]=*(Rows+i+C)<<R | temp2;
      
      if(temp-8==0xFFFFF){
         R=0;
         C=0;
         break;
      }
      
   }
   write(Buffer);
   R+=1;
   if(R==8){
      R=0;
      C+=8;     
    
   }  
}

///////////////////////////////////////////Interrupcion Bluetooth///////////////////////////////////////
void ISR_Alarm0_handler(void){
  unsigned int CHsize;
  hw_clear_bits(&timer_hw->intr, 1 << 0);                     // Clear the alarm0 irq
  timer_hw->alarm[0] = timer_hw->timerawl + Alarm1_us;         // Create the new time target
  CHsize=sprintf(Tx_Buffer, "Contador: %u \n", ++contador);
  printf("%s", Tx_Buffer);
  Tx_BT_RFCOMM(Tx_Buffer,CHsize);
}
////////////////////////////////////////////Interrupción Ledsitos//////////////////////////
void My_ISR_Alarm1_handler(void)
{
 hw_clear_bits(&timer_hw->intr, 1 << 1);                      // Limpiar alarma
 timer_hw->alarm[1] = timer_hw->timerawl + Alarm2_us;         // Create the new time target
 flag = 1;
 printf("cambio flag\n");
 contador_2 ++;
 for(i_2=0; i_2<=contador_2; i_2++){
          RGB_LED(LED,0,0,0);
  }
/////////////////////////////////////////Inicia el efecto cometa/////////////////////////////////  
if (flag==1){
  runLEDSequence = true;
  printf("entra el if\n");
  for(i_2=0; i_2<=1; i_2++){
    
          RGB_LED(LED,r/25,g/25,b/25); 
          
  }
  for(i_2=0; i_2<=1; i_2++){
    
          RGB_LED(LED,r/10,g/10,b/10); 
          
  }
  for(i_2=0; i_2<=1; i_2++){
   
          RGB_LED(LED,r/5,g/5,b/5); 
          
  }
  for(i_2=0; i_2<=1; i_2++){
    
          RGB_LED(LED,r/2,g/2,b/2); 
          
  }
  for(i_2=0; i_2<=1; i_2++){
   
          RGB_LED(LED,r,g,b); 
          
  }
  flag = 0;

}
if(contador_2 >= 53) contador_2 = 0;


}



