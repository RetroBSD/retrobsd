TERMCAP variable contains a list of terminal capabilities.

    co      Numeric     Number of columns
    li      Numeric     Number of lines
    nb      Flag        No bell
    cm      String      Cursor movement
    cl      String      Clear screen
    ho      String      Home: move to top left corner
    up      String      Move up
    do      String      Move down
    nd      String      Move right
    le      String      Move left
    cu      String      Draw cursor label
    kh      String      Key Home
    kH      String      Key End
    ku      String      Key Up
    kd      String      Key Down
    kr      String      Key Right
    kl      String      Key Left
    kP      String      Key Page Up
    kN      String      Key Page Down
    kI      String      Key Insert Char
    kD      String      Key Delete Char
    k1      String      Key F1
    k2      String      Key F2
    k3      String      Key F3
    k4      String      Key F4
    k5      String      Key F5
    k6      String      Key F6
    k7      String      Key F7
    k8      String      Key F8
    k9      String      Key F9
    k0      String      Key F10
    F1      String      Key F11
    F2      String      Key F12


Example for Linux terminal:

TERMCAP=:co#80:li#25:cm=\E[%i%d;%dH:cl=\E[H\E[2J:ho=\E[H:\
up=\E[A:do=\E[B:nd=\E[C:le=\10:cu=\E[7m \E[m:ku=\E[A:kd=\E[B:\
kr=\E[C:kl=\E[D:kP=\E[5~:kN=\E[6~:kI=\E[2~:kD=\E[3~:kh=\E[1~:kH=\E[4~:\
k1=\E[[A:k2=\E[[B:k3=\E[[C:k4=\E[[D:k5=\E[15~:k6=\E[17~:\
k7=\E[18~:k8=\E[19~:k9=\E[20~:k0=\E[21~:F1=\E[23~:F2=\E[24~:
