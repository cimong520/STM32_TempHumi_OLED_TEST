#include "../System/Initialization.h"
#include "../HardWare/OLED.h"
#include "../HardWare/dht11.h"
#include "../HardWare/buzzer.h"
#include "../HardWare/relay.h"
#include "../HardWare/Serial.h"
#include "../HardWare/key.h"
#include "./main.h"
#include "./menu.h"
#include <string.h>

dt data;
char c ='7';
uint8_t buff[512]={0};
//DHT11_Data_TypeDef DHT11_Data = {0}; // 添加DHT11数据结构体变量
uint8_t connect_flag;			// 0：未连接  1：连接成功  2；服务器选择成功

// ESP8266状态信息全局变量
char saved_ip[20] = "未获取";
char saved_mode[30] = "未知";
char saved_status[20] = "未连接";
char saved_wifi_name[50] = "未连接";
//	模块结构体声明
uint8_t temp, humi;
uint8_t key_num;
//	模块结构体声明

uint8_t main_mode_flag  = 0, main_key_flag = 0 ; 
// 任务函数实现
/**
 * @brief 数据获取逻辑
 * 在这里调用各模块的数据获取函数
 */
void GetData(void){
    // 数据获取逻辑
}

// 处理云端控制指令
void Process_Cloud_Control(void){
    static uint16_t last_motor_flag = 0;
    static uint8_t last_led = 0;
    
    // 检查继电器控制变化
    if(data.flag.motor_flag != last_motor_flag) {
        Serial_SendString("云端控制: 继电器状态变更为 ");
        Serial_SendNumber(data.flag.motor_flag, 1);
        Serial_SendString("\r\n");
        
        if(data.flag.motor_flag > 0) {
            relay_ON();
            Serial_SendString("继电器已开启\r\n");
        } else {
            relay_OFF();
            Serial_SendString("继电器已关闭\r\n");
        }
        last_motor_flag = data.flag.motor_flag;
    }
    
    // 检查LED控制变化
    if(data.flag.led != last_led) {
        Serial_SendString("云端控制: LED状态变更为 ");
        Serial_SendNumber(data.flag.led, 1);
        Serial_SendString("\r\n");
        
        if(data.flag.led > 0) {
            // LED_ON();  // 如果有LED控制函数
            Serial_SendString("LED已开启\r\n");
        } else {
            // LED_OFF();
            Serial_SendString("LED已关闭\r\n");
        }
        last_led = data.flag.led;
    }
}

/**
 * @brief 数据显示逻辑
 * 在这里显示各模块的数据
 */
//void ShowData(void){
//    OLED_Clear();
//    if(connect_flag == 2){
//		OLED_ShowImage(0,0,16,16,wifi_int);
//		OLED_ShowImage(16,0,16,16,server_int);
//	}else if(connect_flag == 1){
//		OLED_ShowImage(0,0,16,16,wifi_int);
//		OLED_ShowImage(16,0,16,16,server_out);
//	}
//	else {
//		OLED_ShowImage(0,0,16,16,wifi_out);
//		OLED_ShowImage(16,0,16,16,server_out);
//	}
//	OLED_Printf(32,0,8,"   %02d/%02d/%02d\n    %02d:%02d:%02d\n",MyRTC_Time[0],MyRTC_Time[1],MyRTC_Time[2],MyRTC_Time[3],MyRTC_Time[4],MyRTC_Time[5]);
//	
//    // 默认显示系统信息
//    OLED_Printf(0, 16, 16, "系统就绪你好新\n%d\n%d",data.flag.y,data.flag.z);
//    OLED_Update();
//}
//模式选择
void Mode(void) {
    if(Get_Key_3() == 3) {
        if(main_mode_flag == 0) {
            main_mode_flag = 1;  // 进入手动模式
        } else {
            // 在手动模式下，按键3控制继电器
            main_key_flag++;
            if((main_key_flag % 2) == 1) {
                relay_ON();
            } else {
                relay_OFF();
            }
        }
    }
}

// 按键扫描逻辑
void Menu_key_set(void)									//按键扫描
{
	if (wifi.rxover == 1) {
		Serial_SendString("\r\n【Menu_key_set】检测到wifi.rxover=1，调用DataAnylize()\r\n");
		DataAnylize();
	}
	data.flag.key1 = Get_Key_1();										
	data.flag.key2 = Get_Key_2();										
	data.flag.key3 = Get_Key_3();										
	data.flag.key4 = Get_Key_4();	
	if(data.flag.key1){
		WIFI_FORGET_WIFI();
	}
	if(data.flag.key2){
		data.flag.z=2;
	}
	if(data.flag.key3){
		data.flag.z=3;
	}
	if(data.flag.key4){
		data.flag.z=4;
	}
}
// 报警处理逻辑
void Alarm(void)
{
	
}

void IWDGADD(void){
	MyRTC_ReadTime();
	data.flag.y = data.flag.x;
	data.flag.x=0;

	IWDG_ReloadCounter();

}

void WIFI_Init(void){
	if(connect_flag == 2){			//如果不需要时间可以将这个判断和内容移除掉
		WIFI_GET_TIME();
	}
	else if(connect_flag == 1){
		Connect_tothe_cloud(CMD_CONNECT_BAFA);	//连接到什么云端
	}
	else if(!connect_flag){
		Check_the_network();
	}
}

void upload_data(void){
	if(connect_flag == 2){			// 确保已连接到巴法云
		memset(buff, 0, 512);
		// 构造巴法云数据格式：CMD:SEND_DATA,{"Temp":25.5,"Humi":60.2,"Status":"Normal"}
		sprintf((char*)buff,
			"%s,{\"Temp\":%.1f,\"Humi\":%.1f,\"Status\":\"%s\",\"Relay\":\"%s\",}",
			CMD_SEND_DATA,
			data.Variable.Temp,
			data.Variable.Humi,
			(data.Variable.Temp > 30.0) ? "High" : "Normal",
			(data.flag.motor_flag) ? "ON" : "OFF"
		);
		WifiSendStr((char*)buff);
		
		// 通过串口输出调试信息
		Serial_SendString("Uploading to BaFa Cloud: ");
		Serial_SendString((char*)buff);
		Serial_SendString("\r\n");
	} else {
		// 输出连接状态调试信息
		if(connect_flag == 1) {
			Serial_SendString("错误: WiFi已连接，但巴法云未连接\r\n");
			Serial_SendString("请先按按键1连接巴法云\r\n");
		} else {
			Serial_SendString("错误: WiFi未连接，无法上传数据\r\n");
		}
		Serial_SendString("当前连接状态: ");
		Serial_SendNumber(connect_flag, 1);
		Serial_SendString(" (需要状态2才能上传)\r\n");
	}
}
// 任务ID存储（全局变量）
TaskID keyTaskID, dataTaskID, alarmTaskID, showTaskID, IWDG_ID, wifiID, uploadID;
/**
 * @brief 任务初始化
 * 
 * 添加系统需要的所有任务并保存任务ID
 */
void Task_Initialization(void){
	keyTaskID = Task_Add(Menu_key_set, 10, PRIORITY_CRITICAL, "KeyScan");
    dataTaskID = Task_Add(GetData, 50, PRIORITY_NORMAL, "GetData");
    alarmTaskID = Task_Add(Alarm, 100, PRIORITY_HIGH, "Alarm");
   // showTaskID = Task_Add(ShowData, 0, PRIORITY_NORMAL, "ShowData");
	IWDG_ID = Task_Add(IWDGADD, 1000, PRIORITY_HIGH, "IWDG_ID");
	// 恢复自动化任务
	wifiID = Task_Add(WIFI_Init, 5000, PRIORITY_NORMAL, "WIFI_Init");        // 每5秒检查WiFi和云端连接状态
	uploadID = Task_Add(upload_data, 30000, PRIORITY_NORMAL, "uploadID");    // 每30秒自动上传数据
	
	// 添加云端控制处理任务
	TaskID controlTaskID = Task_Add(Process_Cloud_Control, 100, PRIORITY_HIGH, "CloudControl"); // 每100ms检查控制指令
}
//温度显示图像
// 简单的温度曲线函数
void SimpleDrawTempCurve(void)
{
    static float last_temps[64];  // 存储64个温度点
    static uint8_t temp_pos = 0;
    
    // 添加新温度数据
    last_temps[temp_pos] = (float)temp;
    temp_pos = (temp_pos + 1) % 64;
    
    // 清除曲线区域
    OLED_ClearArea(0, 20, 128, 40);
    
    // 绘制简单曲线
    for(uint8_t i = 1; i < 64; i++) {
        uint8_t x1 = i * 2;
        uint8_t x2 = (i + 1) * 2;
        uint8_t y1 = 60 - (uint8_t)(last_temps[i] / 2);      // 简单缩放
        uint8_t y2 = 60 - (uint8_t)(last_temps[i+1] / 2);
        
        if(x2 < 128 && y1 < 64 && y2 < 64) {
            OLED_DrawLine(x1, y1, x2, y2);
        }
    }
}

//ESP8266测试
void Test_ESP8266(void)
{
    static uint32_t last_status_time = 0;
    uint32_t current_time = Task_GetSystemTime();
    
    // 每10秒输出一次状态信息
    if(current_time - last_status_time >= 10000) {
        Serial_SendString("\r\n=== 系统状态信息 ===\r\n");
        Serial_SendString("WiFi: ");
        Serial_SendString(saved_wifi_name);
        Serial_SendString(" (");
        Serial_SendString(saved_ip);
        Serial_SendString(")\r\n");
        Serial_SendString("巴法云: ");
        if(connect_flag == 2) {
            Serial_SendString("云端已连接");
        } else if(connect_flag == 1) {
            Serial_SendString("WiFi已连接，未连接云端");
        } else {
            Serial_SendString("未连接");
        }
        Serial_SendString("\r\n");
        Serial_SendString("温度: ");
        Serial_SendNumber((uint32_t)data.Variable.Temp, 2);
        Serial_SendString("°C  湿度: ");
        Serial_SendNumber((uint32_t)data.Variable.Humi, 2);
        Serial_SendString("%\r\n");
        Serial_SendString("connect_flag: ");
        Serial_SendNumber(connect_flag, 1);
        Serial_SendString("\r\n");
        Serial_SendString("按键1=连接巴法云  按键2=更新WiFi状态  按键4=上传数据\r\n");
        Serial_SendString("注意: DataAnylize()正在处理ESP8266数据\r\n");
        Serial_SendString("===================\r\n\r\n");
        last_status_time = current_time;
    }
    
    // 监听ESP8266的任何输出
    if(wifi.rxover == 1) {
        // ========== 【关键修复】先处理控制指令 ==========
        if(strstr((char*)wifi.rxbuff, "topic=tang2") && strstr((char*)wifi.rxbuff, "msg=")) {
            Serial_SendString("\r\n【Test_ESP8266】检测到tang2控制指令，调用DataAnylize()\r\n");
            DataAnylize();  // 调用DataAnylize处理控制指令
            return;  // 处理完直接返回
        }
        
        // 过滤掉重复的信息，只显示重要的状态变化
        if(!strstr((char*)wifi.rxbuff, "[单片机接收] AT") && 
           !strstr((char*)wifi.rxbuff, "[单片机接收] CMD:CONNECTED_TCP") &&
           !strstr((char*)wifi.rxbuff, "更新BAFA_PRODUCT_ID") &&
           !strstr((char*)wifi.rxbuff, "更新BAFA_TOPIC_CONTROL") &&
           !strstr((char*)wifi.rxbuff, "巴法云连接成功，认证信息") &&
           !strstr((char*)wifi.rxbuff, "WIFI_CONNECTED:2") &&
           !strstr((char*)wifi.rxbuff, "指令：获取当前时间") &&
           !strstr((char*)wifi.rxbuff, "Get_Time") &&
           !strstr((char*)wifi.rxbuff, "[巴法云->单片机] 原始数据: cmd=1&res=1") &&
           !strstr((char*)wifi.rxbuff, "缺少必要字段") &&
           !strstr((char*)wifi.rxbuff, "[解析成功] 提取") &&
           !strstr((char*)wifi.rxbuff, "[单片机接收] CMD:SEND_DATA") &&
           !strstr((char*)wifi.rxbuff, "[转发到巴法云]")) {
            Serial_SendString("\r\nESP8266 Output: ");
            Serial_SendString((char*)wifi.rxbuff);
            Serial_SendString("\r\n");
            
            // 提取并保存关键信息
            
            // 提取WiFi名称
            char *wifi_pos = strstr((char*)wifi.rxbuff, "Connecting to SAVED AP: ");
            if(wifi_pos != NULL) {
                char *wifi_start = wifi_pos + strlen("Connecting to SAVED AP: ");
                char *wifi_end = strstr(wifi_start, " ");
                if(wifi_end == NULL) wifi_end = strstr(wifi_start, "\r");
                if(wifi_end == NULL) wifi_end = strstr(wifi_start, "\n");
                if(wifi_end == NULL) wifi_end = wifi_start + strlen(wifi_start);
                
                int wifi_len = wifi_end - wifi_start;
                if(wifi_len > 0 && wifi_len < 49) {
                    strncpy(saved_wifi_name, wifi_start, wifi_len);
                    saved_wifi_name[wifi_len] = '\0';
                    Serial_SendString("==> WiFi名称已更新!\r\n");
                }
            }
            
            // 提取IP地址
            char *ip_pos = strstr((char*)wifi.rxbuff, "STA IP Address: ");
            if(ip_pos != NULL) {
                char *ip_start = ip_pos + strlen("STA IP Address: ");
                char *ip_end = strstr(ip_start, " ");
                if(ip_end == NULL) ip_end = strstr(ip_start, "\r");
                if(ip_end == NULL) ip_end = strstr(ip_start, "\n");
                if(ip_end != NULL) {
                    int ip_len = ip_end - ip_start;
                    if(ip_len < 19) {
                        strncpy(saved_ip, ip_start, ip_len);
                        saved_ip[ip_len] = '\0';
                    }
                }
                Serial_SendString("==> IP地址已更新!\r\n");
            }
            
            if(strstr((char*)wifi.rxbuff, "WiFi模式: STA")) {
                strcpy(saved_mode, "STA(客户端模式)");
                Serial_SendString("==> WiFi模式已更新!\r\n");
            }
            
            if(strstr((char*)wifi.rxbuff, "WIFI_CONNECTED:1")) {
                strcpy(saved_status, "已连接");
                connect_flag = 1;  // 更新连接标志
                Serial_SendString("==> 连接状态已更新! (connect_flag=1)\r\n");
            }
            
            if(strstr((char*)wifi.rxbuff, "AutoConnect: SUCCESS")) {
                Serial_SendString("==> WiFi连接成功!\r\n");
                // 当WiFi连接成功时，更新连接状态
                connect_flag = 1;
                strcpy(saved_status, "已连接");
                Serial_SendString("==> 连接状态已更新! (connect_flag=1)\r\n");
            }
            
            // 检测巴法云连接状态
            if(strstr((char*)wifi.rxbuff, "连接巴法云成功") || 
               strstr((char*)wifi.rxbuff, "MQTT连接成功") || 
               strstr((char*)wifi.rxbuff, "TCP连接成功") ||
               strstr((char*)wifi.rxbuff, "巴法云连接成功")) {
                connect_flag = 2;  // 云端连接成功
                strcpy(saved_status, "云端已连接");
                Serial_SendString("==> 巴法云连接成功!\r\n");
            }
            
            if(strstr((char*)wifi.rxbuff, "订阅成功") || 
               strstr((char*)wifi.rxbuff, "Subscribe") ||
               strstr((char*)wifi.rxbuff, "主题订阅成功")) {
                Serial_SendString("==> 主题订阅成功!\r\n");
            }
            
            // 检测连接过程中的状态
            if(strstr((char*)wifi.rxbuff, "更新BAFA_PRODUCT_ID")) {
                Serial_SendString("==> 巴法云参数配置中...\r\n");
            }
            
            Serial_SendString("---\r\n");
        } else {
            // 对于被过滤的信息，检查是否包含重要的连接状态信息
            if(strstr((char*)wifi.rxbuff, "[转发到巴法云]")) {
                Serial_SendString("📤 数据上传成功 ");
                // 提取温湿度信息显示
                char *temp_pos = strstr((char*)wifi.rxbuff, "Temp\":");
                if(temp_pos) {
                    Serial_SendString("(T:");
                    // 简单提取温度值显示
                    char temp_str[6];
                    sscanf(temp_pos + 6, "%5s", temp_str);
                    Serial_SendString(temp_str);
                    Serial_SendString("°C)");
                }
                Serial_SendString("\r\n");
            } else if(strstr((char*)wifi.rxbuff, "巴法云连接成功") || 
                      strstr((char*)wifi.rxbuff, "WIFI_CONNECTED:2")) {
                // 检测到巴法云连接成功，更新状态
                connect_flag = 2;
                Serial_SendString("🎉 检测到巴法云连接成功! (connect_flag=2)\r\n");
            } else {
                // 显示一个点表示ESP8266有活动（但是其他信息被过滤了）
                Serial_SendString(".");
            }
        }
        
        // 清除缓冲区
        Clear_BuffData();
    }
}

// 手动连接巴法云的函数
void Connect_To_BaFa_Cloud(void)
{
    static uint32_t last_connect_time = 0;
    uint32_t current_time = Task_GetSystemTime();
    
    // 防止重复连接（5秒内只能连接一次）
    if(current_time - last_connect_time < 5000) {
        Serial_SendString("请等待5秒后再尝试连接...\r\n");
        return;
    }
    
    // 检查是否已经连接到云端
    if(connect_flag == 2) {
        Serial_SendString("巴法云已连接，无需重复连接\r\n");
        return;
    }
    
    // 检查WiFi连接状态 - 如果有IP地址就认为已连接
    if(connect_flag >= 1 || strstr(saved_status, "已连接") || strstr(saved_ip, "172.168")) {
        Serial_SendString("=== 开始连接巴法云 ===\r\n");
        Serial_SendString("密钥: 51d78bea923947ebbeace0075d96cc21\r\n");
        Serial_SendString("上传主题: tang1\r\n");
        Serial_SendString("控制主题: tang2\r\n");
        Serial_SendString("发送连接指令...\r\n");
        
        Connect_tothe_cloud(CMD_CONNECT_BAFA);
        last_connect_time = current_time;
        
        Serial_SendString("巴法云连接指令已发送，等待响应...\r\n");
        Serial_SendString("===================\r\n");
    } else {
        Serial_SendString("错误: WiFi未连接，无法连接巴法云\r\n");
        Serial_SendString("当前连接状态: ");
        Serial_SendString(saved_status);
        Serial_SendString("\r\n");
    }
}



int main(void)
{	
	SystemInit();  
	Delay_Init();	
	Initialization();  // 系统初始化（包含OLED、按键、ESP8266、任务系统等）
	
	// 补充Initialization()中没有的初始化
	DHT11_Init();      // DHT11温湿度传感器初始化
	Buzzer_Init();     // 蜂鸣器初始化
	relay_Init();      // 继电器初始化
	Serial_Init();     // 串口1初始化（调试用）
	
	uint32_t start_time = Task_GetSystemTime();
	
	//变量
	Serial_SendString("mode");
	static uint32_t last_temp_read = 0;
	while(DHT11_Init())
	{
		OLED_ShowString( 0, 0, "error", OLED_8X16);
		Delay_ms(1000);
	}
	while(1)
	{	
		uint32_t current_time = Task_GetSystemTime();
		//DHT11_Read_Data(&temp,&humi);
		if(current_time - last_temp_read >= 2000)//两秒读取温湿度
        {
            DHT11_Read_Data(&temp, &humi);
            
            // 更新到全局数据结构
            data.Variable.Temp = (float)temp;
            data.Variable.Humi = (float)humi;
			// 只清除温湿度显示区域
			OLED_ClearArea(0, 0, 128, 16);
			OLED_ShowString(6*0, 0, "temp", OLED_6X8);
			OLED_ShowString(6*9, 0, "humi", OLED_6X8);
			OLED_ShowNum(6*6, 0, temp, 2, OLED_6X8);
			OLED_ShowNum(6*15, 0, humi, 2, OLED_6X8);
            SimpleDrawTempCurve();
			// 更新显示
			OLED_Update();
            last_temp_read = current_time;
        }
	    //Delay_ms(1000);
		
		
		if( Get_Key_2() == 2 ){
			// 按键2：手动更新WiFi连接状态
			if(strstr(saved_ip, "172.168") || strstr(saved_ip, "192.168")) {
				Serial_SendString("按键2按下 - 手动更新WiFi连接状态...\r\n");
				connect_flag = 1;
				strcpy(saved_status, "已连接");
				Serial_SendString("WiFi连接状态已更新为：已连接\r\n");
			} else {
				Serial_SendString("按键2按下 - 但未检测到有效IP地址\r\n");
				// menu2();  // 如果需要原来的菜单功能，取消注释这行
			}
		}
		
		// 按键1连接巴法云
		if(Get_Key_1() == 1) {
			Serial_SendString("按键1按下 - 尝试连接巴法云...\r\n");
			Connect_To_BaFa_Cloud();
		}
		
		// 按键3手动设置巴法云连接状态（长按或多次按）
		static uint8_t key3_count = 0;
		if(Get_Key_3() == 3) {
			key3_count++;
			if(key3_count >= 3) {  // 连续按3次按键3
				Serial_SendString("按键3连按3次 - 手动设置巴法云连接状态...\r\n");
				if(connect_flag == 1) {
					connect_flag = 2;  // 设置为巴法云已连接状态
					Serial_SendString("巴法云连接状态已手动设置为：已连接 (connect_flag=2)\r\n");
					Serial_SendString("现在可以尝试上传数据了\r\n");
				} else {
					Serial_SendString("当前状态: ");
					Serial_SendNumber(connect_flag, 1);
					Serial_SendString(" - 请先确保WiFi连接正常\r\n");
				}
				key3_count = 0;  // 重置计数
			}
		} else {
			// 如果没有按键3，重置计数
			if(key3_count > 0) key3_count--;
		}
		
		// 按键4手动上传数据
		if(Get_Key_4() == 4) {
			Serial_SendString("\r\n=== 手动上传数据测试 ===\r\n");
			Serial_SendString("当前温度: ");
			Serial_SendNumber((uint32_t)data.Variable.Temp, 2);
			Serial_SendString("°C\r\n");
			Serial_SendString("当前湿度: ");
			Serial_SendNumber((uint32_t)data.Variable.Humi, 2);
			Serial_SendString("%\r\n");
			Serial_SendString("开始上传...\r\n");
			upload_data();
			Serial_SendString("========================\r\n");
		}
		
		Manual_Mode();
		Test_ESP8266();
		data.flag.x++;
		Task_RunScheduler();
	}
}

