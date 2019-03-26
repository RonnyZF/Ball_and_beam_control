
#include "project.h"
#include "stdio.h"
#include "stdlib.h"
#include "PWM_1.h"

/* Project Defines */
#define FALSE  0
#define TRUE   1
#define TRANSMIT_BUFFER_SIZE  16

uint32 InterruptCnt;


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

int main()
{
    /* Variable to store ADC result */
    uint32 Output;
    /* Variable to store UART received character */
    uint8 Ch;
    uint8 sel;
    uint8 compare=189;
    uint32 value_counter=0;
    float distancia=0.0;
    uint8 entrada=0;
    /* Flags used to store transmit data commands */
    uint8 ContinuouslySendData;
    uint8 settingup;
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

    /* Initialize Variables */
    settingup = FALSE;
    ContinuouslySendData = FALSE;

    /* Start the ADC conversion */
    ADC_DelSig_1_StartConvert();

    /* Send message to verify COM port is connected properly */
    UART_1_PutString("COM Port Open\r\n");

    for(;;)
    {
        /* Non-blocking call to get the latest data recieved  */
        Ch = UART_1_GetChar();

        /* Set flags based on UART command */
        switch(Ch)
        {
            case 0:
                /* No new data was recieved */
                break;
            case 'A':
            case 'a':
                settingup = TRUE;
                sprintf(TransmitBuffer, "Setting up \r\n");
                UART_1_PutString(TransmitBuffer);
                break;
            case 'S':
            case 's':
                ContinuouslySendData = TRUE;
                sprintf(TransmitBuffer, "Inicio de la medicion \r\n");
                UART_1_PutString(TransmitBuffer);
                break;
            case 'D':
            case 'd':
                ContinuouslySendData = FALSE;
                settingup = FALSE;
                sprintf(TransmitBuffer, "Fin de la medicion \r\n\n\n");
                UART_1_PutString(TransmitBuffer);
                break;
            default:
                /* Place error handling code here */
                break;
        }

        while(echo_Read()==0){
            trigger_out_Write(1);
            CyDelay(10u);
            trigger_out_Write(0);
            CyDelay(1);
        }
        while(echo_Read()==1){};
        value_counter= 65535-Timer_2_ReadCounter();
        distancia=value_counter/58;


        if(ContinuouslySendData&&settingup)
        {
            sprintf(TransmitBuffer, "%lu,%u,%.2f\r\n", InterruptCnt,entrada,distancia);
            UART_1_PutString(TransmitBuffer);
            PWM_1_Start();

            if((InterruptCnt==48)||(InterruptCnt==49)||(InterruptCnt==50)||(InterruptCnt==51)){
                compare-=1;
                PWM_1_WriteCompare(compare);
                entrada=1;
            }
            else if(distancia>95){
                ContinuouslySendData = FALSE;
                settingup=FALSE;
                PWM_1_Stop();
            }

        }
        else if(settingup)
        {
            sel = UART_1_GetChar();
            if(sel=='z'){
                PWM_1_Stop();
                entrada=0;
                compare+=1;
                PWM_1_WriteCompare(compare);
                sprintf(TransmitBuffer, "%u\r\n",PWM_1_ReadCompare());
                UART_1_PutString(TransmitBuffer);
            }
            else if(sel=='x'){
                PWM_1_Stop();
                entrada=0;
                compare-=1;
                PWM_1_WriteCompare(compare);
                sprintf(TransmitBuffer, "%u\r\n",PWM_1_ReadCompare());
                UART_1_PutString(TransmitBuffer);
            }
            else if(sel=='c'){
                PWM_1_Start();
                entrada=0;
                sprintf(TransmitBuffer, "%lu,%u,%.2f\r\n", InterruptCnt,entrada,distancia);
                UART_1_PutString(TransmitBuffer);
            }
            else if(sel=='v'){
                settingup=FALSE;
            }
        }
        else{
            InterruptCnt=0;
        }

    }
}
