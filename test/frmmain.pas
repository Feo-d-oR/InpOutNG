unit frmmain;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, Buttons, StdCtrls, inpout32dll;

type

  { IrqWait }
  IrqWait = class(TThread)
    constructor Create(TotalTasks:integer; AOwner: TObject; FuncPtr:TDlIrqWait);
  private
    numTasks     : integer;
    NotifyStatus : LongWord;
    Owner        : TObject;
    DLIrqWait    : TDLIrqWait;
  protected
    procedure Execute; override;
  end;

type

  { TForm1 }

  TForm1 = class(TForm)
    eNumTasks: TEdit;
    eNumRuns: TEdit;
    btnRunTest: TSpeedButton;
    btnSetIrqWait: TSpeedButton;
    btnSendNotify: TSpeedButton;
    procedure btnRunTestClick(Sender: TObject);
    procedure btnSendNotifyClick(Sender: TObject);
    procedure btnSetIrqWaitClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure IrqDone(Sender: TObject);
  private
    DlDrvOpen : TDlDrvOpen;
    DlIs64Bit : TDlIs64Bit;

    DlInp8   : TDlInp8;
    DlInp16  : TDlInp16;
    DlInp32  : TDlInp32;
    DlOutp8  : TDlOutp8;
    DlOutp16 : TDlOutp16;
    DlOutp32 : TDlOutp32;

    DlRegNotify  : TDlRegNotify;
    DlGetNotify  : TDlGetNotify;

    DlIrqWait    : TDlIrqWait;
    DLIrqRequest : TDLIrqRequest;
    DLForceNotify: TDLForceNotify;

    TStartWait   : double;
    TEndWait     : double;
    NotifyCode   : LongWord;
    WaitThread   : IrqWait;
  private

    function FunctionsFound : Boolean;
  public

  end;

var
  Form1: TForm1;

implementation

uses libTime, Windows;

{$R *.lfm}

var
  PrevWndProc: WNDPROC;

{ Message Handler}
function WndCallback(Ahwnd: HWND; uMsg: UINT; wParam: WParam; lParam: LParam):LRESULT; stdcall;
begin
  if uMsg = Form1.NotifyCode then
  begin
    Form1.TEndWait:=GetUNIXTime;
    ShowMessage('Message was Received!');
  end;
  // If Msg is not WM_DEVICECHANGE pass the Msg
  Result:= CallWindowProc(WNDPROC(PrevWndProc),Ahwnd,uMsg,WParam,LParam);
end;

{ TForm1 }
procedure TForm1.FormCreate(Sender: TObject);
begin
  if not DllLoad then Application.Terminate;
  DlDrvOpen := TDlDrvOpen(GetProcFromDll('IsInpOutDriverOpen'));
  DlIs64Bit := TDlIs64Bit(GetProcFromDll('IsXP64Bit'));

  DlInp8        := TDlInp8(  GetProcFromDll('inp8'));
  DlInp16       := TDlInp16( GetProcFromDll('inp16'));
  DlInp32       := TDlInp32( GetProcFromDll('inp32'));
  DlOutp8       := TDlOutp8( GetProcFromDll('outp8'));
  DlOutp16      := TDlOutp16(GetProcFromDll('outp16'));
  DlOutp32      := TDlOutp32(GetProcFromDll('outp32'));
  DlRegNotify   := TDlRegNotify(GetProcFromDll('registerWmNotify'));
  DlGetNotify   := TDlGetNotify(GetProcFromDll('getWmNotify'));
  DlIrqWait     := TDlIrqWait(GetProcFromDll('waitForIrq'));
  DlIrqRequest  := TDlIrqRequest(GetProcFromDll('requestIrqNotify'));
  DlForceNotify := TDlForceNotify(GetProcFromDll('forceNotify'));

  if Assigned(DlRegNotify)
  then
    begin
      DlRegNotify();
      PrevWndProc:= {%H-}Windows.WNDPROC(SetWindowLongPtr(Self.Handle,GWL_WNDPROC,{%H-}PtrInt(@WndCallback)));
    end;
  if Assigned(DlGetNotify)
  then NotifyCode:=DlGetNotify();
  MessageDlg(Format('Everything is %s!', [BoolToStr(FunctionsFound, 'OK', 'crap')]),
             Format('Dll is %s and is %s bits%sMessage code is 0x%4x',
                    [ BoolToStr(DlDrvOpen(), 'loaded', 'NOT loaded'),
                      BoolToStr(DlIs64Bit(), '64', '32'),
                      LineEnding, DlGetNotify() ]),
             mtInformation,
             [mbOK],
             'FormLoadComplete');
end;

procedure TForm1.IrqDone(Sender: TObject);
begin
  ShowMessage(Format('IrqWait thread exited. Check your data, master...%s'+
                     'Time spend in thread is %.9f',
                     [LineEnding, TEndWait-TStartWait]));
  WaitThread.Destroy;
  WaitThread:=NIL;
end;

procedure TForm1.btnRunTestClick(Sender: TObject);
var
  TStart : double = 0;
  TEnd : double = 0;
  dt : double = 0;
  numCycles : Integer = 0;
  i : integer = 0;
  Val : SmallInt;
begin
  TryStrToInt(eNumRuns.Text, numCycles);
  if numCycles<=0
  then begin ShowMessage('numCycles not recognized'); Exit; end;
  TStart:=GetUNIXTime;
  for i:=1 to numCycles do
  begin
    Val:=DlInp16($300);
    DlOutp16($300, Val);
  end;
  TEnd:=GetUNIXTime;
  dt:=TEnd-TStart;
  ShowMessage(Format('%d cycles passed!%sTimespent = %.6f sec (%.6f per r/w cycle)',
                     [numCycles, LineEnding, dt, dt/numCycles]));
end;

procedure TForm1.btnSendNotifyClick(Sender: TObject);
begin
  DLForceNotify();
end;

procedure TForm1.btnSetIrqWaitClick(Sender: TObject);
var
  numTasks : Integer = 0;
begin
  TryStrToInt(eNumTasks.Text, numTasks);
  if numTasks<=0
  then begin ShowMessage('numTasks not recognized'); Exit; end;

  if not Assigned(WaitThread)
  then
    begin
      WaitThread:=IrqWait.Create(numTasks, Self, DlIrqWait);
      WaitThread.OnTerminate:=@IrqDone;
    end;
  (*
  Delay(0.020);
  DLForceNotify();
  *)
end;

function TForm1.FunctionsFound: Boolean;
begin
  Result := Assigned(DlDrvOpen)   and Assigned(DlIs64Bit)    and
            Assigned(DlInp8)      and Assigned(DlOutp8)      and
            Assigned(DlInp16)     and Assigned(DlOutp16)     and
            Assigned(DlInp32)     and Assigned(DlOutp32)     and
            Assigned(DlRegNotify) and Assigned(DlGetNotify)  and
            Assigned(DlIrqWait)   and Assigned(DlIrqRequest) and
            Assigned(DlForceNotify);
end;

{ IrqWait }

constructor IrqWait.Create(TotalTasks: integer; AOwner: TObject; FuncPtr:TDLIrqWait);
begin
  numTasks:=TotalTasks;
  Owner:=AOwner;
  DlIrqWait:=FuncPtr;
  Priority:=tpLower;
  inherited Create(false);
end;

procedure IrqWait.Execute;
var
  inTasks  : array of TPortTask = ();
  outTasks : array of TPortTask = ();
  i        : Integer = 0;
begin
  SetLength(inTasks,  numTasks);
  SetLength(outTasks, numTasks);
  for i:=0 to numTasks-1 do
    begin
      inTasks[i].taskOperation:=Word(INPOUT_READ16);
      inTasks[i].taskPort:=Random(MAXSHORT);
    end;
  TForm1(Owner).TStartWait:=GetUNIXTime;
  DlIrqWait(@inTasks[0], Length(inTasks)*sizeOf(TPortTask),
            @outTasks[0], Length(outTasks)*sizeOf(TPortTask));
  TForm1(Owner).TEndWait:=GetUNIXTime;
end;

end.

