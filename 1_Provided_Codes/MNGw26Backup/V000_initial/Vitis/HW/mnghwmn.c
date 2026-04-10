/*
 * mnghwmn.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xscugic.h"
#include "xil_exception.h"

#define MAXP_REG(k)  (*(volatile unsigned int *)(XPAR_MEASDIF_0_S00_AXI_BASEADDR+4*k))

#define TIMER100ms   9999999
#define MBINT_ENABLE 0x80000000
#define MBINT_ACKN   0x40000000


// ---- interrupt controller -----
static XScuGic  Intc;					// interrupt controller instance
static XScuGic_Config  *IntcConfig;		// configuration instance

static volatile unsigned int Led_Output;
static volatile unsigned int ISR_Count;
static volatile unsigned int Do_Display;


#define CBUF_LEN 64


static volatile int Dummy_Count;


/*
** ------------------------------------------------------------
** Interrupt handler (iqr61 fabric interrupt)
** ------------------------------------------------------------
*/
static void FabricIntrHandler(void *CallBackRef)
{
	// int  ih_par = *(int *)CallBackRef;
	MAXP_REG(3) = MBINT_ENABLE | MBINT_ACKN | TIMER100ms;
	MAXP_REG(0) = Led_Output;
	Led_Output ^= 0x080;
	Do_Display = 1;
	ISR_Count++;
	MAXP_REG(3) = MBINT_ENABLE | TIMER100ms;
}




static unsigned int MaxRead(unsigned int mreg, int quiet)
{
	unsigned int xdata;
	int k;

	mreg <<= 8;
	MAXP_REG(1) = mreg;
	k = 0;
	xdata = 0xc0000000;
	do {
		xdata = MAXP_REG(1);
		k++;
	} while (((xdata & 0x01) == 0) && (k < 1000));
	if (k >= 100) {
		if (quiet == 0) {
			print(" *** MaxRead timeout.");
		}
	} else {
		xdata = MAXP_REG(2);
		xdata &= 0x0ff;
		if (quiet == 0) {
			xil_printf("loop count: %d |  data: %x\n\r", k, xdata);
		}
	}
	return xdata;
}


static void MaxWrite(unsigned int mreg, unsigned int msend, int quiet)
{
	unsigned int transm_data, xdata;
	int k;

	mreg |= 0x080;
	transm_data = (mreg << 8) | (msend & 0x0ff);
	MAXP_REG(1) = transm_data;
	k = 0;
	do {
		xdata = MAXP_REG(1);
		k++;
	} while (((xdata & 0x01) == 0) && (k < 1000));
	if (k >= 100) {
		if (quiet == 0) {
			print(" *** MaxWrite timeout.");
		}
	} else if (quiet == 0) {
		xdata = MAXP_REG(2);
		xil_printf("loop count: %d  (%x)\n\r", k, xdata);
	}
}


static void SimpleDelay()
{
	int  k;

	for (k = 0; k < 40000000; k++) {
		Dummy_Count = k >> 1;
	}
}


int main()
{
	unsigned int int_par, xstatus, xdata, xlsb, xadc;
	unsigned int ad0, ad1;
	char cbuf[CBUF_LEN], *chp;
	int  terminate, regn, loopc, k, ccount, isr_run;
	double mresist, mtemp;

    init_platform();
    print("-- Resistance Measurement PMB1 Test ---\n\r");
    Dummy_Count = 0;
    isr_run = 0;
    Led_Output = 5;
    MAXP_REG(0) = 1;
    MAXP_REG(3) = TIMER100ms;
    printf(" * initialize exceptions...\n\r");
    int_par = XPAR_FABRIC_MEASDIF_0_MD_IRQ_INTR;
    Xil_ExceptionInit();

    printf(" * lookup config GIC...\n\r");
    IntcConfig = XScuGic_LookupConfig(XPAR_SCUGIC_0_DEVICE_ID);
    printf(" * initialize GIC...\n\r");
    XScuGic_CfgInitialize(&Intc, IntcConfig, IntcConfig->CpuBaseAddress);

	// Connect the interrupt controller interrupt handler to the hardware
    printf(" * connect interrupt controller handler...\n\r");
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
				(Xil_ExceptionHandler)XScuGic_InterruptHandler, &Intc);

    printf(" * set up fabric interrupt...\n\r");
    XScuGic_Connect(&Intc, XPAR_FABRIC_MEASDIF_0_MD_IRQ_INTR, (Xil_ExceptionHandler)FabricIntrHandler,
    				(void *)&int_par);
    // level triggered interrupt
    XScuGic_SetPriorityTriggerType(&Intc, XPAR_FABRIC_MEASDIF_0_MD_IRQ_INTR, 0x00, 0x01);
    print(" * enable interrupt for IRQ61 at GIC...\n\r");
    XScuGic_Enable(&Intc, XPAR_FABRIC_MEASDIF_0_MD_IRQ_INTR);

	// Enable interrupts in the Processor.
    printf(" * enable processor interrupts...\n\r");
	Xil_ExceptionEnable();


    terminate = 0;
    do {
    	print(">> "); fflush(stdout);
    	fgets(cbuf, CBUF_LEN, stdin);
    	cbuf[CBUF_LEN-1] = '\0';
    	chp = cbuf;
    	do {
    		if (*chp == '\n' || *chp == '\a' ) {
    			*chp = '\0';
    		}
    	} while (*chp++ != '\0');

    	printf("\n\r");  fflush(stdout);
    	if (!strncmp(cbuf, "exit", 4)) {
    		terminate = 1;
    	} else if (!strncmp(cbuf, "isr", 3)) {
    		if (isr_run == 0) {
    		    // start scu timer
    		    printf(" * start irq61...\n\r");
    		    MAXP_REG(3) = MBINT_ENABLE | TIMER100ms;
    		    isr_run = 1;
    		} else {
    		    // stop scu timer
    		    printf(" * stop irq61...\n\r");
    		    MAXP_REG(3) = TIMER100ms;
    			isr_run = 0;
    		}
    		printf("ISR count: %d\n\r", ISR_Count);
    	} else if (cbuf[0]=='r') {
    		if (sscanf(&cbuf[1], "%d", &regn) != 1) {
    			print(" *** conversion error\n\r");
    		} else {
    			xdata = MAXP_REG(regn);
    			xil_printf("R%d: %x\n\r", regn, xdata);
    		}
    	} else if (cbuf[0]=='w') {
    		if (sscanf(&cbuf[1], "%d %x", &regn, &xdata) != 2) {
    			printf(" *** conversion error\n\r");
    		} else {
    			MAXP_REG(regn) = xdata;
    			xil_printf("R%s <= %x\n\r", regn, xdata);
    		}
    	} else if (!strncmp(cbuf, "mr", 2)) {
    		if (sscanf(&cbuf[2], "%x", &regn) != 1) {
    			printf(" *** conversion error\n\r");
    		} else {
    			if (regn > 7) {
    				xil_printf(" *** illegal read address: %x\n\r", regn);
    			} else {
        			MaxRead(regn, 0);
    			}
    		}
    	} else if (!strncmp(cbuf, "mw", 2)) {
    		if (sscanf(&cbuf[2], "%x %x", &regn, &xdata) != 2) {
    			printf(" *** conversion error\n\r");
    		} else {
    			if ((regn == 1) || (regn == 2) || (regn > 6)) {
    				xil_printf(" *** illegal write address: %x\n\r", regn);
    			}
    			MaxWrite(regn, xdata, 0);
    		}
    	} else if (!strncmp(cbuf, "temp", 4)) {
    		if (sscanf(&cbuf[4], "%d", &loopc) != 1) {
    			printf(" *** conversion error\n\r");
    		} else {
    			MaxWrite(0x00, 0x081, 0);
    			SimpleDelay();
    			print("  k    raw   decimal   resistance   temperature\n\r");
    			print("-----------------------------------------------\n\r");
    			for (k = 0; k < loopc; k++) {
        			MaxWrite(0x00, 0x0a1, 0);
        			// SimpleDelay();
        			ccount = 0;
        			do {
            			xstatus = MAXP_REG(1);
            			xstatus &= 0x03;
            			ccount++;
        			} while ((xstatus == 0x03) && (ccount < 10000000));
        			printf("ccount: %8d\n\r", ccount);
        			xstatus = MAXP_REG(1);
        			xdata = 0xc0000000;
        			if ((xstatus & 0x03) != 0x01) {
        				printf(" *** status error: %d\n\r", xstatus);
        			} else {
        				xdata = MaxRead(0x01, 0);
        				xstatus = MAXP_REG(1);
        				printf("status after read: %6d\n\r", xstatus);
        				xdata <<= 8;
        				xlsb = MaxRead(0x02, 0);
        				xdata |= xlsb & 0x0ff;
        				xadc = xdata >> 1;
        				mresist = xadc * 0.01220703125;
        				mtemp = xadc * 0.03125 - 256.0;
        			}
        			printf("%3d   %4x     %5d   %10.5f    %10.5f\n\r", k, xdata, xadc, mresist, mtemp);
    			}
    			MaxWrite(0x00, 0x001, 1);
    			print("-----------------------------------------------\n\r");
    		}
    	} else if (!strncmp(cbuf, "adc", 3)) {
    		MAXP_REG(4) = 0;
			ccount = 0;
			do {
    			xstatus = MAXP_REG(4);
    			ccount++;
			} while ((xstatus == 0) && (ccount < 10000000));
			ad1 = MAXP_REG(5);
			ad0 = ad1 & 0x0fff;
			ad1 >>= 16;
    		printf(" ad0: %x  ad1: %x  (%d retries)\n\r", ad0, ad1, ccount);
    	} else {
    		printf("*** unknown command.\n\r");
    	}
	} while (terminate == 0);

    print("IVP shutdown.\n\r");
    MaxWrite(0x00, 0x001, 1);
    MAXP_REG(3) = TIMER100ms;
    Xil_ExceptionDisable();
    XScuGic_Disable(&Intc, XPAR_FABRIC_MEASDIF_0_MD_IRQ_INTR);
    MAXP_REG(0) = 0;
    cleanup_platform();
    return 0;
}
