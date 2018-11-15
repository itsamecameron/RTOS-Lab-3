/*
*********************************************************************************************************
*                                               uC/OS-III
*                                         The Real-Time Kernel
*
*                             (c) Copyright 1998-2012, Micrium, Weston, FL
*                                          All Rights Reserved
*
*
*                                            PIC32 Sample code
*                                         modified for CENG 448/548 Lab3
*
* File : APP.C
*********************************************************************************************************
*/

#include <includes.h>

/*
*********************************************************************************************************
*                                                VARIABLES
*********************************************************************************************************
*/
//App TCB and Stacks
static  OS_TCB    App_TaskStartTCB;
static  CPU_STK   App_TaskStartStk[APP_CFG_TASK_START_STK_SIZE];
static  OS_TCB    prime_TCB;
static  OS_TCB    led_TCB;

static  CPU_STK   prime_stk[128];
static  CPU_STK   led_stk[128];

CPU_INT08U primeOut;
OS_MUTEX PrimesMutex;


//Global Variables

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  App_TaskCreate  (void);
static  void  App_ObjCreate   (void);

static  void  App_TaskStart   (void  *p_arg);

static void prime (void *p_arg);
static void led (void *p_arg);

/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.
*
* Arguments   : none
*********************************************************************************************************
*/

int  main (void)
{
    OS_ERR   os_err;

    CPU_Init();                                                           /* Initialize the uC/CPU services                           */

    BSP_IntDisAll();

    OSInit(&os_err);                                                      /* Init uC/OS-III.                                          */

    OSTaskCreate((OS_TCB      *)&App_TaskStartTCB,                        /* Create the start task                                    */
                 (CPU_CHAR    *)"Start",
                 (OS_TASK_PTR  )App_TaskStart,
                 (void        *)0,
                 (OS_PRIO      )APP_CFG_TASK_START_PRIO,
                 (CPU_STK     *)&App_TaskStartStk[0],
                 (CPU_STK_SIZE )APP_CFG_TASK_START_STK_SIZE_LIMIT,
                 (CPU_STK_SIZE )APP_CFG_TASK_START_STK_SIZE,
                 (OS_MSG_QTY   )0u,
                 (OS_TICK      )0u,
                 (void        *)0,
                 (OS_OPT       )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR      *)&os_err);

    OSStart(&os_err);                                                     /* Start multitasking (i.e. give control to uC/OS-III).     */

    (void)&os_err;

    return (0);
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                          STARTUP TASK
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
* Arguments   : p_arg   is the argument passed to 'AppStartTask()' by 'OSTaskCreate()'.
*********************************************************************************************************
*/

static  void  App_TaskStart (void *p_arg)
{
    OS_ERR  err;


    (void)p_arg;

    BSP_InitIO();                                                       /* Initialize BSP functions                                 */

    Mem_Init();                                                 /* Initialize memory managment module                   */
    Math_Init();                                                /* Initialize mathematical module                       */

#if (OS_CFG_STAT_TASK_EN > 0u)
    OSStatTaskCPUUsageInit(&err);                               /* Determine CPU capacity                               */
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif

    // Create new mutex
    OSMutexCreate(&PrimesMutex, "Primes Mutex", &err);

    App_TaskCreate();                                           /* Create Application tasks                             */
    //Create tasks
    //Create Prime number function
    OSTaskCreate((OS_TCB *)&prime_TCB,
            (CPU_CHAR *)"prime",
            (OS_TASK_PTR)prime,
            (void *)0,
            (OS_PRIO )2,
            (CPU_STK *)&prime_stk[0],
            (CPU_STK_SIZE)0,
            (CPU_STK_SIZE)128,
            (OS_MSG_QTY)0,
            (OS_TICK )0,
            (void *)0,
            (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
            (OS_ERR *)&err);

    //Create LED blinking function
    OSTaskCreate((OS_TCB *)&led_TCB,
            (CPU_CHAR *)"led",
            (OS_TASK_PTR)led,
            (void *)0,
            (OS_PRIO )2,
            (CPU_STK *)&led_stk[0],
            (CPU_STK_SIZE)0,
            (CPU_STK_SIZE)128,
            (OS_MSG_QTY)0,
            (OS_TICK )0,
            (void *)0,
            (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
            (OS_ERR *)&err);
    App_ObjCreate();                                            /* Create Application kernel objects                    */



}

/*
*********************************************************************************************************
*                                          AppTaskCreate()
*
* Description : Create application tasks.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : AppTaskStart()
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_TaskCreate (void)
{
}


/*
*********************************************************************************************************
*                                          App_ObjCreate()
*
* Description : Create application kernel objects tasks.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : AppTaskStart()
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_ObjCreate (void)
{

}

static void prime (void *p_arg)
{
    OS_ERR err;
    CPU_TS ts;
    CPU_INT08U primeflag, j;
    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */
      for(primeOut=2; primeOut<0x100; primeOut++)                                  // Cycle through integers 0-255
      {
          OSMutexPend(&PrimesMutex, 100, OS_OPT_PEND_BLOCKING, &ts, &err);
          primeflag=1;                                        // i assumed to be prime until proven otherwise
          for(j=2; j<primeOut; j++)                                  // Test to see if i is prime
          {
              if(primeOut%j==0)
              {
                  primeflag=0;
                  break;
              }
          }
          if(primeflag==1)                                    // if number was prime, light LEDs
          {
              // post task semaphore to LED task and pend for
              OSMutexPost(&PrimesMutex, OS_OPT_POST_NONE, &err);
              OSTaskSemPost(&led_TCB, OS_OPT_POST_NONE, &err);
              OSTaskSemPend(1000, OS_OPT_PEND_BLOCKING, &ts, &err);
          }
      }
    }
    return;
}

static void led (void *p_arg)
{

    OS_ERR err;
    CPU_TS ts;
    CPU_INT08U k, l;
    while (DEF_TRUE) {
      OSMutexPend(&PrimesMutex, 100, OS_OPT_PEND_BLOCKING, &ts, &err);
      for(k=primeOut, l=1; l<8; l++)                         // Test each bit and light appropriate LEDs
      {
          LED_Off(l);
          if(k&1u) {LED_On(l);}
          k >>= 1;
      }
      //Keep the lights on
      OSTimeDlyHMSM(0u, 0u, 0u, 1u, OS_OPT_TIME_HMSM_STRICT, &err);
      OSMutexPost(&PrimesMutex, OS_OPT_POST_NONE, &err);
      OSTaskSemPost(&prime_TCB, OS_OPT_POST_NONE, &err);
    }
    return;
}
