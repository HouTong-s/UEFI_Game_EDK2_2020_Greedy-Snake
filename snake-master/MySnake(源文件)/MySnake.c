
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimplePointer.h>
#include <Protocol/AbsolutePointer.h>
#include <Protocol/SimpleTextInEx.h>
#define height 20
#define width 35
#define JR_RANDOM_NUM   100
static UINT32 RandomPool[JR_RANDOM_NUM];
static UINT32 JR_index = JR_RANDOM_NUM;
static UINT32 RandomResult = 0;
UINT32 IsFailed = 0;
UINT32 ispaused = 0;
typedef struct _SNAKEDIRECTION{
    INT32 x;
    INT32 y;
}SNAKEDIRECTION;
SNAKEDIRECTION direction;
typedef enum _BOARDSTATUS{
    snake, food, space,wall
}BOARDSTATUS;
BOARDSTATUS board[height][width];
typedef struct _NODE{
    UINT32 x;
    UINT32 y;
}NODE;
NODE myfood ;
UINT32 SnakeLen = 3;
UINT32 score = 0;
NODE MySnake[height*width];
VOID Init();
VOID CreateSnake();
VOID CreateFood();
VOID PushSnakeFront(NODE pos);
NODE RemoveSnakeBack();
UINT32 isoverstep(NODE pos);
VOID gameover(EFI_EVENT Event);
UINT32 isSpace(NODE pos);
UINT32 isSnake(NODE pos);
UINT32 isFood(NODE pos);
static VOID JR_InitRandom();
UINT32 JR_randomInt(UINT32 max);
VOID Run(EFI_EVENT Event,VOID *Context);
EFI_STATUS SetTimer();

VOID mySetCursorPos(UINT32 x, UINT32 y);
VOID initMap();
VOID showMap();
VOID showTips();
VOID welcomeUI();
VOID gameOverUI();
VOID clean();

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
    EFI_STATUS Status;
    EFI_SIMPLE_TEXT_OUTPUT_MODE SavedConsoleMode;
    CopyMem(&SavedConsoleMode, gST->ConOut->Mode, sizeof(EFI_SIMPLE_TEXT_OUTPUT_MODE));

    Status = gST->ConOut->EnableCursor(gST->ConOut, FALSE);
    Status = gST->ConOut->ClearScreen(gST->ConOut);
    Status = gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
    Status = gST->ConOut->SetCursorPosition(gST->ConOut, 0, 0);

	welcomeUI();
	while (1) {
		EFI_INPUT_KEY Key;
		Status = gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
		if (Key.UnicodeChar == 's') {
			gST->ConOut->ClearScreen(gST->ConOut);
			SetTimer();
			break;
		}
		else if (Key.ScanCode == SCAN_ESC) {
			gST->ConOut->ClearScreen(gST->ConOut);
			Print(L"Exit Game!\n");
			break;
		}
	}
    return 0;
}


VOID Run(EFI_EVENT Event,VOID *Context)
{
    INT32 headerX, headerY;
    headerX = MySnake[0].x + direction.x;
    headerY = MySnake[0].y + direction.y;
    NODE snakeHeader;
    snakeHeader.x=headerX;
    snakeHeader.y=headerY;
    if(isoverstep(snakeHeader)||isSnake(snakeHeader))
        gameover(Event);
    if(isFood(snakeHeader))
    {
        score ++;
        CreateFood();
    }
    else
    {
        NODE snakeFooter;
        snakeFooter = RemoveSnakeBack();
    }
    PushSnakeFront(snakeHeader);
	showMap();
}
EFI_STATUS SetTimer()
{
    Init();
    CreateSnake();
    CreateFood();
	EFI_STATUS  Status;
	EFI_EVENT myEvent;
	//Print(L"Begin\n");
	Status = gBS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Run, (VOID*)NULL, &myEvent);
	Status = gBS->SetTimer(myEvent, TimerPeriodic, 2 * 1000 * 1000);
	showTips();
    while (1)
    {
        if(IsFailed)
        {
			UINT32 endGame = 0;
			gameOverUI();
			EFI_INPUT_KEY Key;
			while(1){
				Status = gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
				if (Key.UnicodeChar == 'n') {
					endGame = 1;
					break;
				}
				else if (Key.UnicodeChar == 'y') {
					clean();
					score = 0;
					IsFailed = 0;
					SetTimer();
				}
			}
			if (endGame == 1) {
				gST->ConOut->ClearScreen(gST->ConOut);
				Print(L"Exit Game!\n");
				break;
			}
            //Print(L"GameOver\n");
        }
        EFI_INPUT_KEY Key;
        Status = gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
		if (Key.ScanCode == SCAN_ESC) {
			gST->ConOut->ClearScreen(gST->ConOut);
	        Status = gBS->CloseEvent(myEvent);
			Print(L"Exit Game!\n");
			break;
		}
        if(!ispaused)
        {
            if(Key.ScanCode == SCAN_UP&&direction.x!=1  )
            {
                direction.x=-1;
                direction.y=0;
            }
            else if(Key.ScanCode == SCAN_RIGHT&&direction.y!=-1   )
            {
                direction.x=0;
                direction.y=1;
            }
            else if(Key.ScanCode == SCAN_DOWN&&direction.x!=-1   )
            {
                direction.x=1;
                direction.y=0;
            }
            else if(Key.ScanCode == SCAN_LEFT&&direction.y!=1   )
            {
                direction.x=0;
                direction.y=-1;
            }
        }
        if(Key.UnicodeChar == ' ')
        {
            if(ispaused)
            {
                Status = gBS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)Run, (VOID*)NULL, &myEvent);
	            Status = gBS->SetTimer(myEvent, TimerPeriodic, 2 * 1000 * 1000);
                ispaused=0;
            }
            else
            {
                gBS->CloseEvent(myEvent);
                ispaused=1;
            }
        }
    }
	return EFI_SUCCESS;
}
VOID Init()
{
    direction.x = 0;
    direction.y = 1;
    UINT32 i, j ;
	for (i = 0; i < height; i++) {
		j = 0;
		board[i][j] = wall;
		j = width - 1;
		board[i][j] = wall;
	}
	for (j = 0; j < width; j++) {
		i = 0;
		board[i][j] = wall;
		i = height - 1;
		board[i][j] = wall;
	}
    for(i=1;i<height-1;i++)
        for(j=1;j<width-1;j++)
        {
            board[i][j] = space;
        }
}
VOID CreateSnake()
{
    MySnake[0].x = 8;
    MySnake[0].y = 15;
    MySnake[1].x = 8;
    MySnake[1].y = 14;
    MySnake[2].x = 8;
    MySnake[2].y = 13;
    UINT32 i;
	SnakeLen = 3;
    for(i=0;i<SnakeLen;i++)
        board[MySnake[i].x][MySnake[i].y]=snake;
}
VOID CreateFood()
{
    UINT32 randomX, randomY;
    randomX = JR_randomInt(height);
    randomY = JR_randomInt(width);
    if(board[randomX][randomY] != space || board[randomX][randomY] == wall)
        CreateFood();
    else
    {
        myfood.x=randomX;
        myfood.y=randomY;
        board[randomX][randomY] = food;
    }
}
UINT32 isoverstep(NODE pos)
{
    if(pos.x<1||pos.x>height-2||pos.y<1||pos.y>width-2)
		return 1;			
	else
		return 0;
}
static VOID JR_InitRandom(){
    UINT32 i = 0;
    EFI_TIME  Time;
    if(JR_index != JR_RANDOM_NUM) return ;
    JR_index = 0;
    for(i=0; i<JR_RANDOM_NUM; i++){
        if(RandomResult ==0)
        {
            gRT->GetTime(&Time, NULL);
            RandomResult  = Time.Second;
        }
        RandomResult = (RandomResult<<1) | (((RandomResult&0x80)>>7)^((RandomResult&0x40)>>6));
        RandomPool[i] = RandomResult;
    }
}
UINT32 JR_randomInt(UINT32 max){
    JR_InitRandom();
    return (RandomPool[JR_index++] % (max));
}
VOID gameover(EFI_EVENT Event)
{
    EFI_STATUS Status;
    Status = gBS->CloseEvent(Event);
    IsFailed=1;
}
UINT32 isSpace(NODE pos)
{
    if(board[pos.x][pos.y] == space )
        return 1;
    else
        return 0;
}
UINT32 isSnake(NODE pos)
{
    if(board[pos.x][pos.y] == snake )
        return 1;
    else
        return 0;
}
UINT32 isFood(NODE pos)
{
    if(board[pos.x][pos.y] == food )
        return 1;
    else
        return 0;
}
VOID PushSnakeFront(NODE pos)
{
    UINT32 i;
    for(i=SnakeLen;i>0;i--)
    {
        MySnake[i]=MySnake[i-1];
    }
    board[pos.x][pos.y]=snake;
    MySnake[0]=pos;
    SnakeLen++ ;
}
NODE RemoveSnakeBack()
{
    SnakeLen-- ;
    board[MySnake[SnakeLen].x][MySnake[SnakeLen].y]=space;
    return MySnake[SnakeLen];
}

VOID mySetCursorPos(UINT32 x, UINT32 y) {
	gST->ConOut->SetCursorPosition(gST->ConOut, x, y);
}


VOID showMap() {
	UINT32 i = 0;
	UINT32 j = 0;
	for ( i = 0; i < height; i++) 
	{
		for ( j = 0; j < width; j++) 
		{
			mySetCursorPos(j*2,i);
			if (board[i][j] == wall) 
			{
				gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_RED));
				gST->ConOut->OutputString(gST->ConOut, L"  ");
			}
			else if (board[i][j] == snake) 
			{
				gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLUE));
				gST->ConOut->OutputString(gST->ConOut, L"  ");
			}
			else if (board[i][j] == food) 
			{
				gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_GREEN));
				gST->ConOut->OutputString(gST->ConOut, L"  ");
			}
			else 
			{
				gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
				gST->ConOut->OutputString(gST->ConOut, L"  ");
			}
			gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
		}
	}
	mySetCursorPos(width * 2 + 2, height/2);
	gST->ConOut->SetAttribute(gST->ConOut, EFI_BACKGROUND_BLACK | EFI_YELLOW);
	gST->ConOut->OutputString(gST->ConOut, L"Score:");
	Print(L"%d\n", score);
}

VOID showTips() {
	if (!IsFailed) 
	{
		gST->ConOut->SetAttribute(gST->ConOut, EFI_BACKGROUND_BLACK | EFI_YELLOW);
		mySetCursorPos(2, height + 1);
		gST->ConOut->OutputString(gST->ConOut, L" [Tips]    ");
		mySetCursorPos(2, height + 2);
		gST->ConOut->OutputString(gST->ConOut, L" Control Snake's Direction:");
		mySetCursorPos(2, height + 3);
		gST->ConOut->OutputString(gST->ConOut, L" Key[UP] to Move Up       Key[DOWN] to Move down   ");
		gST->ConOut->OutputString(gST->ConOut, L" Press [Space] To PAUSE");
		mySetCursorPos(2, height + 4);
		gST->ConOut->OutputString(gST->ConOut, L" Key[LEFT] to Move Left   Key[RIGHT] to Move right   ");
		gST->ConOut->OutputString(gST->ConOut, L" Press [Esc] To EXIT");
		mySetCursorPos(2, height + 5);
	}
}

VOID welcomeUI() {
	gST->ConOut->SetAttribute(gST->ConOut, EFI_BACKGROUND_BLACK | EFI_YELLOW);
	gST->ConOut->OutputString(gST->ConOut, L"\n\n\r");
	gST->ConOut->OutputString(gST->ConOut, L"\t\t\t\t********************************** SNAKE ***********************************\n\r");
	gST->ConOut->OutputString(gST->ConOut, L"\t\t\t\t*                       Welcome To Our UEFI Game                           *\n\r");
	gST->ConOut->OutputString(gST->ConOut, L"\t\t\t\t*               By Hou Tong(1853636) & Dong Zhenyu(1852143)                *\n\r");
	gST->ConOut->OutputString(gST->ConOut, L"\t\t\t\t*                      School Of SoftWare Engineering                      *\n\r");
	gST->ConOut->OutputString(gST->ConOut, L"\t\t\t\t*                          Press [S] to Start                              *\n\r");
	gST->ConOut->OutputString(gST->ConOut, L"\t\t\t\t*                          Press [ESC] To Exit                             *\n\r");
	gST->ConOut->OutputString(gST->ConOut, L"\t\t\t\t*********************************** SNAKE **********************************\n\r");
}

VOID gameOverUI() {
	gST->ConOut->SetAttribute(gST->ConOut, EFI_BACKGROUND_BLACK | EFI_YELLOW);
	mySetCursorPos(2, height + 1);
	gST->ConOut->OutputString(gST->ConOut, L" GAME OVER!  Your final score is ");
	Print(L"%d !", score);
	mySetCursorPos(2, height + 2);
	gST->ConOut->OutputString(gST->ConOut, L"                                                                           ");
	mySetCursorPos(2, height + 2);
	gST->ConOut->OutputString(gST->ConOut, L" Do you want to try again?");
	mySetCursorPos(2, height + 3);
	gST->ConOut->OutputString(gST->ConOut, L"                                                                           ");
	mySetCursorPos(2, height + 3);
	gST->ConOut->OutputString(gST->ConOut, L" Press [Y] to start a new game | Press [N] to exit Snake game!");
	mySetCursorPos(2, height + 4);
	gST->ConOut->OutputString(gST->ConOut, L"                                                                           ");
}

VOID clean() {
	mySetCursorPos(width * 2 + 2, height / 2);
	gST->ConOut->OutputString(gST->ConOut, L"Score:  ");
	mySetCursorPos(2, height + 1);
	gST->ConOut->OutputString(gST->ConOut, L"                                                                           ");
	mySetCursorPos(2, height + 2);
	gST->ConOut->OutputString(gST->ConOut, L"                                                                           ");
	mySetCursorPos(2, height + 3);
	gST->ConOut->OutputString(gST->ConOut, L"                                                                           ");
	mySetCursorPos(2, height + 4);
	gST->ConOut->OutputString(gST->ConOut, L"                                                                           ");
}