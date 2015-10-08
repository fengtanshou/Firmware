/* Copyright 2015, Daniel Cohen
 * Copyright 2015, Esteban Volentini
 * Copyright 2015, Matias Giori
 * Copyright 2015, Franco Salinas
 * Copyright 2015, Pablo Alcorta
 *
 * This file is part of CIAA Firmware.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/** \brief Update Test source file
 **
 ** This test creates a thread for the master task and a thread for the slave.
 ** A loopback transport layer is used to communicate both threads. A certain
 ** amount of data segments are generated by the master. These data segments
 ** are parsed and sent to the slave. In a real application the data segments
 ** would be ciaaPOSIX_read from an ELF or S19 file and a suitable parser would be used.
 ** The slave writes the received data on the flash device.
 **/

/** \addtogroup CIAA_Firmware CIAA Firmware
 ** @{ */
/** \addtogroup MTests CIAA Firmware Module Tests
 ** @{ */
/** \addtogroup Update Update Module Tests
 ** @{ */

/*
 * Initials     Name
 * ---------------------------
 * DC           Daniel Cohen
 * EV           Esteban Volentini
 * MG           Matias Giori
 * FS           Franco Salinas
 * PA           Pablo Alcorta
 */

/*
 * modification history (new versions first)
 * -----------------------------------------------------------
 * 20150408 v0.0.1   FS   first initial version
 */

/*==================[inclusions]=============================================*/
#include "os.h"               /* <= operating system header */
#include "ciaaLibs_CircBuf.h"
#include "ciaaPOSIX_assert.h" /* <= ciaaPOSIX_assert header */
#include "ciaaPOSIX_stdio.h"  /* <= device handler header */
#include "ciaak.h"            /* <= ciaa kernel header */
#include "UPDT_services.h"
#include "test_protocol_loopback.h"
#include "UPDT_utils.h"

/*==================[macros and definitions]=================================*/
#define DATA_SIZE 1024

typedef struct {
   uint32_t reserved1;
   uint32_t firmware_version;
   uint32_t bootloader_flags;
   uint32_t bootloader_version;
   uint32_t reserved2;
   uint32_t application_version;
   uint32_t vendor_id;
   uint32_t model_id;
   uint32_t unique_id_L;
   uint32_t unique_id_H;
   uint32_t data_size;
} test_updt_configType;

/*==================[internal data declaration]==============================*/

/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/
/* binary image to transmit */
static uint8_t input[DATA_SIZE];

/* master side*/
static test_update_loopbackType master_transport;
static uint8_t payload_size=248;
/* slave side */
static test_update_loopbackType slave_transport;
static int32_t slave_fd = -1;
/*Sequence number*/
static uint8_t SequenceNumber=1;
/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/

static void test_updt_configFormat(test_updt_configType *type, uint8_t *config_buffer,size_t data_size)
{
   ciaaPOSIX_assert(data_size == 32);
   /*Set of the field "reserved1" in the buffer*/
   config_buffer[0] = type->reserved1;
   /*Set of the field "firmware_version" in the buffer*/
   config_buffer[1] = (type->firmware_version) >> 16;
   config_buffer[2] = (type->firmware_version) >> 8;
   config_buffer[3] = type->firmware_version;
   /*Set of the field "bootloader_flags" in the buffer*/
   config_buffer[4] = type->bootloader_flags;
   /*Set of the field "bootloader_version" in the buffer*/
   config_buffer[5] = (type->bootloader_version) >> 16;
   config_buffer[6] = (type->bootloader_version) >> 8;
   config_buffer[7] = type->bootloader_version;
   /*Set of the field "reserved2" in the buffer*/
   config_buffer[8] = type->reserved2;
   /*Set of the field "application_version" in the buffer*/
   config_buffer[9] = (type->application_version) >> 16;
   config_buffer[10] = (type->application_version) >> 8;
   config_buffer[11] = type->application_version;
   /*Set of the field "vendor_id" in the buffer*/
   config_buffer[12] = type->vendor_id;
   /*Set of the field "model_id" in the buffer*/
   config_buffer[13] = (type->model_id) >> 16;
   config_buffer[14] = (type->model_id) >> 8;
   config_buffer[15] = type->model_id;
   /*Set of the field "unique_id" in the buffer*/
   #if CIAAPLATFORM_BIGENDIAN == 0
   config_buffer[16] = (type->unique_id_L) >> 24;
   config_buffer[17] = (type->unique_id_L) >> 16;
   config_buffer[18] = (type->unique_id_L) >> 8;
   config_buffer[19] =  type->unique_id_L;
   config_buffer[20] = (type->unique_id_H) >> 24;
   config_buffer[21] = (type->unique_id_H) >> 16;
   config_buffer[22] = (type->unique_id_H) >> 8;
   config_buffer[23] =  type->unique_id_H;
   #else
   config_buffer[16] = (type->unique_id_H) >> 24;
   config_buffer[17] = (type->unique_id_H) >> 16;
   config_buffer[18] = (type->unique_id_H) >> 8;
   config_buffer[19] =  type->unique_id_H;
   config_buffer[20] = (type->unique_id_L) >> 24;
   config_buffer[21] = (type->unique_id_L) >> 16;
   config_buffer[22] = (type->unique_id_L) >> 8;
   config_buffer[23] =  type->unique_id_L;
   #endif // CIAAPLATFORM_BIGENDIAN
   /*Set of the field "data_size" in the buffer*/
   config_buffer[24] = (type->data_size) >> 24;
   config_buffer[25] = (type->data_size) >> 16;
   config_buffer[26] = (type->data_size) >> 8;
   config_buffer[27] = type->data_size;
}


static void test_update_value (test_updt_configType *values)
{
   values->reserved1 = 0;
   values->firmware_version = 2 << 16;
   values->firmware_version |= 3 << 8;
   values->firmware_version |= 4;
   values->bootloader_flags = 6;
   values->bootloader_version = 8 << 16;
   values->bootloader_version |= 9 << 8;
   values->bootloader_version |= 10;
   values->reserved2 = 0;
   values->application_version = 12 << 16;
   values->application_version |= 13 <<8;
   values->application_version |= 14;
   values->vendor_id = 15;
   values->model_id = 16 << 16;
   values->model_id |= 17 << 8;
   values->model_id |= 18;
   #if CIAAPLATFORM_BIGENDIAN == 0
   values->unique_id_L = 20 << 24;
   values->unique_id_L |= 21 << 16;
   values->unique_id_L |= 22 << 8;
   values->unique_id_L |= 23;
   values->unique_id_H |= 24 << 24;
   values->unique_id_H |= 25 << 16;
   values->unique_id_H |= 26 << 8;
   values->unique_id_H |= 27;
   #else
   values->unique_id_H = 24 << 24;
   values->unique_id_H |= 25 << 16;
   values->unique_id_H |= 26 << 8;
   values->unique_id_H |= 27;
   values->unique_id_L |= 20 << 24;
   values->unique_id_L |= 21 << 16;
   values->unique_id_L |= 22 << 8;
   values->unique_id_L |= 23;
   #endif // CIAAPLATFORM_BIGENDIAN
   values->data_size = 28 << 24;
   values->data_size |= 29 << 16;
   values->data_size |= 30 << 8;
   values->data_size |= 31;
}

static void testUpdtValueData (uint8_t *vector, uint32_t paySize)
{
   uint8_t i;
   for (i=0;i<paySize;i++)
   {
      vector[i]=ciaaPOSIX_rand();
   }
}

static void makeHandshakeOk (test_updt_configType *type,uint8_t *vector)
{
   UPDT_protocolSetHeader (vector,UPDT_PROTOCOL_PACKET_INF,SN,32);
   test_update_value (type);
   test_updt_configFormat (type,vector+4,32);
   SN=SN+1;
}

static uint32_t testHandshakeOk (test_updt_configType *type,uint8_t *vector)
{
   ciaaPOSIX_assert (UPDT_PROTOCOL_PACKET_ACK == UPDT_protocolGetPacketType(vector));
   ciaaPOSIX_assert (SequenceNumber== UPDT_protocolGetSequenceNumber(vector));
   return 0;
}


static void makeHandshakeOkNextSequenceNumber(test_updt_configType *type, uint8_t *vector)
{
   UPDT_protocolSetHeader (vector, UPDT_PROTOCOL_PACKET_INF,SN,32);
   test_update_value (type);
   test_updt_configFormat (type,vector+4,32);
   SN=SN+1;
}

static uint32_t testHandshakeOkNextSN(test_updt_configType *type, uint8_t *vector)
{
   ciaaPOSIX_assert (UPDT_PROTOCOL_PACKET_ACK == UPDT_protocolGetPacketType(vector));
   ciaaPOSIX_assert (SN==UPDT_protocolGetSequenceNumber(vector));
   return 0;
}

static void makeDataOk (test_updt_configType *type, uint8_t *vector, uint8_t payload_size)
{
   UPDT_protocolSetHeader (vector,UPDT_PROTOCOL_PACKET_DAT,SN,32);
   testUpdtValueData (vector+4,payload_size);
}

static uint32_t testDataOk(uint8_t *vector)
{
   ciaaPOSIX_assert (UPDT_PROTOCOL_PACKET_ACK == UPDT_protocolGetPacketType(vector));
   ciaaPOSIX_assert (SN==UPDT_protocolGetSequenceNumber(vector));
   return 0;
}

/*==================[external functions definition]==========================*/
/** \brief Main function
 *
 * This is the main entry point of the software.
 *
 * \return 0
 *
 * \remarks This function never returns. Return value is only to avoid compiler
 *          warnings or errors.
 */
int main(void)
{
   /* Starts the operating system in the Application Mode 1 */
   /* This example has only one Application Mode */
   StartOS(AppMode1);

   /* StartOs shall never returns, but to avoid compiler warnings or errors
    * 0 is returned */
   return 0;
}

/** \brief Error Hook function
 *
 * This function is called from the OS if an OS interface (API) returns an
 * error. Is for debugging proposes. If called this function triggers a
 * ShutdownOs which ends in a while(1).
 *
 * The values:
 *    OSErrorGetServiceId
 *    OSErrorGetParam1
 *    OSErrorGetParam2
 *    OSErrorGetParam3
 *    OSErrorGetRet
 *
 * will provide you the interface, the input parameters and the returned value.
 * For more details see the OSEK specification:
 * http://portal.osek-vdx.org/files/pdf/specs/os223.pdf
 *
 */
void ErrorHook(void)
{
   ciaaPOSIX_printf("ErrorHook was called\n");
   ciaaPOSIX_printf("Service: %d, P1: %d, P2: %d, P3: %d, RET: %d\n", OSErrorGetServiceId(), OSErrorGetParam1(), OSErrorGetParam2(), OSErrorGetParam3(), OSErrorGetRet());
   ShutdownOS(0);
}

/** \brief Initial task
 *
 * This task is started automatically in the application mode 1.
 */
TASK(InitTask)
{
   int32_t ret;
   /* init CIAA kernel and devices */
   ciaak_start();

   ciaaPOSIX_printf("Init Task\n");

   /* initialize loopback transport layers */
   ret = test_update_loopbackInit(&master_transport, MasterTask, MASTER_RECV_EVENT);
   ciaaPOSIX_assert(0 == ret);

   ret = test_update_loopbackInit(&slave_transport, SlaveTask, SLAVE_RECV_EVENT);
   ciaaPOSIX_assert(0 == ret);

   /* connect master and slave */
   test_update_loopbackConnect(&master_transport, &slave_transport);

   /* activate the MasterTask task */
   ActivateTask(MasterTask);

   /* activate the SlaveTask task */
   ActivateTask(SlaveTask);

   /* end InitTask */
   TerminateTask();
}

/** \brief Master Task */
TASK(MasterTask)
{
   uint8_t vector[36];
   test_updt_configType type;
   ciaaPOSIX_printf("Master Task\n");

   /** \todo Handshake */

   /*initialize handshake packet for send*/
   makeHandshakeOk (&type, vector);
   /*send Handshake packet*/
   ciaaPOSIX_assert (UPDT_protocolSend((UPDT_ITransportType *) &master_transport, vector, 32) == UPDT_PROTOCOL_ERROR_NONE);
   /*received answer*/
   ciaaPOSIX_assert (UPDT_protocolRecv((UPDT_ITransportType *) &master_transport, vector,32) == UPDT_PROTOCOL_ERROR_NONE);
   /*testing SequenceNumberand package type of answer*/
   ciaaPOSIX_assert(testHandshakeOk (&type,vector)==0);

   /*initialize data packet for send*/
   makeDataOk (&type,vector, payload_size);
   /*send data packet*/
   ciaaPOSIX_assert (UPDT_protocolSend((UPDT_ITransportType *) &master_transport, vector, payload_size) == UPDT_PROTOCOL_ERROR_NONE);
   /*received answer*/
   ciaaPOSIX_assert (UPDT_protocolRecv((UPDT_ITransportType *) &master_transport, vector,payload_size) == UPDT_PROTOCOL_ERROR_NONE);
   /*testing SequenceNumberand package type of answer*/
   ciaaPOSIX_assert (testDataOk(vector)==0);

   #if(0)
   do
   {
      /* request a block of packed data */
      bytes_packed = UPDT_packerGet(&packer);
      ciaaPOSIX_assert(bytes_packed >= 0);

      /** \todo encrypt */
      /** \todo calculate signature */

      /* send the packet data */
      bytes_sent =  UPDT_masterSendData(&master, master_buffer, bytes_packed);

   /* it finishes when we send a block of data smaller than the others */
   } while(bytes_sent == UPDT_PROTOCOL_PAYLOAD_MAX_SIZE);
   #endif


   /** \todo send signature */

   TerminateTask();
}

/** \brief Slave Task */
TASK(SlaveTask)
{
   ciaaPOSIX_printf("Slave Task\n");

   /* open flash device */
   slave_fd = ciaaPOSIX_open("/dev/block/fd/0", ciaaPOSIX_O_RDWR);
   ciaaPOSIX_assert(0 >= slave_fd);

   /* start the update service */
   UPDT_servicesStart((UPDT_ITransportType *) &slave_transport, slave_fd);

   /* close the device */
   ciaaPOSIX_close(slave_fd);
   TerminateTask();
}

/** @} doxygen end group definition */
/** @} doxygen end group definition */
/** @} doxygen end group definition */
/*==================[end of file]============================================*/

