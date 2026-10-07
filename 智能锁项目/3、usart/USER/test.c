#include "sys.h"	
#include "delay.h"	
#include "led.h"
#include "OLED.h"
#include "usart.h"

int main(void)
{			
	SysTick_Init();	  	 //—” ±≥ı ºªØ
	USART1_Init(); 
  LED_Init();
	OLED_Init();
	
	while(1)
	{
		
    printf("          .--\"\"--.\r\n");
    printf("        /`         `\\\r\n");
    printf("       |    °Ò   °Ò    |\r\n");
    printf("       |      ¶ÿ      |\r\n");
    printf("       \\    \\___/    /\r\n");
    printf("        \\___________/\r\n");
    printf("       /|           |\\\r\n");
    printf("      / |___________| \\\r\n");
    printf("     /    |       |    \\\r\n");
    printf("    /     |       |     \\\r\n");
    printf("   /______|_______|______\\\r\n");

   delay_ms(1000);
	}	 
}




