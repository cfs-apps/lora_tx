/*
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU Lesser General Public License as
**  published by the Free Software Foundation, either version 3 of the
**  License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU Lesser General Public License for more details.
**
**  You should have received a copy of the GNU Lesser General Public License
**  along with this program.  If not, see <https://www.gnu.org/licenses/>.
**
**  Purpose:
**    Implement the Transmit Demo Class methods
**
**  Notes:
**    1. Refactored Stanford's Lora_tx.cpp functions into this object that
**       runs within the context of a cFS Basecamp app. This object provides
**       demo configuration and operations commands and uses a child task
**       to manage a transfer. 
**    2. Bridges SX128X C++ library and the main app and Basecamp's app_c_fw
**       written in C.  
**
*/

/*
** Include Files:
*/

#include <string.h>
#include "SX128x_Linux.hpp"
extern "C"
{
   #include "radio_tx.h"
}

/**********************/
/** Global File Data **/
/**********************/

// Pins based on hardware configuration
SX128x_Linux *Radio = NULL;


/*******************************/
/** Local Function Prototypes **/
/*******************************/

/******************************************************************************
** Function: RADIO_TX_InitRadio
**
** Initialize the Radio object to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**
*/
bool RADIO_TX_InitRadio(const char *SpiDevStr, uint8_t SpiDevNum, const RADIO_TX_Pin_t *RadioPin)
{
   bool RetStatus = false;
   
   SX128x_Linux::PinConfig PinConfig;
   
   PinConfig.busy  = RadioPin->Busy;
   PinConfig.nrst  = RadioPin->Nrst;
   PinConfig.nss   = RadioPin->Nss;
   PinConfig.dio1  = RadioPin->Dio1;
   PinConfig.dio2  = RadioPin->Dio2;
   PinConfig.dio3  = RadioPin->Dio3;
   PinConfig.tx_en = RadioPin->TxEn;
   PinConfig.rx_en = RadioPin->RxEn;
   
   try
   {
      Radio = new SX128x_Linux(SpiDevStr, SpiDevNum, PinConfig);
      RetStatus = true;
   }
   catch (...)
   {
      RetStatus = false;
   }
   
   return RetStatus;
   
} /* End RADIO_TX_InitRadio() */


/******************************************************************************
** Function: RADIO_TX_SetLoraParams
**
** Set the radio Lora parameters
**
** Notes:
**   None
**
*/
bool RADIO_TX_SetLoraParams(uint8_t SpreadingFactor,
                            uint8_t Bandwidth,
                            uint8_t CodingRate)
{
   
   SX128x::ModulationParams_t ModulationParams;
   
   ModulationParams.PacketType                  = SX128x::PACKET_TYPE_LORA;
   ModulationParams.Params.LoRa.CodingRate      = (SX128x::RadioLoRaCodingRates_t)CodingRate;
   ModulationParams.Params.LoRa.Bandwidth       = (SX128x::RadioLoRaBandwidths_t)Bandwidth;
   ModulationParams.Params.LoRa.SpreadingFactor = (SX128x::RadioLoRaSpreadingFactors_t)SpreadingFactor;

   Radio->SetModulationParams(ModulationParams);

   return true;
   
} /* RADIO_TX_SetLoraParams() */
                            
/******************************************************************************
** Function: RADIO_TX_SetSpiSpeed
**
** Set the SPI speed
**
** Notes:
**   1. Assumes Radio has been initialized and speed value has been validated 
**
*/
bool RADIO_TX_SetSpiSpeed(uint32_t SpiSpeed)
{
   
   Radio->SetSpiSpeed(SpiSpeed);
   
   return true;
   
} /* End RADIO_TX_SetSpiSpeed() */



/******************************************************************************
** Function: RADIO_TX_SetRadioFrequency
**
** Set the radio frequency
**
** Notes:
**   1. Assumes Radio has been initialized and frequency (Hz) value has been
**      validated 
**
*/
bool RADIO_TX_SetRadioFrequency(uint32_t Frequency)
{
   
   Radio->SetRfFrequency(Frequency);
   
   return true;
   
} /* End RADIO_TX_SetRadioFrequency() */


/* Pete's initial command list
#define GPIO_CTRL_SET_FREQ_EID     (GPIO_CTRL_BASE_EID + 4)
#define GPIO_CTRL_SET_TCXOEN_EID   (GPIO_CTRL_BASE_EID + 5)
#define GPIO_CTRL_SET_HSM_EID      (GPIO_CTRL_BASE_EID + 6)
#define GPIO_CTRL_SET_POWER_EID    (GPIO_CTRL_BASE_EID + 7)
#define GPIO_CTRL_SET_MOD_EID      (GPIO_CTRL_BASE_EID + 8)
#define GPIO_CTRL_SET_BW_EID       (GPIO_CTRL_BASE_EID + 9)
#define GPIO_CTRL_SET_SF_EID       (GPIO_CTRL_BASE_EID + 10)
#define GPIO_CTRL_SET_CR_EID       (GPIO_CTRL_BASE_EID + 11)
#define GPIO_CTRL_SET_CRC_EID      (GPIO_CTRL_BASE_EID + 12)
#define GPIO_CTRL_SET_LDRO_EID     (GPIO_CTRL_BASE_EID + 13)
#define GPIO_CTRL_SET_NODE_EID     (GPIO_CTRL_BASE_EID + 14)
#define GPIO_CTRL_SET_DEST_EID     (GPIO_CTRL_BASE_EID + 15)
#define GPIO_CTRL_SET_TXPA_EID     (GPIO_CTRL_BASE_EID + 14)
#define GPIO_CTRL_SET_RXLNA_EID    (GPIO_CTRL_BASE_EID + 15)
*/
/*******
int main(int argc, char **argv) {
	if (argc < 3) {
		printf("Usage: Lora_tx <freq in MHz> <file to send>\n");
		return 1;
	}

	// Pins based on hardware configuration
	SX128x_Linux Radio("/dev/spidev0.0", 0, {27, 26, 20, 16, -1, -1, 24, 25});

	// Assume we're running on a high-end Raspberry Pi,
	// so we set the SPI clock speed to the maximum value supported by the chip
	Radio.SetSpiSpeed(8000000);


	Radio.Init();
	puts("Init done");
	Radio.SetStandby(SX128x::STDBY_XOSC);
	puts("SetStandby done");
	Radio.SetRegulatorMode(static_cast<SX128x::RadioRegulatorModes_t>(0));
	puts("SetRegulatorMode done");
	Radio.SetLNAGainSetting(SX128x::LNA_HIGH_SENSITIVITY_MODE);
	puts("SetLNAGainSetting done");
	Radio.SetTxParams(0, SX128x::RADIO_RAMP_20_US);
	puts("SetTxParams done");

	Radio.SetBufferBaseAddresses(0x00, 0x00);
	puts("SetBufferBaseAddresses done");


	SX128x::ModulationParams_t ModulationParams;
	SX128x::PacketParams_t PacketParams;

    // Modulation Parameters
    ModulationParams.PacketType = SX128x::PACKET_TYPE_LORA;
    ModulationParams.Params.LoRa.CodingRate = SX128x::LORA_CR_4_8;
    ModulationParams.Params.LoRa.Bandwidth = SX128x::LORA_BW_1600;
    ModulationParams.Params.LoRa.SpreadingFactor = SX128x::LORA_SF7;

    PacketParams.PacketType = SX128x::PACKET_TYPE_LORA;

    // Packet Parameters
    auto &l = PacketParams.Params.LoRa;
    // l.PayloadLength = PACKET_SIZE;
    l.HeaderType = SX128x::LORA_PACKET_VARIABLE_LENGTH;
    l.PreambleLength = 12;
    l.Crc = SX128x::LORA_CRC_ON;
    l.InvertIQ = SX128x::LORA_IQ_NORMAL;

    Radio.SetPacketType(SX128x::PACKET_TYPE_LORA);

	puts("SetPacketType done");
	Radio.SetModulationParams(ModulationParams);
	puts("SetModulationParams done");
	Radio.SetPacketParams(PacketParams);
	puts("SetPacketParams done");

	auto freq = strtol(argv[1], nullptr, 10);
	Radio.SetRfFrequency(freq * 1000000UL);
	puts("SetRfFrequency done");

	std::cout << "Firmware version: " << Radio.GetFirmwareVersion() << "\n";

    // TX done interrupt handler
	Radio.callbacks.txDone = []{
		puts("Done!");
	};

	auto IrqMask = SX128x::IRQ_TX_DONE | SX128x::IRQ_RX_TX_TIMEOUT;
	Radio.SetDioIrqParams(IrqMask, IrqMask, SX128x::IRQ_RADIO_NONE, SX128x::IRQ_RADIO_NONE);
	puts("SetDioIrqParams done");

	Radio.StartIrqHandler();
	puts("StartIrqHandler done");


	auto pkt_ToA = Radio.GetTimeOnAir();

    // Open file
    std::ifstream fileToSend(argv[2], std::ios::binary);

    // Get size and number of packets
    fileToSend.seekg(0, std::ios::end);
    unsigned int fileSize = fileToSend.tellg();
    unsigned int numPackets = (fileSize + PACKET_SIZE - 1) / PACKET_SIZE;

    // Seek back to start
    fileToSend.seekg(0, std::ios::beg);


    printf("Sending %d packets (%d bytes)...\n", numPackets, fileSize);

    // Send packet with file size
    char numPacketsText[10];
    sprintf(numPacketsText, "%d", numPackets);
    unsigned int packetsTextLen = strlen(numPacketsText);

    printf("Sending num bytes...");
    Radio.SendPayload((uint8_t*)numPacketsText, packetsTextLen, {SX128x::RADIO_TICK_SIZE_1000_US, 1000});
    usleep((pkt_ToA + 20) * 1000);

    // Send all packets
    uint8_t buf[PACKET_SIZE];
    unsigned int index = 0;
    unsigned int totalPackets = 0;

    while (fileToSend) {
        char c = fileToSend.get();
        buf[index] = c;
        index++;

        if (index >= PACKET_SIZE) {
            // Reset index and send package 
            index = 0;
            Radio.SendPayload(buf, PACKET_SIZE, {SX128x::RADIO_TICK_SIZE_1000_US, 1000});
            printf("Sening packet %d...", totalPackets);

            printf("\n\n\nPACKET %d:\n", totalPackets);
            for (int i = 0; i < PACKET_SIZE; i++) {
                printf("%c", buf[i]);
            }
            printf("\n"); 

            usleep((pkt_ToA + 20) * 1000);
            totalPackets++;
        }
    }

    // Send partial last packet
    Radio.SendPayload(buf, index, {SX128x::RADIO_TICK_SIZE_1000_US, 1000});
    printf("Sent packet %d (partial last packet)...", totalPackets);
    usleep((pkt_ToA + 20) * 1000);

    printf("\n\n\nPACKET %d:\n", totalPackets);
    for (int i = 0; i < PACKET_SIZE; i++) {
        printf("%c", buf[i]);
    }


    fileToSend.close();

    printf("Exiting...\n");
    Radio.StopIrqHandler();
    return EXIT_SUCCESS;
}
**********/
