
#include "project.h"
#include "stdio.h"
#include "stdlib.h"
#include "PWM_1.h"

/* Project Defines */
#define REF 23 //Posicion deseada
#define FALSE  0
#define TRUE   1
#define TRANSMIT_BUFFER_SIZE  16
#define TS 0.02 //Tiempo de muestreo
#define CMP_MAX 235
#define CMP_MIN 65

/*Variables*/
uint32 InterruptCnt;
float reference=47;
//Control constants
float KP=0.0079767;
float KD=0.020035;
float N=1.9;
/*Auxiliar variables for control actions*/
float NTS;
float DN;
/*Control variables*/
uint16 compare=140;// dato
volatile float ek=0;// error
volatile float ek_1=0;// error pasado
volatile float pk=0;// accion proporcional
volatile float dk=0;// accion derivativa
volatile float PD =0;// accion total

/*******************************************************************************
* Define Interrupt service routine and allocate an vector to the Interrupt
********************************************************************************/
CY_ISR(InterruptHandler)
{
    /* Read Status register in order to clear the sticky Terminal Count (TC) bit 
     * in the status register. Note that the function is not called, but rather 
     * the status is read directly.
     */
    Timer_1_STATUS;    
    /* Increment the Counter to indicate the keep track of the number of 
     * interrupts received */
    InterruptCnt++;    
}
/*Control function*/
void Control(float distancia){
    ek=reference-distancia;// Calculo del error
    pk=KP*ek;// accion proporcional
    dk=(KD*N*(ek-ek_1)+dk)/(1+N*TS);/// accion derivativo
    PD=pk+dk;// respuesta de control
    ek_1=ek;
    compare= (54.09*PD)+140;
    
    if (compare > CMP_MAX) { 
        compare = CMP_MAX;
        }
    if (compare < CMP_MIN) { 
        compare = CMP_MIN;}
   
     PWM_1_WriteCompare(compare);    
}
void menu(){
        UART_1_PutString("\n");
        UART_1_PutString("************************************\r\n");
        UART_1_PutString("Welcome to Ball and Beam controller \r\n");
        UART_1_PutString("Menu hotkeys    \r\n");
        UART_1_PutString("For setting the reference press -> q\r\n");
        UART_1_PutString("For setting the compare press -> z\r\n");
        UART_1_PutString("To start controlling -> s\r\n");
        UART_1_PutString("************************************\r\n");
    }
int main(void){
    /* Variable to store UART received character */
    uint8 Ch;
    uint8 sel;
    uint32 value_counter=0;
    float distancia=0.0;
    float tiempo;
    /* Flags used to manage menú */
    uint8 StartControlling=FALSE;
    uint8 set_cmp=FALSE;
    uint8 set_ref=FALSE;
    /* Transmit Buffer */
    char TransmitBuffer[TRANSMIT_BUFFER_SIZE];
    /* Enable the global interrupt */
    CyGlobalIntEnable;
    /* Enable the Interrupt component connected to Timer interrupt */
    TimerISR_StartEx(InterruptHandler);
    /* Start the components */
    ADC_DelSig_1_Start();
    UART_1_Start();
    Timer_1_Start();
    Timer_2_Start();
    
    /* Send message to verify COM port is connected properly */
    UART_1_PutString("COM Port Open Control Action Starts \r\n");
    menu();

    
    for(;;)
    {        
        /* Non-blocking call to get the latest data recieved  */
        Ch = UART_1_GetChar();
        tiempo=InterruptCnt/50;
        /* Set flags based on UART command */
        switch(Ch)
        {
            case 0:
                break;
            case 'z':
                set_cmp = TRUE;
                sprintf(TransmitBuffer, "Setting reference \r\n");
                UART_1_PutString(TransmitBuffer);
                break;
            case 'q':
                set_ref = TRUE;
                sprintf(TransmitBuffer, "Setting CMP \r\n");
                UART_1_PutString(TransmitBuffer);
                break;    
            case 's':
                StartControlling = TRUE;
                sprintf(TransmitBuffer, "Inicio de Accion de Control \r\n");
                UART_1_PutString(TransmitBuffer);
                break;
            case 'd':
                StartControlling = FALSE;
                set_cmp = FALSE;
                set_ref=FALSE;
                sprintf(TransmitBuffer, "Fin de Accion de Control \r\n\n\n");
                menu();
                UART_1_PutString(TransmitBuffer);
                break;
            default:
                StartControlling = FALSE;
                set_cmp = FALSE;
                set_ref = FALSE;
                break;    
        }
        /*Sensor sampling*/
        while(echo_Read()==0){
            trigger_out_Write(1);
            CyDelay(10u);
            trigger_out_Write(0);
            CyDelay(1);
        }
        while(echo_Read()==1){};
        value_counter= 65535-Timer_2_ReadCounter();
        distancia=value_counter/58;
        

        if(StartControlling){
            //sprintf(TransmitBuffer, "t=%.2f,dist=%.2f,e= %.2f,pk=%.2f,dk=%.2f,PD=%.2f,CMP=%u\r\n",tiempo,distancia,ek,pk,dk,PD,compare);
            sprintf(TransmitBuffer, "%lu,%.2f,%.2f\r\n",InterruptCnt,distancia,ek);
            UART_1_PutString(TransmitBuffer);
            PWM_1_Start();
            Control(distancia);
            
        }
        else if(set_cmp){
            sel = UART_1_GetChar();
            if(sel=='c'){
                PWM_1_Stop();
                compare+=1;
                PWM_1_WriteCompare(compare);
                sprintf(TransmitBuffer, "%u\r\n",PWM_1_ReadCompare());
                UART_1_PutString(TransmitBuffer);
            }
            else if(sel=='x'){
                PWM_1_Stop();
                compare-=1;
                PWM_1_WriteCompare(compare);
                sprintf(TransmitBuffer, "%u\r\n",PWM_1_ReadCompare());
                UART_1_PutString(TransmitBuffer);
            }
            else if(sel=='v'){
                PWM_1_Start();
                sprintf(TransmitBuffer, "%lu,%.2f\r\n", InterruptCnt,distancia);
                UART_1_PutString(TransmitBuffer);
            }
            else if(sel=='b'){
                UART_1_PutString("Setting CMP ended\r\n");
                set_cmp=FALSE;
                InterruptCnt=0;
            }
        }
            //////////////////////////
        else if(set_ref){
            sel = UART_1_GetChar();
            if(sel=='w'){
                reference+=1;
                sprintf(TransmitBuffer, "%.2f\r\n",reference);
                UART_1_PutString(TransmitBuffer);
            }
            else if(sel=='e'){
                reference-=1;
                sprintf(TransmitBuffer, "%.2f\r\n",reference);
                UART_1_PutString(TransmitBuffer);
            }
            else if(sel=='r'){
                sprintf(TransmitBuffer, "%lu,%.2f\r\n", InterruptCnt,distancia);
                UART_1_PutString(TransmitBuffer);
            }
            else if(sel=='t'){
                UART_1_PutString("Setting reference ended\r\n");
                set_ref=FALSE;
                InterruptCnt=0;
            }
        }
        else{
            InterruptCnt=0;
        }
    }
}

