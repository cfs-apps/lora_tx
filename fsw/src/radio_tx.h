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
**    Define radio interface class
**
**  Notes:
**    1. Serves as a bridge between the C++ Radio object and the
**       Loral Tx app.
**
*/

#ifndef _radio_tx_
#define _radio_tx_

/*
** Includes
*/
#include <stdint.h>

/***********************/
/** Macro Definitions **/
/***********************/


/**********************/
/** Type Definitions **/
/**********************/


typedef struct
{
   uint8_t Busy;
   uint8_t Nrst;
   uint8_t Nss;
   uint8_t Dio1;
   uint8_t Dio2;
   uint8_t Dio3;
   uint8_t TxEn;
   uint8_t RxEn;
      
} RADIO_TX_Pin_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: RADIO_TX_InitRadio
**
** Initialize the Radio object to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**
*/
bool RADIO_TX_InitRadio(const char *SpiDevStr, uint8_t SpiDevNum, const RADIO_TX_Pin_t *RadioPin);


/******************************************************************************
** Function: RADIO_TX_SetSpiSpeed
**
** Set the SPI speed
**
** Notes:
**   None
**
*/
bool RADIO_TX_SetSpiSpeed(uint32_t SpiSpeed);


#endif /* _radio_tx_ */
