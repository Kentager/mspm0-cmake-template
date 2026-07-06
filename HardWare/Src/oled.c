#include "oled.h"
#include "oled_font.h"
#include "ti_msp_dl_config.h"


#define OLED_ADDR 0x3C

#define I2C_TIMEOUT 10000

void OLED_I2C_Init(void) {
  
}

// 向OLED寄存器地址写一个byte的数据
int I2C_WriteByte(uint8_t addr, uint8_t data) {
    uint8_t buff[2] = {0};
    buff[0] = addr;
    buff[1] = data;
    
    uint32_t timeout;

    /* 1. 等待 I2C 控制器空闲 */
    timeout = I2C_TIMEOUT;
    while (!(DL_I2C_getControllerStatus(I2C_OLED_INST) & DL_I2C_CONTROLLER_STATUS_IDLE)) {
        if (--timeout == 0) {
            // 【关键】超时后，强制复位 I2C 控制器
            DL_I2C_disableController(I2C_OLED_INST);
            DL_I2C_enableController(I2C_OLED_INST);
            return -1; 
        }
    }

    /* 2. 先填充数据到 TX FIFO */
    DL_I2C_fillControllerTXFIFO(I2C_OLED_INST, buff, 2);

    /* 3. 启动传输 */
    DL_I2C_startControllerTransfer(I2C_OLED_INST, OLED_ADDR,
                                   DL_I2C_CONTROLLER_DIRECTION_TX, 2);

    /* 4. 等待传输完成 */
    timeout = I2C_TIMEOUT;
    while (DL_I2C_getControllerStatus(I2C_OLED_INST) & DL_I2C_CONTROLLER_STATUS_BUSY) {
        if (--timeout == 0) {
            // 【关键】传输超时，同样强制复位 I2C 控制器
            DL_I2C_disableController(I2C_OLED_INST);
            DL_I2C_enableController(I2C_OLED_INST);
            return -1; 
        }
    }

    return 0; // 成功返回 0
}

// 写指令
int WriteCmd(unsigned char I2C_Command) { return I2C_WriteByte(0x00, I2C_Command); }

// 写数据
int WriteData(unsigned char I2C_Data) { return I2C_WriteByte(0x40, I2C_Data); }

// 厂家初始化代码
// void OLED_Init(void) {
//   OLED_I2C_Init();
//   delay_cycles(1000);
//   WriteCmd(0xAE); // display off
//   delay_cycles(1000);
//   WriteCmd(0x20); // Set Memory Addressing Mode
//   WriteCmd(0x10); // 00,Horizontal Addressing Mode;01,Vertical Addressing
//                   // Mode;10,Page Addressing Mode (RESET);11,Invalid
//   WriteCmd(0xb0); // Set Page Start Address for Page Addressing Mode,0-7
//   WriteCmd(0xc8); // Set COM Output Scan Direction
//   WriteCmd(0x00); //---set low column address
//   WriteCmd(0x10); //---set high column address
//   WriteCmd(0x40); //--set start line address
//   delay_cycles(1000);
//   WriteCmd(0xA1); //--set segment re-map 0 to 127
//   WriteCmd(0xA6); //--set normal display
//   WriteCmd(0xA8); //--set multiplex ratio(1 to 64)
//   WriteCmd(0x3F); //
//   WriteCmd(0xA4); // 0xa4,Output follows RAM content
//   WriteCmd(0xAF); //--turn on oled panel
// }
void OLED_Init(void) {
    OLED_I2C_Init();
    delay_cycles(16000000); // 增加延时
    
    WriteCmd(0xAE); // display off
    WriteCmd(0xD5); // Set Display Clock Divide Ratio/Oscillator Frequency
    WriteCmd(0x80); // Set Display Clock Divide Ratio/Oscillator Frequency
    WriteCmd(0xA8); // Set Multiplex Ratio
    WriteCmd(0x3F); // 1/64 duty
    WriteCmd(0xD3); // Set Display Offset
    WriteCmd(0x00); // 0 offset
    WriteCmd(0x40); // Set Start Line
    WriteCmd(0x8D); // Set Charge Pump
    WriteCmd(0x14); // Enable charge pump regulator
    WriteCmd(0x20); // Set Memory Addressing Mode
    WriteCmd(0x02); // Page Addressing Mode (RESET)
    WriteCmd(0xA1); // Set Segment Re-map
    WriteCmd(0xC8); // Set COM Output Direction
    WriteCmd(0xDA); // Set COM Pins Hardware Configuration
    WriteCmd(0x12); // Alternative COM pin configuration
    WriteCmd(0x81); // Set Contrast Control
    WriteCmd(0xCF); // Set Contrast Control
    WriteCmd(0xD9); // Set Pre-charge Period
    WriteCmd(0xF1); // Set Pre-charge Period
    WriteCmd(0xDB); // Set VCOMH Deselect Level
    WriteCmd(0x40); // Set VCOMH Deselect Level
    WriteCmd(0xA4); // Entire Display ON from RAM
    WriteCmd(0xA6); // Set Normal Display
    WriteCmd(0xAF); // Turn on OLED panel
}

// 设置光标起始坐标（x,y）
void OLED_SetPos(unsigned char x, unsigned char y) {
  WriteCmd(0xb0 + y);
  WriteCmd((x & 0xf0) >> 4 | 0x10);
  WriteCmd((x & 0x0f) | 0x01);
}

// 填充整个屏幕
void OLED_Fill(unsigned char Fill_Data) {
  unsigned char m, n;

  for (m = 0; m < 8; m++) {
    WriteCmd(0xb0 + m);
    WriteCmd(0x00);
    WriteCmd(0x10);

    for (n = 0; n < 128; n++) {
      WriteData(Fill_Data);
    }
  }
}

// 清屏
void OLED_CLS(void) { OLED_Fill(0x00); }

// 将OLED从休眠中唤醒
void OLED_ON(void) {
  WriteCmd(0xAF);
  WriteCmd(0x8D);
  WriteCmd(0x14);
}

// 让OLED休眠 -- 休眠模式下,OLED功耗不到10uA
void OLED_OFF(void) {
  WriteCmd(0xAE);
  WriteCmd(0x8D);
  WriteCmd(0x10);
}
/**
  * @brief  OLED显示字符串
  * @param  line 起始行位置，范围：1~8(TextSize=1)1~4(TextSize=2)
  * @param  column 起始列位置，范围：1~21(TextSize=1)1~16(TextSize=2)<列超范围自动换行>
  * @param  String 要显示的字符串 
  * @param  TextSize 字号
  * @retval 无
  */
void OLED_ShowString(uint8_t line, uint8_t column, char *String,uint8_t TextSize)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		OLED_ShowChar(line, column + i + 1, String[i],TextSize);
	}
}
/**
  * @brief  OLED显示字符
  * @param  line 起始行位置，范围：1~8(TextSize=1)1~4(TextSize=2)
  * @param  column 起始列位置，范围：1~21(TextSize=1)1~16(TextSize=2)<列超范围自动换行>
  * @param  ch 要显示的字符
  * @param  TextSize 字号
  * @retval 无
  */
void OLED_ShowChar(uint8_t line,uint8_t column,char ch,uint8_t TextSize)
{
    if(column>(26-5*TextSize))
    {
        column-=(26-5*TextSize);
        line+=TextSize;
    }
    uint8_t x=(column-1)*(2*TextSize+4);
    uint8_t y=line-1;
    uint8_t c=0,i=0;
    c=ch-' ';
    switch (TextSize)
    {
        case 1: 
        {
            OLED_SetPos(x,y);
            for(i=0;i<6;i++)
                WriteData(F6x8[c][i]);
            break;
        }
        case 2:
        {
            OLED_SetPos(x,y);
            for(i=0;i<8;i++)
                WriteData(F8X16[c*16+i]);
            OLED_SetPos(x,y+1);
            for(i=0;i<8;i++)
                WriteData(F8X16[c*16+i+8]);
            break;
        }
    }
}
uint32_t oled_pow(uint8_t m,uint8_t n)
{
    uint32_t result=1;
    while(n--)result*=m;
    return result;
}
/**
  * @brief  OLED显示数字（十进制，无符号数）
  * @param  line 起始行位置，范围：1~8(TextSize=1)1~4(TextSize=2)
  * @param  column 起始列位置，范围：1~21(TextSize=1)1~16(TextSize=2)
  * @param  num 要显示的数字，范围：-2147483648~2147483647
  * @param  len 要显示数字的长度
  * @param  TextSize 字号
  * @retval 无
  */
void OLED_ShowNum(uint8_t line,uint8_t column,uint32_t num,uint8_t len,uint8_t TextSize)
{
    uint8_t i;
	for (i = 0; i < len; i++)							
	{
		OLED_ShowChar(line, column + i, num / oled_pow(10, len - i - 1) % 10 + '0',TextSize);
	}
}
/**
  * @brief  OLED显示数字（十进制，带符号数）
  * @param  Line 起始行位置，范围：1~8(TextSize=1)1~4(TextSize=2)
  * @param  Column 起始列位置，范围：1~21(TextSize=1)1~16(TextSize=2)
  * @param  Number 要显示的数字，范围：-2147483648~2147483647
  * @param  Length 要显示数字的长度，范围：1~10
  * @param  TextSize 字号
  * @retval 无
  */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length,uint8_t TextSize)
{
	uint8_t i;
	uint32_t Number1;
	if (Number >= 0)
	{
		OLED_ShowChar(Line, Column, '+',TextSize);
		Number1 = Number;
	}
	else
	{
		OLED_ShowChar(Line, Column, '-',TextSize);
		Number1 = -Number;
	}
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i + 1, Number1 / oled_pow(10, Length - i - 1) % 10 + '0',TextSize);
	}
}

/**
  * @brief  OLED显示数字（十六进制，正数）
  * @param  Line 起始行位置，范围：1~8(TextSize=1)1~4(TextSize=2)
  * @param  Column 起始列位置，范围：1~21(TextSize=1)1~16(TextSize=2)
  * @param  Number 要显示的数字，范围：0~0xFFFFFFFF
  * @param  TextSize 字号
  * @param  Length 要显示数字的长度，范围：1~8
  * @retval 无
  */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length,uint8_t TextSize)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)							
	{
		SingleNumber = Number / oled_pow(16, Length - i - 1) % 16;
		if (SingleNumber < 10)
		{
			OLED_ShowChar(Line, Column + i, SingleNumber + '0',TextSize);
		}
		else
		{
			OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A',TextSize);
		}
	}
}
/**
  * @brief  OLED显示数字（二进制，正数）
  * @param  Line 起始行位置，范围：1~8(TextSize=1)1~4(TextSize=2)
  * @param  Column 起始列位置，范围：1~21(TextSize=1)1~16(TextSize=2)
  * @param  Number 要显示的数字，范围：0~1111 1111 1111 1111
  * @param  Length 要显示数字的长度，范围：1~16
  * @param  TextSize 字号
  * @retval 无
  */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length,uint8_t TextSize)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i, Number / oled_pow(2, Length - i - 1) % 2 + '0',TextSize);
	}
}
/**
  * @brief  OLED显示浮点数字（十进制，带符号数）
  * @param  Line 起始行位置，范围：1~8(TextSize=1)1~4(TextSize=2)
  * @param  Column 起始列位置，范围：1~21(TextSize=1)1~16(TextSize=2)
  * @param  Number 要显示的数字
  * @param  Length 要显示数字的长度，范围：1~10
  * @param  Flength 要显示的小数点后几位
  * @param  TextSize 字号
  * @retval 无
  */
void OLED_ShowFNum(uint8_t Line, uint8_t Column, float Number, uint8_t Length,uint8_t Flength,uint8_t TextSize)
{
    uint8_t i;
    uint8_t flag = 1;
    float Number1;
    uint32_t Number2;
    if (Number >= 0)
    {
        OLED_ShowChar(Line, Column, '+',TextSize);
        Number1 = Number;
    }
    else
    {
        OLED_ShowChar(Line, Column, '-',TextSize);
        Number1 = -Number;
    }
    //将浮点数转换成整数然后显示
    Number2 = (int)(Number1 * oled_pow(10,Flength));
    
    
    for (i = Length; i > 0; i--)                            
    {
        if(i == (Length - Flength))
        {
            OLED_ShowChar(Line,Column + i + flag,'.',TextSize);
            flag = 0;
            OLED_ShowChar(Line, Column + i + flag, Number2 / oled_pow(10, Length - i) % 10 + '0',TextSize);
        }
        else
        {
            OLED_ShowChar(Line, Column + i + flag, Number2 / oled_pow(10, Length - i) % 10 + '0',TextSize);
        }
        
    }    
        
}