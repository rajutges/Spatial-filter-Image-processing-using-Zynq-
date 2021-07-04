/*
 * imageIpTest.c
 *
 *  Created on: Jun 29, 2021
 *      Author: Raju
 */



#include "xaxidma.h" // add the dma header file which give info abt give and recieve from IP
#include "xparameters.h"
#include "sleep.h"
#include "xil_cache.h"
#include "xil_io.h"   // will use in intr controller part
#include "xscugic.h"   // will use in intr controller part
#include "imageData.h"
#include "xuartps.h"

#define imageSize 512*512

u32 checkHalted(u32 baseAddress,u32 offset);

XScuGic IntcInstance;
static void imageProcISR(void *CallBackRef);
static void dmaReceiveISR(void *CallBackRef);   // has a pointer dma controller
int done;

int main(){
    u32 status;
	u32 totalTransmittedBytes=0;
	u32 transmittedBytes = 0;
	XUartPs_Config *myUartConfig;
	XUartPs myUart;

	//Initialize UART
// lookup config we need
	// want to see the output use UART
	myUartConfig = XUartPs_LookupConfig(XPAR_PS7_UART_1_DEVICE_ID);
	status = XUartPs_CfgInitialize(&myUart, myUartConfig, myUartConfig->BaseAddress);
	if(status != XST_SUCCESS)
		print("UART initialization failed...\n\r");
	status = XUartPs_SetBaudRate(&myUart, 115200);
	if(status != XST_SUCCESS)
		print("Baudrate init failed....\n\r");

	/*
	 *
	 */

	XAxiDma_Config *myDmaConfig;
	XAxiDma myDma;
    //DMA Controller Configuration
	myDmaConfig = XAxiDma_LookupConfigBaseAddr(XPAR_AXI_DMA_0_BASEADDR);
	status = XAxiDma_CfgInitialize(&myDma, myDmaConfig);
	if(status != XST_SUCCESS){
		print("DMA initialization failed\n");
		return -1;
	}

	/*  need to intrupt enable control , configure that register to enable the intrupt
	 *  need only device to memory intrupt here so we use only
	 *  the function can be taken form axidma.h file declaration
	 */
	// it uses mask - tells which  interrupts are enable or  which are disable
	// check declaration of xaxidma_hw.h from xaxidma_bdring.h form axidma_bd.h
	// XAxiDma_IntrEnable(InstancePtr, Mask, Direction) is the format for enable the intr

	XAxiDma_IntrEnable(&myDma, XAXIDMA_IRQ_IOC_MASK, XAXIDMA_DEVICE_TO_DMA);


	//Interrupt Controller Configuration
	XScuGic_Config *IntcConfig;
	IntcConfig = XScuGic_LookupConfig(XPAR_PS7_SCUGIC_0_DEVICE_ID);
	status =  XScuGic_CfgInitialize(&IntcInstance, IntcConfig, IntcConfig->CpuBaseAddress);

	if(status != XST_SUCCESS){
		xil_printf("Interrupt controller initialization failed..");
		return -1;
	}
/*   now set the priority for interrupts for both the interrupts
 */
	// got to x parameters.h search interrupts with two IRQ no. use that
	// for 1st interrupt
	XScuGic_SetPriorityTriggerType(&IntcInstance,XPAR_FABRIC_IMAGEPROCESS_0_O_INTR_INTR,0xA0,3);

	// this status connect one IRQ  to one particular ISR
	//imageProcISR - fn to tell what happen when the processor get interrupt from IP
	status = XScuGic_Connect(&IntcInstance,XPAR_FABRIC_IMAGEPROCESS_0_O_INTR_INTR,(Xil_InterruptHandler)imageProcISR,(void *)&myDma);
	if(status != XST_SUCCESS){
		xil_printf("Interrupt connection failed");
		return -1;
	}
	XScuGic_Enable(&IntcInstance,XPAR_FABRIC_IMAGEPROCESS_0_O_INTR_INTR);


 // for 2nd interrupt set priority
	XScuGic_SetPriorityTriggerType(&IntcInstance,XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR,0xA1,3);
	// dmaReceiveISR - fn to tell what happen when the processor get interrupt from dma Controller
	status = XScuGic_Connect(&IntcInstance,XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR,(Xil_InterruptHandler)dmaReceiveISR,(void *)&myDma);
	if(status != XST_SUCCESS){
		xil_printf("Interrupt connection failed");
		return -1;
	}
	XScuGic_Enable(&IntcInstance,XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR);
// concentrate the intr controller with the OS
	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,(Xil_ExceptionHandler)XScuGic_InterruptHandler,(void *)&IntcInstance);
	Xil_ExceptionEnable();

// data transfer
	status = XAxiDma_SimpleTransfer(&myDma,(u32)imageData, 512*512,XAXIDMA_DEVICE_TO_DMA);
	status = XAxiDma_SimpleTransfer(&myDma,(u32)imageData, 4*512,XAXIDMA_DMA_TO_DEVICE);
	if(status != XST_SUCCESS){
		print("DMA initialization failed\n");
		return -1;
	}

// done is a static variable use for knowing whether all data has been recieved or not
    while(!done){

    }

	// want to see the output use UART
	while(totalTransmittedBytes < imageSize){
		transmittedBytes =  XUartPs_Send(&myUart,(u8*)&imageData[totalTransmittedBytes],1);
		totalTransmittedBytes += transmittedBytes;
		usleep(1000);  // delay to avoid the data loss
	}


}


//check fro it finished previos or not
u32 checkIdle(u32 baseAddress,u32 offset){
	u32 status;
	status = (XAxiDma_ReadReg(baseAddress,offset))&XAXIDMA_IDLE_MASK;
	return status;
}

// IP gives an intr. basicaaly saying he has finished processing one line buffer so can send a new line buffer
static void imageProcISR(void *CallBackRef){
	static int i=4;
	int status;
	XScuGic_Disable(&IntcInstance,XPAR_FABRIC_IMAGEPROCESS_0_O_INTR_INTR);
	status = checkIdle(XPAR_AXI_DMA_0_BASEADDR,0x4);
	while(status == 0)
		status = checkIdle(XPAR_AXI_DMA_0_BASEADDR,0x4);
	if(i<514){   // 512+2 dummy lines
		status = XAxiDma_SimpleTransfer((XAxiDma *)CallBackRef,(u32)&imageData[i*512],512,XAXIDMA_DMA_TO_DEVICE);
		i++;
	}
	XScuGic_Enable(&IntcInstance,XPAR_FABRIC_IMAGEPROCESS_0_O_INTR_INTR);
}

// when the dma controller sys he has recieved all the configure data frm ip

static void dmaReceiveISR(void *CallBackRef){
	XAxiDma_IntrDisable((XAxiDma *)CallBackRef, XAXIDMA_IRQ_IOC_MASK, XAXIDMA_DEVICE_TO_DMA);
	// clear to get new intrrupt
	XAxiDma_IntrAckIrq((XAxiDma *)CallBackRef, XAXIDMA_IRQ_IOC_MASK, XAXIDMA_DEVICE_TO_DMA);
	done = 1;
	XAxiDma_IntrEnable((XAxiDma *)CallBackRef, XAXIDMA_IRQ_IOC_MASK, XAXIDMA_DEVICE_TO_DMA);
}
