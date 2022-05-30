/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech
 ___ _____ _   ___ _  _____ ___  ___  ___ ___
/ __|_   _/_\ / __| |/ / __/ _ \| _ \/ __| __|
\__ \ | |/ _ \ (__| ' <| _| (_) |   / (__| _|
|___/ |_/_/ \_\___|_|\_\_| \___/|_|_\\___|___|
embedded.connectivity.solutions===============

Description: LoRa MAC region CN470A implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis ( Semtech ), Gregory Cristian ( Semtech ) and Daniel Jaeckle ( STACKFORCE )
*/

#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "radio.h"
#include "timer.h"
#include "LoRaMac.h"

#include "utilities.h"

#include "Region.h"
#include "RegionCommon.h"
#include "RegionCN470A.h"

#include "lwan_config.h"
#include "linkwan.h"
#include "commissioning.h"

// Definitions
#define CHANNELS_MASK_SIZE 8
#define RADIO_WAKEUP_TIME 2
#define CHANNELS_MASK_CNTL_RFU 0x0FF
#define CHANNELS_MASK_ALL_ON   0x0FE

// Global attributes
/*!
 * LoRaMAC channels
 */
static ChannelParams_t Channels[CN470A_MAX_NB_CHANNELS];
static ChannelParams_t TxChannels[CN470A_MAX_NB_CHANNELS];
static uint8_t TxFreqBandNum = 0;

/*!
 * LoRaMac bands
 */
static Band_t Bands[CN470A_MAX_NB_BANDS] = {
    CN470A_BAND0
};

/*!
 * LoRaMac channels mask
 */
static uint16_t ChannelsMask[CHANNELS_MASK_SIZE] = {0};

/*!
 * LoRaMac channels default mask
 */
static uint16_t ChannelsDefaultMask[CHANNELS_MASK_SIZE] = {0};

uint8_t NumFreqBand;
uint8_t FreqBandNum[16] = {0};
uint8_t FreqBandStartChannelNum[16] = {0, 8, 16, 24, 100, 108, 116, 124, 68, 76, 84, 92, 166, 174, 182, 190};
uint8_t ChMaskCntlToStartBandNum[8] = {0, 2, 12, 14, CHANNELS_MASK_ALL_ON, CHANNELS_MASK_CNTL_RFU,
                                       CHANNELS_MASK_CNTL_RFU, CHANNELS_MASK_CNTL_RFU};
uint8_t NextAvailableFreqBandIdx;
uint16_t scan_mask;
uint8_t InterFreqRx2Chan[16] = {75, 83, 91, 99, 173, 181, 189, 197, 7, 15, 23, 31, 107, 115, 123, 131};
uint8_t IntraFreqRx2Chan[16] = {7, 15, 23, 31, 107, 115, 123, 131, 75, 83, 91, 99, 173, 181, 189, 197};

// Static functions
static int8_t GetNextLowerTxDr( int8_t dr, int8_t minDr )
{
    uint8_t nextLowerDr = 0;

    if ( dr == minDr ) {
        nextLowerDr = minDr;
    } else {
        nextLowerDr = dr - 1;
    }
    return nextLowerDr;
}

static uint32_t GetBandwidth( uint32_t drIndex )
{
    switch ( BandwidthsCN470A[drIndex] ) {
        default:
        case 125000:
            return 0;
        case 250000:
            return 1;
        case 500000:
            return 2;
    }
}

static int8_t LimitTxPower( int8_t txPower, int8_t maxBandTxPower, int8_t datarate, uint16_t *channelsMask )
{
    int8_t txPowerResult = txPower;

    // Limit tx power to the band max
    txPowerResult =  MAX( txPower, maxBandTxPower );

    return txPowerResult;
}

static bool VerifyTxFreq( uint32_t freq )
{
    // Check radio driver support
    if ( Radio.CheckRfFrequency( freq ) == false ) {
        return false;
    }

    if ( ( freq < 470000000 ) || ( freq > 515000000 ) ) {
        return false;
    }
    return true;
}

static uint8_t CountNbOfEnabledChannels( bool joined, uint8_t datarate, uint16_t *channelsMask,
                                         ChannelParams_t *channels, Band_t *bands, uint8_t *enabledChannels, uint8_t *delayTx )
{
    uint8_t nbEnabledChannels = 0;
    uint8_t delayTransmission = 0;

    for (uint8_t j = 0; j < 8; j++) {
        if ((channelsMask[0] & (1 << j)) != 0) {
            // Check if the channel is enabled
            if (channels[j].Frequency == 0) {
                continue;
            }
            if (joined == false && (CN470A_JOIN_CHANNELS & (1 << j)) == 0) {
                continue;
            }
            //Check if the current channel selection supports the given datarate
            if (RegionCommonValueInRange(datarate, channels[j].DrRange.Fields.Min,
                                         channels[j].DrRange.Fields.Max) == false) {
                continue;
            }
            // Check if the band is available for transmission
            if (bands[channels[j].Band].TimeOff > 0) {
                delayTransmission++;
                continue;
            }
            enabledChannels[nbEnabledChannels++] = j;
        }
    }

    *delayTx = delayTransmission;
    return nbEnabledChannels;
}

PhyParam_t RegionCN470AGetPhyParam( GetPhyParams_t *getPhy )
{
    PhyParam_t phyParam;

    switch ( getPhy->Attribute ) {
        case PHY_MIN_RX_DR: {
            phyParam.Value = CN470A_RX_MIN_DATARATE;
            break;
        }
        case PHY_MIN_TX_DR: {
            phyParam.Value = CN470A_TX_MIN_DATARATE;
            break;
        }
        case PHY_DEF_TX_DR: {
            phyParam.Value = CN470A_DEFAULT_DATARATE;
            break;
        }
        case PHY_NEXT_LOWER_TX_DR: {
            phyParam.Value = GetNextLowerTxDr( getPhy->Datarate, DR_2);
            break;
        }
        case PHY_DEF_TX_POWER: {
            phyParam.Value = CN470A_DEFAULT_TX_POWER;
            break;
        }
        case PHY_MAX_PAYLOAD: {
            phyParam.Value = MaxPayloadOfDatarateCN470A[getPhy->Datarate];
            break;
        }
        case PHY_MAX_PAYLOAD_REPEATER: {
            phyParam.Value = MaxPayloadOfDatarateRepeaterCN470A[getPhy->Datarate];
            break;
        }
        case PHY_DUTY_CYCLE: {
            phyParam.Value = CN470A_DUTY_CYCLE_ENABLED;
            break;
        }
        case PHY_MAX_RX_WINDOW: {
            phyParam.Value = CN470A_MAX_RX_WINDOW;
            break;
        }
        case PHY_RECEIVE_DELAY1: {
            phyParam.Value = CN470A_RECEIVE_DELAY1;
            break;
        }
        case PHY_RECEIVE_DELAY2: {
            phyParam.Value = CN470A_RECEIVE_DELAY2;
            break;
        }
        case PHY_JOIN_ACCEPT_DELAY1: {
            phyParam.Value = CN470A_JOIN_ACCEPT_DELAY1;
            break;
        }
        case PHY_JOIN_ACCEPT_DELAY2: {
            phyParam.Value = CN470A_JOIN_ACCEPT_DELAY2;
            break;
        }
        case PHY_MAX_FCNT_GAP: {
            phyParam.Value = CN470A_MAX_FCNT_GAP;
            break;
        }
        case PHY_ACK_TIMEOUT: {
            phyParam.Value = ( CN470A_ACKTIMEOUT + randr( -CN470A_ACK_TIMEOUT_RND, CN470A_ACK_TIMEOUT_RND ) );
            break;
        }
        case PHY_DEF_DR1_OFFSET: {
            phyParam.Value = CN470A_DEFAULT_RX1_DR_OFFSET;
            break;
        }
        case PHY_DEF_RX2_FREQUENCY: {
            phyParam.Value = CN470A_RX_WND_2_FREQ;
            break;
        }
        case PHY_DEF_RX2_DR: {
            phyParam.Value = CN470A_RX_WND_2_DR;
            break;
        }
        case PHY_CHANNELS_MASK: {
            phyParam.ChannelsMask = ChannelsMask;
            break;
        }
        case PHY_CHANNELS_DEFAULT_MASK: {
            phyParam.ChannelsMask = ChannelsDefaultMask;
            break;
        }
        case PHY_MAX_NB_CHANNELS: {
            phyParam.Value = CN470A_MAX_NB_CHANNELS;
            break;
        }
        case PHY_CHANNELS: {
            phyParam.Channels = Channels;
            break;
        }
        case PHY_DEF_UPLINK_DWELL_TIME:
        case PHY_DEF_DOWNLINK_DWELL_TIME: {
            phyParam.Value = 0;
            break;
        }
        case PHY_DEF_MAX_EIRP: {
            phyParam.fValue = CN470A_DEFAULT_MAX_EIRP;
            break;
        }
        case PHY_DEF_ANTENNA_GAIN: {
            phyParam.fValue = CN470A_DEFAULT_ANTENNA_GAIN;
            break;
        }
        case PHY_NB_JOIN_TRIALS:
        case PHY_DEF_NB_JOIN_TRIALS: {
            phyParam.Value = 48;
            break;
        }
        case PHY_BEACON_CHANNEL_FREQ:
        {
            phyParam.Value = Channels[0].Frequency;
            break;
        }
        case PHY_BEACON_FORMAT:
        {
            phyParam.BeaconFormat.BeaconSize = CN470A_BEACON_SIZE;
            phyParam.BeaconFormat.Rfu1Size = CN470A_RFU1_SIZE;
            phyParam.BeaconFormat.Rfu2Size = CN470A_RFU2_SIZE;
            break;
        }
        case PHY_BEACON_CHANNEL_DR:
        {
            phyParam.Value = CN470A_BEACON_CHANNEL_DR;
            break;
        }
        case PHY_BEACON_CHANNEL_STEPWIDTH:
        {
            phyParam.Value = CN470A_BEACON_CHANNEL_STEPWIDTH;
            break;
        }
        case PHY_BEACON_NB_CHANNELS:
        {
            phyParam.Value = CN470A_BEACON_NB_CHANNELS;
            break;
        }

        default: {
            break;
        }
    }

    return phyParam;
}

void RegionCN470ASetBandTxDone( SetBandTxDoneParams_t *txDone )
{
    RegionCommonSetBandTxDone(txDone->Joined, &Bands[Channels[txDone->Channel].Band], txDone->LastTxDoneTime );
}

void RegionCN470AInitDefaults( InitType_t type )
{
    switch ( type ) {
        case INIT_TYPE_INIT: {
            // Channels
            Channels[0] = ( ChannelParams_t ) CN470A_LC1;
            Channels[1] = ( ChannelParams_t ) CN470A_LC2;
            Channels[2] = ( ChannelParams_t ) CN470A_LC3;
            Channels[3] = ( ChannelParams_t ) CN470A_LC4;
            Channels[4] = ( ChannelParams_t ) CN470A_LC5;
            Channels[5] = ( ChannelParams_t ) CN470A_LC6;
            Channels[6] = ( ChannelParams_t ) CN470A_LC7;
            Channels[7] = ( ChannelParams_t ) CN470A_LC8;

            // Initialize the channels default mask
            ChannelsDefaultMask[0] = (LC( 1 ) + LC( 2 ) + LC( 3 ) + LC( 4 ) + LC( 5 ) + LC( 6 ) + LC( 7 ) + LC( 8 )) << 8 ;
            // Update the channels mask
            RegionCommonChanMaskCopy( ChannelsMask, ChannelsDefaultMask,  CHANNELS_MASK_SIZE);

            //set default freqband = 1A2(No.=1,471.9Mhz)
            NumFreqBand = 1;
            FreqBandNum[0] = 1; //1A2
            scan_mask = 0;

            //save other freqband from mask
            //for (uint8_t i = 0; i < 16; i++) {
            //    if ((get_lora_freqband_mask() & (1 << i)) != 0 && i != 1) {
            //        FreqBandNum[NumFreqBand++] = i;
            //    }
            //}
            NextAvailableFreqBandIdx = 0;
            break;
        }
        case INIT_TYPE_RESTORE: {
            // Restore channels default mask
            ChannelsMask[0] |= ChannelsDefaultMask[0];
            break;
        }
        default: {
            break;
        }
    }
}

bool RegionCN470AVerify( VerifyParams_t *verify, PhyAttribute_t phyAttribute )
{
    switch ( phyAttribute ) {
        case PHY_TX_DR: {
            return RegionCommonValueInRange( verify->DatarateParams.Datarate, CN470A_TX_MIN_DATARATE, CN470A_TX_MAX_DATARATE );
        }
        case PHY_DEF_TX_DR: {
            return RegionCommonValueInRange( verify->DatarateParams.Datarate, DR_0, DR_5 );
        }
        case PHY_RX_DR: {
            return RegionCommonValueInRange( verify->DatarateParams.Datarate, CN470A_RX_MIN_DATARATE, CN470A_RX_MAX_DATARATE );
        }
        case PHY_DEF_TX_POWER:
        case PHY_TX_POWER: {
            // Remark: switched min and max!
            return RegionCommonValueInRange( verify->TxPower, CN470A_MAX_TX_POWER, CN470A_MIN_TX_POWER );
        }
        case PHY_DUTY_CYCLE: {
            return CN470A_DUTY_CYCLE_ENABLED;
        }
        case PHY_NB_JOIN_TRIALS: {
            break;
        }
        default:
            return false;
    }
    return true;
}

void RegionCN470AApplyCFList( ApplyCFListParams_t *applyCFList )
{
    ChannelParams_t newChannel;
    ChannelAddParams_t channelAdd;
    ChannelRemoveParams_t channelRemove;

    // Setup default datarate range
    newChannel.DrRange.Value = ( DR_5 << 4 ) | DR_0;

    // Size of the optional CF list
    if ( applyCFList->Size != 16 ) {
        return;
    }

    // Last byte is RFU, don't take it into account
    for ( uint8_t i = 0, chanIdx = CN470A_NUMB_DEFAULT_CHANNELS; i < 15; i += 3, chanIdx++ ) {
        // Channel frequency
        newChannel.Frequency = (uint32_t) applyCFList->Payload[i];
        newChannel.Frequency |= ( (uint32_t) applyCFList->Payload[i + 1] << 8 );
        newChannel.Frequency |= ( (uint32_t) applyCFList->Payload[i + 2] << 16 );
        newChannel.Frequency *= 100;

        // Initialize alternative frequency to 0
        newChannel.Rx1Frequency = 0;

        if ( newChannel.Frequency != 0 ) {
            channelAdd.NewChannel = &newChannel;
            channelAdd.ChannelId = chanIdx;

            // Try to add all channels
            RegionCN470AChannelAdd( &channelAdd );
        } else {
            channelRemove.ChannelId = chanIdx;

            RegionCN470AChannelsRemove( &channelRemove );
        }
    }
}

bool RegionCN470AChanMaskSet( ChanMaskSetParams_t *chanMaskSet )
{
    switch ( chanMaskSet->ChannelsMaskType ) {
        case CHANNELS_MASK: {
            //set default freqband = 1A2(No.=1,471.9Mhz)
            if ((chanMaskSet->ChannelsMaskIn[0] & 0xFF00) != 0)
            {
                NumFreqBand = 1;
                FreqBandNum[0] = 1; //1A2
            }
            else
            {
                NumFreqBand = 0;
            }
            scan_mask = 0;
            //save other freqband from mask
            for (uint8_t i = 0; i < 16; i++) {
                if ((*((uint8_t *)chanMaskSet->ChannelsMaskIn +i) != 0) && i != 1) {
                    FreqBandNum[NumFreqBand++] = i;
                }
            }
            NextAvailableFreqBandIdx = 0;
            RegionCommonChanMaskCopy(ChannelsMask, chanMaskSet->ChannelsMaskIn, CHANNELS_MASK_SIZE );
            break;
        }
        case CHANNELS_DEFAULT_MASK: {
            RegionCommonChanMaskCopy( ChannelsDefaultMask, chanMaskSet->ChannelsMaskIn, 1 );
            break;
        }
        default:
            return false;
    }
    return true;
}

bool RegionCN470AAdrNext( AdrNextParams_t *adrNext, int8_t *drOut, int8_t *txPowOut, uint32_t *adrAckCounter )
{
    bool adrAckReq = false;
    int8_t datarate = adrNext->Datarate;
    int8_t txPower = adrNext->TxPower;
    GetPhyParams_t getPhy;
    PhyParam_t phyParam;

    // Report back the adr ack counter
    *adrAckCounter = adrNext->AdrAckCounter;

    if ( adrNext->AdrEnabled == true ) {
        if ( datarate == CN470A_TX_MIN_DATARATE ) {
            *adrAckCounter = 0;
            adrAckReq = false;
        } else {
            if ( adrNext->AdrAckCounter >= CN470A_ADR_ACK_LIMIT ) {
                adrAckReq = true;
                txPower = CN470A_MAX_TX_POWER;
            } else {
                adrAckReq = false;
            }
            if ( adrNext->AdrAckCounter >= ( CN470A_ADR_ACK_LIMIT + CN470A_ADR_ACK_DELAY ) ) {
                if ( ( adrNext->AdrAckCounter % CN470A_ADR_ACK_DELAY ) == 0 ) {
                    // Decrease the datarate
                    getPhy.Attribute = PHY_NEXT_LOWER_TX_DR;
                    getPhy.Datarate = datarate;
                    getPhy.UplinkDwellTime = adrNext->UplinkDwellTime;
                    phyParam = RegionCN470AGetPhyParam( &getPhy );
                    datarate = phyParam.Value;

                    if ( datarate == CN470A_TX_MIN_DATARATE ) {
                        if ( adrNext->UpdateChanMask == true ) {
                            // Re-enable default channels
                            ChannelsMask[0] |= LC( 1 ) + LC( 2 ) + LC( 3 );
                        }
                    }
                }
            }
        }
    }

    *drOut = datarate;
    *txPowOut = txPower;
    return adrAckReq;
}

void RegionCN470AComputeRxWindowParameters( int8_t datarate, uint8_t minRxSymbols, uint32_t rxError,
                                            RxConfigParams_t *rxConfigParams )
{
    double tSymbol = 0.0;

    rxConfigParams->Datarate = datarate;
    rxConfigParams->Bandwidth = GetBandwidth( datarate );

    if ( datarate == DR_7 ) {
        // FSK
        tSymbol = RegionCommonComputeSymbolTimeFsk( DataratesCN470A[datarate] );
    } else {
        // LoRa
        tSymbol = RegionCommonComputeSymbolTimeLoRa( DataratesCN470A[datarate], BandwidthsCN470A[datarate] );
    }

    RegionCommonComputeRxWindowParameters( tSymbol, minRxSymbols, rxError, RADIO_WAKEUP_TIME,
                                           &rxConfigParams->WindowTimeout, &rxConfigParams->WindowOffset );
}

bool RegionCN470ARxConfig( RxConfigParams_t *rxConfig, int8_t *datarate )
{
    RadioModems_t modem;
    int8_t dr = rxConfig->Datarate;
    uint8_t maxPayload = 0;
    int8_t phyDr = 0;
    uint32_t frequency = rxConfig->Frequency;

    bool iqInverted;

    if ( Radio.GetStatus( ) != RF_IDLE ) {
        return false;
    }

    if ( rxConfig->RxSlot == RX_SLOT_WIN_1 ) {
        // Apply window 1 frequency
        frequency = Channels[rxConfig->Channel].Frequency;
        // Apply the alternative RX 1 window frequency, if it is available
        if ( Channels[rxConfig->Channel].Rx1Frequency != 0 ) {
            frequency = Channels[rxConfig->Channel].Rx1Frequency;
        }
    }

    if (rxConfig->RxSlot == RX_SLOT_WIN_2) {
        uint8_t uldl_mode;
        lwan_dev_config_get(DEV_CONFIG_ULDL_MODE, &uldl_mode);
        if (uldl_mode == ULDL_MODE_INTER) {
            frequency = 470300000 + (InterFreqRx2Chan[TxFreqBandNum]) * 200000;
        } else {
            frequency = 470300000 + (IntraFreqRx2Chan[TxFreqBandNum]) * 200000;
        }
    }

    // Read the physical datarate from the datarates table
    phyDr = DataratesCN470A[dr];

    if (rxConfig->NodeWorkMode == WORK_MODE_REPEATER) {
        frequency = rxConfig->RepeaterFrequency;
        iqInverted = false;
    } else {
        iqInverted = true;
    }

    Radio.SetChannel( frequency );

    // Radio configuration
    if ( dr == DR_7 ) {
        modem = MODEM_FSK;
        Radio.SetRxConfig( modem, 50e3, phyDr * 1e3, 0, 83.333e3, 5, rxConfig->WindowTimeout, false, 0, true, 0, 0, false,
                           rxConfig->RxContinuous );
    } else {
        modem = MODEM_LORA;
        Radio.SetRxConfig( modem, rxConfig->Bandwidth, phyDr, 1, 0, 8, rxConfig->WindowTimeout, false, 0, false, 0, 0,
                           iqInverted, rxConfig->RxContinuous );
    }

    if ( rxConfig->RepeaterSupport == true ) {
        maxPayload = MaxPayloadOfDatarateRepeaterCN470A[dr];
    } else {
        maxPayload = MaxPayloadOfDatarateCN470A[dr];
    }
    Radio.SetMaxPayloadLength( modem, maxPayload + LORA_MAC_FRMPAYLOAD_OVERHEAD );

    *datarate = (uint8_t) dr;
    rxConfig->Frequency = frequency;
    
    LOG_PRINTF(LL_DEBUG, "Rx, Freq %u, DR %u, window %u\r\n", (unsigned int)frequency, dr, rxConfig->RxSlot+1);
    return true;
}

bool RegionCN470ATxConfig( TxConfigParams_t *txConfig, int8_t *txPower, TimerTime_t *txTimeOnAir )
{
    RadioModems_t modem;

    uint32_t frequency;
    uint16_t preambleLen;
    bool iqInverted;
    TimerTime_t curTime = TimerGetCurrentTime( );

    int8_t phyDr = DataratesCN470A[txConfig->Datarate];

    int8_t txPowerLimited = LimitTxPower( txConfig->TxPower, Bands[TxChannels[txConfig->Channel].Band].TxMaxPower,
                                          txConfig->Datarate, ChannelsMask );
    uint32_t bandwidth = GetBandwidth( txConfig->Datarate );
    int8_t phyTxPower = 0;

    // Calculate physical TX power
    phyTxPower = RegionCommonComputeTxPower( txPowerLimited, txConfig->MaxEirp, txConfig->AntennaGain );

    if (txConfig->NodeWorkMode == WORK_MODE_REPEATER) {
        frequency = txConfig->RepeaterFrequency;
        preambleLen = PreambleLenthCN470A[txConfig->Datarate];
        iqInverted = true;
    } else {
        frequency = TxChannels[txConfig->Channel].Frequency;
        preambleLen = 8;
        iqInverted = false;
    }

    // Setup the radio frequency
    Radio.SetChannel( frequency);

    if ( txConfig->Datarate == DR_7 ) {
        // High Speed FSK channel
        modem = MODEM_FSK;
        Radio.SetTxConfig( modem, phyTxPower, 25e3, bandwidth, phyDr * 1e3, 0, 5, false, true, 0, 0, false, 3e3 );
    } else {
        modem = MODEM_LORA;
        Radio.SetTxConfig( modem, phyTxPower, 0, bandwidth, phyDr, 1, preambleLen, false, true, 0, 0, iqInverted, 3e3 );
    }
    // Setup maximum payload lenght of the radio driver
    Radio.SetMaxPayloadLength( modem, txConfig->PktLen );
    // Get the time-on-air of the next tx frame
    *txTimeOnAir = Radio.TimeOnAir( modem,  txConfig->PktLen );

    *txPower = txConfig->TxPower;

    LOG_PRINTF(LL_DEBUG, "Tx, Power: %d, Band %u, Freq: %u,DR: %d, len: %u, duration %llu, at %llu\r\n",
                phyTxPower, TxFreqBandNum, (unsigned int)frequency, txConfig->Datarate, txConfig->PktLen, *txTimeOnAir, curTime);
    return true;
}

uint8_t RegionCN470ALinkAdrReq( LinkAdrReqParams_t *linkAdrReq, int8_t *drOut, int8_t *txPowOut, uint8_t *nbRepOut,
                                uint8_t *nbBytesParsed )
{
    uint8_t status = 0x07;
    LinkAdrParams_t linkAdrParams;
    uint8_t nextIndex = 0;
    uint8_t bytesProcessed = 0;
    uint16_t chMask = 0;
    uint8_t bandNum;
    uint16_t chMaskNew[8] = {0};
    ChanMaskSetParams_t chanMaskSet;

    while ( bytesProcessed < linkAdrReq->PayloadSize ) {
        // Get ADR request parameters
        nextIndex = RegionCommonParseLinkAdrReq( &( linkAdrReq->Payload[bytesProcessed] ), &linkAdrParams );

        if ( nextIndex == 0 ) {
            break;    // break loop, since no more request has been found
        }

        // Update bytes processed
        bytesProcessed += nextIndex;

        // Revert status, as we only check the last ADR request for the channel mask KO
        status = 0x07;

        // Setup temporary channels mask
        chMask = linkAdrParams.ChMask;

        // Verify channels mask
        if (linkAdrParams.ChMaskCtrl > 7)
        {
            status &= 0xFE; // Channel Ctrl KO
        }
        bandNum = ChMaskCntlToStartBandNum[linkAdrParams.ChMaskCtrl];
        if (bandNum == CHANNELS_MASK_CNTL_RFU)
        {
            status &= 0xFE; // Channel Ctrl KO
        }
        if ( (( linkAdrParams.ChMaskCtrl == 0 ) && ( chMask == 0 )) ||
             (( bandNum == CHANNELS_MASK_ALL_ON ) && ( chMask != 0xFFFF )) ) {
            status &= 0xFE; // Channel mask KO
        } else if ( bandNum == CHANNELS_MASK_ALL_ON ) {
            memset((uint8_t *)chMaskNew, 0xFF, sizeof(chMaskNew));
            chanMaskSet.ChannelsMaskIn = chMaskNew;
        } else {
            RegionCommonChanMaskCopy(chMaskNew, ChannelsMask, CHANNELS_MASK_SIZE );
            chMaskNew[bandNum / 2] = chMask;
            chanMaskSet.ChannelsMaskIn = chMaskNew;
        }
    }

    // Verify datarate
    if ( RegionCommonChanVerifyDr( CHANNELS_MASK_SIZE, &chMask, linkAdrParams.Datarate, CN470A_TX_MIN_DATARATE,
                                   CN470A_TX_MAX_DATARATE, Channels  ) == false ) {
        status &= 0xFD; // Datarate KO
    }

    // Verify tx power
    if ( RegionCommonValueInRange( linkAdrParams.TxPower, CN470A_MAX_TX_POWER, CN470A_MIN_TX_POWER ) == 0 ) {
        // Verify if the maximum TX power is exceeded
        if ( CN470A_MAX_TX_POWER > linkAdrParams.TxPower ) {
            // Apply maximum TX power. Accept TX power.
            linkAdrParams.TxPower = CN470A_MAX_TX_POWER;
        } else {
            status &= 0xFB; // TxPower KO
        }
    }

    // Update channelsMask if everything is correct
    if ( status == 0x07 ) {
        chanMaskSet.ChannelsMaskType = CHANNELS_MASK;
        if (RegionCN470AChanMaskSet(&chanMaskSet) == false)
        {
            status &= 0xFE;
        } else {
            if ( linkAdrParams.NbRep == 0 ) {
                // Value of 0 is not allowed, revert to default.
                linkAdrParams.NbRep = 1;
            }
        }
    }

    // Update status variables
    *drOut = linkAdrParams.Datarate;
    *txPowOut = linkAdrParams.TxPower;
    *nbRepOut = linkAdrParams.NbRep;
    *nbBytesParsed = bytesProcessed;

    return status;
}

uint8_t RegionCN470ARxParamSetupReq( RxParamSetupReqParams_t *rxParamSetupReq )
{
    uint8_t status = 0x07;

    // Verify radio frequency
    if ( Radio.CheckRfFrequency( rxParamSetupReq->Frequency ) == false ) {
        status &= 0xFE; // Channel frequency KO
    }

    // Verify datarate
    if ( RegionCommonValueInRange( rxParamSetupReq->Datarate, CN470A_RX_MIN_DATARATE, CN470A_RX_MAX_DATARATE ) == false ) {
        status &= 0xFD; // Datarate KO
    }

    // Verify datarate offset
    if ( RegionCommonValueInRange( rxParamSetupReq->DrOffset, CN470A_MIN_RX1_DR_OFFSET,
                                   CN470A_MAX_RX1_DR_OFFSET ) == false ) {
        status &= 0xFB; // Rx1DrOffset range KO
    }

    return status;
}

uint8_t RegionCN470ANewChannelReq( NewChannelReqParams_t *newChannelReq )
{
    uint8_t status = 0x03;
    ChannelAddParams_t channelAdd;
    ChannelRemoveParams_t channelRemove;

    if ( newChannelReq->NewChannel->Frequency == 0 ) {
        channelRemove.ChannelId = newChannelReq->ChannelId;

        // Remove
        if ( RegionCN470AChannelsRemove( &channelRemove ) == false ) {
            status &= 0xFC;
        }
    } else {
        channelAdd.NewChannel = newChannelReq->NewChannel;
        channelAdd.ChannelId = newChannelReq->ChannelId;

        switch ( RegionCN470AChannelAdd( &channelAdd ) ) {
            case LORAMAC_STATUS_OK: {
                break;
            }
            case LORAMAC_STATUS_FREQUENCY_INVALID: {
                status &= 0xFE;
                break;
            }
            case LORAMAC_STATUS_DATARATE_INVALID: {
                status &= 0xFD;
                break;
            }
            case LORAMAC_STATUS_FREQ_AND_DR_INVALID: {
                status &= 0xFC;
                break;
            }
            default: {
                status &= 0xFC;
                break;
            }
        }
    }

    return status;
}

int8_t RegionCN470ATxParamSetupReq( TxParamSetupReqParams_t *txParamSetupReq )
{
    return -1;
}

uint8_t RegionCN470ADlChannelReq( DlChannelReqParams_t *dlChannelReq )
{
    uint8_t status = 0x03;

    // Verify if the frequency is supported
    if ( VerifyTxFreq( dlChannelReq->Rx1Frequency ) == false ) {
        status &= 0xFE;
    }

    // Verify if an uplink frequency exists
    if ( Channels[dlChannelReq->ChannelId].Frequency == 0 ) {
        status &= 0xFD;
    }

    // Apply Rx1 frequency, if the status is OK
    if ( status == 0x03 ) {
        Channels[dlChannelReq->ChannelId].Rx1Frequency = dlChannelReq->Rx1Frequency;
    }

    return status;
}

int8_t RegionCN470AAlternateDr( AlternateDrParams_t *alternateDr )
{
    static int8_t datarate = 0;

    switch (alternateDr->joinmethod) {
        case 0:  // use stored channel and datarate
            datarate = alternateDr->datarate;
            if (alternateDr->NbTrials > 1) {
                datarate = alternateDr->datarate - 1;
                if (datarate < DR_2) {
                    datarate = DR_2;
                }
            }
            break;
        case 1:  // use default channel and datarate
        case 2:  // scan
            if (alternateDr->NbTrials == 1) {
                datarate = DR_3;
            } else {
                datarate = DR_2;
            }
            break;
        default:
            datarate = DR_5;
            break;
    }

    return datarate;
}

void RegionCN470ACalcBackOff( CalcBackOffParams_t *calcBackOff )
{
    uint8_t channel = calcBackOff->Channel;
    uint16_t dutyCycle = Bands[Channels[channel].Band].DCycle;
    uint16_t joinDutyCycle = 0;

    // Reset time-off to initial value.
    Bands[Channels[channel].Band].TimeOff = 0;

    if ( calcBackOff->Joined == false ) {
        // Get the join duty cycle
        joinDutyCycle = RegionCommonGetJoinDc( calcBackOff->ElapsedTime );
        // Apply the most restricting duty cycle
        dutyCycle = MAX( dutyCycle, joinDutyCycle );
        // Apply band time-off.
        Bands[Channels[channel].Band].TimeOff = calcBackOff->TxTimeOnAir * dutyCycle - calcBackOff->TxTimeOnAir;
    } else {
        if ( calcBackOff->DutyCycleEnabled == true ) {
            Bands[Channels[channel].Band].TimeOff = calcBackOff->TxTimeOnAir * dutyCycle - calcBackOff->TxTimeOnAir;
        }
    }
}

static uint8_t find_next_available_freqband_index()
{
    uint8_t index;
    uint8_t freqband_index;

    if (scan_mask == 0) {
        for (index = 1; index < NumFreqBand; index++) {
            scan_mask |= (1 << index);
        }
    }

    if (scan_mask == 0) {
        return 0;
    }

    freqband_index = randr(1, NumFreqBand-1);
    while ((scan_mask & (1 << freqband_index)) == 0) {
        freqband_index = ((freqband_index + 1) % NumFreqBand);
    }
    scan_mask &= (~(1 << freqband_index));
    return freqband_index;
}

bool RegionCN470ANextChannel( NextChanParams_t *nextChanParams, uint8_t *channel, TimerTime_t *time,
                              TimerTime_t *aggregatedTimeOff )
{
    uint8_t nbEnabledChannels = 0;
    uint8_t delayTx = 0;
    uint8_t enabledChannels[CN470A_MAX_NB_CHANNELS] = { 0 };
    TimerTime_t nextTxDelay = 0;
    MibRequestConfirm_t mib_req;
    static uint8_t RxFreqBandNum = 0;
    uint16_t channelMsakOfBand = 0;
    uint8_t index;

    if (RegionCommonCountChannels( ChannelsMask, 0, CHANNELS_MASK_SIZE) == 0 ) { // Reactivate default channels
        ChannelsMask[0] |= LC( 1 ) + LC( 2 ) + LC( 3 );
    }

    mib_req.Type = MIB_NETWORK_JOINED;
    LoRaMacMibGetRequestConfirm(&mib_req);

    if (mib_req.Param.IsNetworkJoined == false) {
        if (nextChanParams->joinmethod == JOIN_METHOD_STORED) {
            if (nextChanParams->freqband > 15) {
                nextChanParams->freqband = 1;  // reset to defautl value
            }
            TxFreqBandNum = nextChanParams->freqband;
        } else {
            if (nextChanParams->joinmethod == JOIN_METHOD_SCAN && nextChanParams->update_freqband) {
                NextAvailableFreqBandIdx = find_next_available_freqband_index();
            } else if (nextChanParams->joinmethod == JOIN_METHOD_DEF) {
                NextAvailableFreqBandIdx = 0;
            }
            nextChanParams->update_freqband = false;
            TxFreqBandNum = FreqBandNum[NextAvailableFreqBandIdx];
        }
    } else {
        TxFreqBandNum = nextChanParams->freqband;
        for (index = 0; index < NumFreqBand; index++) {
            if (TxFreqBandNum == FreqBandNum[index])
            {
                break;
            }
        }
        if (NumFreqBand == index)
        {
            if (NumFreqBand == 0)
            {
                ChannelsMask[0] |= LC( 1 ) + LC( 2 ) + LC( 3 );
                *time = 0;
                return false; 
            }
            else
            {
                TxFreqBandNum = FreqBandNum[NumFreqBand - 1];
            }
        }
    }

    if (nextChanParams->AggrTimeOff <= TimerGetElapsedTime( nextChanParams->LastAggrTx)) {
        // Reset Aggregated time off
        *aggregatedTimeOff = 0;

        // Update bands Time OFF
        nextTxDelay = RegionCommonUpdateBandTimeOff(nextChanParams->Joined, nextChanParams->DutyCycleEnabled, Bands,
                                                    CN470A_MAX_NB_BANDS);

        channelMsakOfBand = ChannelsMask[TxFreqBandNum / 2] >> ((TxFreqBandNum % 2) * 8);
        // Search how many channels are enabled
        nbEnabledChannels = CountNbOfEnabledChannels( nextChanParams->Joined, nextChanParams->Datarate,
                                                      &channelMsakOfBand, Channels,
                                                      Bands, enabledChannels, &delayTx );
    } else {
        delayTx++;
        nextTxDelay = nextChanParams->AggrTimeOff - TimerGetElapsedTime( nextChanParams->LastAggrTx );
    }

    if ( nbEnabledChannels > 0 ) {
        // We found a valid channel
        *channel = enabledChannels[randr( 0, nbEnabledChannels - 1 )];
        *time = 0;
    } else {
        if ( delayTx > 0 ) {
            // Delay transmission due to AggregatedTimeOff or to a band time off
            *time = nextTxDelay;
            return true;
        }
        // Datarate not supported by any channel, restore defaults
        ChannelsMask[0] |= LC( 1 ) + LC( 2 ) + LC( 3 );
        *time = 0;
        return false;
    }

    uint8_t uldl_mode;
    lwan_dev_config_get(DEV_CONFIG_ULDL_MODE, &uldl_mode);
    if (uldl_mode == ULDL_MODE_INTER) {
        if (FreqBandNum[NextAvailableFreqBandIdx] > 7) {
            RxFreqBandNum = TxFreqBandNum - 8;
        } else {
            RxFreqBandNum = TxFreqBandNum + 8;
        }
    } else { //IntraFreq
        RxFreqBandNum = TxFreqBandNum;
    }

    nextChanParams->freqband = TxFreqBandNum;

    nextChanParams->NextAvailableRxFreqBandNum = RxFreqBandNum;
    nextChanParams->NextAvailableTxFreqBandNum = TxFreqBandNum;


    //update the freq due to the change of FreqBand Num
    for ( uint8_t i = 0; i < CN470A_MAX_NB_CHANNELS; i++ ) {
        Channels[i].Frequency = 470300000 + (FreqBandStartChannelNum[nextChanParams->NextAvailableRxFreqBandNum] + i) * 200000;
        TxChannels[i].Frequency = 470300000 + (FreqBandStartChannelNum[nextChanParams->NextAvailableTxFreqBandNum] + i) *
                                  200000;
    }

    return true;
}

LoRaMacStatus_t RegionCN470AChannelAdd( ChannelAddParams_t *channelAdd )
{
    uint8_t band = 0;
    bool drInvalid = false;
    bool freqInvalid = false;
    uint8_t id = channelAdd->ChannelId;

    if ( id >= CN470A_MAX_NB_CHANNELS ) {
        return LORAMAC_STATUS_PARAMETER_INVALID;
    }

    // Validate the datarate range
    if ( RegionCommonValueInRange( channelAdd->NewChannel->DrRange.Fields.Min, CN470A_TX_MIN_DATARATE,
                                   CN470A_TX_MAX_DATARATE ) == false ) {
        drInvalid = true;
    }
    if ( RegionCommonValueInRange( channelAdd->NewChannel->DrRange.Fields.Max, CN470A_TX_MIN_DATARATE,
                                   CN470A_TX_MAX_DATARATE ) == false ) {
        drInvalid = true;
    }
    if ( channelAdd->NewChannel->DrRange.Fields.Min > channelAdd->NewChannel->DrRange.Fields.Max ) {
        drInvalid = true;
    }

    // Default channels don't accept all values
    if ( id < CN470A_NUMB_DEFAULT_CHANNELS ) {
        // Validate the datarate range for min: must be DR_0
        if ( channelAdd->NewChannel->DrRange.Fields.Min > DR_0 ) {
            drInvalid = true;
        }
        // Validate the datarate range for max: must be DR_5 <= Max <= TX_MAX_DATARATE
        if ( RegionCommonValueInRange( channelAdd->NewChannel->DrRange.Fields.Max, DR_5, CN470A_TX_MAX_DATARATE ) == false ) {
            drInvalid = true;
        }
        // We are not allowed to change the frequency
        if ( channelAdd->NewChannel->Frequency != Channels[id].Frequency ) {
            freqInvalid = true;
        }
    }

    // Check frequency
    if ( freqInvalid == false ) {
        if ( VerifyTxFreq( channelAdd->NewChannel->Frequency ) == false ) {
            freqInvalid = true;
        }
    }

    // Check status
    if ( ( drInvalid == true ) && ( freqInvalid == true ) ) {
        return LORAMAC_STATUS_FREQ_AND_DR_INVALID;
    }
    if ( drInvalid == true ) {
        return LORAMAC_STATUS_DATARATE_INVALID;
    }
    if ( freqInvalid == true ) {
        return LORAMAC_STATUS_FREQUENCY_INVALID;
    }

    memcpy( &(Channels[id]), channelAdd->NewChannel, sizeof( Channels[id] ) );
    Channels[id].Band = band;
    *((uint8_t *)ChannelsMask + TxFreqBandNum) |= ( 1 << id );
    return LORAMAC_STATUS_OK;
}

bool RegionCN470AChannelsRemove( ChannelRemoveParams_t *channelRemove  )
{
    uint8_t id = channelRemove->ChannelId;
    uint16_t channelMaskOfBand = 0;

    if ( id > CN470A_NUMB_DEFAULT_CHANNELS ) {
        return false;
    }
    channelMaskOfBand |= *((uint8_t *)ChannelsMask + TxFreqBandNum);

    // Remove the channel from the list of channels
    Channels[id] = ( ChannelParams_t ) {
        0, 0, { 0 }, 0
    };

    return RegionCommonChanDisable( &channelMaskOfBand, id, CN470A_MAX_NB_CHANNELS );
}

void RegionCN470ASetContinuousWave( ContinuousWaveParams_t *continuousWave )
{
    int8_t txPowerLimited = LimitTxPower( continuousWave->TxPower, Bands[Channels[continuousWave->Channel].Band].TxMaxPower,
                                          continuousWave->Datarate, ChannelsMask );
    int8_t phyTxPower = 0;
    uint32_t frequency = Channels[continuousWave->Channel].Frequency;

    // Calculate physical TX power
    phyTxPower = RegionCommonComputeTxPower( txPowerLimited, continuousWave->MaxEirp, continuousWave->AntennaGain );

    Radio.SetTxContinuousWave( frequency, phyTxPower, continuousWave->Timeout );
}

uint8_t RegionCN470AApplyDrOffset( uint8_t downlinkDwellTime, int8_t dr, int8_t drOffset )
{
    int8_t datarate = dr - drOffset;

    if ( datarate < 0 ) {
        datarate = DR_0;
    }
    return datarate;
}


void RegionCN470ARxBeaconSetup( RxBeaconSetup_t* rxBeaconSetup, uint8_t* outDr )
{
    RegionCommonRxBeaconSetupParams_t regionCommonRxBeaconSetup;

    regionCommonRxBeaconSetup.Datarates = DataratesCN470A;
    regionCommonRxBeaconSetup.Frequency = rxBeaconSetup->Frequency;
    regionCommonRxBeaconSetup.BeaconSize = CN470A_BEACON_SIZE;
    regionCommonRxBeaconSetup.BeaconDatarate = CN470A_BEACON_CHANNEL_DR;
    regionCommonRxBeaconSetup.BeaconChannelBW = CN470A_BEACON_CHANNEL_BW;
    regionCommonRxBeaconSetup.RxTime = rxBeaconSetup->RxTime;
    regionCommonRxBeaconSetup.SymbolTimeout = rxBeaconSetup->SymbolTimeout;

    RegionCommonRxBeaconSetup( &regionCommonRxBeaconSetup );

    // Store downlink datarate
    *outDr = CN470A_BEACON_CHANNEL_DR;
}

