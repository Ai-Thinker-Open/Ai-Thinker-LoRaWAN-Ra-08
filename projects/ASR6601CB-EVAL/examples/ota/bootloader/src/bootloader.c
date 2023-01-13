#include <string.h>
#include <stdio.h>
#include "system_cm4.h"
#include "bootloader.h"
#include "tremo_flash.h"
#include "tremo_crc.h"
#include "tremo_delay.h"
#include "tremo_system.h"
#include "radio.h"


#define RF_FREQUENCY                    470000000
#define TX_OUTPUT_POWER                             22        // dBm
#define LORA_BANDWIDTH                              2         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         5         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

#define FSK_FIX_LENGTH_PAYLOAD_ON                   false


//global variables
static RadioEvents_t RadioEvents;
static uint32_t g_freq = RF_FREQUENCY;
static uint32_t g_txp = TX_OUTPUT_POWER;
static uint32_t g_modem = MODEM_LORA;
static uint32_t g_lora_bw = LORA_BANDWIDTH;
static uint32_t g_lora_sf = LORA_SPREADING_FACTOR;
static uint32_t g_lora_cr = LORA_CODINGRATE;
static uint32_t g_lora_preamble = LORA_PREAMBLE_LENGTH;
static uint32_t g_lora_iqi = LORA_IQ_INVERSION_ON;
static uint32_t g_fsk_bw = 100000;
static uint32_t g_fsk_dr = 50000;
static uint32_t g_fsk_dev = 25000;
static uint32_t g_fsk_preamble = 5;
static uint32_t g_fsk_afcbw = 166666;
static volatile int g_tx_done = 0;

/**************************functions declaration**************************/
int sync_cmd_func(volatile loader_req_t *req, loader_res_t *res);
int jump_cmd_func(volatile loader_req_t *req, loader_res_t *res);
int flash_cmd_func(volatile loader_req_t *req, loader_res_t *res);
int erase_cmd_func(volatile loader_req_t *req, loader_res_t *res);
int verify_cmd_func(volatile loader_req_t *req, loader_res_t *res);
int reboot_cmd_func(volatile loader_req_t *req, loader_res_t *res);
int rdsn_cmd_func(volatile loader_req_t *req, loader_res_t *res);

void lora_init();
void lora_tx(uint8_t *data, uint32_t size);
void lora_rx(uint32_t timeout);

/**************************variables declaration**************************/
typedef struct _cmd_entry_
{
    uint8_t command;
    int (*function)(loader_req_t *req, loader_res_t *res);
}cmd_entry;

// command table
static const cmd_entry boot_cmd_table[] =
{
    {BOOTLOADER_CMD_SYNC, (void *)sync_cmd_func},
    {BOOTLOADER_CMD_JUMP, (void *)jump_cmd_func},
    {BOOTLOADER_CMD_FLASH, (void *)flash_cmd_func},
    {BOOTLOADER_CMD_ERASE,(void *)erase_cmd_func},
    {BOOTLOADER_CMD_VERIFY,(void *)verify_cmd_func},
    {BOOTLOADER_CMD_REBOOT,(void *)reboot_cmd_func},
    {BOOTLOADER_CMD_SN,(void *)rdsn_cmd_func},
}; 

#define BOOT_CMD_TABLE_SIZE (sizeof(boot_cmd_table) / sizeof(boot_cmd_table[0]))

uint8_t g_bootloader_cmd[BOOTLOADER_MAX_CMD_SIZE];
loader_req_t g_request;
loader_res_t g_response;


/**port functions**/
char boot_buf[BOOTLOADER_MAX_CMD_SIZE];
volatile uint16_t boot_wr_idx = 0;
volatile uint16_t boot_rd_idx = 0;

/**************************boot buffer functions**************************************/

void boot_buffer_clear()
{
    boot_wr_idx = 0;
    boot_rd_idx = 0;
}

uint16_t boot_buffer_len()
{   
    if(boot_rd_idx <= boot_wr_idx)
        return boot_wr_idx-boot_rd_idx;
    
    return 0;
}

void boot_buffer_write_byte(uint8_t byte)
{   
    if(boot_wr_idx < BOOTLOADER_MAX_CMD_SIZE)
        boot_buf[boot_wr_idx++] = byte;
}

uint8_t boot_buffer_read_byte()
{   
    uint8_t b = 0;
    
    if(boot_rd_idx < boot_wr_idx)
        b = boot_buf[boot_rd_idx++];
    
    return b;
}

void boot_buffer_read_bytes(uint8_t *data, uint16_t size)
{   
    for(int i=0; i<size; i++){
        data[i] = boot_buffer_read_byte();
    }
}


/**************************functions**************************************/
uint32_t crc32(uint8_t *data, uint32_t size)
{
    uint32_t crc_value = 0;
    
    //CRC-32
    crc_config_t config;
    config.init_value = 0xFFFFFFFF;
    config.poly_size = CRC_POLY_SIZE_32;
    config.poly = 0x04C11DB7;
    config.reverse_in = CRC_CR_REVERSE_IN_BYTE;
    config.reverse_out = true;
    
    crc_init(&config);
    
    crc_value = crc_calc8(data, size);
    crc_value ^= 0xFFFFFFFF;

	return crc_value;
}

int get_request_from_lora(loader_req_t *req)
{
    int ret = 0;	
    uint8_t len_lsb, len_msb;
    
    //min cmd size
		if(boot_buffer_len()<BOOTLOADER_MIN_CMD_SIZE)
				return -1;
		
		if(boot_buffer_read_byte()!=BOOTLOADER_SYMBOL_CMD_START)
				return -1;
		
    g_bootloader_cmd[BOOTLOADER_POS_START] = BOOTLOADER_SYMBOL_CMD_START;
    
    //read the min cmd     
    boot_buffer_read_bytes((uint8_t *)(g_bootloader_cmd+1), BOOTLOADER_MIN_CMD_SIZE-1);

    //parse cmd
    req->cmd = g_bootloader_cmd[BOOTLOADER_POS_CMD];
    //parse len
    len_lsb = g_bootloader_cmd[BOOTLOADER_POS_LEN_LSB];
    len_msb = g_bootloader_cmd[BOOTLOADER_POS_LEN_MSB];
    req->data_len = len_lsb | (len_msb<<8);
    
    //check len
    if(req->data_len>BOOTLOADER_MAX_CMD_SIZE)
        return BOOTLOADER_STATUS_ERR_SIZE;
    
    //read data
    if(req->data_len){
        boot_buffer_read_bytes((uint8_t *)(g_bootloader_cmd+BOOTLOADER_MIN_CMD_SIZE), req->data_len);
        req->data = g_bootloader_cmd+BOOTLOADER_POS_DATA;
    }
    
    //check cmd end
    if(g_bootloader_cmd[req->data_len+BOOTLOADER_MIN_CMD_SIZE-1] != BOOTLOADER_SYMBOL_CMD_END)
        return BOOTLOADER_STATUS_ERR_DATA;
    
    //checksum
    uint32_t crc32_value = crc32((uint8_t *)g_bootloader_cmd, 4+req->data_len);
    uint32_t checksum = *(uint32_t *)(g_bootloader_cmd+4+req->data_len);
    if(checksum != crc32_value)
        return BOOTLOADER_STATUS_ERR_CHECKSUM;
    
    return ret;
}

void send_response_to_lora(loader_res_t *res)
{
    g_bootloader_cmd[BOOTLOADER_POS_START] = BOOTLOADER_SYMBOL_CMD_START;
    g_bootloader_cmd[BOOTLOADER_POS_STATUS] = res->status;
    g_bootloader_cmd[BOOTLOADER_POS_LEN_LSB] = res->data_len & 0xFF;
    g_bootloader_cmd[BOOTLOADER_POS_LEN_MSB] = (res->data_len>>8) & 0xFF;
    
    uint32_t crc32_value = crc32((uint8_t *)g_bootloader_cmd, 4+res->data_len);
    *(uint32_t *)(g_bootloader_cmd+4+res->data_len) = crc32_value;
    g_bootloader_cmd[res->data_len+BOOTLOADER_MIN_CMD_SIZE-1] = BOOTLOADER_SYMBOL_CMD_END;
    
	delay_ms(5);
	lora_tx(g_bootloader_cmd, res->data_len+BOOTLOADER_MIN_CMD_SIZE);	
}

int copy_image_data_to_flash(uint32_t addr, uint8_t *data, uint32_t size)
{
    int ret = 0;
    
    if(FLASH_LINE_SIZE == size){
        FLASH_OP_BEGIN();
        ret = flash_program_line(addr, data);
        FLASH_OP_END();
    }else{
        FLASH_OP_BEGIN();
        ret = flash_program_bytes(addr, data, size);
        FLASH_OP_END();
    }

    return ret;
}

int jump_cmd_func(volatile loader_req_t *req, loader_res_t *res)
{
    uint32_t addr;
    
    if(req->data_len<sizeof(uint32_t)){
        res->status = BOOTLOADER_STATUS_ERR_PARAM;
        return RES_UNSENT;
    }
    
    addr = *(uint32_t *)req->data;

    boot_buffer_clear();
    send_response_to_lora(res);
    while(1){
        Radio.IrqProcess( );
        if(g_tx_done)
            break;
    }
    
    Radio.Sleep();
 
    boot_to_app(addr);
    
    return RES_SENT;
}

int flash_cmd_func(volatile loader_req_t *req, loader_res_t *res)
{
    uint32_t addr, size;
    
    if(req->data_len<2*sizeof(uint32_t)){
        res->status = BOOTLOADER_STATUS_ERR_PARAM;
        return RES_UNSENT;
    }
    
    addr = *(uint32_t *)req->data;
    size = *(uint32_t *)(req->data+sizeof(uint32_t));

    //verify parameter
    if((addr < FLASH_START_ADDR)
        || ((addr + size) > (FLASH_START_ADDR + FLASH_MAX_SIZE))
        || (0 == size)){
        res->status = BOOTLOADER_STATUS_ERR_PARAM;
        return RES_UNSENT;
    }
        
    if(copy_image_data_to_flash(addr, req->data+2*sizeof(uint32_t), size) != 0){
        res->status = BOOTLOADER_STATUS_ERR_FLASH;
        return RES_UNSENT;
    }
    
    return RES_UNSENT;
}

//erase flash comand
int erase_cmd_func(volatile loader_req_t *req, loader_res_t *res)
{
    uint32_t addr, size;

    if(req->data_len<2*sizeof(uint32_t)){
        res->status = BOOTLOADER_STATUS_ERR_PARAM;
        return RES_UNSENT;
    }
    
    addr = *(uint32_t *)req->data;
    size = *(uint32_t *)(req->data+sizeof(uint32_t));
    
    //verify parameter
    if((addr < FLASH_START_ADDR)
        || ((addr + size) > (FLASH_START_ADDR + FLASH_MAX_SIZE))){
        res->status = BOOTLOADER_STATUS_ERR_PARAM;
        return RES_UNSENT;
    }

    while(size){
        FLASH_OP_BEGIN();
        if(flash_erase_page(addr)<0) {
            FLASH_OP_END();
            res->status = BOOTLOADER_STATUS_ERR_FLASH;
            return RES_UNSENT;
        }
        FLASH_OP_END();
        if(size >= FLASH_PAGE_SIZE){
            size -= FLASH_PAGE_SIZE;
            addr += FLASH_PAGE_SIZE;
        }else{
            size = 0;
        }
    }
    
    return RES_UNSENT;
}

int verify_cmd_func(volatile loader_req_t *req, loader_res_t *res)
{
    uint32_t addr, size, checksum;
    
    if(req->data_len<3*sizeof(uint32_t)){
        res->status = BOOTLOADER_STATUS_ERR_PARAM;
        return RES_UNSENT;
    }
    
    addr = *(uint32_t *)(req->data);
    size = *(uint32_t *)(req->data+sizeof(uint32_t));
    checksum = *(uint32_t *)(req->data+2*sizeof(uint32_t));
    
    uint32_t crc32_value = crc32((uint8_t *)addr, size);
    if(crc32_value != checksum){
        res->status = BOOTLOADER_STATUS_ERR_VERIFY;
        return RES_UNSENT;
    }
    
    return RES_UNSENT;
}

int reboot_cmd_func(volatile loader_req_t *req, loader_res_t *res)
{
    uint8_t mode = 0;
    
    if(req->data_len)
        mode = *(uint8_t *)req->data;
    if(mode>0)
        SYSCFG->CR4 |= BOOT_MODE_REG_BIT;
    
    boot_buffer_clear();
    send_response_to_lora(res);
    while(1){
        Radio.IrqProcess( );
        if(g_tx_done)
            break;
    }
    
    NVIC_SystemReset();
    
    return RES_SENT;
}

int rdsn_cmd_func(volatile loader_req_t *req, loader_res_t *res)
{
    uint8_t sn[8];
    system_get_chip_id((uint32_t *)sn);

    res->data_len = 8;
    memcpy(res->data, (uint8_t *)sn, res->data_len);
    
    return RES_UNSENT;
}

int sync_cmd_func(volatile loader_req_t *req, loader_res_t *res)
{
    boot_buffer_clear();
    
    return RES_UNSENT;
}


void OnTxDone( void )
{
    g_tx_done = 1;
    lora_rx(0);
    printf("%s\r\n", __func__);
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    boot_buffer_clear();
    lora_rx(0);
    printf("%s\r\n", __func__);
    for(int i=0; i<size; i++)
        boot_buffer_write_byte(payload[i]);
}

void OnTxTimeout( void )
{
    lora_rx(0);
    printf("%s\r\n", __func__);
}

void OnRxTimeout( void )
{
    lora_rx(0);
    printf("%s\r\n", __func__);
}

void OnRxError( void )
{
    lora_rx(0);
    printf("%s\r\n", __func__);
}

void lora_init()
{
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.RxDone = OnRxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxTimeout = OnRxTimeout;
    RadioEvents.RxError = OnRxError;

    Radio.Init( &RadioEvents );
    
    Radio.SetChannel(g_freq);
    
    if(g_modem == MODEM_LORA) {
        
        Radio.SetTxConfig( MODEM_LORA, g_txp, 0, g_lora_bw,
                                       g_lora_sf, g_lora_cr,
                                       g_lora_preamble, LORA_FIX_LENGTH_PAYLOAD_ON,
                                       true, 0, 0, g_lora_iqi, 6000 );
    
        Radio.SetRxConfig( MODEM_LORA, g_lora_bw, g_lora_sf,
                                       g_lora_cr, 0, g_lora_preamble,
                                       LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                       0, true, 0, 0, g_lora_iqi, true );
    }else{
        Radio.SetTxConfig( MODEM_FSK, g_txp, g_fsk_dev, 0,
                                      g_fsk_dr, 0,
                                      g_fsk_preamble, FSK_FIX_LENGTH_PAYLOAD_ON,
                                      true, 0, 0, 0, 6000 );
    
        Radio.SetRxConfig( MODEM_FSK, g_fsk_bw, g_fsk_dr,
                                      0, g_fsk_afcbw, g_fsk_preamble,
                                      0, FSK_FIX_LENGTH_PAYLOAD_ON, 0, true,
                                      0, 0, false, true );
    }

    Radio.Rx( 0 );
}

void lora_tx(uint8_t *data, uint32_t size)
{
    if(g_modem == MODEM_LORA) {
        
        Radio.SetTxConfig( MODEM_LORA, g_txp, 0, g_lora_bw,
                                       g_lora_sf, g_lora_cr,
                                       g_lora_preamble, LORA_FIX_LENGTH_PAYLOAD_ON,
                                       true, 0, 0, g_lora_iqi, 6000 );
    
        Radio.SetRxConfig( MODEM_LORA, g_lora_bw, g_lora_sf,
                                       g_lora_cr, 0, g_lora_preamble,
                                       LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                       0, true, 0, 0, g_lora_iqi, true );
    }else{
        Radio.SetTxConfig( MODEM_FSK, g_txp, g_fsk_dev, 0,
                                      g_fsk_dr, 0,
                                      g_fsk_preamble, FSK_FIX_LENGTH_PAYLOAD_ON,
                                      true, 0, 0, 0, 6000 );
    
        Radio.SetRxConfig( MODEM_FSK, g_fsk_bw, g_fsk_dr,
                                      0, g_fsk_afcbw, g_fsk_preamble,
                                      0, FSK_FIX_LENGTH_PAYLOAD_ON, 0, true,
                                      0, 0, false, true );
    }

    g_tx_done = 0;
    Radio.Send( data, size );
}

void lora_rx(uint32_t timeout)
{
    if(g_modem == MODEM_LORA) {
        
        Radio.SetTxConfig( MODEM_LORA, g_txp, 0, g_lora_bw,
                                       g_lora_sf, g_lora_cr,
                                       g_lora_preamble, LORA_FIX_LENGTH_PAYLOAD_ON,
                                       true, 0, 0, g_lora_iqi, 6000 );
    
        Radio.SetRxConfig( MODEM_LORA, g_lora_bw, g_lora_sf,
                                       g_lora_cr, 0, g_lora_preamble,
                                       LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                       0, true, 0, 0, g_lora_iqi, true );
    }else{
        Radio.SetTxConfig( MODEM_FSK, g_txp, g_fsk_dev, 0,
                                      g_fsk_dr, 0,
                                      g_fsk_preamble, FSK_FIX_LENGTH_PAYLOAD_ON,
                                      true, 0, 0, 0, 6000 );
    
        Radio.SetRxConfig( MODEM_FSK, g_fsk_bw, g_fsk_dr,
                                      0, g_fsk_afcbw, g_fsk_preamble,
                                      0, FSK_FIX_LENGTH_PAYLOAD_ON, 0, true,
                                      0, 0, false, true );
    }

    Radio.Rx( timeout );
}


void boot_handle_cmd(void)
{
    lora_init();
	
	printf("boot_handle_cmd\r\n");
    while(1) {
        int i = 0;
        int ret = 0;
      
        Radio.IrqProcess( );			
			
        //get requeset
        ret = get_request_from_lora((loader_req_t *)&g_request);
        if(ret!=0) {
            boot_buffer_clear();
            continue;
        }
        
        //init the response
        memset((loader_res_t *)&g_response, 0, sizeof(loader_res_t));
        g_response.data = g_bootloader_cmd + BOOTLOADER_POS_DATA;
        
        ret = RES_UNSENT;
        for(i = 0; i < BOOT_CMD_TABLE_SIZE; i++){
            if(g_request.cmd == boot_cmd_table[i].command) {
                ret = boot_cmd_table[i].function((loader_req_t *)&g_request, (loader_res_t *)&g_response);
                break;
            }
        }
        
        if(i >= BOOT_CMD_TABLE_SIZE)
            g_response.status = BOOTLOADER_STATUS_UNKNOWN_CMD;
        
        //send response
        if(ret == RES_UNSENT) {
            boot_buffer_clear();
            send_response_to_lora((loader_res_t *)&g_response);
        }
     } 
}
