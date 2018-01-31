/*
 * ILI9341 registers.
 */
#define ILI9341_No_Operation                                0x00
#define ILI9341_Software_Reset                              0x01
#define ILI9341_Read_Display_Identification_Information     0x04
#define ILI9341_Read_Display_Status                         0x09
#define ILI9341_Read_Display_Power_Mode                     0x0A
#define ILI9341_Read_Display_MADCTL                         0x0B
#define ILI9341_Read_Display_Pixel_Format                   0x0C
#define ILI9341_Read_Display_Image_Format                   0x0D
#define ILI9341_Read_Display_Signal_Mode                    0x0E
#define ILI9341_Read_Display_Self_Diagnostic_Result         0x0F
#define ILI9341_Enter_Sleep_Mode                            0x10
#define ILI9341_Sleep_OUT                                   0x11
#define ILI9341_Partial_Mode_ON                             0x12
#define ILI9341_Normal_Display_Mode_ON                      0x13
#define ILI9341_Display_Inversion_OFF                       0x20
#define ILI9341_Display_Inversion_ON                        0x21
#define ILI9341_Gamma_Set                                   0x26
#define ILI9341_Display_OFF                                 0x28
#define ILI9341_Display_ON                                  0x29
#define ILI9341_Column_Address_Set                          0x2A
#define ILI9341_Page_Address_Set                            0x2B
#define ILI9341_Memory_Write                                0x2C
#define ILI9341_Color_SET                                   0x2D
#define ILI9341_Memory_Read                                 0x2E
#define ILI9341_Partial_Area                                0x30
#define ILI9341_Vertical_Scrolling_Definition               0x33
#define ILI9341_Tearing_Effect_Line_OFF                     0x34
#define ILI9341_Tearing_Effect_Line_ON                      0x35
#define ILI9341_Memory_Access_Control                       0x36
#define ILI9341_Vertical_Scrolling_Start_Address            0x37
#define ILI9341_Idle_Mode_OFF                               0x38
#define ILI9341_Idle_Mode_ON                                0x39
#define ILI9341_Pixel_Format_Set                            0x3A
#define ILI9341_Write_Memory_Continue                       0x3C
#define ILI9341_Read_Memory_Continue                        0x3E
#define ILI9341_Set_Tear_Scanline                           0x44
#define ILI9341_Get_Scanline                                0x45
#define ILI9341_Write_Display_Brightness                    0x51
#define ILI9341_Read_Display_Brightness                     0x52
#define ILI9341_Write_CTRL_Display                          0x53
#define ILI9341_Read_CTRL_Display                           0x54
#define ILI9341_Write_Content_Adaptive_Brightness_Control   0x55
#define ILI9341_Read_Content_Adaptive_Brightness_Control    0x56
#define ILI9341_Write_CABC_Minimum_Brightness               0x5E
#define ILI9341_Read_CABC_Minimum_Brightness                0x5F
#define ILI9341_Read_ID1                                    0xDA
#define ILI9341_Read_ID2                                    0xDB
#define ILI9341_Read_ID3                                    0xDC
#define ILI9341_RGB_Interface_Signal_Control                0xB0
#define ILI9341_Frame_Control_In_Normal_Mode                0xB1
#define ILI9341_Frame_Control_In_Idle_Mode                  0xB2
#define ILI9341_Frame_Control_In_Partial_Mode               0xB3
#define ILI9341_Display_Inversion_Control                   0xB4
#define ILI9341_Blanking_Porch_Control                      0xB5
#define ILI9341_Display_Function_Control                    0xB6
#define ILI9341_Entry_Mode_Set                              0xB7
#define ILI9341_Backlight_Control_1                         0xB8
#define ILI9341_Backlight_Control_2                         0xB9
#define ILI9341_Backlight_Control_3                         0xBA
#define ILI9341_Backlight_Control_4                         0xBB
#define ILI9341_Backlight_Control_5                         0xBC
#define ILI9341_Backlight_Control_7                         0xBE
#define ILI9341_Backlight_Control_8                         0xBF
#define ILI9341_Power_Control_1                             0xC0
#define ILI9341_Power_Control_2                             0xC1
#define ILI9341_VCOM_Control_1                              0xC5
#define ILI9341_VCOM_Control_2                              0xC7
#define ILI9341_NV_Memory_Write                             0xD0
#define ILI9341_NV_Memory_Protection_Key                    0xD1
#define ILI9341_NV_Memory_Status_Read                       0xD2
#define ILI9341_Read_ID4                                    0xD3
#define ILI9341_Positive_Gamma_Correction                   0xE0
#define ILI9341_Negative_Gamma_Correction                   0xE1
#define ILI9341_Digital_Gamma_Control_1                     0xE2
#define ILI9341_Digital_Gamma_Control_2                     0xE3
#define ILI9341_Interface_Control                           0xF6

/*
 * Memory Access Control register
 */
#define MADCTL_MY           0x80    /* Row address order */
#define MADCTL_MX           0x40    /* Column address order */
#define MADCTL_MV           0x20    /* Row/column exchange */
#define MADCTL_ML           0x10    /* Vertical refresh order */
#define MADCTL_BGR          0x08    /* Color filter selector: 0=RGB, 1=BGR */
#define MADCTL_MH           0x04    /* Horisontal refresh direction: 1=left-to-right */
