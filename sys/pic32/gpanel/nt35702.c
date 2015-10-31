
#define LCD_RD   A0
#define LCD_WR   A1
#define LCD_RS   A2
#define LCD_CS   A3
#define LCD_REST A4

void Lcd_Writ_Bus(unsigned char VH)
{
    unsigned int i,temp,data;
    data=VH;
    for (i=8; i<=9; i++) {
        temp=(data&0x01);
        if (temp)
            digitalWrite(i,HIGH);
        else
            digitalWrite(i,LOW);
        data=data>>1;
    }
    for (i=2; i<=7; i++) {
        temp=(data&0x01);
        if (temp)
            digitalWrite(i,HIGH);
        else
            digitalWrite(i,LOW);
        data=data>>1;
    }

    digitalWrite(LCD_WR,LOW);
    digitalWrite(LCD_WR,HIGH);
}

void Lcd_Write_Com(unsigned char VH)
{
    digitalWrite(LCD_RS,LOW);
    Lcd_Writ_Bus(VH);
}

void Lcd_Write_Data(unsigned char VH)
{
    digitalWrite(LCD_RS,HIGH);
    Lcd_Writ_Bus(VH);
}

void Lcd_Write_Com_Data(unsigned char com,unsigned char dat)
{
    Lcd_Write_Com(com);
    Lcd_Write_Data(dat);
}

void Address_set(unsigned int x1,unsigned int y1,unsigned int x2,unsigned int y2)
{
    Lcd_Write_Com_Data(0x2a,x1>>8);
    Lcd_Write_Com_Data(0x2a,x1);
    Lcd_Write_Com_Data(0x2a,x2>>8);
    Lcd_Write_Com_Data(0x2a,x2);
    Lcd_Write_Com_Data(0x2b,y1>>8);
    Lcd_Write_Com_Data(0x2b,y1);
    Lcd_Write_Com_Data(0x2b,y2>>8);
    Lcd_Write_Com_Data(0x2b,y2);
    Lcd_Write_Com(0x2c);
}

void Lcd_Init(void)
{
    digitalWrite(LCD_REST, HIGH);
    delay(5);
    digitalWrite(LCD_REST, LOW);
    delay(15);
    digitalWrite(LCD_REST, HIGH);
    delay(15);

    digitalWrite(LCD_CS, HIGH);
    digitalWrite(LCD_WR, HIGH);
    digitalWrite(LCD_CS, LOW);  //CS
    Lcd_Write_Com(0x01);// Software Reset
    delay(20);

    Lcd_Write_Com(0x11);//Sleep Out
    delay(120);

    Lcd_Write_Com(0xc2);//Power Control 3
    Lcd_Write_Data(0x05);//APA2 APA1 APA0   Large
    Lcd_Write_Data(0x00);//Step-up cycle in Booster circuit 1
                         //Step-up cycle in Booster circuit 2,3
    Lcd_Write_Com(0xc3);//Power Control 4
    Lcd_Write_Data(0x05);//APA2 APA1 APA0   Large
    Lcd_Write_Data(0x00);//Step-up cycle in Booster circuit 1
                         //Step-up cycle in Booster circuit 2,3
    Lcd_Write_Com(0xc4);//Power Control 5
    Lcd_Write_Data(0x05);//APA2 APA1 APA0   Large
    Lcd_Write_Data(0x00);//Step-up cycle in Booster circuit 1
                         //Step-up cycle in Booster circuit 2,3
    Lcd_Write_Com(0x3A);
    Lcd_Write_Data(0x55);

    Lcd_Write_Com(0xD7);
    Lcd_Write_Data(0x40);
    Lcd_Write_Data(0xE0);

    Lcd_Write_Com(0xFD);
    Lcd_Write_Data(0x06);
    Lcd_Write_Data(0x11);

    Lcd_Write_Com(0xFA);
    Lcd_Write_Data(0x38);
    Lcd_Write_Data(0x20);
    Lcd_Write_Data(0x1C);
    Lcd_Write_Data(0x10);
    Lcd_Write_Data(0x37);
    Lcd_Write_Data(0x12);
    Lcd_Write_Data(0x22);
    Lcd_Write_Data(0x1E);

    Lcd_Write_Com(0xC0);//Set GVDD
    Lcd_Write_Data(0x05);

    Lcd_Write_Com(0xC5);//Set Vcom
    Lcd_Write_Data(0x60);
    Lcd_Write_Data(0x00);

    Lcd_Write_Com(0xC7);//Set VCOM-OFFSET
    Lcd_Write_Data(0xA9);//  可以微调改善flicker

    Lcd_Write_Com(0x36);//Memory data  access control
    Lcd_Write_Data(0xC8);//MY MX MV ML RGB MH 0 0

    ////Gamma//////////////////
    Lcd_Write_Com(0xE0);//E0H Set
    Lcd_Write_Data(0x23);
    Lcd_Write_Data(0x23);
    Lcd_Write_Data(0x24);
    Lcd_Write_Data(0x02);
    Lcd_Write_Data(0x08);
    Lcd_Write_Data(0x0F);
    Lcd_Write_Data(0x35);
    Lcd_Write_Data(0x7B);
    Lcd_Write_Data(0x43);
    Lcd_Write_Data(0x0E);
    Lcd_Write_Data(0x1F);
    Lcd_Write_Data(0x25);
    Lcd_Write_Data(0x10);
    Lcd_Write_Data(0x16);
    Lcd_Write_Data(0x31);

    Lcd_Write_Com(0xE1);//E1H Set
    Lcd_Write_Data(0x0D);
    Lcd_Write_Data(0x28);
    Lcd_Write_Data(0x2E);
    Lcd_Write_Data(0x0B);
    Lcd_Write_Data(0x11);
    Lcd_Write_Data(0x12);
    Lcd_Write_Data(0x3E);
    Lcd_Write_Data(0x59);
    Lcd_Write_Data(0x4C);
    Lcd_Write_Data(0x10);
    Lcd_Write_Data(0x26);
    Lcd_Write_Data(0x2B);
    Lcd_Write_Data(0x1B);
    Lcd_Write_Data(0x1B);
    Lcd_Write_Data(0x1B);

    Lcd_Write_Com(0x29);//display on

    Lcd_Write_Com(0x2c);//Memory Write
}

void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c)
{
    unsigned int i, j;

    Lcd_Write_Com(0x02c); //write_memory_start
    digitalWrite(LCD_RS, HIGH);
    digitalWrite(LCD_CS, LOW);
    l = l+x;
    Address_set(x,y,l,y);
    j = l*2;
    for(i=1;i<=j;i++)
    {
        Lcd_Write_Data(c>>8);
        Lcd_Write_Data(c);
    }
    digitalWrite(LCD_CS, HIGH);
}

void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c)
{
    unsigned int i, j;

    Lcd_Write_Com(0x02c); //write_memory_start
    digitalWrite(LCD_RS, HIGH);
    digitalWrite(LCD_CS, LOW);
    l = l+y;
    Address_set(x,y,x,l);
    j = l*2;
    for(i=1;i<=j;i++)
    {
        Lcd_Write_Data(c>>8);
        Lcd_Write_Data(c);
    }
    digitalWrite(LCD_CS, HIGH);
}

void Rect(unsigned int x,unsigned int y,unsigned int w,unsigned int h,unsigned int c)
{
    H_line(x  , y  , w, c);
    H_line(x  , y+h, w, c);
    V_line(x  , y  , h, c);
    V_line(x+w, y  , h, c);
}

void Rectf(unsigned int x,unsigned int y,unsigned int w,unsigned int h,unsigned int c)
{
    unsigned int i;

    for (i=0; i<h; i++) {
        H_line(x  , y  , w, c);
        H_line(x  , y+i, w, c);
    }
}

int RGB(int r, int g, int b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void LCD_Clear(unsigned int j)
{
    unsigned int i, m;

    Address_set(0, 0, 320, 240);
    Lcd_Write_Com(0x02c); //write_memory_start
    digitalWrite(LCD_RS,HIGH);
    digitalWrite(LCD_CS,LOW);

    for (i=0; i<320; i++) {
        for (m=0; m<240; m++) {
            //ch=((fcolorr&248)|fcolorg>>5);
            //cl=((fcolorg&28)<<3|fcolorb>>3);

            Lcd_Write_Data(j>>8);
            Lcd_Write_Data(j);
        }
    }
    digitalWrite(LCD_CS,HIGH);
}

void draw_Pixel(int x,int y, unsigned int c)
{
    Address_set(x, y, x, y);
    Lcd_Write_Data(c >> 8);
    Lcd_Write_Data(c);
}

void swap(int x, int y)
{
    int temp = x;
    x = y;
    y = temp;
}

void draw_Hline(int x, int y, int l,unsigned int c)
{
    Address_set(x, y, x+l, y);
    for (int i=0; i<l+1; i++)
    {
        Lcd_Write_Data(c >> 8);
        Lcd_Write_Data(c);
    }
}

void draw_Vline(int x, int y, int l, unsigned int c)
{
    Address_set(x, y, x, y+l);
    for (int i=0; i<l+1; i++)
    {
        Lcd_Write_Data(c);
        Lcd_Write_Data(c);
    }
}

void RoundRect(int x1, int y1, int x2, int y2,unsigned int c)
{
    Lcd_Write_Com(0x02c); //write_memory_start
    digitalWrite(LCD_RS,HIGH);
    digitalWrite(LCD_CS,LOW);

    int tmp;
    if (x1 > x2) {
        swap(x1, x2);
    }
    if (y1 > y2) {
        swap(y1, y2);
    }

    if ((x2-x1 > 4 && (y2-y1) > 4) {
        draw_Pixel(x1+1,y1+1,c);
        draw_Pixel(x2-1,y1+1,c);
        draw_Pixel(x1+1,y2-1,c);
        draw_Pixel(x2-1,y2-1,c);
        draw_Hline(x1+2, y1, x2-x1-4,c);
        draw_Hline(x1+2, y2, x2-x1-4,c);
        draw_Vline(x1, y1+2, y2-y1-4,c);
        draw_Vline(x2, y1+2, y2-y1-4,c);
    }
}

void setup()
{
    for (int p=2; p<10; p++)
        pinMode(p, OUTPUT);

    pinMode(A0, OUTPUT);
    pinMode(A1, OUTPUT);
    pinMode(A2, OUTPUT);
    pinMode(A3, OUTPUT);
    pinMode(A4, OUTPUT);
    digitalWrite(A0, HIGH);
    digitalWrite(A1, HIGH);
    digitalWrite(A2, HIGH);
    digitalWrite(A3, HIGH);
    digitalWrite(A4, HIGH);

    Lcd_Init();
    //LCD_Clear(RGB(255, 0,   0));
    //LCD_Clear(RGB(255, 255, 0));
    //LCD_Clear(RGB(255, 0,   255));
    //LCD_Clear(RGB(0,   255, 255));
    //LCD_Clear(RGB(0,   255, 0));
}
