/*
 * Eight Queens puzzle
 *
 * (C) 2010 by Mark Sproul
 * Open source as per standard Arduino code
 * Modified by Pito 12/2012 for SmallC
 */
#define TRUE    1
#define FALSE   0

unsigned int gChessBoard[8];
unsigned int gLoopCounter;
int gValidCount;

CheckCurrentBoard()
{
    int ii;
    int jj;
    int theRow;
    int theLongRow;
    int theLongColumns;
    int bitCount;

    //* we know we have 1 in each row,
    //* Check for 1 in each column
    theRow = 0;
    for (ii=0; ii<8; ii++) {
        theRow |= gChessBoard[ii];
    }
    if (theRow != 0x0ff) {
        return FALSE;
    }

    //* we have 1 in each column, now check the diagonals
    theLongColumns = 0;
    for (ii=0; ii<8; ii++) {
        theLongRow = gChessBoard[ii] & 0x0ff;
        theLongRow = theLongRow << ii;

        theLongColumns |= theLongRow;
    }

    //* now count the bits
    bitCount = 0;
    for (ii=0; ii<16; ii++) {
        if ((theLongColumns & 0x01) == 0x01) {
            bitCount++;
        }
        theLongColumns = theLongColumns >> 1;
    }

    if (bitCount != 8) {
        return FALSE;
    }

    //* we now have to check the other diagonal
    theLongColumns = 0;
    for (ii=0; ii<8; ii++) {
        theLongRow = gChessBoard[ii] & 0x0ff;
        theLongRow = theLongRow << 8;
        theLongRow = theLongRow >> ii;

        theLongColumns |= theLongRow;
    }

    //* now count the bits
    bitCount = 0;
    for (ii=0; ii<16; ii++) {
        if ((theLongColumns & 0x01) == 0x01) {
            bitCount++;
        }
        theLongColumns = theLongColumns >> 1;
    }

    if (bitCount != 8) {
        return FALSE;
    }
    return TRUE;
}

CheckForDone()
{
    int ii;
    int weAreDone;
    int theRow;

    weAreDone = FALSE;

    //* we know we have 1 in each row,
    //* Check for 1 in each column
    theRow = 0;
    for (ii=0; ii<8; ii++) {
        theRow |= gChessBoard[ii];
    }

    if (theRow == 0x01) {
        weAreDone = TRUE;
    }
    return weAreDone;
}

RotateQueens()
{
    int ii;
    int keepGoing;
    int theRow;

    ii = 0;
    keepGoing = TRUE;
    while (keepGoing && (ii < 8)) {
        theRow = gChessBoard[ii] & 0x0ff;
        theRow = (theRow >> 1) & 0x0ff;
        if (theRow != 0) {
            gChessBoard[ii] = theRow;
            keepGoing = FALSE;
        } else {
            gChessBoard[ii] = 0x080;
        }
        ii++;
    }
}

PrintChessBoard()
{
    int ii;
    int jj;
    int theRow;
    char textString[32];

    printf("\nLoop= %d\n", gLoopCounter);
    printf("Solution count= %d\n", gValidCount);

    printf("+----------------+\n");
    for (ii=0; ii<8; ii++) {
        theRow = gChessBoard[ii];

        printf("|");
        for (jj=0; jj<8; jj++) {
            if (theRow & 0x080) {
                printf("Q ");
            } else {
                printf(". ");
            }
            theRow = theRow << 1;
        }
        printf("|\n");
    }
    printf("+----------------+\n");
}

main()
{
    int ii;

    printf("\nEight Queens brute force");
    printf("\n************************\n");
    //* put the 8 queens on the board, 1 in each row
    for (ii=0; ii<8; ii++) {
        gChessBoard[ii] = 0x080;
    }
    PrintChessBoard();

    gLoopCounter = 0;
    gValidCount = 0;


    while (1) {
        gLoopCounter++;

        if (CheckCurrentBoard()) {
            gValidCount++;
            PrintChessBoard();
        } else if ((gLoopCounter % 1000) == 0) {
            //PrintChessBoard();
        }

        RotateQueens();
        if (CheckForDone()) {
            //int elapsedSeconds;
            //int elapsedMinutes;
            //int elapsedHours;

            //elapsedSeconds = millis() / 1000;
            //elapsedMinutes = elapsedSeconds / 60;
            //elapsedHours = elapsedMinutes / 60;

            printf("----------------------------------\n");
            printf("All done\n");

            PrintChessBoard();
            printf("----------------------------------\n");

            //Serial.print("total seconds=");
            //Serial.println(elapsedSeconds);

            //Serial.print("hours=");
            //Serial.println(elapsedHours);

            //Serial.print("minutes=");
            //Serial.println(elapsedMinutes % 60);

            //Serial.print("seconds=");
            //Serial.println(elapsedSeconds % 60);

            return (1);
        }
    }
}
