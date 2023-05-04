#include <platform_opts_bt.h>
//#if defined(CONFIG_MULTI_ADV) && CONFIG_MULTI_ADV
#include "string.h"
#include "gap.h"
//#include "gap_msg.h"
#include "gap_le.h"
//#include "ble_ms_adapter_app_flags.h"
//#include "ble_ms_adapter_app.h"
#include "os_mem.h"
#include "gap_le.h"
#include "gap_adv.h"
#include "gap_conn_le.h"
#include "platform_stdlib.h"
#include "os_sync.h"
#include "os_sched.h"
//#include "os_msg.h"
#include "os_timer.h"
#include "matter_blemgr_common.h"
#include "ble_ms_adapter_app_main.h"
#include "ble_ms_adapter_app_flags.h"
#include "ble_ms_adapter_app.h"
//#include "ble_ms_adapter_service.h"

#include "os_sched.h"

//extern T_SERVER_ID ble_matter_adapter_service_id;
//extern M_MULTI_ADV_PARAM ms_multi_adv_param_array[MAX_ADV_NUMBER];
extern int ble_ms_adapter_peripheral_app_max_links;
extern T_MULTI_ADV_CONCURRENT ms_multi_adapter;

//Temp
#define MAX_ADV_NUMBER 2

matter_blemgr_callback matter_blemgr_callback_func = NULL;
void *matter_blemgr_callback_data = NULL;
uint8_t matter_adv_id = MAX_ADV_NUMBER;
uint16_t matter_adv_interval = 0;
uint16_t matter_adv_int_min = 0x20;
uint16_t matter_adv_int_max = 0x20;
uint8_t matter_adv_data[31] = {0};
uint8_t matter_adv_data_length = 0;

extern T_SERVER_ID ble_matter_adapter_service_id;
extern int ble_ms_adapter_app_init(void);
extern void ble_ms_adapter_multi_adv_init();
int matter_blemgr_init(void) {
	printf("[%s]enter...\r\n", __func__);
	ble_ms_adapter_app_init();
	ble_ms_adapter_multi_adv_init();
	return 0;
}
void matter_blemgr_set_callback_func(matter_blemgr_callback p, void *data) {
	printf("[%s]enter...\r\n", __func__);
	matter_blemgr_callback_func = p;
	matter_blemgr_callback_data = data;
}


int matter_blemgr_start_adv(void) {
	printf("[%s]enter...\r\n", __func__);
	bool result = 0;
#if CONFIG_MS_MULTI_ADV
	result = msmart_matter_ble_adv_start_by_adv_id(&matter_adv_id, NULL, 0, NULL, 0, 1);
	if (result == 1)
		return 1;
#endif
	return 0;
}

int matter_blemgr_stop_adv(void) {
	printf("[%s]enter...\r\n", __func__);
	bool result = 0;
#if CONFIG_MS_MULTI_ADV
	if (ms_multi_adapter.matter_sta_sto_flag != false) {
		printf("[%s]adv already stop...\r\n", __func__);
		return 1;
	}

	result = ms_matter_ble_adv_stop_by_adv_id(&matter_adv_id);
	if (result == 1)
		return 1;
	ms_multi_adapter.matter_sta_sto_flag = true;
#endif
	return 0;
}

int matter_blemgr_config_adv(uint16_t adv_int_min, uint16_t adv_int_max, uint8_t *adv_data, uint8_t adv_data_length) {
	printf("[%s]enter...\r\n", __func__);
	matter_adv_interval = adv_int_max;
	matter_adv_int_min = adv_int_min;
	matter_adv_int_max = adv_int_max;
	matter_adv_data_length = adv_data_length;
	memcpy(matter_adv_data, adv_data, adv_data_length);

	return 0;
}

uint16_t matter_blemgr_get_mtu(uint8_t connect_id) {
	printf("[%s]enter...\r\n", __func__);
	int ret;
	uint16_t mtu_size;

	if (ble_ms_adapter_peripheral_app_max_links == 0) {
		printf("[%s]matter as slave, no connection\r\n", __func__);
		return 1;
	}
	ret = le_get_conn_param(GAP_PARAM_CONN_MTU_SIZE, &mtu_size, connect_id);
	if (ret == 0)
	{
		printf("printing MTU size\r\n");
		return mtu_size;
	}
	else
		return 0;
}

int matter_blemgr_set_device_name(char *device_name, uint8_t device_name_length) {
	printf("[%s]enter...\r\n", __func__);
	if (device_name == NULL || device_name_length > GAP_DEVICE_NAME_LEN) {
		printf("[%s]:invalid name or len:name 0x%x,len %d\r\n",__func__, device_name, device_name_length);
		return 1;
	}
	le_set_gap_param(GAP_PARAM_DEVICE_NAME, device_name_length, device_name);

	return 0;

}

int matter_blemgr_disconnect(uint8_t connect_id) {
	printf("[%s]enter...\r\n", __func__);
	if (connect_id >= BLE_MS_ADAPTER_APP_MAX_LINKS) {
		printf("[%s]:invalid conn_hdl[%d]\r\n", __func__, connect_id);
		return 1;
	}
	T_GAP_DEV_STATE new_state;
	uint8_t *conn_id = (uint8_t *)os_mem_alloc(0, sizeof(uint8_t));
	*conn_id = connect_id;

	if ((ble_ms_adapter_app_send_api_msg(5, conn_id)) == false) {
		printf("[%s] send msg fail\r\n", __func__);
		os_mem_free(conn_id);
		return 1;
	}
	return 0;
}

int matter_blemgr_send_indication(uint8_t connect_id, uint8_t *data, uint16_t data_length) {
	printf("[%s]enter...\r\n", __func__);

	if (connect_id >= BLE_MS_ADAPTER_APP_MAX_LINKS || data == NULL || data_length == 0) {
		printf("[%s]:invalid param:conn_hdl %d,data 0x%x data_length 0x%x\r\n",__func__, connect_id, data, data_length);
		return 1;
	}
	BT_MATTER_SERVER_SEND_DATA *indication_param = (BT_MATTER_SERVER_SEND_DATA *)os_mem_alloc(0, sizeof(BT_MATTER_SERVER_SEND_DATA));
	if (indication_param)
    	{
        	indication_param->conn_id = connect_id;
		indication_param->service_id = ble_matter_adapter_service_id;
        	indication_param->attrib_index = BT_MATTER_ADAPTER_SERVICE_CHAR_TX_INDEX;
		indication_param->data_len = data_length;
		indication_param->type = GATT_PDU_TYPE_INDICATION;
        	if (indication_param->data_len != 0)
        	{
            		indication_param->p_data = os_mem_alloc(0, indication_param->data_len);
            		memcpy(indication_param->p_data, data, indication_param->data_len);
        	}
        	if (ble_ms_adapter_app_send_api_msg(4, indication_param) == false)
        	{
            		printf("[%s] os_mem_free\r\n");
            		os_mem_free(indication_param->p_data);
            		os_mem_free(indication_param);
            		return 1;
        	}
    	}
	return 0;
}
void ble_ms_adapter_switch_bt_address(uint8_t *address) {
	uint8_t tmp=0;
	for(int i=0;i<6/2;++i){
	tmp=address[6-1-i];
	address[6-1-i]=address[i];
	address[i]=tmp;
	}

}

//#endif
